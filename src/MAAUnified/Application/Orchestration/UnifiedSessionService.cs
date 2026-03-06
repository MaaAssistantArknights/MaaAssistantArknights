using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;
using MAAUnified.CoreBridge;
using System.Text.Json;

namespace MAAUnified.Application.Orchestration;

public sealed class UnifiedSessionService
{
    private readonly IMaaCoreBridge _bridge;
    private readonly UnifiedConfigurationService _configService;
    private readonly UiLogService _logService;
    private readonly SessionStateMachine _stateMachine;
    private readonly object _taskIndexMapGate = new();
    private readonly object _runOwnerGate = new();
    private readonly Dictionary<int, int> _taskIndexByCoreTaskId = new();
    private string? _currentRunOwner;

    public UnifiedSessionService(
        IMaaCoreBridge bridge,
        UnifiedConfigurationService configService,
        UiLogService logService,
        SessionStateMachine stateMachine)
    {
        _bridge = bridge;
        _configService = configService;
        _logService = logService;
        _stateMachine = stateMachine;
        _stateMachine.StateChanged += OnSessionStateChanged;
    }

    public SessionState CurrentState => _stateMachine.CurrentState;

    public event Action<SessionState>? SessionStateChanged;

    public event Action<CoreCallbackEvent>? CallbackReceived;

    public string? CurrentRunOwner
    {
        get
        {
            lock (_runOwnerGate)
            {
                return _currentRunOwner;
            }
        }
    }

    public bool TryBeginRun(string owner, out string? currentOwner)
    {
        currentOwner = null;
        if (string.IsNullOrWhiteSpace(owner))
        {
            return false;
        }

        var normalizedOwner = owner.Trim();
        lock (_runOwnerGate)
        {
            if (!string.IsNullOrWhiteSpace(_currentRunOwner)
                && !string.Equals(_currentRunOwner, normalizedOwner, StringComparison.Ordinal))
            {
                currentOwner = _currentRunOwner;
                return false;
            }

            _currentRunOwner = normalizedOwner;
            currentOwner = _currentRunOwner;
            return true;
        }
    }

    public bool IsRunOwner(string owner)
    {
        if (string.IsNullOrWhiteSpace(owner))
        {
            return false;
        }

        lock (_runOwnerGate)
        {
            return string.Equals(_currentRunOwner, owner.Trim(), StringComparison.Ordinal);
        }
    }

    public void EndRun(string owner)
    {
        if (string.IsNullOrWhiteSpace(owner))
        {
            return;
        }

        lock (_runOwnerGate)
        {
            if (string.Equals(_currentRunOwner, owner.Trim(), StringComparison.Ordinal))
            {
                _currentRunOwner = null;
            }
        }
    }

    public bool TryResolveTaskIndexByCoreTaskId(int taskId, out int taskIndex)
    {
        lock (_taskIndexMapGate)
        {
            return _taskIndexByCoreTaskId.TryGetValue(taskId, out taskIndex);
        }
    }

    private void ClearTaskIdMappings()
    {
        lock (_taskIndexMapGate)
        {
            _taskIndexByCoreTaskId.Clear();
        }
    }

    private void SetTaskIdMapping(int coreTaskId, int queueIndex)
    {
        lock (_taskIndexMapGate)
        {
            _taskIndexByCoreTaskId[coreTaskId] = queueIndex;
        }
    }

    public async Task<CoreResult<bool>> ConnectAsync(string address, string connectConfig, string? adbPath, CancellationToken cancellationToken = default)
    {
        MoveToState(SessionState.Connecting, "Session.Connect", "begin");
        var result = await _bridge.ConnectAsync(new CoreConnectionInfo(address, connectConfig, adbPath), cancellationToken);
        if (result.Success)
        {
            MoveToState(SessionState.Connected, "Session.Connect", "connected");
            _logService.Info($"Connected to {address}");
        }
        else
        {
            var fallbackState = await ResolveFailureStateAsync("Session.Connect", SessionState.Idle, cancellationToken);
            MoveToState(fallbackState, "Session.Connect", "connect-failed-fallback");
            _logService.Warn($"Failed to connect to {address}: {result.Error?.Code} {result.Error?.Message}");
        }

        return result;
    }

