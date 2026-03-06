using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Text.Json;
using System.Text.Json.Nodes;
using Avalonia.Threading;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.CoreBridge;
using MAAUnified.Compat.Mapping;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class TaskQueuePageViewModel : PageViewModelBase
{
    private const string TaskQueueRunOwner = "TaskQueue";
    private readonly SemaphoreSlim _taskBindingLock = new(1, 1);
    private readonly SemaphoreSlim _queueMutationLock = new(1, 1);
    private readonly SemaphoreSlim _runTransitionLock = new(1, 1);
    private readonly object _pendingBindingGate = new();
    private readonly ConnectionGameSharedStateViewModel _connectionGameSharedState;
    private readonly Action<LocalizationFallbackInfo>? _localizationFallbackReporter;
    private readonly IAppDialogService _dialogService;
    private Task _pendingBindingTask = Task.CompletedTask;
    private CancellationTokenSource? _pendingBindingCts;
    private int _pendingBindingVersion;
    private bool _suppressTaskEnabledSync;
    private SessionState _currentSessionState;
    private bool _hasBlockingConfigIssues;
    private int _blockingConfigIssueCount;
    private bool _autoReload;
    private bool _showAdvanced;
    private bool _isWaitingForStop;
    private string _dailyStageHint = string.Empty;
    private string _selectedTaskModule = TaskModuleTypes.StartUp;
    private string _newTaskName = string.Empty;
    private string _renameTargetName = string.Empty;
    private string _overlayStatusText = string.Empty;
    private OverlayTarget? _selectedOverlayTarget = new("preview", "Preview + Logs", true);
    private bool _overlayVisible;
    private string _currentRunId = "-";
    private string _lastPostActionRunId = string.Empty;
    private string _selectedTaskValidationSummary = string.Empty;
    private bool _selectedTaskHasBlockingValidationIssues;
    private int _selectedTaskValidationIssueCount;
    private string _startPrecheckWarningMessage = string.Empty;
    private TaskRuntimeStatusSnapshot? _lastRuntimeStatus;
    private TaskQueueItemViewModel? _selectedTask;

    public TaskQueuePageViewModel(
        MAAUnifiedRuntime runtime,
        ConnectionGameSharedStateViewModel connectionGameSharedState,
        Action<LocalizationFallbackInfo>? localizationFallbackReporter = null,
        IAppDialogService? dialogService = null)
        : base(runtime)
    {
        _connectionGameSharedState = connectionGameSharedState;
        _localizationFallbackReporter = localizationFallbackReporter;
        _dialogService = dialogService ?? NoOpAppDialogService.Instance;
        TaskModules = new ObservableCollection<string>(WpfFeatureBaseline.TaskModules);
        Tasks = new ObservableCollection<TaskQueueItemViewModel>();
        Logs = new ObservableCollection<string>();
        OverlayTargets = new ObservableCollection<OverlayTarget>();

        Texts = new LocalizedTextMap
        {
            Language = ResolveLanguage(),
        };
        _dailyStageHint = Texts.GetOrDefault("TaskQueue.DailyStageHintDefault", "Daily stage hints will be shown after resources are loaded.");
        _overlayStatusText = Texts.GetOrDefault("TaskQueue.OverlayDisconnected", "Overlay disconnected");
        StartUpModule = new StartUpTaskModuleViewModel(runtime, Texts, _connectionGameSharedState);
        FightModule = new FightTaskModuleViewModel(runtime, Texts);
        RecruitModule = new RecruitTaskModuleViewModel(runtime, Texts);
        InfrastModule = new InfrastModuleViewModel(runtime, Texts);
        MallModule = new MallModuleViewModel(runtime, Texts);
        AwardModule = new AwardModuleViewModel(runtime, Texts);
        RoguelikeModule = new RoguelikeModuleViewModel(runtime, Texts);
        ReclamationModule = new ReclamationModuleViewModel(runtime, Texts);
        CustomModule = new CustomModuleViewModel(runtime, Texts);
        PostActionModule = new PostActionModuleViewModel(runtime, Texts);

        runtime.LogService.LogReceived += log =>
        {
            Dispatcher.UIThread.Post(() =>
            {
                Logs.Add($"[{log.Timestamp:HH:mm:ss}] {log.Level} {log.Message}");
                const int maxLogs = 300;
                while (Logs.Count > maxLogs)
                {
                    Logs.RemoveAt(0);
                }
            });
        };

        runtime.SessionService.CallbackReceived += callback => _ = HandleCallbackAsync(callback);
        runtime.SessionService.SessionStateChanged += OnSessionStateChanged;
        runtime.ConfigurationService.ConfigChanged += _ =>
        {
            Dispatcher.UIThread.Post(() =>
            {
                SetLanguage(ResolveLanguage());
                RefreshConfigValidationState(runtime.ConfigurationService.CurrentValidationIssues);
            });
        };
        _currentSessionState = runtime.SessionService.CurrentState;
    }

    public ObservableCollection<string> TaskModules { get; }

    public ObservableCollection<TaskQueueItemViewModel> Tasks { get; }

    public ObservableCollection<string> Logs { get; }

    public ObservableCollection<OverlayTarget> OverlayTargets { get; }

    public LocalizedTextMap Texts { get; }

    public StartUpTaskModuleViewModel StartUpModule { get; }

    public FightTaskModuleViewModel FightModule { get; }

    public RecruitTaskModuleViewModel RecruitModule { get; }

    public InfrastModuleViewModel InfrastModule { get; }

    public MallModuleViewModel MallModule { get; }

    public AwardModuleViewModel AwardModule { get; }

    public RoguelikeModuleViewModel RoguelikeModule { get; }

    public ReclamationModuleViewModel ReclamationModule { get; }

    public CustomModuleViewModel CustomModule { get; }

    public PostActionModuleViewModel PostActionModule { get; }

    public TaskRuntimeStatusSnapshot? LastRuntimeStatus
    {
        get => _lastRuntimeStatus;
        private set => SetProperty(ref _lastRuntimeStatus, value);
    }

    public string SelectedTaskValidationSummary
    {
        get => _selectedTaskValidationSummary;
        private set => SetProperty(ref _selectedTaskValidationSummary, value);
    }

    public bool SelectedTaskHasBlockingValidationIssues
    {
        get => _selectedTaskHasBlockingValidationIssues;
        private set => SetProperty(ref _selectedTaskHasBlockingValidationIssues, value);
    }

    public int SelectedTaskValidationIssueCount
    {
        get => _selectedTaskValidationIssueCount;
        private set => SetProperty(ref _selectedTaskValidationIssueCount, value);
    }

    public string StartPrecheckWarningMessage
    {
        get => _startPrecheckWarningMessage;
        private set
        {
            if (SetProperty(ref _startPrecheckWarningMessage, value))
            {
                OnPropertyChanged(nameof(HasStartPrecheckWarningMessage));
            }
        }
    }

    public bool HasStartPrecheckWarningMessage => !string.IsNullOrWhiteSpace(StartPrecheckWarningMessage);

    public TaskQueueItemViewModel? SelectedTask
    {
        get => _selectedTask;
        set
        {
            if (!SetProperty(ref _selectedTask, value))
            {
                return;
            }

            RenameTargetName = value?.Name ?? string.Empty;
            ScheduleBindSelectedTask();
        }
    }

    public string SelectedTaskModule
    {
        get => _selectedTaskModule;
        set => SetProperty(ref _selectedTaskModule, value);
    }

    public string NewTaskName
    {
        get => _newTaskName;
        set => SetProperty(ref _newTaskName, value);
    }

    public string RenameTargetName
    {
        get => _renameTargetName;
        set => SetProperty(ref _renameTargetName, value);
    }

    public SessionState CurrentSessionState
    {
        get => _currentSessionState;
        private set
        {
            if (SetProperty(ref _currentSessionState, value))
            {
                OnPropertyChanged(nameof(IsRunning));
                OnPropertyChanged(nameof(CanEdit));
                OnPropertyChanged(nameof(RunButtonText));
                OnPropertyChanged(nameof(CanToggleRun));
                OnPropertyChanged(nameof(CanWaitAndStop));
            }
        }
    }

    public bool IsRunning => CurrentSessionState is SessionState.Running or SessionState.Stopping;

    public bool CanEdit => !IsRunning;

    public string RunButtonText => IsRunning ? "Stop" : "LinkStart";

    public bool HasBlockingConfigIssues
    {
        get => _hasBlockingConfigIssues;
        private set
        {
            if (SetProperty(ref _hasBlockingConfigIssues, value))
            {
                OnPropertyChanged(nameof(CanToggleRun));
            }
        }
    }

    public int BlockingConfigIssueCount
    {
        get => _blockingConfigIssueCount;
        private set => SetProperty(ref _blockingConfigIssueCount, value);
    }

    public bool CanToggleRun =>
        !IsWaitingForStop
        && (CurrentSessionState == SessionState.Running
            || (CurrentSessionState == SessionState.Connected && !HasBlockingConfigIssues));

    public bool CanWaitAndStop =>
        !IsWaitingForStop
        && CurrentSessionState == SessionState.Running;

    public bool IsWaitingForStop
    {
        get => _isWaitingForStop;
        private set
        {
            if (SetProperty(ref _isWaitingForStop, value))
            {
                OnPropertyChanged(nameof(CanToggleRun));
                OnPropertyChanged(nameof(CanWaitAndStop));
                OnPropertyChanged(nameof(WaitAndStopButtonText));
            }
        }
    }

    public string WaitAndStopButtonText => IsWaitingForStop ? "Waiting..." : "WaitAndStop";

    public bool AutoReload
    {
        get => _autoReload;
        set => SetProperty(ref _autoReload, value);
    }

    public bool ShowAdvanced
    {
        get => _showAdvanced;
        set => SetProperty(ref _showAdvanced, value);
    }

    public string DailyStageHint
    {
        get => _dailyStageHint;
        set => SetProperty(ref _dailyStageHint, value);
    }

    public string OverlayStatusText
    {
        get => _overlayStatusText;
        set => SetProperty(ref _overlayStatusText, value);
    }

    public OverlayTarget? SelectedOverlayTarget
    {
        get => _selectedOverlayTarget;
        set => SetProperty(ref _selectedOverlayTarget, value);
    }

    public bool OverlayVisible
    {
        get => _overlayVisible;
        set => SetProperty(ref _overlayVisible, value);
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        SetLanguage(ResolveLanguage());
        RefreshConfigValidationState(Runtime.ConfigurationService.CurrentValidationIssues);
        await ReloadTasksAsync(cancellationToken);
        await ReloadOverlayTargetsAsync(cancellationToken);
        await PostActionModule.InitializeAsync(cancellationToken);
    }

    public void SetLanguage(string language)
    {
        Texts.Language = UiLanguageCatalog.Normalize(language);
        DailyStageHint = Texts.GetOrDefault(
            "TaskQueue.DailyStageHintDefault",
            "Daily stage hints will be shown after resources are loaded.");
        OverlayStatusText = Texts.GetOrDefault("TaskQueue.OverlayDisconnected", "Overlay disconnected");
        _ = RefreshOverlayStatusTextAsync();
    }

    public async Task ReloadTasksAsync(CancellationToken cancellationToken = default)
    {
        var previousSelectedIndex = SelectedTask is null ? -1 : Tasks.IndexOf(SelectedTask);
        var tasks = await ApplyResultAsync(
            await Runtime.TaskQueueFeatureService.GetCurrentTaskQueueAsync(cancellationToken),
            "TaskQueue.Reload",
            cancellationToken);

        if (tasks is null)
        {
            return;
        }

        foreach (var task in Tasks)
        {
            task.PropertyChanged -= OnTaskPropertyChanged;
        }

        _suppressTaskEnabledSync = true;
        try
        {
            Tasks.Clear();
            foreach (var task in tasks)
            {
                var item = TaskQueueItemViewModel.FromUnifiedTask(task);
                item.PropertyChanged += OnTaskPropertyChanged;
                Tasks.Add(item);
            }
        }
        finally
        {
            _suppressTaskEnabledSync = false;
        }

        if (previousSelectedIndex >= 0 && previousSelectedIndex < Tasks.Count)
        {
            SelectedTask = Tasks[previousSelectedIndex];
        }
        else
        {
            SelectedTask = Tasks.FirstOrDefault();
        }

        await WaitForPendingBindingAsync(cancellationToken);
    }

    public async Task WaitForPendingBindingAsync(CancellationToken cancellationToken = default)
    {
        while (true)
        {
            Task pending;
            lock (_pendingBindingGate)
            {
                pending = _pendingBindingTask;
            }

            await pending.WaitAsync(cancellationToken);

            lock (_pendingBindingGate)
            {
                if (ReferenceEquals(pending, _pendingBindingTask))
                {
                    return;
                }
            }
        }
    }

    private async Task<bool> ExecuteQueueMutationAsync(
        string scope,
        Func<CancellationToken, Task<UiOperationResult>> mutationAsync,
        Func<CancellationToken, Task>? onSuccessAsync = null,
        CancellationToken cancellationToken = default)
    {
        if (!await EnsureEditableAsync(scope, cancellationToken))
        {
            return false;
        }

        await _queueMutationLock.WaitAsync(cancellationToken);
        try
        {
            await WaitForPendingBindingAsync(cancellationToken);
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return false;
            }

            var result = await mutationAsync(cancellationToken);
            if (!await ApplyResultAsync(result, scope, cancellationToken))
            {
                return false;
            }

            await ReloadTasksAsync(cancellationToken);
            await WaitForPendingBindingAsync(cancellationToken);

            if (onSuccessAsync is not null)
            {
                await onSuccessAsync(cancellationToken);
                await WaitForPendingBindingAsync(cancellationToken);
            }

            return true;
        }
        finally
        {
            _queueMutationLock.Release();
        }
    }

    private async Task SelectTaskByIndexAsync(int index, CancellationToken cancellationToken)
    {
        if (index < 0 || index >= Tasks.Count)
        {
            return;
        }

        SelectedTask = Tasks[index];
        await WaitForPendingBindingAsync(cancellationToken);
    }

    private async void OnTaskPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!string.Equals(e.PropertyName, nameof(TaskQueueItemViewModel.IsEnabled), StringComparison.Ordinal))
        {
            return;
        }

        if (sender is not TaskQueueItemViewModel task || _suppressTaskEnabledSync)
        {
            return;
        }

        try
        {
            await PersistTaskEnabledStateAsync(task);
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "TaskQueue.SetTaskEnabled",
                ex,
                UiErrorCode.UiOperationFailed,
                "Failed to persist task enabled state.");
        }
    }

    private async Task PersistTaskEnabledStateAsync(TaskQueueItemViewModel task, CancellationToken cancellationToken = default)
    {
        var desiredEnabled = task.IsEnabled;
        if (!await EnsureEditableAsync("TaskQueue.SetTaskEnabled", cancellationToken))
        {
            _suppressTaskEnabledSync = true;
            try
            {
                task.IsEnabled = !desiredEnabled;
            }
            finally
            {
                _suppressTaskEnabledSync = false;
            }

            return;
        }

        var index = Tasks.IndexOf(task);
        if (index < 0)
        {
            return;
        }

        await _queueMutationLock.WaitAsync(cancellationToken);
        try
        {
            await WaitForPendingBindingAsync(cancellationToken);
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return;
            }

            var result = await Runtime.TaskQueueFeatureService.SetTaskEnabledAsync(index, desiredEnabled, cancellationToken);
            if (!await ApplyResultAsync(result, "TaskQueue.SetTaskEnabled", cancellationToken))
            {
                _suppressTaskEnabledSync = true;
                try
                {
                    task.IsEnabled = !desiredEnabled;
                }
                finally
                {
                    _suppressTaskEnabledSync = false;
                }
            }
        }
        finally
        {
            _queueMutationLock.Release();
        }
    }

    public async Task AddTaskAsync(CancellationToken cancellationToken = default)
    {
        var taskType = SelectedTaskModule;
        var taskName = string.IsNullOrWhiteSpace(NewTaskName) ? taskType : NewTaskName.Trim();
        await ExecuteQueueMutationAsync(
            "TaskQueue.AddTask",
            ct => Runtime.TaskQueueFeatureService.AddTaskAsync(taskType, taskName, true, ct),
            _ =>
            {
                NewTaskName = string.Empty;
                return Task.CompletedTask;
            },
            cancellationToken);
    }

    public async Task RemoveSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRemove"];
            return;
        }

        var index = Tasks.IndexOf(SelectedTask);
        if (index < 0)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRemove"];
            return;
        }

        await ExecuteQueueMutationAsync(
            "TaskQueue.RemoveTask",
            ct => Runtime.TaskQueueFeatureService.RemoveTaskAsync(index, ct),
            cancellationToken: cancellationToken);
    }

    public async Task RenameSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        var index = Tasks.IndexOf(SelectedTask);
        if (index < 0)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        await ExecuteQueueMutationAsync(
            "TaskQueue.RenameTask",
            ct => Runtime.TaskQueueFeatureService.RenameTaskAsync(index, RenameTargetName, ct),
            cancellationToken: cancellationToken);
    }

    public async Task RenameSelectedTaskWithDialogAsync(CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        var index = Tasks.IndexOf(SelectedTask);
        if (index < 0)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        var request = new TextDialogRequest(
            Title: $"Rename Task {index + 1}",
            Prompt: Texts.GetOrDefault("TaskQueue.Error.SelectTaskToRename", "Rename task"),
            DefaultText: SelectedTask.Name,
            MultiLine: false,
            ConfirmText: "Confirm",
            CancelText: "Cancel");
        var dialogResult = await _dialogService.ShowTextAsync(request, "TaskQueue.RenameTask.Dialog", cancellationToken);
        if (dialogResult.Return == DialogReturnSemantic.Confirm && dialogResult.Payload is not null)
        {
            var nextName = (dialogResult.Payload.Text ?? string.Empty).Trim();
            if (nextName.Length == 0)
            {
                LastErrorMessage = "Task name cannot be empty.";
                return;
            }

            RenameTargetName = nextName;
            await RenameSelectedTaskAsync(cancellationToken);
            return;
        }

        StatusMessage = dialogResult.Return == DialogReturnSemantic.Cancel
            ? "Rename cancelled."
            : "Rename dialog closed.";
    }

    public async Task MoveSelectedTaskAsync(int delta, CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToMove"];
            return;
        }

        var from = Tasks.IndexOf(SelectedTask);
        var to = Math.Clamp(from + delta, 0, Tasks.Count - 1);
        if (from < 0 || from == to)
        {
            return;
        }

        await ExecuteQueueMutationAsync(
            "TaskQueue.MoveTask",
            ct => Runtime.TaskQueueFeatureService.MoveTaskAsync(from, to, ct),
            ct => SelectTaskByIndexAsync(to, ct),
            cancellationToken);
    }

    public async Task SelectAllAsync(bool enabled, CancellationToken cancellationToken = default)
    {
        await ExecuteQueueMutationAsync(
            "TaskQueue.SelectAll",
            ct => Runtime.TaskQueueFeatureService.SetAllTasksEnabledAsync(enabled, ct),
            cancellationToken: cancellationToken);
    }

    public async Task InverseSelectionAsync(CancellationToken cancellationToken = default)
    {
        await ExecuteQueueMutationAsync(
            "TaskQueue.InverseSelection",
            ct => Runtime.TaskQueueFeatureService.InvertTasksEnabledAsync(ct),
            cancellationToken: cancellationToken);
    }

    public async Task SaveAsync(CancellationToken cancellationToken = default)
    {
        if (!await EnsureEditableAsync("TaskQueue.Save", cancellationToken))
        {
            return;
        }

        if (!await SaveBoundTaskModulesAsync(cancellationToken))
        {
            return;
        }

        await ApplyResultAsync(await Runtime.TaskQueueFeatureService.SaveAsync(cancellationToken), "TaskQueue.Save", cancellationToken);
    }

    public async Task ToggleRunAsync(CancellationToken cancellationToken = default)
    {
        CurrentSessionState = Runtime.SessionService.CurrentState;
        if (CurrentSessionState == SessionState.Running)
        {
            await StopAsync(cancellationToken);
            return;
        }

        if (CurrentSessionState != SessionState.Connected)
        {
            LastErrorMessage = $"Session state `{CurrentSessionState}` does not allow LinkStart.";
            await RecordFailedResultAsync(
                "TaskQueue.ToggleRun",
                UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, LastErrorMessage),
                cancellationToken);
            return;
        }

        await StartAsync(cancellationToken);
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        await _runTransitionLock.WaitAsync(cancellationToken);
        try
        {
            CurrentSessionState = Runtime.SessionService.CurrentState;
            if (CurrentSessionState != SessionState.Connected)
            {
                LastErrorMessage = $"Session state `{CurrentSessionState}` does not allow LinkStart.";
                await RecordFailedResultAsync(
                    "TaskQueue.Start",
                    UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, LastErrorMessage),
                    cancellationToken);
                return;
            }

            if (!Runtime.SessionService.TryBeginRun(TaskQueueRunOwner, out var currentOwner))
            {
                var owner = string.IsNullOrWhiteSpace(currentOwner) ? "Unknown" : currentOwner;
                var message = $"TaskQueue start blocked by active run owner `{owner}`.";
                LastErrorMessage = message;
                await RecordFailedResultAsync(
                    "TaskQueue.Start.RunOwner",
                    UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, message),
                    cancellationToken);
                return;
            }

            var keepRunOwner = false;
            try
            {
                await WaitForPendingBindingAsync(cancellationToken);
                if (!await SaveBoundTaskModulesAsync(cancellationToken))
                {
                    return;
                }

                var precheckWarnings = await Runtime.TaskQueueFeatureService.GetStartPrecheckWarningsAsync(cancellationToken);
                if (!precheckWarnings.Success)
                {
                    _ = await ApplyResultAsync(precheckWarnings, "TaskQueue.Start.Precheck", cancellationToken);
                    return;
                }

                var warnings = precheckWarnings.Value ?? [];
                if (warnings.Count > 0)
                {
                    StartPrecheckWarningMessage = string.Join(
                        " ",
                        warnings.Select(static warning => warning.Message));
                    await RecordEventAsync(
                        "TaskQueue.Start.PrecheckWarning",
                        StartPrecheckWarningMessage,
                        cancellationToken);
                }
                else
                {
                    StartPrecheckWarningMessage = string.Empty;
                }

                if (warnings.Any(static warning => string.Equals(
                        warning.Code,
                        UiErrorCode.MallCreditFightDowngraded,
                        StringComparison.Ordinal)))
                {
                    var downgradeResult = await Runtime.TaskQueueFeatureService.ApplyStartPrecheckDowngradesAsync(cancellationToken);
                    if (!downgradeResult.Success)
                    {
                        _ = await ApplyResultAsync(downgradeResult, "TaskQueue.Start.PrecheckApply", cancellationToken);
                        return;
                    }
                }

                if (!await ValidateEnabledTasksBeforeStartAsync(cancellationToken))
                {
                    return;
                }

                RefreshConfigValidationState(Runtime.ConfigurationService.RevalidateCurrentConfig());
                if (HasBlockingConfigIssues)
                {
                    var first = Runtime.ConfigurationService.CurrentValidationIssues.FirstOrDefault(i => i.Blocking);
                    LastErrorMessage = first is null
                        ? "Config validation has blocking issues."
                        : $"{first.Scope}:{first.Code}:{first.Field}:{first.Message}";
                    await RecordConfigValidationFailureAsync(first, cancellationToken);
                    return;
                }

                var appendResult = await Runtime.TaskQueueFeatureService.QueueEnabledTasksAsync(cancellationToken);
                if (!appendResult.Success)
                {
                    var error = UiOperationResult<int>.FromCore(appendResult, "Tasks queued.");
                    _ = await ApplyResultAsync(error, "TaskQueue.Append", cancellationToken);
                    return;
                }

                if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StartAsync(cancellationToken), "TaskQueue.Start", cancellationToken))
                {
                    return;
                }

                CurrentSessionState = Runtime.SessionService.CurrentState;
                _currentRunId = Guid.NewGuid().ToString("N");
                _lastPostActionRunId = string.Empty;
                keepRunOwner = true;
            }
            finally
            {
                if (!keepRunOwner)
                {
                    Runtime.SessionService.EndRun(TaskQueueRunOwner);
                }
            }
        }
        finally
        {
            _runTransitionLock.Release();
        }
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        await _runTransitionLock.WaitAsync(cancellationToken);
        try
        {
            CurrentSessionState = Runtime.SessionService.CurrentState;
            if (CurrentSessionState != SessionState.Running)
            {
                LastErrorMessage = $"Session state `{CurrentSessionState}` is already non-running.";
                await RecordFailedResultAsync(
                    "TaskQueue.Stop",
                    UiOperationResult.Fail(UiErrorCode.OperationAlreadyStopped, LastErrorMessage),
                    cancellationToken);
                SyncStoppedUiStateIfSessionNotActive();

                return;
            }

            var currentOwner = Runtime.SessionService.CurrentRunOwner;
            if (!string.IsNullOrWhiteSpace(currentOwner) && !Runtime.SessionService.IsRunOwner(TaskQueueRunOwner))
            {
                var owner = currentOwner;
                LastErrorMessage = $"TaskQueue stop blocked by active run owner `{owner}`.";
                await RecordFailedResultAsync(
                    "TaskQueue.Stop.RunOwner",
                    UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, LastErrorMessage),
                    cancellationToken);
                return;
            }

            await WaitForPendingBindingAsync(cancellationToken);
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return;
            }

            if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StopAsync(cancellationToken), "TaskQueue.Stop", cancellationToken))
            {
                CurrentSessionState = Runtime.SessionService.CurrentState;
                SyncStoppedUiStateIfSessionNotActive();

                return;
            }

            CurrentSessionState = Runtime.SessionService.CurrentState;
            SyncStoppedUiStateIfSessionNotActive();
        }
        finally
        {
            _runTransitionLock.Release();
        }
    }

    public async Task WaitAndStopAsync(CancellationToken cancellationToken = default)
    {
        await _runTransitionLock.WaitAsync(cancellationToken);
        try
        {
            if (IsWaitingForStop)
            {
                LastErrorMessage = "WaitAndStop is already in progress.";
                await RecordFailedResultAsync(
                    "TaskQueue.WaitAndStop",
                    UiOperationResult.Fail(UiErrorCode.OperationAlreadyRunning, LastErrorMessage),
                    cancellationToken);
                return;
            }

            CurrentSessionState = Runtime.SessionService.CurrentState;
            if (CurrentSessionState != SessionState.Running)
            {
                LastErrorMessage = $"Session state `{CurrentSessionState}` is already non-running.";
                await RecordFailedResultAsync(
                    "TaskQueue.WaitAndStop",
                    UiOperationResult.Fail(UiErrorCode.OperationAlreadyStopped, LastErrorMessage),
                    cancellationToken);
                SyncStoppedUiStateIfSessionNotActive();

                return;
            }

            var currentOwner = Runtime.SessionService.CurrentRunOwner;
            if (!string.IsNullOrWhiteSpace(currentOwner) && !Runtime.SessionService.IsRunOwner(TaskQueueRunOwner))
            {
                var owner = currentOwner;
                LastErrorMessage = $"TaskQueue wait-stop blocked by active run owner `{owner}`.";
                await RecordFailedResultAsync(
                    "TaskQueue.WaitAndStop.RunOwner",
                    UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, LastErrorMessage),
                    cancellationToken);
                return;
            }

            IsWaitingForStop = true;
            if (!await ApplyResultAsync(
                    await Runtime.ConnectFeatureService.WaitAndStopAsync(TimeSpan.FromSeconds(15), cancellationToken),
                    "TaskQueue.WaitAndStop",
                    cancellationToken))
            {
                CurrentSessionState = Runtime.SessionService.CurrentState;
                SyncStoppedUiStateIfSessionNotActive();

                return;
            }

            CurrentSessionState = Runtime.SessionService.CurrentState;
            SyncStoppedUiStateIfSessionNotActive();
        }
        finally
        {
            IsWaitingForStop = false;
            _runTransitionLock.Release();
        }
    }

    public async Task ReloadOverlayTargetsAsync(CancellationToken cancellationToken = default)
    {
        var targets = await ApplyResultAsync(
            await Runtime.OverlayFeatureService.GetOverlayTargetsAsync(cancellationToken),
            "Overlay.QueryTargets",
            cancellationToken);

        if (targets is null)
        {
            return;
        }

        OverlayTargets.Clear();
        foreach (var target in targets)
        {
            OverlayTargets.Add(target);
        }

        if (OverlayTargets.Count > 0)
        {
            SelectedOverlayTarget = OverlayTargets.FirstOrDefault(t => t.IsPrimary) ?? OverlayTargets[0];
        }

        var snapshotResult = await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
        if (snapshotResult.Success && snapshotResult.Value is not null)
        {
            OverlayStatusText = BuildCapabilityLine(PlatformCapabilityId.Overlay, snapshotResult.Value.Overlay);
        }
        else
        {
            OverlayStatusText = PlatformCapabilityTextMap.FormatSnapshotUnavailable(
                Texts.Language,
                snapshotResult.Message,
                _localizationFallbackReporter);
        }
    }

    public async Task PickOverlayTargetWithDialogAsync(CancellationToken cancellationToken = default)
    {
        if (OverlayTargets.Count == 0)
        {
            await ReloadOverlayTargetsAsync(cancellationToken);
        }

        if (OverlayTargets.Count == 0)
        {
            LastErrorMessage = "No overlay target is available.";
            return;
        }

        var request = new ProcessPickerDialogRequest(
            Title: "Overlay Target Picker",
            Items: OverlayTargets.Select(t => new ProcessPickerItem(t.Id, t.DisplayName, t.IsPrimary)).ToArray(),
            SelectedId: SelectedOverlayTarget?.Id,
            ConfirmText: "Select",
            CancelText: "Cancel");
        var dialogResult = await _dialogService.ShowProcessPickerAsync(request, "TaskQueue.Overlay.PickTarget", cancellationToken);
        if (dialogResult.Return != DialogReturnSemantic.Confirm || dialogResult.Payload is null)
        {
            StatusMessage = dialogResult.Return == DialogReturnSemantic.Cancel
                ? "Overlay target selection cancelled."
                : "Overlay target picker closed.";
            return;
        }

        await ApplyOverlayTargetAsync(dialogResult.Payload.SelectedId, cancellationToken);
    }

    public async Task ApplyOverlayTargetAsync(string targetId, CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(targetId))
        {
            LastErrorMessage = "Overlay target id is missing.";
            return;
        }

        var targetResult = await Runtime.OverlayFeatureService.SelectOverlayTargetAsync(targetId, cancellationToken);
        if (!await ApplyResultAsync(targetResult, "Overlay.Select", cancellationToken))
        {
            return;
        }

        SelectedOverlayTarget = OverlayTargets.FirstOrDefault(t => string.Equals(t.Id, targetId, StringComparison.Ordinal))
                                ?? SelectedOverlayTarget;
    }

    public async Task ToggleOverlayAsync(CancellationToken cancellationToken = default)
    {
        var targetResult = await Runtime.OverlayFeatureService.SelectOverlayTargetAsync(SelectedOverlayTarget?.Id ?? "preview", cancellationToken);
        await ApplyResultAsync(targetResult, "Overlay.Select", cancellationToken);

        OverlayVisible = !OverlayVisible;
        var visibleResult = await Runtime.OverlayFeatureService.ToggleOverlayVisibilityAsync(OverlayVisible, cancellationToken);
        await ApplyResultAsync(visibleResult, "Overlay.Toggle", cancellationToken);
    }

    private string BuildCapabilityLine(PlatformCapabilityId capability, PlatformCapabilityStatus status)
    {
        return PlatformCapabilityTextMap.FormatCapabilityLine(
            Texts.Language,
            capability,
            status,
            _localizationFallbackReporter);
    }

    private void RefreshConfigValidationState(IReadOnlyList<ConfigValidationIssue> issues)
    {
        BlockingConfigIssueCount = issues.Count(i => i.Blocking);
        HasBlockingConfigIssues = BlockingConfigIssueCount > 0;
    }

    private void ResetSelectedTaskValidationSummary()
    {
        SelectedTaskValidationIssueCount = 0;
        SelectedTaskHasBlockingValidationIssues = false;
        SelectedTaskValidationSummary = string.Empty;
    }

    private async Task RefreshSelectedTaskValidationSummaryAsync(int index, CancellationToken cancellationToken)
    {
        var result = await Runtime.TaskQueueFeatureService.ValidateTaskAsync(index, cancellationToken);
        if (!result.Success || result.Value is null)
        {
            SelectedTaskValidationIssueCount = 0;
            SelectedTaskHasBlockingValidationIssues = false;
            SelectedTaskValidationSummary = Texts.GetOrDefault(
                "TaskQueue.Validation.LoadFailed",
                "Failed to load validation report.");
            LastErrorMessage = result.Message;
            return;
        }

        UpdateSelectedTaskValidationSummary(result.Value);
    }

    private void UpdateSelectedTaskValidationSummary(TaskValidationReport report)
    {
        SelectedTaskValidationIssueCount = report.Issues.Count;
        SelectedTaskHasBlockingValidationIssues = report.HasBlockingIssues;

        if (report.Issues.Count == 0)
        {
            SelectedTaskValidationSummary = Texts.GetOrDefault(
                "TaskQueue.Validation.Clean",
                "Validation passed.");
            return;
        }

        if (report.HasBlockingIssues)
        {
            var blockingCount = report.Issues.Count(i => i.Blocking);
            SelectedTaskValidationSummary = string.Format(
                Texts.GetOrDefault("TaskQueue.Validation.BlockingCount", "{0} blocking issue(s)."),
                blockingCount);
            return;
        }

        SelectedTaskValidationSummary = string.Format(
            Texts.GetOrDefault("TaskQueue.Validation.WarningCount", "{0} warning issue(s)."),
            report.Issues.Count);
    }

    private async Task<bool> ValidateEnabledTasksBeforeStartAsync(CancellationToken cancellationToken)
    {
        for (var index = 0; index < Tasks.Count; index++)
        {
            if (!Tasks[index].IsEnabled)
            {
                continue;
            }

            var result = await Runtime.TaskQueueFeatureService.ValidateTaskAsync(index, cancellationToken);
            if (!result.Success || result.Value is null)
            {
                LastErrorMessage = result.Message;
                await RecordFailedResultAsync(
                    "TaskQueue.ValidateTask",
                    UiOperationResult.Fail(
                        result.Error?.Code ?? UiErrorCode.TaskValidationFailed,
                        result.Message,
                        result.Error?.Details),
                    cancellationToken);
                return false;
            }

            var report = result.Value;
            if (SelectedTask is not null && Tasks.IndexOf(SelectedTask) == index)
            {
                UpdateSelectedTaskValidationSummary(report);
            }

            if (!report.HasBlockingIssues)
            {
                continue;
            }

            var firstBlocking = report.Issues.First(i => i.Blocking);
            var issueDetail = $"{firstBlocking.Code}:{firstBlocking.Field}:{firstBlocking.Message}";
            LastErrorMessage = string.Format(
                Texts.GetOrDefault(
                    "TaskQueue.Error.BlockingValidation",
                    "Task `{0}` blocked by validation: {1}"),
                report.TaskName,
                issueDetail);
            await RecordFailedResultAsync(
                "TaskQueue.ValidateTask",
                UiOperationResult.Fail(
                    UiErrorCode.TaskValidationFailed,
                    LastErrorMessage,
                    issueDetail),
                cancellationToken);
            return false;
        }

        return true;
    }

    private Task BindSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        int expectedVersion;
        lock (_pendingBindingGate)
        {
            expectedVersion = _pendingBindingVersion;
        }

        return BindSelectedTaskCoreAsync(expectedVersion, cancellationToken);
    }

    private async Task BindSelectedTaskCoreAsync(int expectedVersion, CancellationToken cancellationToken = default)
    {
        await _taskBindingLock.WaitAsync(cancellationToken);
        try
        {
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return;
            }

            if (IsBindingStale(expectedVersion, cancellationToken))
            {
                return;
            }

            if (SelectedTask is null)
            {
                ClearTaskModuleBindings();
                ResetSelectedTaskValidationSummary();
                return;
            }

            var index = Tasks.IndexOf(SelectedTask);
            if (index < 0)
            {
                ResetSelectedTaskValidationSummary();
                return;
            }

            var moduleType = TaskModuleTypes.Normalize(SelectedTask.Type);
            if (string.Equals(moduleType, TaskModuleTypes.StartUp, StringComparison.OrdinalIgnoreCase))
            {
                await StartUpModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                ReclamationModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Fight, StringComparison.OrdinalIgnoreCase))
            {
                await FightModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                ReclamationModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Recruit, StringComparison.OrdinalIgnoreCase))
            {
                await RecruitModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                ReclamationModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Roguelike, StringComparison.OrdinalIgnoreCase))
            {
                await RoguelikeModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                ReclamationModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Reclamation, StringComparison.OrdinalIgnoreCase))
            {
                await ReclamationModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                CustomModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Custom, StringComparison.OrdinalIgnoreCase))
            {
                await CustomModule.BindAsync(index, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }

                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                RoguelikeModule.ClearBinding();
                ReclamationModule.ClearBinding();
                await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
                return;
            }

            StartUpModule.ClearBinding();
            FightModule.ClearBinding();
            RecruitModule.ClearBinding();
            RoguelikeModule.ClearBinding();
            ReclamationModule.ClearBinding();
            CustomModule.ClearBinding();

            var paramsResult = await Runtime.TaskQueueFeatureService.GetTaskParamsAsync(index, cancellationToken);
            if (!paramsResult.Success || paramsResult.Value is null)
            {
                LastErrorMessage = paramsResult.Message;
                ResetSelectedTaskValidationSummary();
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Infrast, StringComparison.OrdinalIgnoreCase))
            {
                await InfrastModule.BindAsync(index, paramsResult.Value, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }
            }
            else
            {
                InfrastModule.ClearBinding();
            }

            if (string.Equals(moduleType, TaskModuleTypes.Mall, StringComparison.OrdinalIgnoreCase))
            {
                await MallModule.BindAsync(index, paramsResult.Value, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }
            }
            else
            {
                MallModule.ClearBinding();
            }

            if (string.Equals(moduleType, TaskModuleTypes.Award, StringComparison.OrdinalIgnoreCase))
            {
                await AwardModule.BindAsync(index, paramsResult.Value, cancellationToken);
                if (IsBindingStale(expectedVersion, cancellationToken))
                {
                    return;
                }
            }
            else
            {
                AwardModule.ClearBinding();
            }

            await RefreshSelectedTaskValidationSummaryAsync(index, cancellationToken);
        }
        finally
        {
            _taskBindingLock.Release();
        }
    }

    private void ScheduleBindSelectedTask()
    {
        lock (_pendingBindingGate)
        {
            _pendingBindingVersion++;
            var expectedVersion = _pendingBindingVersion;

            _pendingBindingCts?.Cancel();
            _pendingBindingCts?.Dispose();
            _pendingBindingCts = new CancellationTokenSource();

            _pendingBindingTask = ExecuteTrackedBindingAsync(expectedVersion, _pendingBindingCts.Token);
        }
    }

    private async Task ExecuteTrackedBindingAsync(int expectedVersion, CancellationToken cancellationToken)
    {
        try
        {
            await BindSelectedTaskCoreAsync(expectedVersion, cancellationToken);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            // no-op
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "TaskQueue.BindSelectedTask",
                ex,
                UiErrorCode.TaskLoadFailed,
                "Bind selected task failed.");
        }
    }

    private bool IsBindingVersionCurrent(int expectedVersion)
    {
        lock (_pendingBindingGate)
        {
            return expectedVersion == _pendingBindingVersion;
        }
    }

    private bool IsBindingStale(int expectedVersion, CancellationToken cancellationToken)
    {
        return cancellationToken.IsCancellationRequested || !IsBindingVersionCurrent(expectedVersion);
    }

    private void ClearTaskModuleBindings()
    {
        StartUpModule.ClearBinding();
        FightModule.ClearBinding();
        RecruitModule.ClearBinding();
        InfrastModule.ClearBinding();
        MallModule.ClearBinding();
        AwardModule.ClearBinding();
        RoguelikeModule.ClearBinding();
        ReclamationModule.ClearBinding();
        CustomModule.ClearBinding();
        ResetSelectedTaskValidationSummary();
    }

    private async Task<bool> SaveBoundTaskModulesAsync(CancellationToken cancellationToken = default)
    {
        if (!await StartUpModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = StartUpModule.LastErrorMessage;
            return false;
        }

        if (!await FightModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = FightModule.LastErrorMessage;
            return false;
        }

        if (!await RecruitModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = RecruitModule.LastErrorMessage;
            return false;
        }

        if (!await RoguelikeModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = RoguelikeModule.LastErrorMessage;
            return false;
        }

        if (!await ReclamationModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = ReclamationModule.LastErrorMessage;
            return false;
        }

        if (!await CustomModule.SaveIfDirtyAsync(cancellationToken))
        {
            LastErrorMessage = CustomModule.LastErrorMessage;
            return false;
        }

        if (!await InfrastModule.FlushPendingChangesAsync(cancellationToken))
        {
            LastErrorMessage = InfrastModule.LastErrorMessage;
            return false;
        }

        if (!await MallModule.FlushPendingChangesAsync(cancellationToken))
        {
            LastErrorMessage = MallModule.LastErrorMessage;
            return false;
        }

        if (!await AwardModule.FlushPendingChangesAsync(cancellationToken))
        {
            LastErrorMessage = AwardModule.LastErrorMessage;
            return false;
        }

        if (!await PostActionModule.FlushPendingChangesAsync(cancellationToken))
        {
            LastErrorMessage = PostActionModule.LastErrorMessage;
            return false;
        }

        var flushResult = await Runtime.TaskQueueFeatureService.FlushTaskParamWritesAsync(cancellationToken);
        if (!flushResult.Success)
        {
            LastErrorMessage = flushResult.Message;
            await RecordFailedResultAsync("TaskQueue.FlushParams", flushResult, cancellationToken);
            return false;
        }

        return true;
    }

    private async Task HandleCallbackAsync(CoreCallbackEvent callback)
    {
        await Dispatcher.UIThread.InvokeAsync(() => HandleCallbackCoreAsync(callback));
    }

    private async Task HandleCallbackCoreAsync(CoreCallbackEvent callback)
    {
        var currentOwner = Runtime.SessionService.CurrentRunOwner;
        if (!string.IsNullOrWhiteSpace(currentOwner) && !Runtime.SessionService.IsRunOwner(TaskQueueRunOwner))
        {
            return;
        }

        var metadata = ParseCallbackPayload(callback.PayloadJson);
        if (metadata.HasParseError)
        {
            var warning = $"msgId={callback.MsgId}; msgName={callback.MsgName}; {metadata.ParseError}";
            Runtime.LogService.Warn($"TaskQueue callback payload parse failed: {warning}");
            await RecordEventAsync("TaskQueue.Callback.Parse", warning);
        }

        var taskChain = metadata.TaskChain;
        var runId = ResolveRunId(metadata.RunId);
        var taskResolution = ResolveCallbackTaskIndex(
            metadata.TaskIndex,
            metadata.TaskId,
            taskChain,
            callback.MsgName);
        var taskIndex = taskResolution.TaskIndex;
        var resolveSource = taskResolution.ResolveSource;
        var module = ResolveCallbackModule(taskChain, taskIndex);
        await RecordTaskResolutionWarningIfNeededAsync(
            taskResolution,
            callback.MsgName,
            runId,
            taskChain,
            metadata.TaskIndex,
            metadata.TaskId);

        switch (callback.MsgName)
        {
            case "TaskChainStart":
                _currentRunId = runId;
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Running);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Running,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
            case "SubTaskStart":
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Running);
                StatusMessage = $"{taskChain ?? "Task"}::{metadata.SubTask ?? "SubTask"} running.";
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Running,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
            case "SubTaskCompleted":
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Running);
                StatusMessage = $"{taskChain ?? "Task"}::{metadata.SubTask ?? "SubTask"} completed.";
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Running,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
            case "TaskChainCompleted":
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Success);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Success,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                CompleteTaskQueueRunOwnership();
                break;
            case "TaskChainError":
            case "SubTaskError":
                UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Error);
                LastErrorMessage = $"{callback.MsgName}: {callback.PayloadJson}";
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Error,
                    callback.PayloadJson,
                    UiErrorCode.TaskRuntimeCallbackError,
                    resolveSource);
                CompleteTaskQueueRunOwnership();
                break;
            case "TaskChainStopped":
                if (taskIndex.HasValue || !string.IsNullOrWhiteSpace(taskChain))
                {
                    UpdateTaskStatus(taskIndex, taskChain, TaskQueueItemStatus.Skipped);
                }
                else
                {
                    MarkRunningTasks(TaskQueueItemStatus.Skipped);
                }

                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Skipped,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                CompleteTaskQueueRunOwnership();
                break;
            case "AllTasksCompleted":
                MarkRunningTasks(TaskQueueItemStatus.Success);
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    TaskQueueItemStatus.Success,
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                if (!string.Equals(_lastPostActionRunId, runId, StringComparison.Ordinal))
                {
                    _lastPostActionRunId = runId;
                    await ExecutePostActionAfterCompletionAsync(callback, runId, taskIndex);
                }

                CompleteTaskQueueRunOwnership();

                break;
            default:
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Observed",
                    callback.PayloadJson,
                    resolveSource: resolveSource);
                break;
        }
    }

    private async Task<bool> EnsureEditableAsync(string scope, CancellationToken cancellationToken = default)
    {
        if (CanEdit)
        {
            return true;
        }

        return await ApplyResultAsync(
            UiOperationResult.Fail(
                UiErrorCode.TaskQueueEditBlocked,
                Texts.GetOrDefault(
                    "TaskQueue.Error.EditBlockedWhileRunning",
                    "Task editing is blocked while running.")),
            scope,
            cancellationToken);
    }

    private static bool IsValidTaskIndex(int? taskIndex, int count)
    {
        return taskIndex.HasValue && taskIndex.Value >= 0 && taskIndex.Value < count;
    }

    private CallbackTaskResolution ResolveCallbackTaskIndex(
        int? callbackTaskIndex,
        int? callbackTaskId,
        string? taskChain,
        string action)
    {
        if (IsValidTaskIndex(callbackTaskIndex, Tasks.Count))
        {
            return new CallbackTaskResolution(callbackTaskIndex, "task_index");
        }

        if (callbackTaskId.HasValue
            && Runtime.SessionService.TryResolveTaskIndexByCoreTaskId(callbackTaskId.Value, out var mappedIndex)
            && IsValidTaskIndex(mappedIndex, Tasks.Count))
        {
            return new CallbackTaskResolution(mappedIndex, "task_id_map");
        }

        return ResolveCallbackTaskByChain(taskChain, action);
    }

    private CallbackTaskResolution ResolveCallbackTaskByChain(string? taskChain, string action)
    {
        var matchedIndices = FindTaskIndicesByChain(taskChain);
        if (matchedIndices.Count == 0)
        {
            return new CallbackTaskResolution(
                null,
                "unresolved",
                WarningDetail: $"taskChain={taskChain ?? "-"} action={action} reason=no-matching-task-chain");
        }

        if (matchedIndices.Count == 1)
        {
            return new CallbackTaskResolution(matchedIndices[0], "chain_unique");
        }

        int selectedIndex;
        var strategy = "fallback-min-index";
        if (ShouldPreferRunningTask(action))
        {
            if (TryFindIndexByStatus(matchedIndices, TaskQueueItemStatus.Running, out selectedIndex))
            {
                strategy = "prefer-running";
            }
            else
            {
                selectedIndex = matchedIndices[0];
            }
        }
        else if (ShouldPreferIdleTask(action))
        {
            if (TryFindIndexByStatus(matchedIndices, TaskQueueItemStatus.Idle, out selectedIndex))
            {
                strategy = "prefer-idle";
            }
            else
            {
                selectedIndex = matchedIndices[0];
            }
        }
        else
        {
            selectedIndex = matchedIndices[0];
        }

        return new CallbackTaskResolution(
            selectedIndex,
            "chain_heuristic",
            WarningDetail: $"taskChain={taskChain ?? "-"} action={action} candidates={string.Join(",", matchedIndices)} selected={selectedIndex} strategy={strategy}");
    }

    private static bool ShouldPreferRunningTask(string action)
    {
        return action is "TaskChainCompleted" or "TaskChainError" or "SubTaskError" or "TaskChainStopped";
    }

    private static bool ShouldPreferIdleTask(string action)
    {
        return action is "TaskChainStart" or "SubTaskStart" or "SubTaskCompleted";
    }

    private bool TryFindIndexByStatus(IReadOnlyList<int> candidateIndices, string expectedStatus, out int selectedIndex)
    {
        foreach (var index in candidateIndices)
        {
            var task = Tasks[index];
            if (string.Equals(expectedStatus, TaskQueueItemStatus.Idle, StringComparison.OrdinalIgnoreCase))
            {
                if (!task.IsStatusIdle)
                {
                    continue;
                }
            }
            else if (!string.Equals(task.Status, expectedStatus, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            selectedIndex = index;
            return true;
        }

        selectedIndex = -1;
        return false;
    }

    private List<int> FindTaskIndicesByChain(string? taskChain)
    {
        var matches = new List<int>();
        if (string.IsNullOrWhiteSpace(taskChain))
        {
            return matches;
        }

        for (var i = 0; i < Tasks.Count; i++)
        {
            if (string.Equals(
                    TaskModuleTypes.Normalize(Tasks[i].Type),
                    taskChain,
                    StringComparison.OrdinalIgnoreCase))
            {
                matches.Add(i);
            }
        }

        return matches;
    }

    private async Task RecordTaskResolutionWarningIfNeededAsync(
        CallbackTaskResolution resolution,
        string action,
        string runId,
        string? taskChain,
        int? callbackTaskIndex,
        int? callbackTaskId)
    {
        if (resolution.ResolveSource is not ("chain_heuristic" or "unresolved"))
        {
            return;
        }

        var payload =
            $"runId={runId} action={action} taskChain={taskChain ?? "-"} " +
            $"taskIndex={callbackTaskIndex?.ToString() ?? "-"} taskId={callbackTaskId?.ToString() ?? "-"} " +
            $"resolveSource={resolution.ResolveSource} detail={resolution.WarningDetail ?? "-"}";
        await RecordEventAsync("TaskQueue.Callback.ResolveTask", payload);
    }

    private string ResolveCallbackModule(string? taskChain, int? taskIndex)
    {
        if (!string.IsNullOrWhiteSpace(taskChain))
        {
            return taskChain!;
        }

        if (IsValidTaskIndex(taskIndex, Tasks.Count))
        {
            return TaskModuleTypes.Normalize(Tasks[taskIndex!.Value].Type);
        }

        return "TaskQueue";
    }

    private void UpdateTaskStatus(int? taskIndex, string? taskChain, string status)
    {
        if (IsValidTaskIndex(taskIndex, Tasks.Count))
        {
            Tasks[taskIndex!.Value].Status = status;
        }
    }

    private async Task ExecutePostActionAfterCompletionAsync(CoreCallbackEvent callback, string runId, int? taskIndex)
    {
        var result = await Runtime.PostActionFeatureService.ExecuteAfterCompletionAsync(
            new PostActionExecutionContext(
                callback.MsgName,
                WasSuccessfulTaskChain: true,
                RunId: runId,
                TaskIndex: taskIndex),
            PostActionModule.BuildRuntimeConfig());

        if (!result.Success)
        {
            LastErrorMessage = result.Message;
            await RecordFailedResultAsync("PostAction.Execute", result);
            return;
        }

        StatusMessage = result.Message;
        if (PostActionModule.Once)
        {
            await PostActionModule.ReloadPersistentConfigAsync();
        }
    }

    private void MarkRunningTasks(string status)
    {
        foreach (var task in Tasks.Where(t => string.Equals(t.Status, TaskQueueItemStatus.Running, StringComparison.OrdinalIgnoreCase)))
        {
            task.Status = status;
        }
    }

    private void SyncStoppedUiStateIfSessionNotActive()
    {
        if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
        {
            return;
        }

        StartPrecheckWarningMessage = string.Empty;
        MarkRunningTasks(TaskQueueItemStatus.Skipped);
        CompleteTaskQueueRunOwnership();
    }

    private void CompleteTaskQueueRunOwnership()
    {
        Runtime.SessionService.EndRun(TaskQueueRunOwner);
    }

    private async Task RecordRuntimeStatusAsync(
        string runId,
        int? taskIndex,
        string module,
        string action,
        string status,
        string message,
        string? errorCode = null,
        string resolveSource = "unresolved")
    {
        LastRuntimeStatus = new TaskRuntimeStatusSnapshot(
            RunId: runId,
            TaskIndex: taskIndex,
            Module: module,
            Action: action,
            Status: status,
            Message: message,
            Timestamp: DateTimeOffset.UtcNow);

        var code = string.IsNullOrWhiteSpace(errorCode) ? "-" : errorCode;
        var payload =
            $"runId={runId} taskIndex={taskIndex?.ToString() ?? "-"} module={module} action={action} " +
            $"resolveSource={resolveSource} errorCode={code} message={message}";
        if (code == "-")
        {
            await RecordEventAsync("TaskQueue.Callback", payload);
        }
        else
        {
            await RecordFailedResultAsync(
                "TaskQueue.Callback",
                UiOperationResult.Fail(code, payload));
        }
    }

    private readonly record struct CallbackTaskResolution(int? TaskIndex, string ResolveSource, string? WarningDetail = null);

    private string ResolveRunId(string? callbackRunId)
    {
        if (!string.IsNullOrWhiteSpace(callbackRunId))
        {
            return callbackRunId!;
        }

        if (string.IsNullOrWhiteSpace(_currentRunId) || _currentRunId == "-")
        {
            _currentRunId = Guid.NewGuid().ToString("N");
        }

        return _currentRunId;
    }

    private static CallbackPayload ParseCallbackPayload(string payloadJson)
    {
        if (string.IsNullOrWhiteSpace(payloadJson))
        {
            return CallbackPayload.Empty;
        }

        try
        {
            using var doc = JsonDocument.Parse(payloadJson);
            if (doc.RootElement.ValueKind != JsonValueKind.Object)
            {
                return new CallbackPayload(null, null, null, null, null, "payload is not a JSON object");
            }

            string? taskChain = null;
            string? subTask = null;
            string? runId = null;
            int? taskIndex = null;
            int? taskId = null;
            foreach (var prop in doc.RootElement.EnumerateObject())
            {
                var key = prop.Name.ToLowerInvariant();
                if ((key is "taskchain" or "task_chain") && prop.Value.ValueKind == JsonValueKind.String)
                {
                    taskChain = prop.Value.GetString();
                    continue;
                }

                if ((key is "subtask" or "sub_task") && prop.Value.ValueKind == JsonValueKind.String)
                {
                    subTask = prop.Value.GetString();
                    continue;
                }

                if ((key is "runid" or "run_id" or "uuid" or "id") && prop.Value.ValueKind == JsonValueKind.String)
                {
                    runId = prop.Value.GetString();
                    continue;
                }

                if (key is "taskindex" or "task_index")
                {
                    if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out var index))
                    {
                        taskIndex = index;
                    }
                    else if (prop.Value.ValueKind == JsonValueKind.String && int.TryParse(prop.Value.GetString(), out index))
                    {
                        taskIndex = index;
                    }

                    continue;
                }

                if (key is "taskid" or "task_id")
                {
                    if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out var id))
                    {
                        taskId = id;
                    }
                    else if (prop.Value.ValueKind == JsonValueKind.String && int.TryParse(prop.Value.GetString(), out id))
                    {
                        taskId = id;
                    }
                }
            }

            return new CallbackPayload(taskChain, subTask, runId, taskIndex, taskId);
        }
        catch (JsonException ex)
        {
            return new CallbackPayload(null, null, null, null, null, $"payload parse failed: {ex.Message}");
        }
    }

    private readonly record struct CallbackPayload(
        string? TaskChain,
        string? SubTask,
        string? RunId,
        int? TaskIndex,
        int? TaskId,
        string? ParseError = null)
    {
        public static CallbackPayload Empty { get; } = new(null, null, null, null, null, null);

        public bool HasParseError => !string.IsNullOrWhiteSpace(ParseError);
    }

    private string ResolveLanguage()
    {
        if (Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue("GUI.Localization", out var value)
            && value is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? language)
            && !string.IsNullOrWhiteSpace(language))
        {
            return UiLanguageCatalog.Normalize(language);
        }

        return UiLanguageCatalog.DefaultLanguage;
    }

    private async Task RefreshOverlayStatusTextAsync(CancellationToken cancellationToken = default)
    {
        var snapshotResult = await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
        if (snapshotResult.Success && snapshotResult.Value is not null)
        {
            OverlayStatusText = BuildCapabilityLine(PlatformCapabilityId.Overlay, snapshotResult.Value.Overlay);
            return;
        }

        OverlayStatusText = PlatformCapabilityTextMap.FormatSnapshotUnavailable(
            Texts.Language,
            snapshotResult.Message,
            _localizationFallbackReporter);
    }

    private void OnSessionStateChanged(SessionState state)
    {
        void Apply(SessionState changedState)
        {
            CurrentSessionState = changedState;
            if (changedState is SessionState.Running or SessionState.Stopping)
            {
                return;
            }

            IsWaitingForStop = false;
            SyncStoppedUiStateIfSessionNotActive();
        }

        if (Dispatcher.UIThread.CheckAccess())
        {
            Apply(state);
            return;
        }

        Dispatcher.UIThread.Post(() => Apply(state));
    }
}
