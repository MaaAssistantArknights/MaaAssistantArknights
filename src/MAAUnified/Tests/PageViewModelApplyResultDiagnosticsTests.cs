using System.Runtime.CompilerServices;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class PageViewModelApplyResultDiagnosticsTests
{
    [Fact]
    public async Task ApplyResultAsync_Exception_ShouldRecordErrorAndFailedResult()
    {
        await using var fixture = await TestFixture.CreateAsync();

        var ok = await fixture.ViewModel.ApplyFailingAsync(
            "PageViewModel.ApplyResult",
            UiErrorCode.PlatformOperationFailed);

        Assert.False(ok);
        Assert.Contains("synthetic-non-generic", fixture.ViewModel.LastErrorMessage, StringComparison.Ordinal);

        var errorLog = await File.ReadAllTextAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath);
        Assert.Contains("[ERROR] [PageViewModel.ApplyResult]", errorLog, StringComparison.Ordinal);
        Assert.Contains("[FAILED] [PageViewModel.ApplyResult]", errorLog, StringComparison.Ordinal);
        Assert.Contains($"code={UiErrorCode.PlatformOperationFailed}", errorLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ApplyResultAsync_GenericException_ShouldRecordErrorAndFailedResult()
    {
        await using var fixture = await TestFixture.CreateAsync();

        var value = await fixture.ViewModel.ApplyFailingGenericAsync(
            "PageViewModel.ApplyResult.Generic",
            UiErrorCode.UiError);

        Assert.Null(value);
        Assert.Contains("synthetic-generic", fixture.ViewModel.LastErrorMessage, StringComparison.Ordinal);

        var errorLog = await File.ReadAllTextAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath);
        Assert.Contains("[ERROR] [PageViewModel.ApplyResult.Generic]", errorLog, StringComparison.Ordinal);
        Assert.Contains("[FAILED] [PageViewModel.ApplyResult.Generic]", errorLog, StringComparison.Ordinal);
        Assert.Contains($"code={UiErrorCode.UiError}", errorLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ApplyResultAsync_FailedResult_ShouldRaiseDialogErrorEvent()
    {
        await using var fixture = await TestFixture.CreateAsync();

        DialogErrorRaisedEvent? raised = null;
        fixture.Runtime.DialogFeatureService.ErrorRaised += (_, e) => raised = e;

        var ok = await fixture.ViewModel.ApplyFailedResultAsync("PageViewModel.ApplyResult.FailedResult");

        Assert.False(ok);
        Assert.NotNull(raised);
        Assert.Equal("PageViewModel.ApplyResult.FailedResult", raised!.Context);
        Assert.Equal(UiErrorCode.TaskLoadFailed, raised.Result.Error?.Code);
        Assert.Equal("synthetic-failed-result", raised.Result.Message);
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(string root, MAAUnifiedRuntime runtime, ProbePageViewModel viewModel)
        {
            Root = root;
            Runtime = runtime;
            ViewModel = viewModel;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public ProbePageViewModel ViewModel { get; }

        public static async Task<TestFixture> CreateAsync()
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
                ToolboxFeatureService = new ToolboxFeatureService(),
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(config, diagnostics, platform.PostActionExecutorService),
            };

            return new TestFixture(root, runtime, new ProbePageViewModel(runtime));
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

    private sealed class ProbePageViewModel(MAAUnifiedRuntime runtime) : PageViewModelBase(runtime)
    {
        public Task<bool> ApplyFailedResultAsync(string scope, CancellationToken cancellationToken = default)
        {
            return ApplyResultAsync(
                UiOperationResult.Fail(UiErrorCode.TaskLoadFailed, "synthetic-failed-result"),
                scope,
                cancellationToken);
        }

        public Task<bool> ApplyFailingAsync(string scope, string exceptionCode, CancellationToken cancellationToken = default)
        {
            return ApplyResultAsync(
                _ => throw new InvalidOperationException("synthetic-non-generic"),
                scope,
                exceptionCode,
                cancellationToken);
        }

        public Task<string?> ApplyFailingGenericAsync(string scope, string exceptionCode, CancellationToken cancellationToken = default)
        {
            return ApplyResultAsync<string>(
                _ => throw new InvalidOperationException("synthetic-generic"),
                scope,
                exceptionCode,
                cancellationToken);
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
