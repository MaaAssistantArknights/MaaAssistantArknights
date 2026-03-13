using System.Collections.ObjectModel;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using Avalonia.Threading;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.CoreBridge;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.App.ViewModels.Copilot;

internal readonly record struct CopilotCallbackPayload(
    string? TaskChain,
    string? SubTask,
    int? TaskId,
    string? What = null,
    string? Why = null,
    JsonObject? Details = null,
    JsonObject? Root = null,
    string? ParseError = null)
{
    public static CopilotCallbackPayload Empty { get; } = new(null, null, null, null, null, null, null, null);

    public bool HasParseError => !string.IsNullOrWhiteSpace(ParseError);
}

public sealed partial class CopilotPageViewModel : PageViewModelBase
{
    private const string CopilotTaskListConfigScope = "Config.Copilot.CopilotTaskList";
    private const string CopilotRunOwner = "Copilot";
    private const string MainStageStoryCollectionSideStoryType = "主线/故事集/SideStory";
    private const string SecurityServiceStationType = "保全派驻";
    private const string ParadoxSimulationType = "悖论模拟";
    private const string OtherActivityType = "其他活动";

    private string _filePath = string.Empty;
    private int _selectedTypeIndex;
    private bool _autoSquad = true;
    private bool _useSupportUnit;
    private bool _addTrust;
    private bool _overlayEnabled;
    private SessionState _currentSessionState;
    private string? _activeItemName;
    private int? _activeItemCoreTaskId;
    private string? _activeTaskChain;
    private bool _hasActiveRun;
    private CopilotItemViewModel? _selectedItem;
    private bool _suppressSelectionFeedback;
    private readonly RootLocalizationTextMap _rootTexts;

