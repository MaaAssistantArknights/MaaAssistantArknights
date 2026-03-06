using System.Diagnostics;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.Features.Dialogs;

public sealed class AvaloniaDialogService : IAppDialogService
{
    private const string IssueReportIssueEntryUrl = "https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/new/choose";
    private const string WorkPackageTitlePrefix = "[工作包P] ";
    private readonly MAAUnifiedRuntime _runtime;

    public AvaloniaDialogService(MAAUnifiedRuntime runtime)
    {
        _runtime = runtime;
    }

    public async Task<DialogCompletion<AnnouncementDialogPayload>> ShowAnnouncementAsync(
        AnnouncementDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        var normalizedRequest = request with
        {
            Title = EnsureWorkPackageTitle(request.Title),
        };
        var token = await _runtime.DialogFeatureService.BeginDialogAsync(DialogType.Announcement, sourceScope, normalizedRequest.Title, cancellationToken);
        var owner = ResolveOwnerWindow();
        if (owner is null)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "owner", "owner-unavailable", cancellationToken);
            await _runtime.DialogFeatureService.CompleteDialogAsync(token, DialogReturnSemantic.Close, "owner-unavailable", cancellationToken);
            return new DialogCompletion<AnnouncementDialogPayload>(DialogReturnSemantic.Close, null, "owner-unavailable");
        }

        var dialog = new AnnouncementDialogView();
        dialog.ApplyRequest(normalizedRequest);
        var semantic = await dialog.ShowDialog<DialogReturnSemantic?>(owner) ?? DialogReturnSemantic.Close;
        var payload = semantic == DialogReturnSemantic.Confirm ? dialog.BuildPayload() : null;
        await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "return", semantic.ToString(), cancellationToken);
        await _runtime.DialogFeatureService.CompleteDialogAsync(token, semantic, "announcement-dialog-complete", cancellationToken);
        return new DialogCompletion<AnnouncementDialogPayload>(semantic, payload, "announcement-dialog-complete");
    }

    public async Task<DialogCompletion<VersionUpdateDialogPayload>> ShowVersionUpdateAsync(
        VersionUpdateDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        var normalizedRequest = request with
        {
            Title = EnsureWorkPackageTitle(request.Title),
        };
        var token = await _runtime.DialogFeatureService.BeginDialogAsync(DialogType.VersionUpdate, sourceScope, normalizedRequest.Title, cancellationToken);
        var owner = ResolveOwnerWindow();
        if (owner is null)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "owner", "owner-unavailable", cancellationToken);
            await _runtime.DialogFeatureService.CompleteDialogAsync(token, DialogReturnSemantic.Close, "owner-unavailable", cancellationToken);
            return new DialogCompletion<VersionUpdateDialogPayload>(DialogReturnSemantic.Close, null, "owner-unavailable");
        }

        var dialog = new VersionUpdateDialogView();
        dialog.ApplyRequest(normalizedRequest);
        var semantic = await dialog.ShowDialog<DialogReturnSemantic?>(owner) ?? DialogReturnSemantic.Close;
        var payload = semantic == DialogReturnSemantic.Confirm ? dialog.BuildPayload() : null;
        await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "return", semantic.ToString(), cancellationToken);
        await _runtime.DialogFeatureService.CompleteDialogAsync(token, semantic, "version-update-dialog-complete", cancellationToken);
        return new DialogCompletion<VersionUpdateDialogPayload>(semantic, payload, "version-update-dialog-complete");
    }

    public async Task<DialogCompletion<ProcessPickerDialogPayload>> ShowProcessPickerAsync(
        ProcessPickerDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        var normalizedRequest = request with
        {
            Title = EnsureWorkPackageTitle(request.Title),
        };
        var token = await _runtime.DialogFeatureService.BeginDialogAsync(DialogType.ProcessPicker, sourceScope, normalizedRequest.Title, cancellationToken);
        var owner = ResolveOwnerWindow();
        if (owner is null)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "owner", "owner-unavailable", cancellationToken);
            await _runtime.DialogFeatureService.CompleteDialogAsync(token, DialogReturnSemantic.Close, "owner-unavailable", cancellationToken);
            return new DialogCompletion<ProcessPickerDialogPayload>(DialogReturnSemantic.Close, null, "owner-unavailable");
        }

        var dialog = new ProcessPickerDialogView();
        dialog.ApplyRequest(normalizedRequest);
        var semantic = await dialog.ShowDialog<DialogReturnSemantic?>(owner) ?? DialogReturnSemantic.Close;
        var payload = semantic == DialogReturnSemantic.Confirm ? dialog.BuildPayload() : null;
        await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "return", semantic.ToString(), cancellationToken);
        await _runtime.DialogFeatureService.CompleteDialogAsync(token, semantic, "process-picker-dialog-complete", cancellationToken);
        return new DialogCompletion<ProcessPickerDialogPayload>(semantic, payload, "process-picker-dialog-complete");
    }

    public async Task<DialogCompletion<EmulatorPathDialogPayload>> ShowEmulatorPathAsync(
        EmulatorPathDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        var normalizedRequest = request with
        {
            Title = EnsureWorkPackageTitle(request.Title),
        };
        var token = await _runtime.DialogFeatureService.BeginDialogAsync(DialogType.EmulatorPath, sourceScope, normalizedRequest.Title, cancellationToken);
        var owner = ResolveOwnerWindow();
        if (owner is null)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "owner", "owner-unavailable", cancellationToken);
            await _runtime.DialogFeatureService.CompleteDialogAsync(token, DialogReturnSemantic.Close, "owner-unavailable", cancellationToken);
            return new DialogCompletion<EmulatorPathDialogPayload>(DialogReturnSemantic.Close, null, "owner-unavailable");
        }

        var dialog = new EmulatorPathSelectionDialogView();
        dialog.ApplyRequest(normalizedRequest);
        var semantic = await dialog.ShowDialog<DialogReturnSemantic?>(owner) ?? DialogReturnSemantic.Close;
        var payload = semantic == DialogReturnSemantic.Confirm ? dialog.BuildPayload() : null;
        await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "return", semantic.ToString(), cancellationToken);
        await _runtime.DialogFeatureService.CompleteDialogAsync(token, semantic, "emulator-path-dialog-complete", cancellationToken);
        return new DialogCompletion<EmulatorPathDialogPayload>(semantic, payload, "emulator-path-dialog-complete");
    }

    public async Task<DialogCompletion<ErrorDialogPayload>> ShowErrorAsync(
        ErrorDialogRequest request,
        string sourceScope,
        Func<CancellationToken, Task<UiOperationResult>>? openIssueReportAsync = null,
        CancellationToken cancellationToken = default)
    {
        var normalizedRequest = request with
        {
            Title = EnsureWorkPackageTitle(request.Title),
        };
        var token = await _runtime.DialogFeatureService.BeginDialogAsync(DialogType.Error, sourceScope, normalizedRequest.Title, cancellationToken);
        var owner = ResolveOwnerWindow();
        if (owner is null)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "owner", "owner-unavailable", cancellationToken);
            await _runtime.DialogFeatureService.CompleteDialogAsync(token, DialogReturnSemantic.Close, "owner-unavailable", cancellationToken);
            return new DialogCompletion<ErrorDialogPayload>(DialogReturnSemantic.Close, null, "owner-unavailable");
        }

        var dialog = new ErrorDialogView();
        dialog.ApplyRequest(normalizedRequest, openIssueReportAsync ?? OpenIssueReportAsync);
        var semantic = await dialog.ShowDialog<DialogReturnSemantic?>(owner) ?? DialogReturnSemantic.Close;
        var payload = dialog.BuildPayload();
        if (payload.Copied)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "copy", "clipboard", cancellationToken);
        }

        if (payload.IssueReportOpened)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "open-issue", "issue-report-entry", cancellationToken);
        }

        await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "return", semantic.ToString(), cancellationToken);
        await _runtime.DialogFeatureService.CompleteDialogAsync(token, semantic, "error-dialog-complete", cancellationToken);
        return new DialogCompletion<ErrorDialogPayload>(semantic, payload, "error-dialog-complete");
    }

    public async Task<DialogCompletion<AchievementListDialogPayload>> ShowAchievementListAsync(
        AchievementListDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        var normalizedRequest = request with
        {
            Title = EnsureWorkPackageTitle(request.Title),
        };
        var token = await _runtime.DialogFeatureService.BeginDialogAsync(DialogType.AchievementList, sourceScope, normalizedRequest.Title, cancellationToken);
        var owner = ResolveOwnerWindow();
        if (owner is null)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "owner", "owner-unavailable", cancellationToken);
            await _runtime.DialogFeatureService.CompleteDialogAsync(token, DialogReturnSemantic.Close, "owner-unavailable", cancellationToken);
            return new DialogCompletion<AchievementListDialogPayload>(DialogReturnSemantic.Close, null, "owner-unavailable");
        }

        var dialog = new AchievementListDialogView();
        dialog.ApplyRequest(normalizedRequest);
        var semantic = await dialog.ShowDialog<DialogReturnSemantic?>(owner) ?? DialogReturnSemantic.Close;
        var payload = dialog.BuildPayload();
        await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "return", semantic.ToString(), cancellationToken);
        await _runtime.DialogFeatureService.CompleteDialogAsync(token, semantic, "achievement-list-dialog-complete", cancellationToken);
        return new DialogCompletion<AchievementListDialogPayload>(semantic, payload, "achievement-list-dialog-complete");
    }

    public async Task<DialogCompletion<TextDialogPayload>> ShowTextAsync(
        TextDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        var normalizedRequest = request with
        {
            Title = EnsureWorkPackageTitle(request.Title),
        };
        var token = await _runtime.DialogFeatureService.BeginDialogAsync(DialogType.Text, sourceScope, normalizedRequest.Title, cancellationToken);
        var owner = ResolveOwnerWindow();
        if (owner is null)
        {
            await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "owner", "owner-unavailable", cancellationToken);
            await _runtime.DialogFeatureService.CompleteDialogAsync(token, DialogReturnSemantic.Close, "owner-unavailable", cancellationToken);
            return new DialogCompletion<TextDialogPayload>(DialogReturnSemantic.Close, null, "owner-unavailable");
        }

        var dialog = new TextDialogView();
        dialog.ApplyRequest(normalizedRequest);
        var semantic = await dialog.ShowDialog<DialogReturnSemantic?>(owner) ?? DialogReturnSemantic.Close;
        var payload = semantic == DialogReturnSemantic.Confirm ? dialog.BuildPayload() : null;
        await _runtime.DialogFeatureService.RecordDialogActionAsync(token, "return", semantic.ToString(), cancellationToken);
        await _runtime.DialogFeatureService.CompleteDialogAsync(token, semantic, "text-dialog-complete", cancellationToken);
        return new DialogCompletion<TextDialogPayload>(semantic, payload, "text-dialog-complete");
    }

    private static Window? ResolveOwnerWindow()
    {
        if (Avalonia.Application.Current?.ApplicationLifetime is not IClassicDesktopStyleApplicationLifetime desktop)
        {
            return null;
        }

        return desktop.MainWindow;
    }

    private static Task<UiOperationResult> OpenIssueReportAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        try
        {
            Process.Start(new ProcessStartInfo(IssueReportIssueEntryUrl)
            {
                UseShellExecute = true,
            });
            return Task.FromResult(UiOperationResult.Ok("IssueReport entry opened."));
        }
        catch (Exception ex)
        {
            return Task.FromResult(UiOperationResult.Fail(
                UiErrorCode.PlatformOperationFailed,
                $"Failed to open IssueReport entry: {ex.Message}",
                ex.Message));
        }
    }

    private static string EnsureWorkPackageTitle(string title)
    {
        var trimmed = string.IsNullOrWhiteSpace(title) ? "Dialog" : title.Trim();
        return trimmed.Contains("工作包P", StringComparison.Ordinal)
            ? trimmed
            : $"{WorkPackageTitlePrefix}{trimmed}";
    }
}
