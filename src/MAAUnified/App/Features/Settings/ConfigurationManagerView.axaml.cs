using System.IO;
using System.Linq;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Platform.Storage;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Settings;

public partial class ConfigurationManagerView : UserControl
{
    private static readonly FilePickerFileType JsonFileType = new("JSON")
    {
        Patterns = ["*.json"],
        MimeTypes = ["application/json"],
    };
    public ConfigurationManagerView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnConfigurationProfileSelectionChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SwitchConfigurationProfileAsync();
        }
    }

    private async void OnCreateProfileClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.AddConfigurationProfileAsync();
        }
    }

    private async void OnDeleteCurrentProfileClick(object? sender, RoutedEventArgs e)
    {
        var vm = VM;
        if (vm is null)
        {
            return;
        }

        var target = vm.ConfigurationManagerSelectedProfile?.Trim() ?? string.Empty;
        if (string.IsNullOrWhiteSpace(target))
        {
            return;
        }

        var owner = TopLevel.GetTopLevel(this) as Window;
        var language = vm.Language;
        var confirmed = await ShowWarningDialogAsync(
            owner,
            language,
            Localize(language, "删除配置", "Delete Profile"),
            string.Format(
                Localize(
                    language,
                    "确认删除配置“{0}”？",
                    "Delete profile \"{0}\"?"),
                target),
            confirmText: Localize(language, "删除", "Delete"),
            cancelText: Localize(language, "取消", "Cancel"));
        if (!confirmed)
        {
            return;
        }

        await vm.DeleteConfigurationProfileAsync();
    }

    private async void OnExportAllProfilesClick(object? sender, RoutedEventArgs e)
    {
        var vm = VM;
        if (vm is null)
        {
            return;
        }

        var savePath = await PickSavePathAsync(
            "导出所有配置",
            BuildSuggestedFileName("config-all", profileName: null));
        if (string.IsNullOrWhiteSpace(savePath))
        {
            return;
        }

        await vm.ExportAllConfigurationsAsync(savePath);
    }

    private async void OnExportCurrentProfileClick(object? sender, RoutedEventArgs e)
    {
        var vm = VM;
        if (vm is null)
        {
            return;
        }

        var savePath = await PickSavePathAsync(
            "导出当前配置",
            BuildSuggestedFileName("config-current", vm.ConfigurationManagerSelectedProfile));
        if (string.IsNullOrWhiteSpace(savePath))
        {
            return;
        }

        await vm.ExportCurrentConfigurationAsync(savePath);
    }

    private async void OnImportProfilesClick(object? sender, RoutedEventArgs e)
    {
        var vm = VM;
        if (vm is null)
        {
            return;
        }
        var owner = TopLevel.GetTopLevel(this) as Window;
        var language = vm.Language;

        while (true)
        {
            var importPaths = await PickImportPathsAsync();
            if (importPaths.Count == 0)
            {
                return;
            }

            var analysis = ConfigurationImportSelectionAnalyzer.Analyze(importPaths);
            switch (analysis.Kind)
            {
                case ConfigurationImportSelectionKind.UnifiedConfig:
                    await vm.ImportConfigurationsAsync(analysis.UnifiedConfigPath!);
                    return;

                case ConfigurationImportSelectionKind.LegacyReady:
                    if (await TryRunLegacyImportAsync(vm, analysis, owner, language))
                    {
                        return;
                    }

                    continue;

                case ConfigurationImportSelectionKind.LegacyPartial:
                {
                    var forceImport = await ShowWarningDialogAsync(
                        owner,
                        language,
                        Localize(language, "导入旧配置", "Import Legacy Config"),
                        analysis.Message,
                        confirmText: Localize(language, "强行导入", "Import Anyway"),
                        cancelText: Localize(language, "重新选择", "Choose Again"));
                    if (!forceImport)
                    {
                        continue;
                    }

                    if (await TryRunLegacyImportAsync(vm, analysis, owner, language, allowPartialImport: true))
                    {
                        return;
                    }

                    continue;
                }

                default:
                {
                    var close = await ShowWarningDialogAsync(
                        owner,
                        language,
                        Localize(language, "导入配置", "Import Config"),
                        analysis.Message,
                        confirmText: Localize(language, "关闭", "Close"),
                        cancelText: Localize(language, "重新选择", "Choose Again"));
                    if (!close)
                    {
                        continue;
                    }

                    return;
                }
            }
        }
    }

    private async Task<string?> PickSavePathAsync(string title, string suggestedFileName)
    {
        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel?.StorageProvider is not { CanSave: true } storageProvider)
        {
            return null;
        }

        var file = await storageProvider.SaveFilePickerAsync(
            new FilePickerSaveOptions
            {
                Title = title,
                SuggestedFileName = suggestedFileName,
                DefaultExtension = "json",
                ShowOverwritePrompt = true,
                FileTypeChoices =
                [
                    JsonFileType,
                ],
            });
        return file?.TryGetLocalPath();
    }

    private async Task<IReadOnlyList<string>> PickImportPathsAsync()
    {
        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel?.StorageProvider is not { CanOpen: true } storageProvider)
        {
            return [];
        }

        var files = await storageProvider.OpenFilePickerAsync(
            new FilePickerOpenOptions
            {
                Title = "导入配置",
                AllowMultiple = true,
                FileTypeFilter =
                [
                    JsonFileType,
                ],
            });
        return files
            .Select(file => file.TryGetLocalPath())
            .Where(path => !string.IsNullOrWhiteSpace(path))
            .Cast<string>()
            .ToArray();
    }

    private static async Task<bool> TryRunLegacyImportAsync(
        SettingsPageViewModel vm,
        ConfigurationImportSelectionAnalysis analysis,
        Window? owner,
        string? language,
        bool allowPartialImport = false)
    {
        var request = new LegacyImportRequest(
            LegacyConfigSnapshot.FromPaths(analysis.GuiNewPath, analysis.GuiPath),
            analysis.LegacyImportSource,
            ManualImport: true,
            AllowPartialImport: allowPartialImport);
        var report = await vm.ImportLegacyConfigurationsAsync(request);
        if (report.AppliedConfig)
        {
            return true;
        }

        if (report.DamagedFiles.Count > 0 && report.ImportedFiles.Count > 0 && !allowPartialImport)
        {
            var importUsable = await ShowWarningDialogAsync(
                owner,
                language,
                Localize(language, "导入旧配置", "Import Legacy Config"),
                Localize(
                    language,
                    $"{string.Join("、", report.DamagedFiles)} 文件损坏。",
                    $"These files are damaged: {string.Join(", ", report.DamagedFiles)}."),
                confirmText: Localize(language, "导入可用内容", "Import Valid Content"),
                cancelText: Localize(language, "重新选择", "Choose Again"));
            if (!importUsable)
            {
                return false;
            }

            var retriedReport = await vm.ImportLegacyConfigurationsAsync(request with { AllowPartialImport = true });
            if (retriedReport.AppliedConfig)
            {
                return true;
            }

            if (retriedReport.DamagedFiles.Count > 0)
            {
                var close = await ShowWarningDialogAsync(
                    owner,
                    language,
                    Localize(language, "导入旧配置", "Import Legacy Config"),
                    Localize(
                        language,
                        $"{string.Join("、", retriedReport.DamagedFiles)} 文件损坏，未导入任何配置。",
                        $"These files are damaged and no configuration was imported: {string.Join(", ", retriedReport.DamagedFiles)}."),
                    confirmText: Localize(language, "关闭", "Close"),
                    cancelText: Localize(language, "重新选择", "Choose Again"));
                return close;
            }

            return true;
        }

        if (report.DamagedFiles.Count > 0)
        {
            var close = await ShowWarningDialogAsync(
                owner,
                language,
                Localize(language, "导入旧配置", "Import Legacy Config"),
                Localize(
                    language,
                    $"{string.Join("、", report.DamagedFiles)} 文件损坏，未导入任何配置。",
                    $"These files are damaged and no configuration was imported: {string.Join(", ", report.DamagedFiles)}."),
                confirmText: Localize(language, "关闭", "Close"),
                cancelText: Localize(language, "重新选择", "Choose Again"));
            return close;
        }

        return true;
    }

    private static async Task<bool> ShowWarningDialogAsync(
        Window? owner,
        string? language,
        string title,
        string message,
        string confirmText,
        string cancelText)
    {
        if (owner is null)
        {
            return false;
        }

        var dialog = new WarningConfirmDialogView();
        dialog.ApplyRequest(title, message, confirmText, cancelText, language);
        return await dialog.ShowDialog<bool>(owner);
    }

    private static string Localize(string? language, string zh, string en)
    {
        return DialogTextCatalog.Select(language, zh, en);
    }

    private static string BuildSuggestedFileName(string prefix, string? profileName)
    {
        var timestamp = DateTime.Now.ToString("yyyyMMdd-HHmmss");
        if (string.IsNullOrWhiteSpace(profileName))
        {
            return $"maa-{prefix}-{timestamp}.json";
        }

        var invalidChars = Path.GetInvalidFileNameChars();
        var safeName = new string(profileName
            .Trim()
            .Select(c => invalidChars.Contains(c) ? '_' : c)
            .ToArray());
        if (string.IsNullOrWhiteSpace(safeName))
        {
            return $"maa-{prefix}-{timestamp}.json";
        }

        return $"maa-{prefix}-{safeName}-{timestamp}.json";
    }
}