    public CopilotPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
        Types =
        [
            MainStageStoryCollectionSideStoryType,
            SecurityServiceStationType,
            ParadoxSimulationType,
            OtherActivityType,
        ];
        Items = new ObservableCollection<CopilotItemViewModel>();
        Items.CollectionChanged += (_, _) => NotifySelectionDerivedPropertiesChanged();
        Logs = new ObservableCollection<TaskQueueLogEntryViewModel>();
        _rootTexts = new RootLocalizationTextMap("Root.Localization.Copilot")
        {
            Language = ResolveLanguage(),
        };
        runtime.SessionService.CallbackReceived += callback => _ = HandleCallbackAsync(callback);
        runtime.SessionService.SessionStateChanged += OnSessionStateChanged;
        runtime.ConfigurationService.ConfigChanged += _ =>
            Dispatcher.UIThread.Post(() => _rootTexts.Language = ResolveLanguage());
        _currentSessionState = runtime.SessionService.CurrentState;
        LoadPersistedItems();
        InitializeWpfParityState();
    }

    public IReadOnlyList<string> Types { get; }

    public ObservableCollection<CopilotItemViewModel> Items { get; }

    public ObservableCollection<TaskQueueLogEntryViewModel> Logs { get; }

    public string FilePath
    {
        get => _filePath;
        set => SetProperty(ref _filePath, value);
    }

    public int SelectedTypeIndex
    {
        get => _selectedTypeIndex;
        set
        {
            if (SetProperty(ref _selectedTypeIndex, Math.Clamp(value, 0, Types.Count - 1)))
            {
                OnSelectedTypeIndexChanged();
            }
        }
    }

    public bool AutoSquad
    {
        get => _autoSquad;
        set
        {
            if (SetProperty(ref _autoSquad, value))
            {
                OnPropertyChanged(nameof(Form));
                RefreshVisibilityState();
            }
        }
    }

    public bool UseSupportUnit
    {
        get => _useSupportUnit;
        set
        {
            if (SetProperty(ref _useSupportUnit, value))
            {
                OnPropertyChanged(nameof(UseSupportUnitUsage));
            }
        }
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
        set
        {
            if (!SetProperty(ref _selectedItem, value))
            {
                return;
            }

            OnSelectedItemChanged(value);
        }
    }

    public SessionState CurrentSessionState
    {
        get => _currentSessionState;
        private set
        {
            if (SetProperty(ref _currentSessionState, value))
            {
                OnPropertyChanged(nameof(IsRunning));
                OnPropertyChanged(nameof(CanStart));
                OnPropertyChanged(nameof(CanStop));
                OnPropertyChanged(nameof(CanEdit));
            }
        }
    }

    public bool IsRunning => CurrentSessionState is SessionState.Running or SessionState.Stopping;

    public bool CanStart => CurrentSessionState == SessionState.Connected;

    public bool CanStop => CurrentSessionState == SessionState.Running;

    public bool HasSelection => SelectedItem is not null;

    public bool CanMoveSelectedUp => SelectedItem is not null && Items.IndexOf(SelectedItem) > 0;

    public bool CanMoveSelectedDown
    {
        get
        {
            if (SelectedItem is null)
            {
                return false;
            }

            var index = Items.IndexOf(SelectedItem);
            return index >= 0 && index < Items.Count - 1;
        }
    }

    public Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return RecordEventAsync("Copilot", "Copilot page initialized.", cancellationToken);
    }

    public async Task ImportFromFileAsync(CancellationToken cancellationToken = default)
    {
        var snapshot = CaptureListSnapshot();
        try
        {
            var importResult = await Runtime.CopilotFeatureService.ImportFromFileAsync(FilePath, cancellationToken);
            if (!importResult.Success)
            {
                RestoreListSnapshot(snapshot);
                StatusMessage = "导入作业文件失败。";
                LastErrorMessage = importResult.Message;
                await RecordFailedResultAsync("Copilot.ImportFile", importResult, cancellationToken);
                return;
            }

            var displayName = Path.GetFileName((FilePath ?? string.Empty).Trim());
            if (string.IsNullOrWhiteSpace(displayName))
            {
                displayName = $"Imported-{DateTime.Now:HHmmss}";
            }

            Items.Add(new CopilotItemViewModel(
                displayName,
                Types[SelectedTypeIndex],
                sourcePath: (FilePath ?? string.Empty).Trim()));
            SetSelectedItemSilently(Items.LastOrDefault());

            var persistResult = await PersistItemsAsync(cancellationToken);
            if (!persistResult.Success)
            {
                RestoreListSnapshot(snapshot);
                StatusMessage = "导入作业成功，但列表保存失败，已回滚。";
                LastErrorMessage = persistResult.Message;
                await RecordFailedResultAsync(
                    "Copilot.ImportFile",
                    BuildPersistFailedResult(StatusMessage, persistResult),
                    cancellationToken);
                return;
            }

            StatusMessage = importResult.Message;
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Copilot.ImportFile", StatusMessage, cancellationToken);
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            RestoreListSnapshot(snapshot);
            StatusMessage = "导入作业文件失败。";
            LastErrorMessage = "导入作业文件失败，请检查路径和 JSON 格式后重试。";
            await RecordUnhandledExceptionAsync(
                "Copilot.ImportFile",
                ex,
                UiErrorCode.CopilotFileReadFailed,
                LastErrorMessage,
                cancellationToken);
        }
    }

    public async Task ImportFromClipboardAsync(string payload, CancellationToken cancellationToken = default)
    {
        var snapshot = CaptureListSnapshot();
        try
        {
            var importResult = await Runtime.CopilotFeatureService.ImportFromClipboardAsync(payload, cancellationToken);
            if (!importResult.Success)
            {
                RestoreListSnapshot(snapshot);
                StatusMessage = "导入剪贴板作业失败。";
                LastErrorMessage = importResult.Message;
                await RecordFailedResultAsync("Copilot.ImportClipboard", importResult, cancellationToken);
                return;
            }

            var normalizedPayload = (payload ?? string.Empty).Trim();
            var sourcePath = ResolveClipboardPathCandidate(normalizedPayload);
            var inlinePayload = sourcePath is null ? normalizedPayload : string.Empty;
            Items.Add(new CopilotItemViewModel(
                $"Clipboard-{DateTime.Now:HHmmss}",
                Types[SelectedTypeIndex],
                sourcePath: sourcePath,
                inlinePayload: inlinePayload));
            SetSelectedItemSilently(Items.LastOrDefault());

            var persistResult = await PersistItemsAsync(cancellationToken);
            if (!persistResult.Success)
            {
                RestoreListSnapshot(snapshot);
                StatusMessage = "导入剪贴板成功，但列表保存失败，已回滚。";
                LastErrorMessage = persistResult.Message;
                await RecordFailedResultAsync(
                    "Copilot.ImportClipboard",
                    BuildPersistFailedResult(StatusMessage, persistResult),
                    cancellationToken);
                return;
            }

            StatusMessage = importResult.Message;
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Copilot.ImportClipboard", StatusMessage, cancellationToken);
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            RestoreListSnapshot(snapshot);
            StatusMessage = "导入剪贴板作业失败。";
            LastErrorMessage = "导入剪贴板作业失败，请检查路径或 JSON 内容后重试。";
            await RecordUnhandledExceptionAsync(
                "Copilot.ImportClipboard",
                ex,
                UiErrorCode.CopilotPayloadInvalidJson,
                LastErrorMessage,
                cancellationToken);
        }
    }

    public async Task AddEmptyTaskAsync(CancellationToken cancellationToken = default)
    {
        await MutateListAndPersistAsync(
            () =>
            {
                var item = new CopilotItemViewModel($"Task-{Items.Count + 1}", Types[SelectedTypeIndex]);
                Items.Add(item);
                SetSelectedItemSilently(item);
            },
            "Copilot.Add",
            "已新增空白作业。",
            "新增作业失败：列表保存失败。",
            cancellationToken);
    }

    public async Task RemoveSelectedAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (SelectedItem is null)
        {
            StatusMessage = "删除作业失败。";
            LastErrorMessage = "请选择要删除的作业。";
            await RecordFailedResultAsync(
                "Copilot.Remove",
                UiOperationResult.Fail(UiErrorCode.CopilotSelectionMissing, LastErrorMessage),
                cancellationToken);
            return;
        }

        await MutateListAndPersistAsync(
            () =>
            {
                var current = SelectedItem;
                if (current is null)
                {
                    return;
                }

                var currentIndex = Items.IndexOf(current);
                Items.Remove(current);
                if (Items.Count == 0)
                {
                    SetSelectedItemSilently(null);
                    return;
                }

                var nextIndex = Math.Clamp(currentIndex, 0, Items.Count - 1);
                SetSelectedItemSilently(Items[nextIndex]);
            },
            "Copilot.Remove",
            "已删除选中作业。",
            "删除作业失败：列表保存失败。",
            cancellationToken);
    }

    public async Task ClearAllAsync(CancellationToken cancellationToken = default)
    {
        await MutateListAndPersistAsync(
            () =>
            {
                Items.Clear();
                SetSelectedItemSilently(null);
            },
            "Copilot.Clear",
            "已清空作业列表。",
            "清空作业列表失败：列表保存失败。",
            cancellationToken);
    }

    public async Task MoveSelectedUpAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (SelectedItem is null)
        {
            StatusMessage = "排序失败。";
            LastErrorMessage = "请选择要排序的作业。";
            await RecordFailedResultAsync(
                "Copilot.Sort",
                UiOperationResult.Fail(UiErrorCode.CopilotSelectionMissing, LastErrorMessage),
                cancellationToken);
            return;
        }

        var index = Items.IndexOf(SelectedItem);
        if (index <= 0)
        {
            StatusMessage = "当前作业已在顶部。";
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Copilot.Sort", StatusMessage, cancellationToken);
            return;
        }

        await MutateListAndPersistAsync(
            () => Items.Move(index, index - 1),
            "Copilot.Sort",
            "已将选中作业上移。",
            "排序失败：列表保存失败。",
            cancellationToken);
    }

    public async Task MoveSelectedDownAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (SelectedItem is null)
        {
            StatusMessage = "排序失败。";
            LastErrorMessage = "请选择要排序的作业。";
            await RecordFailedResultAsync(
                "Copilot.Sort",
                UiOperationResult.Fail(UiErrorCode.CopilotSelectionMissing, LastErrorMessage),
                cancellationToken);
            return;
        }

        var index = Items.IndexOf(SelectedItem);
        if (index < 0 || index >= Items.Count - 1)
        {
            StatusMessage = "当前作业已在底部。";
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Copilot.Sort", StatusMessage, cancellationToken);
            return;
        }

        await MutateListAndPersistAsync(
            () => Items.Move(index, index + 1),
            "Copilot.Sort",
            "已将选中作业下移。",
            "排序失败：列表保存失败。",
            cancellationToken);
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        if (!CanStart)
        {
            LastErrorMessage = BuildSessionStateNotAllowedMessage(CurrentSessionState, "启动", "start");
            await RecordFailedResultAsync(
                "Copilot.Start",
                UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, LastErrorMessage),
                cancellationToken);
            return;
        }

        if (!Runtime.SessionService.TryBeginRun(CopilotRunOwner, out var currentOwner))
        {
            var owner = string.IsNullOrWhiteSpace(currentOwner) ? "Unknown" : currentOwner;
            StatusMessage = "启动失败。";
            LastErrorMessage = $"Copilot 启动被拦截：当前运行所有者为 `{owner}`。";
            await RecordFailedResultAsync(
                "Copilot.Start.RunOwner",
                UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, LastErrorMessage),
                cancellationToken);
            return;
        }

        var keepRunOwner = false;
        try
        {
            var appendPlan = await AppendConfiguredCopilotAsync(cancellationToken);
            if (appendPlan is null)
            {
                return;
            }

            _activeItemName = appendPlan.ActiveItemName;
            _activeItemCoreTaskId = appendPlan.TaskId;
            _activeTaskChain = appendPlan.TaskChain;
            _hasActiveRun = true;
            foreach (var item in Items.Where(item => item.IsChecked || string.Equals(item.Name, _activeItemName, StringComparison.Ordinal)))
            {
                item.Status = "Queued";
            }

            AddLog(GetRootText("ConnectingToEmulator", "Connecting to emulator……"));
            if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StartAsync(cancellationToken), "Copilot.Start", cancellationToken))
            {
                _hasActiveRun = false;
                _activeItemName = null;
                _activeItemCoreTaskId = null;
                _activeTaskChain = null;
                foreach (var item in Items.Where(item => item.Status == "Queued"))
                {
                    item.Status = "Ready";
                }
                return;
            }

            AddLog(GetRootText("Running", "Running……"));
            CurrentSessionState = Runtime.SessionService.CurrentState;
            keepRunOwner = true;
        }
        finally
        {
            if (!keepRunOwner)
            {
                Runtime.SessionService.EndRun(CopilotRunOwner);
            }
        }
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        if (!CanStop)
        {
            LastErrorMessage = BuildSessionStateNotAllowedMessage(CurrentSessionState, "停止", "stop");
            await RecordFailedResultAsync(
                "Copilot.Stop",
                UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, LastErrorMessage),
                cancellationToken);
            return;
        }

        if (!Runtime.SessionService.IsRunOwner(CopilotRunOwner))
        {
            var owner = Runtime.SessionService.CurrentRunOwner ?? "Unknown";
            StatusMessage = "停止失败。";
            LastErrorMessage = $"Copilot 停止被拦截：当前运行所有者为 `{owner}`。";
            await RecordFailedResultAsync(
                "Copilot.Stop.RunOwner",
                UiOperationResult.Fail(UiErrorCode.TaskQueueEditBlocked, LastErrorMessage),
                cancellationToken);
            return;
        }

        if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StopAsync(cancellationToken), "Copilot.Stop", cancellationToken))
        {
            CurrentSessionState = Runtime.SessionService.CurrentState;
            SyncStoppedUiStateIfSessionNotActive();
            return;
        }

        CurrentSessionState = Runtime.SessionService.CurrentState;
        SyncStoppedUiStateIfSessionNotActive();
    }

    private async Task<int?> AppendSelectedItemAsync(CopilotItemViewModel item, CancellationToken cancellationToken)
    {
        var filePath = await ResolveExecutionFilePathAsync(item, cancellationToken);
        if (filePath is null)
        {
            return null;
        }

        var request = BuildCopilotTaskRequest(item, filePath);
        var appendResult = await Runtime.CoreBridge.AppendTaskAsync(request, cancellationToken);
        if (!appendResult.Success)
        {
            StatusMessage = "启动失败。";
            LastErrorMessage = $"追加 Copilot 任务失败：{appendResult.Error?.Code} {appendResult.Error?.Message}";
            await RecordFailedResultAsync(
                "Copilot.Append",
                UiOperationResult.Fail(
                    UiErrorCode.CopilotFileReadFailed,
                    LastErrorMessage),
                cancellationToken);
            return null;
        }

        await RecordEventAsync(
            "Copilot.Append",
            $"Appended copilot task #{appendResult.Value}: {item.Name}",
            cancellationToken);
        return appendResult.Value;
    }

    private async Task<string?> ResolveExecutionFilePathAsync(CopilotItemViewModel item, CancellationToken cancellationToken)
    {
        var resolvedPath = await ResolveExecutionFilePathAsync(
            item.SourcePath,
            item.InlinePayload,
            item.Name,
            cancellationToken);
        if (resolvedPath is not null)
        {
            item.SourcePath = resolvedPath;
        }

        return resolvedPath;
    }

    private CoreTaskRequest BuildCopilotTaskRequest(CopilotItemViewModel item, string filePath)
    {
        var taskType = ResolveCopilotTaskType(item.Type);
        JsonObject payload;
        if (string.Equals(taskType, "ParadoxCopilot", StringComparison.Ordinal))
        {
            payload = new JsonObject
            {
                ["filename"] = filePath,
            };
        }
        else
        {
            payload = new JsonObject
            {
                ["filename"] = filePath,
                ["formation"] = Form,
                ["support_unit_usage"] = UseSupportUnitUsage ? SupportUnitUsage : 0,
                ["add_trust"] = AddTrust,
                ["ignore_requirements"] = IgnoreRequirements,
                ["loop_times"] = ShowLoopSetting && Loop ? LoopTimes : 1,
                ["use_sanity_potion"] = false,
            };

            if (UseFormation)
            {
                payload["formation_index"] = FormationIndex;
            }

            var userAdditional = BuildUserAdditionalPayload();
            if (userAdditional.Count > 0)
            {
                payload["user_additional"] = userAdditional;
            }
        }

        return new CoreTaskRequest(taskType, item.Name, true, payload.ToJsonString());
    }

    private static string ResolveCopilotTaskType(string type)
    {
        return NormalizeTypeDisplayName(type) switch
        {
            SecurityServiceStationType => "SSSCopilot",
            ParadoxSimulationType => "ParadoxCopilot",
            _ => "Copilot",
        };
    }

    private static string ResolveCopilotTaskChain(string type)
    {
        return ResolveCopilotTaskType(type);
    }

    private static string BuildSessionStateNotAllowedMessage(
        SessionState state,
        string actionZh,
        string actionEn)
    {
        var zh = $"会话状态 `{state}` 不允许{actionZh}。";
        var en = $"Session state `{state}` does not allow {actionEn}.";
        return $"{zh}{Environment.NewLine}{en}";
    }

    public async Task SendLikeAsync(bool like, CancellationToken cancellationToken = default)
    {
        if (SelectedItem is null)
        {
            StatusMessage = "反馈失败。";
            LastErrorMessage = "请选择要反馈的作业。";
            await RecordFailedResultAsync(
                "Copilot.Feedback",
                UiOperationResult.Fail(UiErrorCode.CopilotSelectionMissing, LastErrorMessage),
                cancellationToken);
            return;
        }

        var itemName = SelectedItem.Name;
        var feedbackTarget = SelectedItem.CopilotId > 0 ? SelectedItem.CopilotId.ToString() : itemName;
        var result = await Runtime.CopilotFeatureService.SubmitFeedbackAsync(feedbackTarget, like, cancellationToken);
        if (!result.Success)
        {
            StatusMessage = "反馈失败。";
            LastErrorMessage = result.Message;
            await RecordFailedResultAsync("Copilot.Feedback", result, cancellationToken);
            return;
        }

        StatusMessage = $"已对作业 `{itemName}` 提交{(like ? "点赞" : "点踩")}。";
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Copilot.Feedback", StatusMessage, cancellationToken);
    }

    private static string? ResolveClipboardPathCandidate(string payload)
    {
        if (string.IsNullOrWhiteSpace(payload))
        {
            return null;
        }

        if (payload.StartsWith('{') || payload.StartsWith('['))
        {
            return null;
        }

        var expanded = Environment.ExpandEnvironmentVariables(payload);
        var normalized = Path.GetFullPath(expanded);
        return File.Exists(normalized) ? normalized : null;
    }

    private bool IsCopilotCallbackForActiveRun(CopilotCallbackPayload payload)
    {
        if (payload.TaskId.HasValue && _activeItemCoreTaskId.HasValue && payload.TaskId.Value != _activeItemCoreTaskId.Value)
        {
            return false;
        }

        if (string.IsNullOrWhiteSpace(payload.TaskChain))
        {
            return true;
        }

        if (string.IsNullOrWhiteSpace(_activeTaskChain))
        {
            return true;
        }

        if (string.Equals(payload.TaskChain, _activeTaskChain, StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        return string.Equals(payload.TaskChain, "Copilot", StringComparison.OrdinalIgnoreCase)
            && _activeTaskChain is "SSSCopilot" or "ParadoxCopilot";
    }

    private void CompleteActiveRun()
    {
        _hasActiveRun = false;
        _activeItemName = null;
        _activeItemCoreTaskId = null;
        _activeTaskChain = null;
        Runtime.SessionService.EndRun(CopilotRunOwner);
    }

    private static CopilotCallbackPayload ParseCopilotCallbackPayload(string payloadJson)
    {
        if (string.IsNullOrWhiteSpace(payloadJson))
        {
            return CopilotCallbackPayload.Empty;
        }

        try
        {
            if (JsonNode.Parse(payloadJson) is not JsonObject root)
            {
                return new CopilotCallbackPayload(null, null, null, null, null, null, null, "payload is not a JSON object");
            }

            var taskChain = GetStringValue(root, "task_chain") ?? GetStringValue(root, "taskchain");
            var subTask = GetStringValue(root, "sub_task") ?? GetStringValue(root, "subtask");
            var taskId = GetIntValue(root, "task_id") ?? GetIntValue(root, "taskid");
            var what = GetStringValue(root, "what");
            var why = GetStringValue(root, "why");
            var details = GetObjectValue(root, "details");
            return new CopilotCallbackPayload(taskChain, subTask, taskId, what, why, details, root, null);
        }
        catch (JsonException ex)
        {
            return new CopilotCallbackPayload(null, null, null, null, null, null, null, $"payload parse failed: {ex.Message}");
        }
    }

    private async Task HandleCallbackAsync(CoreCallbackEvent callback)
    {
        await Dispatcher.UIThread.InvokeAsync(() => ApplyRuntimeCallback(callback));
    }

    internal void ApplyRuntimeCallback(CoreCallbackEvent callback)
    {
        var metadata = ParseCopilotCallbackPayload(callback.PayloadJson);
        if (metadata.HasParseError)
        {
            var warning = $"msgId={callback.MsgId}; msgName={callback.MsgName}; {metadata.ParseError}";
            Runtime.LogService.Warn($"Copilot callback payload parse failed: {warning}");
            _ = RecordEventAsync("Copilot.Callback.Parse", warning);
        }

        if (!_hasActiveRun || string.IsNullOrWhiteSpace(_activeItemName))
        {
            return;
        }

        var active = Items.FirstOrDefault(item => string.Equals(item.Name, _activeItemName, StringComparison.Ordinal));
        if (active is null)
        {
            return;
        }

        if (!IsCopilotCallbackForActiveRun(metadata))
        {
            return;
        }

        AppendWpfCallbackLog(callback, metadata);

        switch (callback.MsgName)
        {
            case "TaskChainStart":
                active.Status = "Running";
                break;
            case "TaskChainCompleted":
            case "AllTasksCompleted":
                active.Status = "Success";
                CompleteActiveRun();
                break;
            case "TaskChainStopped":
                active.Status = "Stopped";
                CompleteActiveRun();
                break;
            case "TaskChainError":
            case "SubTaskError":
                active.Status = "Error";
                LastErrorMessage = $"{callback.MsgName}: {callback.PayloadJson}";
                CompleteActiveRun();
                break;
        }
    }

    private void AppendWpfCallbackLog(CoreCallbackEvent callback, CopilotCallbackPayload payload)
    {
        switch (callback.MsgName)
        {
            case "TaskChainError":
                AddLog(GetRootText("CombatError", "Combat error"), "ERROR", timestamp: callback.Timestamp);
                break;
            case "SubTaskError":
                AppendSubTaskErrorLog(payload, callback.Timestamp);
                break;
            case "SubTaskStart":
                AppendSubTaskStartLog(payload, callback.Timestamp);
                break;
            case "SubTaskExtraInfo":
                AppendSubTaskExtraInfoLog(payload, callback.Timestamp);
                break;
        }
    }

    private void AppendSubTaskErrorLog(CopilotCallbackPayload payload, DateTimeOffset timestamp)
    {
        if (string.Equals(payload.SubTask, "BattleFormationTask", StringComparison.OrdinalIgnoreCase)
            && string.Equals(payload.Why, "OperatorMissing", StringComparison.OrdinalIgnoreCase))
        {
            var builder = new StringBuilder(GetRootText("MissingOperators", "Missing operators:"));
            var groups = GetObjectValue(payload.Details, "opers");
            if (groups is not null && groups.Count > 0)
            {
                foreach (var pair in groups)
                {
                    if (pair.Value is not JsonArray opers)
                    {
                        continue;
                    }

                    builder.AppendLine();
                    if (opers.Count <= 1)
                    {
                        builder.Append(pair.Key);
                        continue;
                    }

                    var names = opers
                        .Select(oper => oper is JsonObject obj ? GetStringValue(obj, "name") : oper?.ToString())
                        .Where(static name => !string.IsNullOrWhiteSpace(name))
                        .ToArray();
                    builder.Append(pair.Key);
                    builder.Append("=> ");
                    builder.Append(string.Join(" / ", names));
                }
            }

            AddLog(builder.ToString().TrimEnd(), "ERROR", timestamp: timestamp);
            return;
        }

        if (string.Equals(payload.SubTask, "CopilotTask", StringComparison.OrdinalIgnoreCase)
            && string.Equals(payload.What, "UserAdditionalOperInvalid", StringComparison.OrdinalIgnoreCase))
        {
            var operName = GetStringValue(payload.Details, "name") ?? string.Empty;
            AddLog(
                string.Format(
                    GetRootText("CopilotUserAdditionalNameInvalid", "Additional custom operator name invalid: {0}, please check spelling"),
                    operName),
                "ERROR",
                timestamp: timestamp);
        }
    }

    private void AppendSubTaskStartLog(CopilotCallbackPayload payload, DateTimeOffset timestamp)
    {
        if (string.Equals(payload.SubTask, "CombatRecordRecognitionTask", StringComparison.OrdinalIgnoreCase)
            && !string.IsNullOrWhiteSpace(payload.What))
        {
            AddLog(payload.What, timestamp: timestamp);
            return;
        }

        if (!string.Equals(payload.SubTask, "ProcessTask", StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        var taskName = GetStringValue(payload.Details, "task");
        switch (taskName)
        {
            case "BattleStartAll":
                AddLog(GetRootText("MissionStart", "Mission started"), timestamp: timestamp);
                break;
            case "StageDrops-Stars-3":
            case "StageDrops-Stars-Adverse":
                AddLog(GetRootText("CompleteCombat", "Complete combat"), "SUCCESS", timestamp: timestamp);
                break;
        }
    }

    private void AppendSubTaskExtraInfoLog(CopilotCallbackPayload payload, DateTimeOffset timestamp)
    {
        switch (payload.What)
        {
            case "BattleFormation":
                AppendBattleFormationLog(payload.Details, timestamp);
                break;
            case "BattleFormationParseFailed":
                AddLog(GetRootText("BattleFormationParseFailed", "Formation parse failed"), timestamp: timestamp);
                break;
            case "BattleFormationSelected":
                AppendBattleFormationSelectedLog(payload.Details, timestamp);
                break;
            case "BattleFormationOperUnavailable":
                AppendBattleFormationUnavailableLog(payload.Details, timestamp);
                break;
            case "CopilotAction":
                AppendCopilotActionLog(payload.Details, timestamp);
                break;
            case "CopilotListLoadTaskFileSuccess":
                AddLog(
                    $"Parse {GetStringValue(payload.Details, "file_name")}[{GetStringValue(payload.Details, "stage_name")}] Success",
                    timestamp: timestamp);
                break;
            case "SSSStage":
                AddLog(
                    string.Format(
                        GetRootText("CurrentStage", "Current Stage: {0}"),
                        GetStringValue(payload.Details, "stage") ?? string.Empty),
                    timestamp: timestamp);
                break;
            case "SSSSettlement":
                if (!string.IsNullOrWhiteSpace(payload.Why))
                {
                    AddLog(payload.Why, timestamp: timestamp);
                }

                break;
            case "SSSGamePass":
                AddLog(GetRootText("SSSGamePass", "Game cleared! congratulations!"), timestamp: timestamp);
                break;
            case "UnsupportedLevel":
                AddLog(GetRootText("UnsupportedLevel", "Unsupported stage, please update resources and try again!"), "ERROR", timestamp: timestamp);
                break;
        }
    }

    private void AppendBattleFormationLog(JsonObject? details, DateTimeOffset timestamp)
    {
        var formation = GetArrayValue(details, "formation");
        var names = formation?
            .Select(oper => oper?.ToString())
            .Where(static name => !string.IsNullOrWhiteSpace(name))
            .ToArray() ?? [];
        AddLog(
            $"{GetRootText("BattleFormation", "Start formation")}{Environment.NewLine}[{string.Join(", ", names)}]",
            timestamp: timestamp);
    }

    private void AppendBattleFormationSelectedLog(JsonObject? details, DateTimeOffset timestamp)
    {
        var selected = GetStringValue(details, "selected") ?? string.Empty;
        var groupName = GetStringValue(details, "group_name");
        if (!string.IsNullOrWhiteSpace(groupName) && !string.Equals(groupName, selected, StringComparison.Ordinal))
        {
            selected = $"{groupName} => {selected}";
        }

        AddLog(
            $"{GetRootText("BattleFormationSelected", "Selected: ")}{selected}",
            timestamp: timestamp);
    }

    private void AppendBattleFormationUnavailableLog(JsonObject? details, DateTimeOffset timestamp)
    {
        var operName = GetStringValue(details, "oper_name") ?? string.Empty;
        var type = GetStringValue(details, "requirement_type") ?? string.Empty;
        var level = !IgnoreRequirements;
        var reasonKey = type switch
        {
            "elite" => "BattleFormationOperUnavailable.Elite",
            "level" => "BattleFormationOperUnavailable.Level",
            "skill_level" => "BattleFormationOperUnavailable.SkillLevel",
            "module" => "BattleFormationOperUnavailable.Module",
            _ => string.Empty,
        };
        if (string.Equals(type, "elite", StringComparison.OrdinalIgnoreCase))
        {
            level = true;
        }

        var reason = string.IsNullOrWhiteSpace(reasonKey) ? type : GetRootText(reasonKey, type);
        AddLog(
            string.Format(
                GetRootText("BattleFormationOperUnavailable", "Operator unavailable: {0}, reason: {1}"),
                operName,
                reason),
            level ? "ERROR" : "WARN",
            timestamp: timestamp);
    }

    private void AppendCopilotActionLog(JsonObject? details, DateTimeOffset timestamp)
    {
        var doc = GetStringValue(details, "doc");
        if (!string.IsNullOrWhiteSpace(doc))
        {
            AddLog(doc, MapDocColorToLevel(GetStringValue(details, "doc_color")), timestamp: timestamp);
        }

        var action = GetStringValue(details, "action") ?? "UnknownAction";
        var target = GetStringValue(details, "target") ?? string.Empty;
        AddLog(
            string.Format(
                GetRootText("CurrentSteps", "Step: {0} {1}"),
                GetRootText(action, action),
                target),
            timestamp: timestamp);

        var elapsed = GetIntValue(details, "elapsed_time");
        if (elapsed.HasValue && elapsed.Value >= 0)
        {
            AddLog(
                string.Format(
                    GetRootText("ElapsedTime", "Elapsed time: {0}ms"),
                    elapsed.Value),
                timestamp: timestamp);
        }
    }

    private string GetRootText(string key, string fallback)
    {
        return _rootTexts.GetOrDefault(key, fallback);
    }

    private void AddLog(string? content, string level = "INFO", bool showTime = true, DateTimeOffset? timestamp = null)
    {
        if (string.IsNullOrWhiteSpace(content))
        {
            return;
        }

        var time = showTime ? (timestamp ?? DateTimeOffset.Now).ToLocalTime().ToString("HH:mm:ss") : string.Empty;
        Logs.Add(new TaskQueueLogEntryViewModel(time, content.TrimEnd(), level));
        const int maxLogs = 200;
        while (Logs.Count > maxLogs)
        {
            Logs.RemoveAt(0);
        }
    }

    private static string MapDocColorToLevel(string? color)
    {
        if (string.IsNullOrWhiteSpace(color))
        {
            return "INFO";
        }

        if (color.Contains("error", StringComparison.OrdinalIgnoreCase))
        {
            return "ERROR";
        }

        if (color.Contains("warn", StringComparison.OrdinalIgnoreCase))
        {
            return "WARN";
        }

        if (color.Contains("success", StringComparison.OrdinalIgnoreCase))
        {
            return "SUCCESS";
        }

        return "INFO";
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

    private static string? GetStringValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node) || node is null)
        {
            return null;
        }

        if (node is JsonValue value && value.TryGetValue(out string? text))
        {
            return text;
        }

        return node.ToString();
    }

    private static int? GetIntValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node) || node is null)
        {
            return null;
        }

        if (node is JsonValue value)
        {
            if (value.TryGetValue(out int number))
            {
                return number;
            }

            if (value.TryGetValue(out string? text) && int.TryParse(text, out number))
            {
                return number;
            }
        }

        return null;
    }

    private static JsonObject? GetObjectValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node))
        {
            return null;
        }

        return node as JsonObject;
    }

    private static JsonArray? GetArrayValue(JsonObject? obj, string key)
    {
        if (obj is null || !obj.TryGetPropertyValue(key, out var node))
        {
            return null;
        }

        return node as JsonArray;
    }

    private void OnSessionStateChanged(SessionState state)
    {
        void Apply(SessionState changedState)
        {
            CurrentSessionState = changedState;
            SyncStoppedUiStateIfSessionNotActive();
        }

        if (Dispatcher.UIThread.CheckAccess())
        {
            Apply(state);
            return;
        }

        Dispatcher.UIThread.Post(() => Apply(state));
    }

    private void SyncStoppedUiStateIfSessionNotActive()
    {
        if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
        {
            return;
        }

        foreach (var item in Items.Where(item => item.Status is "Queued" or "Running"))
        {
            item.Status = "Stopped";
        }

        if (_hasActiveRun)
        {
            CompleteActiveRun();
        }
    }

    private void OnSelectedItemChanged(CopilotItemViewModel? value)
    {
        NotifySelectionDerivedPropertiesChanged();
        if (_suppressSelectionFeedback)
        {
            return;
        }

        if (value is null)
        {
            StatusMessage = "未选中作业。";
            LastErrorMessage = string.Empty;
            _ = RecordEventAsync("Copilot.Select", "Cleared selected copilot item.");
            return;
        }

        StatusMessage = $"已选中作业：{value.Name}";
        LastErrorMessage = string.Empty;
        _ = RecordEventAsync("Copilot.Select", $"Selected copilot item `{value.Name}`.");
    }

    private void NotifySelectionDerivedPropertiesChanged()
    {
        OnPropertyChanged(nameof(HasSelection));
        OnPropertyChanged(nameof(CanMoveSelectedUp));
        OnPropertyChanged(nameof(CanMoveSelectedDown));
    }

    private async Task<bool> MutateListAndPersistAsync(
        Action mutation,
        string scope,
        string successMessage,
        string persistenceFailureMessage,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var snapshot = CaptureListSnapshot();

        try
        {
            mutation();
        }
        catch (Exception ex)
        {
            RestoreListSnapshot(snapshot);
            StatusMessage = persistenceFailureMessage;
            LastErrorMessage = ex.Message;
            await RecordUnhandledExceptionAsync(
                scope,
                ex,
                UiErrorCode.CopilotListPersistenceFailed,
                persistenceFailureMessage,
                cancellationToken);
            return false;
        }

        var persistResult = await PersistItemsAsync(cancellationToken);
        if (!persistResult.Success)
        {
            RestoreListSnapshot(snapshot);
            StatusMessage = persistenceFailureMessage;
            LastErrorMessage = persistResult.Message;
            await RecordFailedResultAsync(
                scope,
                BuildPersistFailedResult(persistenceFailureMessage, persistResult),
                cancellationToken);
            return false;
        }

        StatusMessage = successMessage;
        LastErrorMessage = string.Empty;
        await RecordEventAsync(scope, successMessage, cancellationToken);
        return true;
    }

    private async Task<UiOperationResult> PersistItemsAsync(CancellationToken cancellationToken)
    {
        var payload = SerializeItemsPayload();
        var saveResult = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(
            LegacyConfigurationKeys.CopilotTaskList,
            payload,
            cancellationToken);
        if (!saveResult.Success)
        {
            await RecordFailedResultAsync(CopilotTaskListConfigScope, saveResult, cancellationToken);
            return saveResult;
        }

        await RecordEventAsync(
            CopilotTaskListConfigScope,
            $"Saved {Items.Count} copilot item(s).",
            cancellationToken);
        return saveResult;
    }

    private static UiOperationResult BuildPersistFailedResult(string actionMessage, UiOperationResult persistResult)
    {
        return UiOperationResult.Fail(
            persistResult.Error?.Code ?? UiErrorCode.CopilotListPersistenceFailed,
            $"{actionMessage} {persistResult.Message}",
            persistResult.Error?.Details);
    }

    private CopilotListSnapshot CaptureListSnapshot()
    {
        return new CopilotListSnapshot([.. Items], SelectedItem);
    }

    private void RestoreListSnapshot(CopilotListSnapshot snapshot)
    {
        var previousSuppress = _suppressSelectionFeedback;
        _suppressSelectionFeedback = true;
        try
        {
            Items.Clear();
            foreach (var item in snapshot.Items)
            {
                Items.Add(item);
            }

            if (snapshot.SelectedItem is not null && Items.Contains(snapshot.SelectedItem))
            {
                SelectedItem = snapshot.SelectedItem;
            }
            else
            {
                SelectedItem = Items.LastOrDefault();
            }
        }
        finally
        {
            _suppressSelectionFeedback = previousSuppress;
            NotifySelectionDerivedPropertiesChanged();
        }
    }

    private void SetSelectedItemSilently(CopilotItemViewModel? item)
    {
        var previousSuppress = _suppressSelectionFeedback;
        _suppressSelectionFeedback = true;
        try
        {
            SelectedItem = item;
        }
        finally
        {
            _suppressSelectionFeedback = previousSuppress;
            NotifySelectionDerivedPropertiesChanged();
        }
    }

    private void LoadPersistedItems()
    {
        if (!TryReadPersistedPayload(out var payload))
        {
            return;
        }

        if (!TryDeserializePersistedItems(payload, out var loadedItems, out var warning))
        {
            StatusMessage = "已忽略损坏的 Copilot 列表配置。";
            LastErrorMessage = warning;
            _ = RecordFailedResultAsync(
                "Copilot.List.Load",
                UiOperationResult.Fail(UiErrorCode.CopilotPayloadInvalidJson, warning));
            return;
        }

        foreach (var item in loadedItems)
        {
            Items.Add(new CopilotItemViewModel(item.Name, item.Type, item.SourcePath, item.InlinePayload)
            {
                CopilotId = Math.Max(0, item.CopilotId),
                IsChecked = item.IsChecked,
                IsRaid = item.IsRaid,
                TabIndex = item.TabIndex,
            });
        }

        if (Items.Count > 0)
        {
            SetSelectedItemSilently(Items[0]);
        }

        _ = RecordEventAsync(
            "Copilot.List.Load",
            $"Loaded {Items.Count} copilot item(s) from persisted config.");
    }

    private bool TryReadPersistedPayload(out string payload)
    {
        payload = string.Empty;
        if (!Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(LegacyConfigurationKeys.CopilotTaskList, out var node)
            || node is null)
        {
            return false;
        }

        if (node is JsonValue jsonValue && jsonValue.TryGetValue(out string? raw))
        {
            payload = raw ?? string.Empty;
            return !string.IsNullOrWhiteSpace(payload);
        }

        if (node is JsonArray || node is JsonObject)
        {
            payload = node.ToJsonString();
            return !string.IsNullOrWhiteSpace(payload);
        }

        return false;
    }

    private bool TryDeserializePersistedItems(
        string payload,
        out List<PersistedCopilotItem> items,
        out string warning)
    {
        items = [];
        warning = string.Empty;
        JsonNode? root;
        try
        {
            root = JsonNode.Parse(payload);
        }
        catch (Exception ex)
        {
            warning = $"读取作业列表失败：配置不是合法 JSON。{ex.Message}";
            return false;
        }

        if (root is not JsonArray array)
        {
            warning = "读取作业列表失败：配置必须是 JSON 数组。";
            return false;
        }

        for (var index = 0; index < array.Count; index++)
        {
            if (array[index] is not JsonObject obj)
            {
                continue;
            }

            if (!TryGetRequiredStringProperty(obj, "name", out var name))
            {
                continue;
            }

            var type = ResolvePersistedType(obj);
            _ = TryGetOptionalStringProperty(obj, "source_path", out var sourcePath)
                || TryGetOptionalStringProperty(obj, "SourcePath", out sourcePath);
            _ = TryGetOptionalStringProperty(obj, "file_path", out var legacyFilePath)
                || TryGetOptionalStringProperty(obj, "FilePath", out legacyFilePath);
            _ = TryGetOptionalStringProperty(obj, "inline_payload", out var inlinePayload)
                || TryGetOptionalStringProperty(obj, "InlinePayload", out inlinePayload);
            if (string.IsNullOrWhiteSpace(sourcePath))
            {
                sourcePath = legacyFilePath;
            }

            var copilotId = TryReadOptionalIntProperty(obj, "copilot_id", out var persistedCopilotId)
                ? persistedCopilotId ?? 0
                : TryReadOptionalIntProperty(obj, "CopilotId", out persistedCopilotId)
                    ? persistedCopilotId ?? 0
                    : 0;
            var tabIndex = TryReadOptionalIntProperty(obj, "tab_index", out var persistedTabIndex)
                ? persistedTabIndex
                : TryReadOptionalIntProperty(obj, "TabIndex", out persistedTabIndex)
                    ? persistedTabIndex
                    : (int?)null;
            var isRaid = TryReadOptionalBoolProperty(obj, "is_raid", out var persistedIsRaid)
                ? persistedIsRaid
                : TryReadOptionalBoolProperty(obj, "IsRaid", out persistedIsRaid) && persistedIsRaid;
            var isChecked = TryReadOptionalBoolProperty(obj, "is_checked", out var persistedIsChecked)
                ? persistedIsChecked
                : !TryReadOptionalBoolProperty(obj, "IsChecked", out persistedIsChecked) || persistedIsChecked;
            items.Add(new PersistedCopilotItem(name, type, sourcePath, inlinePayload, copilotId, tabIndex, isRaid, isChecked));
        }

        if (array.Count > 0 && items.Count == 0)
        {
            warning = "读取作业列表失败：列表项缺少可识别字段（例如 name）。";
            return false;
        }

        return true;
    }

    private string ResolvePersistedType(JsonObject obj)
    {
        if (TryGetRequiredStringProperty(obj, "type", out var type))
        {
            return NormalizeTypeDisplayName(type);
        }

        if (TryGetPropertyCaseInsensitive(obj, "tab_index", out var tabNode)
            && tabNode is JsonValue tabValue
            && tabValue.TryGetValue(out int tabIndex)
            && tabIndex >= 0
            && tabIndex < Types.Count)
        {
            return Types[tabIndex];
        }

        return Types[0];
    }

    private static string NormalizeTypeDisplayName(string? type)
    {
        var normalized = type?.Trim();
        if (string.IsNullOrWhiteSpace(normalized))
        {
            return MainStageStoryCollectionSideStoryType;
        }

        return normalized switch
        {
            "主线" or MainStageStoryCollectionSideStoryType or "MainStageStoryCollectionSideStory" or "Copilot"
                => MainStageStoryCollectionSideStoryType,
            "SSS" or SecurityServiceStationType or "SSSCopilot"
                => SecurityServiceStationType,
            "悖论" or ParadoxSimulationType or "ParadoxCopilot"
                => ParadoxSimulationType,
            "活动" or OtherActivityType or "OtherActivityStage"
                => OtherActivityType,
            _ => normalized,
        };
    }

    private static bool TryGetRequiredStringProperty(JsonObject obj, string key, out string value)
    {
        value = string.Empty;
        if (!TryGetPropertyCaseInsensitive(obj, key, out var node)
            || node is not JsonValue jsonValue
            || !jsonValue.TryGetValue(out string? raw))
        {
            return false;
        }

        value = raw?.Trim() ?? string.Empty;
        return value.Length > 0;
    }

    private static bool TryGetOptionalStringProperty(JsonObject obj, string key, out string value)
    {
        value = string.Empty;
        if (!TryGetPropertyCaseInsensitive(obj, key, out var node)
            || node is not JsonValue jsonValue
            || !jsonValue.TryGetValue(out string? raw))
        {
            return false;
        }

        value = raw?.Trim() ?? string.Empty;
        return value.Length > 0;
    }

    private static bool TryReadOptionalIntProperty(JsonObject obj, string key, out int? value)
    {
        value = null;
        if (!TryGetPropertyCaseInsensitive(obj, key, out var node)
            || node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            value = parsedInt;
            return true;
        }

        if (jsonValue.TryGetValue(out string? text) && int.TryParse(text, out parsedInt))
        {
            value = parsedInt;
            return true;
        }

        return false;
    }

    private static bool TryReadOptionalBoolProperty(JsonObject obj, string key, out bool value)
    {
        value = false;
        if (!TryGetPropertyCaseInsensitive(obj, key, out var node)
            || node is not JsonValue jsonValue)
        {
            return false;
        }

        if (jsonValue.TryGetValue(out bool parsedBool))
        {
            value = parsedBool;
            return true;
        }

        if (jsonValue.TryGetValue(out int parsedInt))
        {
            value = parsedInt != 0;
            return true;
        }

        if (jsonValue.TryGetValue(out string? text))
        {
            if (bool.TryParse(text, out parsedBool))
            {
                value = parsedBool;
                return true;
            }

            if (int.TryParse(text, out parsedInt))
            {
                value = parsedInt != 0;
                return true;
            }
        }

        return false;
    }

    private static bool TryGetPropertyCaseInsensitive(JsonObject obj, string key, out JsonNode? value)
    {
        foreach (var property in obj)
        {
            if (string.Equals(property.Key, key, StringComparison.OrdinalIgnoreCase))
            {
                value = property.Value;
                return true;
            }
        }

        value = null;
        return false;
    }

    private string SerializeItemsPayload()
    {
        var items = Items
            .Select(item => new PersistedCopilotItem(
                item.Name,
                item.Type,
                item.SourcePath,
                item.InlinePayload,
                item.CopilotId,
                item.TabIndex,
                item.IsRaid,
                item.IsChecked))
            .ToArray();
        return JsonSerializer.Serialize(items);
    }

    private sealed record CopilotListSnapshot(IReadOnlyList<CopilotItemViewModel> Items, CopilotItemViewModel? SelectedItem);

    private sealed record PersistedCopilotItem(
        string Name,
        string Type,
        string SourcePath,
        string InlinePayload,
        int CopilotId,
        int? TabIndex,
        bool IsRaid,
        bool IsChecked);
}
