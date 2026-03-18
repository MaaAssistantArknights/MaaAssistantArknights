using MAAUnified.App.ViewModels.Copilot;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Infrastructure;

public enum OverlayLogSource
{
    TaskQueue = 0,
    Copilot = 1,
}

public sealed class OverlayPresentationViewModel : ObservableObject
{
    private const string TaskQueueRunOwner = "TaskQueue";
    private const string CopilotRunOwner = "Copilot";
    private readonly MAAUnifiedRuntime _runtime;
    private readonly TaskQueuePageViewModel _taskQueuePage;
    private readonly CopilotPageViewModel _copilotPage;
    private IEnumerable<TaskQueueLogEntryViewModel> _currentLogs;
    private OverlayLogSource _preferredSource = OverlayLogSource.TaskQueue;
    private OverlayLogSource _resolvedSource = OverlayLogSource.TaskQueue;

    public OverlayPresentationViewModel(
        MAAUnifiedRuntime runtime,
        TaskQueuePageViewModel taskQueuePage,
        CopilotPageViewModel copilotPage)
    {
        _runtime = runtime;
        _taskQueuePage = taskQueuePage;
        _copilotPage = copilotPage;
        _currentLogs = taskQueuePage.OverlayLogs;
        _runtime.SessionService.SessionStateChanged += OnSessionStateChanged;
        RefreshResolvedSource();
    }

    public OverlayLogSource PreferredSource
    {
        get => _preferredSource;
        private set => SetProperty(ref _preferredSource, value);
    }

    public OverlayLogSource ResolvedSource
    {
        get => _resolvedSource;
        private set => SetProperty(ref _resolvedSource, value);
    }

    public IEnumerable<TaskQueueLogEntryViewModel> CurrentLogs
    {
        get => _currentLogs;
        private set => SetProperty(ref _currentLogs, value);
    }

    public void PreferTaskQueue()
    {
        SetPreferredSource(OverlayLogSource.TaskQueue);
    }

    public void PreferCopilot()
    {
        SetPreferredSource(OverlayLogSource.Copilot);
    }

    public void RefreshResolvedSource()
    {
        var nextSource = ResolveCurrentSource();
        ResolvedSource = nextSource;
        CurrentLogs = nextSource == OverlayLogSource.Copilot
            ? _copilotPage.Logs
            : _taskQueuePage.OverlayLogs;
    }

    private void SetPreferredSource(OverlayLogSource source)
    {
        PreferredSource = source;
        RefreshResolvedSource();
    }

    private OverlayLogSource ResolveCurrentSource()
    {
        var currentOwner = _runtime.SessionService.CurrentRunOwner;
        if (string.Equals(currentOwner, CopilotRunOwner, StringComparison.Ordinal))
        {
            return OverlayLogSource.Copilot;
        }

        if (string.Equals(currentOwner, TaskQueueRunOwner, StringComparison.Ordinal))
        {
            return OverlayLogSource.TaskQueue;
        }

        return PreferredSource;
    }

    private void OnSessionStateChanged(SessionState _)
    {
        RefreshResolvedSource();
    }
}
