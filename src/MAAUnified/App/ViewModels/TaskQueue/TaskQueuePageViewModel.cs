using System.Collections.ObjectModel;
using System.Text.Json;
using System.Text.Json.Nodes;
using Avalonia.Threading;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.CoreBridge;
using MAAUnified.Compat.Mapping;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class TaskQueuePageViewModel : PageViewModelBase
{
    private readonly SemaphoreSlim _taskBindingLock = new(1, 1);
    private readonly ConnectionGameSharedStateViewModel _connectionGameSharedState;
    private bool _isRunning;
    private bool _hasBlockingConfigIssues;
    private int _blockingConfigIssueCount;
    private bool _autoReload;
    private bool _showAdvanced;
    private string _dailyStageHint = string.Empty;
    private string _selectedTaskModule = TaskModuleTypes.StartUp;
    private string _newTaskName = string.Empty;
    private string _renameTargetName = string.Empty;
    private string _overlayStatusText = string.Empty;
    private OverlayTarget? _selectedOverlayTarget = new("preview", "Preview + Logs", true);
    private bool _overlayVisible;
    private string _currentRunId = "-";
    private TaskRuntimeStatusSnapshot? _lastRuntimeStatus;
    private TaskQueueItemViewModel? _selectedTask;

    public TaskQueuePageViewModel(MAAUnifiedRuntime runtime, ConnectionGameSharedStateViewModel connectionGameSharedState)
        : base(runtime)
    {
        _connectionGameSharedState = connectionGameSharedState;
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
        runtime.ConfigurationService.ConfigChanged += _ =>
        {
            Dispatcher.UIThread.Post(() =>
            {
                Texts.Language = ResolveLanguage();
                RefreshConfigValidationState(runtime.ConfigurationService.CurrentValidationIssues);
            });
        };
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

    public PostActionModuleViewModel PostActionModule { get; }

    public TaskRuntimeStatusSnapshot? LastRuntimeStatus
    {
        get => _lastRuntimeStatus;
        private set => SetProperty(ref _lastRuntimeStatus, value);
    }

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
            _ = BindSelectedTaskAsync();
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

    public bool IsRunning
    {
        get => _isRunning;
        private set
        {
            if (SetProperty(ref _isRunning, value))
            {
                OnPropertyChanged(nameof(CanEdit));
                OnPropertyChanged(nameof(RunButtonText));
                OnPropertyChanged(nameof(CanToggleRun));
            }
        }
    }

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

    public bool CanToggleRun => IsRunning || !HasBlockingConfigIssues;

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
        Texts.Language = ResolveLanguage();
        RefreshConfigValidationState(Runtime.ConfigurationService.CurrentValidationIssues);
        await ReloadTasksAsync(cancellationToken);
        await ReloadOverlayTargetsAsync(cancellationToken);
        await PostActionModule.InitializeAsync(cancellationToken);
    }

    public void SetLanguage(string language)
    {
        Texts.Language = language;
    }

    public async Task ReloadTasksAsync(CancellationToken cancellationToken = default)
    {
        var tasks = await ApplyResultAsync(
            await Runtime.TaskQueueFeatureService.GetCurrentTaskQueueAsync(cancellationToken),
            "TaskQueue.Reload",
            cancellationToken);

        if (tasks is null)
        {
            return;
        }

        Tasks.Clear();
        foreach (var task in tasks)
        {
            Tasks.Add(TaskQueueItemViewModel.FromUnifiedTask(task));
        }

        SelectedTask = Tasks.FirstOrDefault();
    }

    public async Task AddTaskAsync(CancellationToken cancellationToken = default)
    {
        var taskType = SelectedTaskModule;
        var taskName = string.IsNullOrWhiteSpace(NewTaskName) ? taskType : NewTaskName.Trim();
        if (!await ApplyResultAsync(
                await Runtime.TaskQueueFeatureService.AddTaskAsync(taskType, taskName, true, cancellationToken),
                "TaskQueue.AddTask",
                cancellationToken))
        {
            return;
        }

        await ReloadTasksAsync(cancellationToken);
        NewTaskName = string.Empty;
    }

    public async Task RemoveSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRemove"];
            return;
        }

        var index = Tasks.IndexOf(SelectedTask);
        if (!await ApplyResultAsync(
                await Runtime.TaskQueueFeatureService.RemoveTaskAsync(index, cancellationToken),
                "TaskQueue.RemoveTask",
                cancellationToken))
        {
            return;
        }

        await ReloadTasksAsync(cancellationToken);
    }

    public async Task RenameSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        if (SelectedTask is null)
        {
            LastErrorMessage = Texts["TaskQueue.Error.SelectTaskToRename"];
            return;
        }

        var index = Tasks.IndexOf(SelectedTask);
        if (!await ApplyResultAsync(
                await Runtime.TaskQueueFeatureService.RenameTaskAsync(index, RenameTargetName, cancellationToken),
                "TaskQueue.RenameTask",
                cancellationToken))
        {
            return;
        }

        await ReloadTasksAsync(cancellationToken);
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
        if (from == to)
        {
            return;
        }

        if (!await ApplyResultAsync(
                await Runtime.TaskQueueFeatureService.MoveTaskAsync(from, to, cancellationToken),
                "TaskQueue.MoveTask",
                cancellationToken))
        {
            return;
        }

        await ReloadTasksAsync(cancellationToken);
        SelectedTask = Tasks.ElementAtOrDefault(to);
    }

    public async Task SelectAllAsync(bool enabled, CancellationToken cancellationToken = default)
    {
        for (var index = 0; index < Tasks.Count; index++)
        {
            await Runtime.TaskQueueFeatureService.SetTaskEnabledAsync(index, enabled, cancellationToken);
        }

        await ReloadTasksAsync(cancellationToken);
    }

    public async Task InverseSelectionAsync(CancellationToken cancellationToken = default)
    {
        for (var index = 0; index < Tasks.Count; index++)
        {
            await Runtime.TaskQueueFeatureService.SetTaskEnabledAsync(index, !Tasks[index].IsEnabled, cancellationToken);
        }

        await ReloadTasksAsync(cancellationToken);
    }

    public async Task SaveAsync(CancellationToken cancellationToken = default)
    {
        if (!await SaveBoundTaskModulesAsync(cancellationToken))
        {
            return;
        }

        await ApplyResultAsync(await Runtime.TaskQueueFeatureService.SaveAsync(cancellationToken), "TaskQueue.Save", cancellationToken);
    }

    public async Task ToggleRunAsync(CancellationToken cancellationToken = default)
    {
        if (IsRunning)
        {
            await StopAsync(cancellationToken);
            return;
        }

        await StartAsync(cancellationToken);
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        if (!await SaveBoundTaskModulesAsync(cancellationToken))
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
            await Runtime.DiagnosticsService.RecordConfigValidationFailureAsync(first, cancellationToken);
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

        _currentRunId = Guid.NewGuid().ToString("N");
        IsRunning = true;
        foreach (var task in Tasks.Where(t => t.IsEnabled))
        {
            task.Status = "Running";
        }
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StopAsync(cancellationToken), "TaskQueue.Stop", cancellationToken))
        {
            return;
        }

        IsRunning = false;
        foreach (var task in Tasks.Where(t => t.Status == "Running"))
        {
            task.Status = "Success";
        }
    }

    public async Task WaitAndStopAsync(CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(
                await Runtime.ConnectFeatureService.WaitAndStopAsync(TimeSpan.FromSeconds(15), cancellationToken),
                "TaskQueue.WaitAndStop",
                cancellationToken))
        {
            return;
        }

        IsRunning = false;
        foreach (var task in Tasks.Where(t => t.Status == "Running"))
        {
            task.Status = "Skipped";
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
            OverlayStatusText = PlatformCapabilityTextMap.FormatSnapshotUnavailable(Texts.Language, snapshotResult.Message);
        }
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
        return PlatformCapabilityTextMap.FormatCapabilityLine(Texts.Language, capability, status);
    }

    private void RefreshConfigValidationState(IReadOnlyList<ConfigValidationIssue> issues)
    {
        BlockingConfigIssueCount = issues.Count(i => i.Blocking);
        HasBlockingConfigIssues = BlockingConfigIssueCount > 0;
    }

    private async Task BindSelectedTaskAsync(CancellationToken cancellationToken = default)
    {
        await _taskBindingLock.WaitAsync(cancellationToken);
        try
        {
            if (!await SaveBoundTaskModulesAsync(cancellationToken))
            {
                return;
            }

            if (SelectedTask is null)
            {
                ClearTaskModuleBindings();
                return;
            }

            var index = Tasks.IndexOf(SelectedTask);
            if (index < 0)
            {
                return;
            }

            var moduleType = TaskModuleTypes.Normalize(SelectedTask.Type);
            if (string.Equals(moduleType, TaskModuleTypes.StartUp, StringComparison.OrdinalIgnoreCase))
            {
                await StartUpModule.BindAsync(index, cancellationToken);
                FightModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Fight, StringComparison.OrdinalIgnoreCase))
            {
                await FightModule.BindAsync(index, cancellationToken);
                StartUpModule.ClearBinding();
                RecruitModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Recruit, StringComparison.OrdinalIgnoreCase))
            {
                await RecruitModule.BindAsync(index, cancellationToken);
                StartUpModule.ClearBinding();
                FightModule.ClearBinding();
                InfrastModule.ClearBinding();
                MallModule.ClearBinding();
                AwardModule.ClearBinding();
                return;
            }

            StartUpModule.ClearBinding();
            FightModule.ClearBinding();
            RecruitModule.ClearBinding();

            var paramsResult = await Runtime.TaskQueueFeatureService.GetTaskParamsAsync(index, cancellationToken);
            if (!paramsResult.Success || paramsResult.Value is null)
            {
                LastErrorMessage = paramsResult.Message;
                return;
            }

            if (string.Equals(moduleType, TaskModuleTypes.Infrast, StringComparison.OrdinalIgnoreCase))
            {
                await InfrastModule.BindAsync(index, paramsResult.Value, cancellationToken);
            }
            else
            {
                InfrastModule.ClearBinding();
            }

            if (string.Equals(moduleType, TaskModuleTypes.Mall, StringComparison.OrdinalIgnoreCase))
            {
                await MallModule.BindAsync(index, paramsResult.Value, cancellationToken);
            }
            else
            {
                MallModule.ClearBinding();
            }

            if (string.Equals(moduleType, TaskModuleTypes.Award, StringComparison.OrdinalIgnoreCase))
            {
                await AwardModule.BindAsync(index, paramsResult.Value, cancellationToken);
            }
            else
            {
                AwardModule.ClearBinding();
            }
        }
        finally
        {
            _taskBindingLock.Release();
        }
    }

    private void ClearTaskModuleBindings()
    {
        StartUpModule.ClearBinding();
        FightModule.ClearBinding();
        RecruitModule.ClearBinding();
        InfrastModule.ClearBinding();
        MallModule.ClearBinding();
        AwardModule.ClearBinding();
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
            await Runtime.DiagnosticsService.RecordFailedResultAsync("TaskQueue.FlushParams", flushResult, cancellationToken);
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
        var metadata = ParseCallbackPayload(callback.PayloadJson);
        var taskChain = metadata.TaskChain;
        var runId = ResolveRunId(metadata.RunId);
        var taskIndex = metadata.TaskIndex ?? FindTaskIndexByChain(taskChain);
        var module = string.IsNullOrWhiteSpace(taskChain) ? "TaskQueue" : taskChain!;

        switch (callback.MsgName)
        {
            case "TaskChainStart":
                _currentRunId = runId;
                IsRunning = true;
                UpdateTaskStatusByChain(taskChain, "Running");
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Running",
                    callback.PayloadJson);
                break;
            case "SubTaskStart":
                UpdateTaskStatusByChain(taskChain, "Running");
                StatusMessage = $"{taskChain ?? "Task"}::{metadata.SubTask ?? "SubTask"} running.";
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Running",
                    callback.PayloadJson);
                break;
            case "SubTaskCompleted":
                StatusMessage = $"{taskChain ?? "Task"}::{metadata.SubTask ?? "SubTask"} completed.";
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Running",
                    callback.PayloadJson);
                break;
            case "TaskChainCompleted":
                UpdateTaskStatusByChain(taskChain, "Success");
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Success",
                    callback.PayloadJson);
                break;
            case "TaskChainError":
            case "SubTaskError":
                UpdateTaskStatusByChain(taskChain, "Error");
                LastErrorMessage = $"{callback.MsgName}: {callback.PayloadJson}";
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Error",
                    callback.PayloadJson,
                    UiErrorCode.TaskRuntimeCallbackError);
                break;
            case "TaskChainStopped":
                IsRunning = false;
                MarkRunningTasks("Skipped");
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Skipped",
                    callback.PayloadJson);
                break;
            case "AllTasksCompleted":
                IsRunning = false;
                MarkRunningTasks("Success");
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Success",
                    callback.PayloadJson);
                await ExecutePostActionAfterCompletionAsync(callback, runId, taskIndex);
                break;
            default:
                await RecordRuntimeStatusAsync(
                    runId,
                    taskIndex,
                    module,
                    callback.MsgName,
                    "Observed",
                    callback.PayloadJson);
                break;
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
            await Runtime.DiagnosticsService.RecordFailedResultAsync("PostAction.Execute", result);
            return;
        }

        StatusMessage = result.Message;
        if (PostActionModule.Once)
        {
            await PostActionModule.ReloadPersistentConfigAsync();
        }
    }

    private void UpdateTaskStatusByChain(string? taskChain, string status)
    {
        if (string.IsNullOrWhiteSpace(taskChain))
        {
            return;
        }

        foreach (var task in Tasks.Where(t =>
                     string.Equals(TaskModuleTypes.Normalize(t.Type), taskChain, StringComparison.OrdinalIgnoreCase)))
        {
            task.Status = status;
        }
    }

    private void MarkRunningTasks(string status)
    {
        foreach (var task in Tasks.Where(t => string.Equals(t.Status, "Running", StringComparison.OrdinalIgnoreCase)))
        {
            task.Status = status;
        }
    }

    private int? FindTaskIndexByChain(string? taskChain)
    {
        if (string.IsNullOrWhiteSpace(taskChain))
        {
            return null;
        }

        for (var i = 0; i < Tasks.Count; i++)
        {
            if (string.Equals(
                    TaskModuleTypes.Normalize(Tasks[i].Type),
                    taskChain,
                    StringComparison.OrdinalIgnoreCase))
            {
                return i;
            }
        }

        return null;
    }

    private async Task RecordRuntimeStatusAsync(
        string runId,
        int? taskIndex,
        string module,
        string action,
        string status,
        string message,
        string? errorCode = null)
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
        var payload = $"runId={runId} taskIndex={taskIndex?.ToString() ?? "-"} module={module} action={action} errorCode={code} message={message}";
        if (code == "-")
        {
            await Runtime.DiagnosticsService.RecordEventAsync("TaskQueue.Callback", payload);
        }
        else
        {
            await Runtime.DiagnosticsService.RecordErrorAsync("TaskQueue.Callback", payload);
        }
    }

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
                return CallbackPayload.Empty;
            }

            string? taskChain = null;
            string? subTask = null;
            string? runId = null;
            int? taskIndex = null;
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

                if (key is "taskindex" or "task_index" or "taskid" or "task_id")
                {
                    if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out var index))
                    {
                        taskIndex = index;
                    }
                    else if (prop.Value.ValueKind == JsonValueKind.String && int.TryParse(prop.Value.GetString(), out index))
                    {
                        taskIndex = index;
                    }
                }
            }

            return new CallbackPayload(taskChain, subTask, runId, taskIndex);
        }
        catch
        {
            return CallbackPayload.Empty;
        }
    }

    private readonly record struct CallbackPayload(string? TaskChain, string? SubTask, string? RunId, int? TaskIndex)
    {
        public static CallbackPayload Empty { get; } = new(null, null, null, null);
    }

    private string ResolveLanguage()
    {
        if (Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue("GUI.Localization", out var value)
            && value is JsonValue jsonValue
            && jsonValue.TryGetValue(out string? language)
            && !string.IsNullOrWhiteSpace(language))
        {
            return language;
        }

        return "zh-cn";
    }
}
