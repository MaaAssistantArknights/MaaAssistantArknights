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
        _stateMachine.StateChanged += state => _logService.Info($"Session state -> {state}");
    }

    public SessionState CurrentState => _stateMachine.CurrentState;

    public event Action<CoreCallbackEvent>? CallbackReceived;

    public async Task<CoreResult<bool>> ConnectAsync(string address, string connectConfig, string? adbPath, CancellationToken cancellationToken = default)
    {
        _stateMachine.MoveTo(SessionState.Connecting);
        var result = await _bridge.ConnectAsync(new CoreConnectionInfo(address, connectConfig, adbPath), cancellationToken);
        if (result.Success)
        {
            _stateMachine.MoveTo(SessionState.Connected);
            _logService.Info($"Connected to {address}");
        }
        else
        {
            _stateMachine.MoveTo(SessionState.Idle);
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

        int appended = 0;
        foreach (var task in profile.TaskQueue.Where(t => t.IsEnabled))
        {
            var compiled = TaskParamCompiler.CompileTask(task, profile, _configService.CurrentConfig, strict: true);
            var blockingIssues = compiled.Issues.Where(i => i.Blocking).ToList();
            if (blockingIssues.Count > 0)
            {
                var details = string.Join(
                    "; ",
                    blockingIssues.Select(i => $"{i.Code}:{i.Field}:{i.Message}"));
                _logService.Warn($"Append task blocked `{task.Name}`: {details}");
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
                return CoreResult<int>.Fail(appendResult.Error!);
            }

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
        _stateMachine.MoveTo(result.Success ? SessionState.Running : SessionState.Connected);
        _logService.Info(result.Success ? "Task execution started" : $"Task execution failed to start: {result.Error?.Code} {result.Error?.Message}");
        return result;
    }

    public async Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
    {
        _stateMachine.MoveTo(SessionState.Stopping);
        var result = await _bridge.StopAsync(cancellationToken);
        _stateMachine.MoveTo(result.Success ? SessionState.Connected : SessionState.Idle);
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
            ApplyCallbackToState(callback);
            CallbackReceived?.Invoke(callback);
            await onEvent(callback);
        }
    }

    private void ApplyCallbackToState(CoreCallbackEvent callback)
    {
        if (string.Equals(callback.MsgName, "TaskChainStart", StringComparison.OrdinalIgnoreCase))
        {
            _stateMachine.MoveTo(SessionState.Running);
            return;
        }

        if (string.Equals(callback.MsgName, "TaskChainCompleted", StringComparison.OrdinalIgnoreCase)
            || string.Equals(callback.MsgName, "TaskChainStopped", StringComparison.OrdinalIgnoreCase)
            || string.Equals(callback.MsgName, "AllTasksCompleted", StringComparison.OrdinalIgnoreCase))
        {
            _stateMachine.MoveTo(SessionState.Connected);
            return;
        }

        if (!string.Equals(callback.MsgName, "ConnectionInfo", StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        var what = ReadCallbackWhat(callback.PayloadJson);
        if (string.IsNullOrWhiteSpace(what))
        {
            return;
        }

        if (string.Equals(what, "Connected", StringComparison.OrdinalIgnoreCase)
            || string.Equals(what, "Reconnected", StringComparison.OrdinalIgnoreCase))
        {
            _stateMachine.MoveTo(SessionState.Connected);
            return;
        }

        if (string.Equals(what, "Disconnect", StringComparison.OrdinalIgnoreCase))
        {
            _stateMachine.MoveTo(SessionState.Idle);
        }
    }

    private static string? ReadCallbackWhat(string payloadJson)
    {
        if (string.IsNullOrWhiteSpace(payloadJson))
        {
            return null;
        }

        try
        {
            using var doc = JsonDocument.Parse(payloadJson);
            if (doc.RootElement.TryGetProperty("what", out var whatProp) && whatProp.ValueKind == JsonValueKind.String)
            {
                return whatProp.GetString();
            }
        }
        catch
        {
            // ignore malformed callback payload
        }

        return null;
    }
}
