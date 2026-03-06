using System.Runtime.CompilerServices;
using System.Threading.Channels;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;

namespace MAAUnified.Tests;

public sealed class ConnectFeatureWaitAndStopTests
{
    [Fact]
    public async Task WaitAndStopAsync_InvalidWait_ShouldReturnInvalidWaitTime()
    {
        await using var fixture = await TestFixture.CreateAsync();

        var result = await fixture.Connect.WaitAndStopAsync(TimeSpan.Zero);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.InvalidWaitTime, result.Error?.Code);
        Assert.Equal(0, fixture.Bridge.StopCallCount);
    }

    [Fact]
    public async Task WaitAndStopAsync_StoppedDuringWait_ShouldNotCallStopTwice()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.Connect.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Connect.StartAsync()).Success);

        var waitTask = fixture.Connect.WaitAndStopAsync(TimeSpan.FromMilliseconds(500));
        await Task.Delay(50);

        var manualStop = await fixture.Connect.StopAsync();
        var waitResult = await waitTask;

        Assert.True(manualStop.Success);
        Assert.True(waitResult.Success);
        Assert.Equal(1, fixture.Bridge.StopCallCount);
        Assert.Equal(SessionState.Connected, fixture.Session.CurrentState);
    }

    [Fact]
    public async Task WaitAndStopAsync_TimeoutWhileRunning_ShouldCallStopOnce()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.Connect.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Connect.StartAsync()).Success);

        var result = await fixture.Connect.WaitAndStopAsync(TimeSpan.FromMilliseconds(20));

        Assert.True(result.Success);
        Assert.Equal(1, fixture.Bridge.StopCallCount);
        Assert.Equal(SessionState.Connected, fixture.Session.CurrentState);
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(
            string root,
            WaitStopBridge bridge,
            UnifiedSessionService session,
            ConnectFeatureService connect)
        {
            Root = root;
            Bridge = bridge;
            Session = session;
            Connect = connect;
        }

        public string Root { get; }

        public WaitStopBridge Bridge { get; }

        public UnifiedSessionService Session { get; }

        public ConnectFeatureService Connect { get; }

        public static async Task<TestFixture> CreateAsync()
        {
            var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(Path.Combine(root, "config"));

            var store = new AvaloniaJsonConfigStore(root);
            var log = new UiLogService();
            var config = new UnifiedConfigurationService(store, new GuiNewJsonConfigImporter(), new GuiJsonConfigImporter(), log, root);
            await config.LoadOrBootstrapAsync();

            var bridge = new WaitStopBridge();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var connect = new ConnectFeatureService(session, config);
            return new TestFixture(root, bridge, session, connect);
        }

        public async ValueTask DisposeAsync()
        {
            await Bridge.DisposeAsync();
            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // ignore temp cleanup failures
            }
        }
    }

    private sealed class WaitStopBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _callbacks = Channel.CreateUnbounded<CoreCallbackEvent>();
        private bool _connected;
        private bool _running;

        public int StopCallCount { get; private set; }

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
        {
            _connected = !string.IsNullOrWhiteSpace(connectionInfo.Address);
            return Task.FromResult(_connected
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.ConnectFailed, "connect failed")));
        }

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(1));

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            if (!_connected)
            {
                return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StartFailed, "not connected")));
            }

            _running = true;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            StopCallCount++;
            var wasRunning = _running;
            _running = false;
            return Task.FromResult(wasRunning
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StopFailed, "not running")));
        }

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, _connected, _running)));

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await foreach (var callback in _callbacks.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public ValueTask DisposeAsync()
        {
            _callbacks.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }
    }
}
