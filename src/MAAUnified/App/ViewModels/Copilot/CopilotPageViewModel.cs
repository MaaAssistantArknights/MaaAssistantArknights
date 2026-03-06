using System.Collections.ObjectModel;
using System.Text.Json;
using System.Text.Json.Nodes;
using Avalonia.Threading;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.CoreBridge;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.App.ViewModels.Copilot;

internal readonly record struct CopilotCallbackPayload(string? TaskChain, int? TaskId, string? ParseError = null)
{
    public static CopilotCallbackPayload Empty { get; } = new(null, null, null);

    public bool HasParseError => !string.IsNullOrWhiteSpace(ParseError);
}

public sealed class CopilotPageViewModel : PageViewModelBase
{
    private const string CopilotTaskListConfigScope = "Config.Copilot.CopilotTaskList";
    private const string CopilotRunOwner = "Copilot";

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

    public CopilotPageViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
        Types = new[] { "主线", "SSS", "悖论", "活动" };
        Items = new ObservableCollection<CopilotItemViewModel>();
        Items.CollectionChanged += (_, _) => NotifySelectionDerivedPropertiesChanged();
        Logs = new ObservableCollection<string>();
        runtime.SessionService.CallbackReceived += callback => _ = HandleCallbackAsync(callback);
        runtime.SessionService.SessionStateChanged += OnSessionStateChanged;
        _currentSessionState = runtime.SessionService.CurrentState;
        LoadPersistedItems();
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
        if (SelectedItem is null)
        {
            StatusMessage = "启动失败。";
            LastErrorMessage = "请选择要执行的作业。";
            await RecordFailedResultAsync(
                "Copilot.Start",
                UiOperationResult.Fail(UiErrorCode.CopilotSelectionMissing, LastErrorMessage),
                cancellationToken);
            return;
        }

