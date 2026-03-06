using System.Runtime.CompilerServices;
using System.Threading.Channels;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.CoreBridge;

namespace MAAUnified.Tests;

public sealed class SessionStateSyncTests
{
    [Fact]
    public async Task CallbackStream_DrivesSessionStateTransitions()
    {
        var bridge = new FakeBridge();
        var logService = new UiLogService();
        var configService = CreateConfigService();
        var stateMachine = new SessionStateMachine();
        var session = new UnifiedSessionService(bridge, configService, logService, stateMachine);

        using var cts = new CancellationTokenSource();
        var pumpTask = Task.Run(async () =>
        {
            try
            {
                await session.StartCallbackPumpAsync(_ => Task.CompletedTask, cts.Token);
            }
            catch (OperationCanceledException)
            {
                // expected on cancellation
            }
        });

        bridge.Publish(new CoreCallbackEvent(10001, "TaskChainStart", "{}", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Running);

        bridge.Publish(new CoreCallbackEvent(10004, "TaskChainStopped", "{}", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Connected);

        bridge.Publish(new CoreCallbackEvent(2, "ConnectionInfo", """{"what":"Disconnect"}""", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Idle);

        cts.Cancel();
        bridge.Complete();
        await pumpTask;
    }

    [Fact]
    public async Task CallbackStream_MalformedConnectionInfoPayload_LogsWarning_AndPumpContinues()
    {
        var bridge = new FakeBridge();
        var logService = new UiLogService();
        var session = new UnifiedSessionService(bridge, CreateConfigService(), logService, new SessionStateMachine());

        using var cts = new CancellationTokenSource();
        var pumpTask = Task.Run(async () =>
        {
            try
            {
                await session.StartCallbackPumpAsync(_ => Task.CompletedTask, cts.Token);
            }
            catch (OperationCanceledException)
            {
                // expected on cancellation
            }
        });

        bridge.Publish(new CoreCallbackEvent(2, "ConnectionInfo", "{not-json}", DateTimeOffset.UtcNow));
        bridge.Publish(new CoreCallbackEvent(10001, "TaskChainStart", "{}", DateTimeOffset.UtcNow));

        await WaitUntilAsync(() => session.CurrentState == SessionState.Running);
        Assert.Contains(
            logService.Snapshot,
            log => string.Equals(log.Level, "WARN", StringComparison.Ordinal)
                && log.Message.Contains("ConnectionInfo payload", StringComparison.Ordinal)
                && log.Message.Contains("parse failed", StringComparison.Ordinal));

        cts.Cancel();
        bridge.Complete();
        await pumpTask;
    }

    [Fact]
    public async Task ConnectFailure_FallbackState_IsResolvedFromRuntimeStatus()
    {
        var bridge = new FakeBridge
        {
            ConnectSucceeds = false,
            RuntimeStatus = new CoreRuntimeStatus(Initialized: true, Connected: true, Running: false),
        };
        var session = new UnifiedSessionService(bridge, CreateConfigService(), new UiLogService(), new SessionStateMachine());

        var result = await session.ConnectAsync("127.0.0.1:5555", "General", null);

        Assert.False(result.Success);
        Assert.Equal(SessionState.Connected, session.CurrentState);
    }

    [Fact]
    public async Task StartFailure_FallbackState_IsResolvedFromRuntimeStatus()
    {
        var bridge = new FakeBridge
        {
            StartSucceeds = false,
            RuntimeStatus = new CoreRuntimeStatus(Initialized: true, Connected: true, Running: true),
        };
        var session = new UnifiedSessionService(bridge, CreateConfigService(), new UiLogService(), new SessionStateMachine());

        Assert.True((await session.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        var result = await session.StartAsync();

        Assert.False(result.Success);
        Assert.Equal(SessionState.Running, session.CurrentState);
    }

    [Fact]
    public async Task StopFailure_FallbackState_IsResolvedFromRuntimeStatus()
    {
        var bridge = new FakeBridge
        {
            StopSucceeds = false,
            RuntimeStatus = new CoreRuntimeStatus(Initialized: true, Connected: true, Running: false),
        };
        var session = new UnifiedSessionService(bridge, CreateConfigService(), new UiLogService(), new SessionStateMachine());

        Assert.True((await session.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await session.StartAsync()).Success);
        var result = await session.StopAsync();

        Assert.False(result.Success);
        Assert.Equal(SessionState.Connected, session.CurrentState);
    }

    [Fact]
    public async Task CallbackStream_ConnectionInfoAndTaskChainEvents_ShouldMapToExpectedStates()
    {
        var bridge = new FakeBridge();
        var session = new UnifiedSessionService(bridge, CreateConfigService(), new UiLogService(), new SessionStateMachine());

        using var cts = new CancellationTokenSource();
        var pumpTask = Task.Run(async () =>
        {
            try
            {
                await session.StartCallbackPumpAsync(_ => Task.CompletedTask, cts.Token);
            }
            catch (OperationCanceledException)
            {
                // expected on cancellation
            }
        });

        bridge.Publish(new CoreCallbackEvent(2, "ConnectionInfo", """{"what":"Connected"}""", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Connected);

        bridge.Publish(new CoreCallbackEvent(10001, "TaskChainStart", "{}", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Running);

        bridge.Publish(new CoreCallbackEvent(3, "AllTasksCompleted", "{}", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Connected);

        bridge.Publish(new CoreCallbackEvent(2, "ConnectionInfo", """{"what":"Reconnected"}""", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Connected);

        bridge.Publish(new CoreCallbackEvent(2, "ConnectionInfo", """{"what":"Disconnect"}""", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Idle);

        cts.Cancel();
        bridge.Complete();
        await pumpTask;
    }

    [Fact]
    public async Task CallbackStream_OnEventThrows_ShouldWarnAndContinuePump()
    {
        var bridge = new FakeBridge();
        var logService = new UiLogService();
        var session = new UnifiedSessionService(bridge, CreateConfigService(), logService, new SessionStateMachine());

        using var cts = new CancellationTokenSource();
        var pumpTask = Task.Run(async () =>
        {
            try
            {
                await session.StartCallbackPumpAsync(
                    callback =>
                    {
                        if (string.Equals(callback.MsgName, "TaskChainStart", StringComparison.Ordinal))
                        {
                            throw new InvalidOperationException("synthetic callback error");
                        }

                        return Task.CompletedTask;
                    },
                    cts.Token);
            }
            catch (OperationCanceledException)
            {
                // expected on cancellation
            }
        });

        bridge.Publish(new CoreCallbackEvent(10001, "TaskChainStart", "{}", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Running);

        bridge.Publish(new CoreCallbackEvent(10004, "TaskChainStopped", "{}", DateTimeOffset.UtcNow));
        await WaitUntilAsync(() => session.CurrentState == SessionState.Connected);

        Assert.Contains(
            logService.Snapshot,
            log => string.Equals(log.Level, "WARN", StringComparison.OrdinalIgnoreCase)
                   && log.Message.Contains("Session.Callback handler failed", StringComparison.OrdinalIgnoreCase));

        cts.Cancel();
        bridge.Complete();
        await pumpTask;
    }

    private static async Task WaitUntilAsync(Func<bool> condition, int timeoutMs = 2000)
    {
        var startedAt = Environment.TickCount64;
        while (!condition())
        {
            if (Environment.TickCount64 - startedAt > timeoutMs)
            {
                throw new TimeoutException("Condition not reached in expected time.");
            }

            await Task.Delay(25);
        }
    }

    private static UnifiedConfigurationService CreateConfigService()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(Path.Combine(root, "config"));

        var store = new AvaloniaJsonConfigStore(root);
        var log = new UiLogService();
        return new UnifiedConfigurationService(store, new GuiNewJsonConfigImporter(), new GuiJsonConfigImporter(), log, root);
    }

    private sealed class FakeBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _channel = Channel.CreateUnbounded<CoreCallbackEvent>();

        public bool ConnectSucceeds { get; init; } = true;

        public bool StartSucceeds { get; init; } = true;

        public bool StopSucceeds { get; init; } = true;

        public CoreRuntimeStatus RuntimeStatus { get; init; } = new(true, true, false);

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
            => Task.FromResult(ConnectSucceeds
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.ConnectFailed, "connect failed")));

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(1));

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(StartSucceeds
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StartFailed, "start failed")));

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(StopSucceeds
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StopFailed, "stop failed")));

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(RuntimeStatus));

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await foreach (var callback in _channel.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public ValueTask DisposeAsync()
        {
            _channel.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }

        public void Publish(CoreCallbackEvent callback) => _channel.Writer.TryWrite(callback);

        public void Complete() => _channel.Writer.TryComplete();
    }
}