    public async Task<CoreResult<int>> AppendTasksFromCurrentProfileAsync(CancellationToken cancellationToken = default)
    {
        if (!_configService.CurrentConfig.Profiles.TryGetValue(_configService.CurrentConfig.CurrentProfile, out var profile))
        {
            _logService.Warn($"Current profile `{_configService.CurrentConfig.CurrentProfile}` not found");
            return CoreResult<int>.Fail(new CoreError(CoreErrorCode.InvalidRequest, "Current profile was not found."));
        }

        ClearTaskIdMappings();

        int appended = 0;
        for (var queueIndex = 0; queueIndex < profile.TaskQueue.Count; queueIndex++)
        {
            var task = profile.TaskQueue[queueIndex];
            if (!task.IsEnabled)
            {
                continue;
            }

            var compiled = TaskParamCompiler.CompileTask(task, profile, _configService.CurrentConfig, strict: true);
            var blockingIssues = compiled.Issues.Where(i => i.Blocking).ToList();
            if (blockingIssues.Count > 0)
            {
                var details = string.Join(
                    "; ",
                    blockingIssues.Select(i => $"{i.Code}:{i.Field}:{i.Message}"));
                _logService.Warn($"Append task blocked `{task.Name}`: {details}");
                ClearTaskIdMappings();
                return CoreResult<int>.Fail(new CoreError(
                    CoreErrorCode.InvalidRequest,
                    $"Task `{task.Name}` validation failed: {details}"));
            }

            foreach (var warning in compiled.Issues.Where(i => !i.Blocking))
            {
                _logService.Warn($"Task warning `{task.Name}`: {warning.Code}:{warning.Field}:{warning.Message}");
            }

            task.Type = compiled.NormalizedType;
            task.Params = compiled.Params;
            var appendResult = await _bridge.AppendTaskAsync(
                new CoreTaskRequest(compiled.NormalizedType, task.Name, task.IsEnabled, compiled.Params.ToJsonString()),
                cancellationToken);

            if (!appendResult.Success)
            {
                _logService.Warn($"Append task failed `{task.Name}`: {appendResult.Error?.Code} {appendResult.Error?.Message}");
                ClearTaskIdMappings();
                return CoreResult<int>.Fail(appendResult.Error!);
            }

            SetTaskIdMapping(appendResult.Value, queueIndex);

            appended += 1;
            _logService.Info($"Appended task #{appendResult.Value}: {task.Name}");
        }

        if (appended == 0)
        {
            _logService.Warn("No enabled tasks in current profile to append");
        }

        return CoreResult<int>.Ok(appended);
    }

    public async Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
    {
        var result = await _bridge.StartAsync(cancellationToken);
        if (result.Success)
        {
            MoveToState(SessionState.Running, "Session.Start", "started");
        }
        else
        {
            var fallbackState = await ResolveFailureStateAsync("Session.Start", SessionState.Connected, cancellationToken);
            MoveToState(fallbackState, "Session.Start", "start-failed-fallback");
        }

        _logService.Info(result.Success ? "Task execution started" : $"Task execution failed to start: {result.Error?.Code} {result.Error?.Message}");
        return result;
    }

    public async Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
    {
        MoveToState(SessionState.Stopping, "Session.Stop", "begin-stop");
        var result = await _bridge.StopAsync(cancellationToken);
        if (result.Success)
        {
            MoveToState(SessionState.Connected, "Session.Stop", "stopped");
        }
        else
        {
            var fallbackState = await ResolveFailureStateAsync("Session.Stop", SessionState.Connected, cancellationToken);
            MoveToState(fallbackState, "Session.Stop", "stop-failed-fallback");
        }

        _logService.Info(result.Success ? "Task execution stopped" : $"Task execution stop failed: {result.Error?.Code} {result.Error?.Message}");
        return result;
    }

    public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
    {
        return _bridge.GetRuntimeStatusAsync(cancellationToken);
    }

    public async Task StartCallbackPumpAsync(Func<CoreCallbackEvent, Task> onEvent, CancellationToken cancellationToken = default)
    {
        await foreach (var callback in _bridge.CallbackStreamAsync(cancellationToken))
        {
            try
            {
                ApplyCallbackToState(callback);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                throw;
            }
            catch (Exception ex)
            {
                _logService.Warn(
                    $"Session.Callback state mapping failed for {callback.MsgName}({callback.MsgId}): {ex.Message}");
            }

            try
            {
                CallbackReceived?.Invoke(callback);
            }
            catch (Exception ex)
            {
                _logService.Warn(
                    $"Session.Callback subscriber failed for {callback.MsgName}({callback.MsgId}): {ex.Message}");
            }

            try
            {
                await onEvent(callback);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                throw;
            }
            catch (Exception ex)
            {
                _logService.Warn(
                    $"Session.Callback handler failed for {callback.MsgName}({callback.MsgId}): {ex.Message}");
            }
        }
    }

