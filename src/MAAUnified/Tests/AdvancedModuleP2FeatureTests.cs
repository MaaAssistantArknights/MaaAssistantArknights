using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Advanced;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class AdvancedModuleP2FeatureTests
{
    [Fact]
    public async Task ExternalNotificationProviders_RefreshProviders_ShouldLoadAndRecordEvent()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new ExternalNotificationProvidersPageViewModel(fixture.Runtime);

        await vm.RefreshProvidersAsync();

        Assert.NotEmpty(vm.Providers);
        Assert.False(string.IsNullOrWhiteSpace(vm.SelectedProvider));
        Assert.Contains("Loaded", vm.StatusMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.EventLogPath,
            "Advanced.ExternalNotificationProviders.Query"));
    }

    [Fact]
    public async Task ExternalNotificationProviders_ValidateWithoutProvider_ShouldFailAndWriteErrorLog()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new ExternalNotificationProvidersPageViewModel(fixture.Runtime);
        await vm.InitializeAsync();
        vm.SelectedProvider = string.Empty;

        await vm.ValidateAsync();

        Assert.Contains("unsupported", vm.LastErrorMessage, StringComparison.OrdinalIgnoreCase);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "Advanced.ExternalNotificationProviders.Validate"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            $"code={UiErrorCode.NotificationProviderUnsupported}"));
    }

    [Fact]
    public async Task RemoteControlCenter_TestConnectivity_InvalidEndpoint_ShouldFailAndWriteErrorLog()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new RemoteControlCenterPageViewModel(fixture.Runtime)
        {
            GetTaskEndpoint = "bad-uri",
            ReportEndpoint = "https://example.com/report",
            PollIntervalMs = 5000,
        };

        await vm.TestConnectivityAsync();

        Assert.False(vm.HasWarningMessage);
        Assert.Contains("invalid", vm.LastErrorMessage, StringComparison.OrdinalIgnoreCase);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "Advanced.RemoteControlCenter.Test"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            $"code={UiErrorCode.RemoteControlInvalidParameters}"));
    }

    [Fact]
    public async Task StageManager_Validate_InvalidStageCode_ShouldFailAndWriteErrorLog()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new StageManagerPageViewModel(fixture.Runtime)
        {
            StageCodesText = "1-7\nbad$code",
        };

        await vm.ValidateAsync();

        Assert.Contains("Invalid stage code", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "Advanced.StageManager.Validate"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            $"code={UiErrorCode.StageManagerInvalidStageCode}"));
    }

    [Fact]
    public async Task WebApi_StartFailure_ShouldNotRefreshRunningState()
    {
        var scripted = new ScriptedWebApiFeatureService(
            loadResult: UiOperationResult<WebApiConfig>.Ok(
                new WebApiConfig(
                    Enabled: true,
                    Host: "127.0.0.1",
                    Port: 51888,
                    AccessToken: string.Empty),
                "Loaded scripted config."),
            statusResults: [
                UiOperationResult<bool>.Ok(true, "running"),
                UiOperationResult<bool>.Ok(false, "stopped"),
            ],
            startResult: UiOperationResult.Fail(UiErrorCode.WebApiDisabled, "WebApi is disabled by configuration."),
            stopResult: UiOperationResult.Ok("stopped"));

        await using var fixture = await RuntimeFixture.CreateAsync(webApiFeatureService: scripted);
        var vm = new WebApiPageViewModel(fixture.Runtime);
        await vm.InitializeAsync();

        Assert.True(vm.IsRunning);
        Assert.Equal(1, scripted.StatusCallCount);

        await vm.StartAsync();

        Assert.True(vm.IsRunning);
        Assert.Equal(1, scripted.StatusCallCount);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "Advanced.WebApi.Start"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            $"code={UiErrorCode.WebApiDisabled}"));
    }

    [Fact]
    public async Task Overlay_ToggleFailure_ShouldNotMutateVisibleState()
    {
        var overlay = new ScriptedOverlayFeatureService
        {
            QueryTargetsResult = UiOperationResult<IReadOnlyList<OverlayTarget>>.Ok(
                [new OverlayTarget("preview", "Preview", IsPrimary: true)],
                "Loaded overlay targets."),
            SelectTargetResult = UiOperationResult.Ok("Overlay target selected."),
            ToggleVisibleResult = UiOperationResult.Fail(
                UiErrorCode.PlatformOperationFailed,
                "Overlay toggle failed."),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(overlayFeatureService: overlay);
        var vm = new OverlayAdvancedPageViewModel(fixture.Runtime);
        await vm.InitializeAsync();

        Assert.False(vm.Visible);
        await vm.ToggleOverlayAsync();

        Assert.False(vm.Visible);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "Advanced.Overlay.ToggleVisible"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            $"code={UiErrorCode.PlatformOperationFailed}"));
    }

    [Fact]
    public async Task TrayIntegration_SetVisible_Failure_ShouldBeVisible_AndWritePlatformEventLog()
    {
        var tray = new ScriptedTrayService
        {
            SetVisibleResult = PlatformOperation.Failed(
                provider: "scripted-tray",
                message: "Set visible failed.",
                errorCode: PlatformErrorCodes.TrayNotInitialized,
                operationId: "tray.set-visible"),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(trayService: tray);
        var vm = new TrayIntegrationPageViewModel(fixture.Runtime);
        await vm.InitializeAsync();
        vm.TrayVisible = false;

        await vm.ApplyTrayVisibilityAsync();

        Assert.False(string.IsNullOrWhiteSpace(vm.LastErrorMessage));
        Assert.Contains("Set visible failed.", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "Advanced.TrayIntegration.SetVisible"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            $"code={PlatformErrorCodes.TrayNotInitialized}"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.PlatformEventLogPath,
            "\"Action\":\"set-visible\""));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.PlatformEventLogPath,
            "\"Success\":false"));
    }

    [Fact]
    public async Task TrayIntegration_ShowMessage_Failure_ShouldBeVisible_AndWritePlatformEventLog()
    {
        var tray = new ScriptedTrayService
        {
            ShowResult = PlatformOperation.Failed(
                provider: "scripted-tray",
                message: "Tray show failed.",
                errorCode: PlatformErrorCodes.TrayNotInitialized,
                operationId: "tray.show"),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(trayService: tray);
        var vm = new TrayIntegrationPageViewModel(fixture.Runtime);
        await vm.InitializeAsync();
        vm.TrayMessageTitle = "Title";
        vm.TrayMessageBody = "Body";

        await vm.SendTrayMessageAsync();

        Assert.False(string.IsNullOrWhiteSpace(vm.LastErrorMessage));
        Assert.Contains("Tray show failed.", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "Advanced.TrayIntegration.Notify"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            $"code={PlatformErrorCodes.TrayNotInitialized}"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.PlatformEventLogPath,
            "\"Action\":\"show\""));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.PlatformEventLogPath,
            "\"Success\":false"));
    }

    private static async Task<bool> WaitForLogContainsAsync(string path, string expected, int retry = 30, int delayMs = 25)
    {
        for (var i = 0; i < retry; i++)
        {
            if (File.Exists(path))
            {
                var text = await File.ReadAllTextAsync(path);
                if (text.Contains(expected, StringComparison.Ordinal))
                {
                    return true;
                }
            }

            await Task.Delay(delayMs);
        }

        return false;
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

        public static async Task<RuntimeFixture> CreateAsync(
            IRemoteControlFeatureService? remoteControlFeatureService = null,
            INotificationProviderFeatureService? notificationProviderFeatureService = null,
            IStageManagerFeatureService? stageManagerFeatureService = null,
            IWebApiFeatureService? webApiFeatureService = null,
            IOverlayFeatureService? overlayFeatureService = null,
            ITrayService? trayService = null)
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
                TrayService = trayService ?? new NoOpTrayService(),
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
                RemoteControlFeatureService = remoteControlFeatureService ?? new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = overlayFeatureService ?? new OverlayFeatureService(capability),
                NotificationProviderFeatureService = notificationProviderFeatureService ?? new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                StageManagerFeatureService = stageManagerFeatureService ?? new StageManagerFeatureService(config),
                WebApiFeatureService = webApiFeatureService ?? new WebApiFeatureService(config),
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

    private sealed class ScriptedWebApiFeatureService(
        UiOperationResult<WebApiConfig> loadResult,
        IReadOnlyList<UiOperationResult<bool>> statusResults,
        UiOperationResult startResult,
        UiOperationResult stopResult) : IWebApiFeatureService
    {
        private int _statusCursor;

        public int StatusCallCount { get; private set; }

        public Task<UiOperationResult<WebApiConfig>> LoadConfigAsync(CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(loadResult);
        }

        public Task<UiOperationResult> SaveConfigAsync(WebApiConfig config, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(UiOperationResult.Ok("saved"));
        }

        public Task<UiOperationResult<bool>> GetRunningStatusAsync(CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            StatusCallCount++;
            if (_statusCursor >= statusResults.Count)
            {
                return Task.FromResult(UiOperationResult<bool>.Ok(false, "stopped"));
            }

            var result = statusResults[_statusCursor];
            _statusCursor++;
            return Task.FromResult(result);
        }

        public Task<UiOperationResult> StartAsync(CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(startResult);
        }

        public Task<UiOperationResult> StopAsync(CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(stopResult);
        }
    }

    private sealed class ScriptedOverlayFeatureService : IOverlayFeatureService
    {
        public UiOperationResult<IReadOnlyList<OverlayTarget>> QueryTargetsResult { get; init; } =
            UiOperationResult<IReadOnlyList<OverlayTarget>>.Ok([], "No target.");

        public UiOperationResult SelectTargetResult { get; init; } = UiOperationResult.Ok("selected");

        public UiOperationResult ToggleVisibleResult { get; init; } = UiOperationResult.Ok("toggled");

        public Task<string> GetOverlayModeAsync(CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult("scripted");
        }

        public Task<UiOperationResult<IReadOnlyList<OverlayTarget>>> GetOverlayTargetsAsync(CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(QueryTargetsResult);
        }

        public Task<UiOperationResult> SelectOverlayTargetAsync(string targetId, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(SelectTargetResult);
        }

        public Task<UiOperationResult> ToggleOverlayVisibilityAsync(bool visible, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(ToggleVisibleResult);
        }
    }

    private sealed class ScriptedTrayService : ITrayService
    {
        public event EventHandler<TrayCommandEvent>? CommandInvoked;

        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "Scripted tray service.",
            Provider: "scripted-tray",
            HasFallback: false,
            FallbackMode: null);

        public PlatformOperationResult InitializeResult { get; init; } =
            PlatformOperation.NativeSuccess("scripted-tray", "initialized");

        public PlatformOperationResult ShutdownResult { get; init; } =
            PlatformOperation.NativeSuccess("scripted-tray", "shutdown");

        public PlatformOperationResult ShowResult { get; init; } =
            PlatformOperation.NativeSuccess("scripted-tray", "shown");

        public PlatformOperationResult SetMenuStateResult { get; init; } =
            PlatformOperation.NativeSuccess("scripted-tray", "menu-updated");

        public PlatformOperationResult SetVisibleResult { get; init; } =
            PlatformOperation.NativeSuccess("scripted-tray", "visibility-updated");

        public Task<PlatformOperationResult> InitializeAsync(string appTitle, TrayMenuText? menuText, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(InitializeResult);
        }

        public Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(ShutdownResult);
        }

        public Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(ShowResult);
        }

        public Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(SetMenuStateResult);
        }

        public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(SetVisibleResult);
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