        if (!CanStart)
        {
            LastErrorMessage = $"Session state `{CurrentSessionState}` does not allow start.";
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
            var selected = SelectedItem;
            if (selected is null)
            {
                StatusMessage = "启动失败。";
                LastErrorMessage = "请选择要执行的作业。";
                await RecordFailedResultAsync(
                    "Copilot.Start",
                    UiOperationResult.Fail(UiErrorCode.CopilotSelectionMissing, LastErrorMessage),
                    cancellationToken);
                return;
            }

            var appendResult = await AppendSelectedItemAsync(selected, cancellationToken);
            if (appendResult is null)
            {
                return;
            }

            _activeItemName = selected.Name;
            _activeItemCoreTaskId = appendResult.Value;
            _activeTaskChain = ResolveCopilotTaskChain(selected.Type);
            _hasActiveRun = true;
            selected.Status = "Queued";

            if (!await ApplyResultAsync(await Runtime.ConnectFeatureService.StartAsync(cancellationToken), "Copilot.Start", cancellationToken))
            {
                _hasActiveRun = false;
                _activeItemName = null;
                _activeItemCoreTaskId = null;
                _activeTaskChain = null;
                selected.Status = "Ready";
                return;
            }

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
            LastErrorMessage = $"Session state `{CurrentSessionState}` does not allow stop.";
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
        cancellationToken.ThrowIfCancellationRequested();
        if (!string.IsNullOrWhiteSpace(item.SourcePath))
        {
            if (!File.Exists(item.SourcePath))
            {
                StatusMessage = "启动失败。";
                LastErrorMessage = $"作业文件不存在：{item.SourcePath}";
                await RecordFailedResultAsync(
                    "Copilot.Start.Input",
                    UiOperationResult.Fail(UiErrorCode.CopilotFileNotFound, LastErrorMessage),
                    cancellationToken);
                return null;
            }

            return item.SourcePath;
        }

        if (string.IsNullOrWhiteSpace(item.InlinePayload))
        {
            StatusMessage = "启动失败。";
            LastErrorMessage = "当前作业缺少可执行来源（文件路径或 JSON 内容）。";
            await RecordFailedResultAsync(
                "Copilot.Start.Input",
                UiOperationResult.Fail(UiErrorCode.CopilotFileMissing, LastErrorMessage),
                cancellationToken);
            return null;
        }

        var debugDirectory = Path.GetDirectoryName(Runtime.DiagnosticsService.EventLogPath)
            ?? Path.Combine(AppContext.BaseDirectory, "debug");
        var directory = Path.Combine(debugDirectory, "copilot-cache");
        Directory.CreateDirectory(directory);
        var filePath = Path.Combine(directory, $"copilot-inline-{DateTimeOffset.UtcNow.ToUnixTimeMilliseconds()}.json");
        await File.WriteAllTextAsync(filePath, item.InlinePayload, cancellationToken);
        item.SourcePath = filePath;
        return filePath;
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
                ["formation"] = AutoSquad,
                ["support_unit_usage"] = UseSupportUnit ? 1 : 0,
                ["add_trust"] = AddTrust,
            };
        }

        return new CoreTaskRequest(taskType, item.Name, true, payload.ToJsonString());
    }

    private static string ResolveCopilotTaskType(string type)
    {
        return type switch
        {
            "SSS" => "SSSCopilot",
            "悖论" => "ParadoxCopilot",
            _ => "Copilot",
        };
    }

    private static string ResolveCopilotTaskChain(string type)
    {
        return ResolveCopilotTaskType(type);
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
        var result = await Runtime.CopilotFeatureService.SubmitFeedbackAsync(itemName, like, cancellationToken);
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
            using var doc = JsonDocument.Parse(payloadJson);
            if (doc.RootElement.ValueKind != JsonValueKind.Object)
            {
                return new CopilotCallbackPayload(null, null, "payload is not a JSON object");
            }

            string? taskChain = null;
            int? taskId = null;
            foreach (var prop in doc.RootElement.EnumerateObject())
            {
                var key = prop.Name.ToLowerInvariant();
                if ((key is "taskchain" or "task_chain")
                    && prop.Value.ValueKind == JsonValueKind.String)
                {
                    taskChain = prop.Value.GetString();
                    continue;
                }

                if (key is not ("taskid" or "task_id"))
                {
                    continue;
                }

                if (prop.Value.ValueKind == JsonValueKind.Number && prop.Value.TryGetInt32(out var taskIdValue))
                {
                    taskId = taskIdValue;
                }
                else if (prop.Value.ValueKind == JsonValueKind.String
                         && int.TryParse(prop.Value.GetString(), out taskIdValue))
                {
                    taskId = taskIdValue;
                }
            }

            return new CopilotCallbackPayload(taskChain, taskId);
        }
        catch (JsonException ex)
        {
            return new CopilotCallbackPayload(null, null, $"payload parse failed: {ex.Message}");
        }
    }

    private async Task HandleCallbackAsync(CoreCallbackEvent callback)
    {
        await Dispatcher.UIThread.InvokeAsync(() => ApplyRuntimeCallback(callback));
    }

    internal void ApplyRuntimeCallback(CoreCallbackEvent callback)
    {
        Logs.Add($"[{callback.Timestamp:HH:mm:ss}] {callback.MsgName} {callback.PayloadJson}");
        const int maxLogs = 200;
        while (Logs.Count > maxLogs)
        {
            Logs.RemoveAt(0);
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

        var metadata = ParseCopilotCallbackPayload(callback.PayloadJson);
        if (metadata.HasParseError)
        {
            var warning = $"msgId={callback.MsgId}; msgName={callback.MsgName}; {metadata.ParseError}";
            Runtime.LogService.Warn($"Copilot callback payload parse failed: {warning}");
            _ = RecordEventAsync("Copilot.Callback.Parse", warning);
        }

        if (!IsCopilotCallbackForActiveRun(metadata))
        {
            return;
        }

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
            Items.Add(new CopilotItemViewModel(item.Name, item.Type, item.SourcePath, item.InlinePayload));
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
            _ = TryGetOptionalStringProperty(obj, "inline_payload", out var inlinePayload)
                || TryGetOptionalStringProperty(obj, "InlinePayload", out inlinePayload);
            items.Add(new PersistedCopilotItem(name, type, sourcePath, inlinePayload));
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
            return type;
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
            .Select(item => new PersistedCopilotItem(item.Name, item.Type, item.SourcePath, item.InlinePayload))
            .ToArray();
        return JsonSerializer.Serialize(items);
    }

    private sealed record CopilotListSnapshot(IReadOnlyList<CopilotItemViewModel> Items, CopilotItemViewModel? SelectedItem);

    private sealed record PersistedCopilotItem(string Name, string Type, string SourcePath, string InlinePayload);
}