    private void ApplyCallbackToState(CoreCallbackEvent callback)
    {
        if (string.Equals(callback.MsgName, "TaskChainStart", StringComparison.OrdinalIgnoreCase))
        {
            MoveToState(SessionState.Running, "Session.Callback", "TaskChainStart");
            return;
        }

        if (string.Equals(callback.MsgName, "TaskChainCompleted", StringComparison.OrdinalIgnoreCase)
            || string.Equals(callback.MsgName, "TaskChainStopped", StringComparison.OrdinalIgnoreCase)
            || string.Equals(callback.MsgName, "AllTasksCompleted", StringComparison.OrdinalIgnoreCase))
        {
            MoveToState(SessionState.Connected, "Session.Callback", callback.MsgName);
            return;
        }

        if (!string.Equals(callback.MsgName, "ConnectionInfo", StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        var parse = ParseCallbackWhat(callback.PayloadJson);
        if (!parse.Success)
        {
            _logService.Warn(
                $"Session.Callback ignored ConnectionInfo payload: {parse.ParseError}; msgId={callback.MsgId}; msgName={callback.MsgName}");
            return;
        }

        var what = parse.What;
        if (string.IsNullOrWhiteSpace(what))
        {
            _logService.Warn(
                $"Session.Callback ignored ConnectionInfo payload: {parse.ParseError ?? "property `what` is missing"}; msgId={callback.MsgId}; msgName={callback.MsgName}");
            return;
        }

        if (string.Equals(what, "Connected", StringComparison.OrdinalIgnoreCase)
            || string.Equals(what, "Reconnected", StringComparison.OrdinalIgnoreCase))
        {
            MoveToState(SessionState.Connected, "Session.Callback", $"ConnectionInfo:{what}");
            return;
        }

        if (string.Equals(what, "Disconnect", StringComparison.OrdinalIgnoreCase))
        {
            MoveToState(SessionState.Idle, "Session.Callback", "ConnectionInfo:Disconnect");
            return;
        }

        _logService.Warn(
            $"Session.Callback ignored unknown ConnectionInfo.what `{what}`; msgId={callback.MsgId}; msgName={callback.MsgName}");
    }

    private void OnSessionStateChanged(SessionState state)
    {
        _logService.Info($"Session state -> {state}");
        SessionStateChanged?.Invoke(state);
    }

    private void MoveToState(SessionState state, string scope, string reason)
    {
        var previous = _stateMachine.CurrentState;
        if (_stateMachine.TryMoveTo(state))
        {
            return;
        }

        _logService.Warn(
            $"{scope} ignored invalid state transition: {previous} -> {state}; reason={reason}");
    }

    private async Task<SessionState> ResolveFailureStateAsync(
        string scope,
        SessionState fallbackWhenStatusUnavailable,
        CancellationToken cancellationToken)
    {
        var runtimeResult = await _bridge.GetRuntimeStatusAsync(cancellationToken);
        if (!runtimeResult.Success || runtimeResult.Value is null)
        {
            _logService.Warn(
                $"{scope} fallback runtime status unavailable, use {fallbackWhenStatusUnavailable}: {runtimeResult.Error?.Code} {runtimeResult.Error?.Message}");
            return fallbackWhenStatusUnavailable;
        }

        var resolved = MapRuntimeStatusToSessionState(runtimeResult.Value);
        _logService.Warn(
            $"{scope} fallback resolved from runtime status: initialized={runtimeResult.Value.Initialized}, connected={runtimeResult.Value.Connected}, running={runtimeResult.Value.Running} -> {resolved}");
        return resolved;
    }

    private static SessionState MapRuntimeStatusToSessionState(CoreRuntimeStatus status)
    {
        if (status.Running)
        {
            return SessionState.Running;
        }

        if (status.Connected)
        {
            return SessionState.Connected;
        }

        return SessionState.Idle;
    }

    private static (bool Success, string? What, string? ParseError) ParseCallbackWhat(string payloadJson)
    {
        if (string.IsNullOrWhiteSpace(payloadJson))
        {
            return (false, null, "payload is empty");
        }

        try
        {
            using var doc = JsonDocument.Parse(payloadJson);
            if (doc.RootElement.ValueKind != JsonValueKind.Object)
            {
                return (false, null, "payload is not a JSON object");
            }

            foreach (var property in doc.RootElement.EnumerateObject())
            {
                if (!string.Equals(property.Name, "what", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                if (property.Value.ValueKind != JsonValueKind.String)
                {
                    return (false, null, "property `what` is not a string");
                }

                var value = property.Value.GetString();
                return string.IsNullOrWhiteSpace(value)
                    ? (false, null, "property `what` is empty")
                    : (true, value, null);
            }

            return (false, null, "property `what` is missing");
        }
        catch (JsonException ex)
        {
            return (false, null, $"payload parse failed: {ex.Message}");
        }
    }
}
