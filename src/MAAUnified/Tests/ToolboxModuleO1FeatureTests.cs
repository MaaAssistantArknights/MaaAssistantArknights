using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Toolbox;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class ToolboxModuleO1FeatureTests
{
    [Fact]
    public async Task ExecuteToolAsync_ParameterTooLong_ShouldFailWithInvalidParameters()
    {
        var service = CreateService(static (request, _) => Task.FromResult(UiOperationResult<string>.Ok("ok", "ok")));
        var request = new ToolboxExecuteRequest(ToolboxToolKind.Recruit, new string('x', 4097));

        var result = await service.ExecuteToolAsync(request);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.ToolboxInvalidParameters, result.Error?.Code);
    }

    [Fact]
    public async Task ExecuteToolAsync_Timeout_ShouldFailWithTimeoutCode()
    {
        var service = CreateService(
            static async (_, cancellationToken) =>
            {
                await Task.Delay(200, cancellationToken);
                return UiOperationResult<string>.Ok("ok", "ok");
            },
            timeout: TimeSpan.FromMilliseconds(30));

        var result = await service.ExecuteToolAsync(new ToolboxExecuteRequest(ToolboxToolKind.OperBox, "a=1"));

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.ToolboxExecutionTimedOut, result.Error?.Code);
    }

    [Fact]
    public async Task ExecuteToolAsync_CallerCancelled_ShouldFailWithCancelledCode()
    {
        var service = CreateService(static (_, _) => Task.FromResult(UiOperationResult<string>.Ok("ok", "ok")));
        using var cts = new CancellationTokenSource();
        cts.Cancel();

        var result = await service.ExecuteToolAsync(
            new ToolboxExecuteRequest(ToolboxToolKind.Depot, "a=1"),
            cts.Token);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.ToolboxExecutionCancelled, result.Error?.Code);
    }

    [Fact]
    public async Task ExecuteToolAsync_HandlerFailure_ShouldNormalizeUnknownCode()
    {
        var service = CreateService(static (_, _) =>
            Task.FromResult(UiOperationResult<string>.Fail("UnknownCode", "boom")));

        var result = await service.ExecuteToolAsync(new ToolboxExecuteRequest(ToolboxToolKind.Gacha, "a=1"));

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.ToolboxExecutionFailed, result.Error?.Code);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_Success_ShouldTransitToSucceeded()
    {
        var toolbox = new ScriptedToolboxFeatureService(
            static request => Task.FromResult(UiOperationResult<ToolboxExecuteResult>.Ok(
                new ToolboxExecuteResult(
                    request.Tool,
                    $"done:{request.Tool}",
                    "k=v",
                    DateTimeOffset.Now),
                "ok")));

        await using var fixture = await RuntimeFixture.CreateAsync(toolbox);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
            CurrentToolParameters = "k=v",
        };

        Assert.Equal(ToolboxExecutionState.Idle, vm.ExecutionState);

        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(ToolboxExecutionState.Succeeded, vm.ExecutionState);
        Assert.False(vm.IsExecuting);
        Assert.Equal(string.Empty, vm.LastExecutionErrorCode);
        Assert.Contains("done:", vm.ResultText, StringComparison.Ordinal);
        var record = Assert.Single(vm.ExecutionHistory);
        Assert.True(record.Success);
        Assert.Equal(1, toolbox.CallCount);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_DisclaimerNotAccepted_ShouldNotCallService()
    {
        var toolbox = new ScriptedToolboxFeatureService(
            static _ => Task.FromResult(UiOperationResult<ToolboxExecuteResult>.Ok(
                new ToolboxExecuteResult(ToolboxToolKind.Recruit, "ok", "none", DateTimeOffset.Now),
                "ok")));

        await using var fixture = await RuntimeFixture.CreateAsync(toolbox);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = false,
            CurrentToolParameters = "a=1",
        };

        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(0, toolbox.CallCount);
        Assert.Equal(ToolboxExecutionState.Failed, vm.ExecutionState);
        Assert.Equal(UiErrorCode.ToolboxDisclaimerNotAccepted, vm.LastExecutionErrorCode);
        Assert.Contains(UiErrorCode.ToolboxDisclaimerNotAccepted, vm.ResultText, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_Failure_ShouldTransitToFailed_AndWriteErrorLog()
    {
        var toolbox = new ScriptedToolboxFeatureService(
            static _ => Task.FromResult(UiOperationResult<ToolboxExecuteResult>.Fail(
                UiErrorCode.ToolboxExecutionTimedOut,
                "timed out")));

        await using var fixture = await RuntimeFixture.CreateAsync(toolbox);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
            CurrentToolParameters = "a=1",
        };

        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(ToolboxExecutionState.Failed, vm.ExecutionState);
        Assert.False(vm.IsExecuting);
        Assert.Equal(UiErrorCode.ToolboxExecutionTimedOut, vm.LastExecutionErrorCode);
        Assert.Contains(UiErrorCode.ToolboxExecutionTimedOut, vm.ResultText, StringComparison.Ordinal);
        var errorLog = await File.ReadAllTextAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath);
        Assert.Contains("Toolbox.Recruit", errorLog, StringComparison.Ordinal);
        Assert.Contains($"code={UiErrorCode.ToolboxExecutionTimedOut}", errorLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_SameErrorCodeAcrossTools_ShouldUseSameResultText()
    {
        var toolbox = new ScriptedToolboxFeatureService(
            static _ => Task.FromResult(UiOperationResult<ToolboxExecuteResult>.Fail(
                UiErrorCode.ToolboxInvalidParameters,
                "bad params")));

        await using var fixture = await RuntimeFixture.CreateAsync(toolbox);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        vm.SelectedTabIndex = 0;
        await vm.ExecuteCurrentToolAsync();
        var first = vm.ResultText;

        vm.SelectedTabIndex = 1;
        await vm.ExecuteCurrentToolAsync();
        var second = vm.ResultText;

        Assert.Equal(first, second);
        Assert.Contains(UiErrorCode.ToolboxInvalidParameters, first, StringComparison.Ordinal);
    }

    private static ToolboxFeatureService CreateService(
        Func<ToolboxExecuteRequest, CancellationToken, Task<UiOperationResult<string>>> handler,
        TimeSpan? timeout = null)
    {
        var handlers = Enum.GetValues<ToolboxToolKind>()
            .ToDictionary(tool => tool, _ => handler);
        var timeoutValue = timeout ?? TimeSpan.FromSeconds(10);
        var timeouts = Enum.GetValues<ToolboxToolKind>()
            .ToDictionary(tool => tool, _ => timeoutValue);
        return new ToolboxFeatureService(handlers, timeouts);
    }

    private sealed class ScriptedToolboxFeatureService(
        Func<ToolboxExecuteRequest, Task<UiOperationResult<ToolboxExecuteResult>>> handler) : IToolboxFeatureService
    {
        public int CallCount { get; private set; }

        public async Task<UiOperationResult<ToolboxExecuteResult>> ExecuteToolAsync(
            ToolboxExecuteRequest request,
            CancellationToken cancellationToken = default)
        {
            CallCount++;
            cancellationToken.ThrowIfCancellationRequested();
            return await handler(request);
        }
    }

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private RuntimeFixture(string root, MAAUnifiedRuntime runtime)
        {
            Root = root;
            Runtime = runtime;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public static async Task<RuntimeFixture> CreateAsync(IToolboxFeatureService toolboxFeatureService)
        {
            var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(Path.Combine(root, "config"));

            var log = new UiLogService();
            var diagnostics = new UiDiagnosticsService(root, log);
            var config = new UnifiedConfigurationService(
                new AvaloniaJsonConfigStore(root),
                new GuiNewJsonConfigImporter(),
                new GuiJsonConfigImporter(),
                log,
                root);
            await config.LoadOrBootstrapAsync();

            var bridge = new NullBridge();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var platform = new PlatformServiceBundle
            {
                TrayService = new NoOpTrayService(),
                NotificationService = new NoOpNotificationService(),
                HotkeyService = new NoOpGlobalHotkeyService(),
                AutostartService = new NoOpAutostartService(),
                FileDialogService = new NoOpFileDialogService(),
                OverlayService = new NoOpOverlayCapabilityService(),
                PostActionExecutorService = new NoOpPostActionExecutorService(),
            };

            var capability = new PlatformCapabilityFeatureService(platform, diagnostics);
            var connect = new ConnectFeatureService(session, config);

            var runtime = new MAAUnifiedRuntime
            {
                CoreBridge = bridge,
                ConfigurationService = config,
                ResourceWorkflowService = new ResourceWorkflowService(root, bridge, log),
                SessionService = session,
                Platform = platform,
                LogService = log,
                DiagnosticsService = diagnostics,
                ConnectFeatureService = connect,
                ShellFeatureService = new ShellFeatureService(connect),
                TaskQueueFeatureService = new TaskQueueFeatureService(session, config),
                CopilotFeatureService = new CopilotFeatureService(),
                ToolboxFeatureService = toolboxFeatureService,
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(config, diagnostics, platform.PostActionExecutorService),
            };

            return new RuntimeFixture(root, runtime);
        }

        public async ValueTask DisposeAsync()
        {
            await Runtime.DisposeAsync();
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

    private sealed class NullBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _callbacks = Channel.CreateUnbounded<CoreCallbackEvent>();

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(1));

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));

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
