using System.Collections.ObjectModel;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Copilot;

public sealed class CopilotPageViewModel : PageViewModelBase
{
    private string _filePath = string.Empty;
    private int _selectedTypeIndex;
    private bool _autoSquad = true;
    private bool _useSupportUnit;
    private bool _addTrust;
    private bool _overlayEnabled;
    private CopilotItemViewModel? _selectedItem;

    public CopilotPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
        Types = new[] { "主线", "SSS", "悖论", "活动" };
        Items = new ObservableCollection<CopilotItemViewModel>();
        Logs = new ObservableCollection<string>();
    }

    public IReadOnlyList<string> Types { get; }

    public ObservableCollection<CopilotItemViewModel> Items { get; }

    public ObservableCollection<string> Logs { get; }

    public string FilePath
    {
        get => _filePath;
        set => SetProperty(ref _filePath, value);
    }

    public int SelectedTypeIndex
    {
        get => _selectedTypeIndex;
        set => SetProperty(ref _selectedTypeIndex, Math.Clamp(value, 0, Types.Count - 1));
    }

    public bool AutoSquad
    {
        get => _autoSquad;
        set => SetProperty(ref _autoSquad, value);
    }

    public bool UseSupportUnit
    {
        get => _useSupportUnit;
        set => SetProperty(ref _useSupportUnit, value);
    }

    public bool AddTrust
    {
        get => _addTrust;
        set => SetProperty(ref _addTrust, value);
    }

    public bool OverlayEnabled
    {
        get => _overlayEnabled;
        set => SetProperty(ref _overlayEnabled, value);
    }

    public CopilotItemViewModel? SelectedItem
    {
        get => _selectedItem;
        set => SetProperty(ref _selectedItem, value);
    }

    public Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Runtime.DiagnosticsService.RecordEventAsync("Copilot", "Copilot page initialized.", cancellationToken);
    }

    public async Task ImportFromFileAsync(CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(
                await Runtime.CopilotFeatureService.ImportFromFileAsync(FilePath, cancellationToken),
                "Copilot.ImportFile",
                cancellationToken))
        {
            return;
        }

        Items.Add(new CopilotItemViewModel(Path.GetFileName(FilePath), Types[SelectedTypeIndex]));
        SelectedItem = Items.LastOrDefault();
    }

    public async Task ImportFromClipboardAsync(string payload, CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(
                await Runtime.CopilotFeatureService.ImportFromClipboardAsync(payload, cancellationToken),
                "Copilot.ImportClipboard",
                cancellationToken))
        {
            return;
        }

        Items.Add(new CopilotItemViewModel($"Clipboard-{DateTime.Now:HHmmss}", Types[SelectedTypeIndex]));
        SelectedItem = Items.LastOrDefault();
    }

    public Task AddEmptyTaskAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        Items.Add(new CopilotItemViewModel($"Task-{Items.Count + 1}", Types[SelectedTypeIndex]));
        SelectedItem = Items.LastOrDefault();
        StatusMessage = "Added empty copilot item.";
        return Runtime.DiagnosticsService.RecordEventAsync("Copilot", "Added empty task entry.", cancellationToken);
    }

    public Task RemoveSelectedAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (SelectedItem is null)
        {
            LastErrorMessage = "请选择要删除的作业。";
            return Task.CompletedTask;
        }

        Items.Remove(SelectedItem);
        SelectedItem = Items.LastOrDefault();
        StatusMessage = "Selected copilot item removed.";
        return Runtime.DiagnosticsService.RecordEventAsync("Copilot", "Removed selected task entry.", cancellationToken);
    }

    public Task ClearAllAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        Items.Clear();
        SelectedItem = null;
        StatusMessage = "Copilot list cleared.";
        return Runtime.DiagnosticsService.RecordEventAsync("Copilot", "Cleared copilot list.", cancellationToken);
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StartAsync(cancellationToken), "Copilot.Start", cancellationToken))
        {
            return;
        }

        foreach (var item in Items)
        {
            item.Status = "Running";
        }
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StopAsync(cancellationToken), "Copilot.Stop", cancellationToken))
        {
            return;
        }

        foreach (var item in Items.Where(i => i.Status == "Running"))
        {
            item.Status = "Stopped";
        }
    }

    public async Task SendLikeAsync(bool like, CancellationToken cancellationToken = default)
    {
        if (SelectedItem is null)
        {
            LastErrorMessage = "请选择要反馈的作业。";
            return;
        }

        await ApplyResultAsync(
            await Runtime.CopilotFeatureService.SubmitFeedbackAsync(SelectedItem.Name, like, cancellationToken),
            "Copilot.Feedback",
            cancellationToken);
    }
}
