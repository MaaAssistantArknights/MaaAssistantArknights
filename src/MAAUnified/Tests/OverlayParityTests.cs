using System.Text.Json.Nodes;
using System.Runtime.CompilerServices;
using System.Threading.Channels;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels;
using MAAUnified.App.ViewModels.Advanced;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.Tests;

public sealed class OverlayParityTests
{
    [Fact]
    public void OverlayTargetPersistence_SerializeAndLoad_ShouldRoundTripSelection()
    {
        var target = new OverlayTarget(
            "hwnd:1A2B",
            "Game Window",
            false,
            NativeHandle: 0x1A2B,
            ProcessId: 2048,
            ProcessName: "emulator",
            WindowTitle: "Arknights");
        var globals = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create(OverlayTargetPersistence.Serialize(target)),
        };

        var persisted = OverlayTargetPersistence.Load(globals);

        Assert.NotNull(persisted);
        Assert.Equal(target.Id, persisted!.TargetId);
        Assert.Equal(target.NativeHandle, persisted.NativeHandle);
        Assert.Equal(target.ProcessId, persisted.ProcessId);
        Assert.Equal(target.ProcessName, persisted.ProcessName);
        Assert.Equal(target.WindowTitle, persisted.WindowTitle);
    }

    [Fact]
    public void OverlayTargetPersistence_ResolveSelection_ShouldRestoreByExactId()
    {
        var preview = new OverlayTarget("preview", "Preview + Logs", true);
        var target = new OverlayTarget("hwnd:100", "Game", false, NativeHandle: 0x100, ProcessId: 1, ProcessName: "emu", WindowTitle: "Title");
        var globals = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create("""{"TargetId":"hwnd:100","NativeHandle":9999,"ProcessId":7,"ProcessName":"other","WindowTitle":"other"}"""),
        };

        var resolved = OverlayTargetPersistence.ResolveSelection([preview, target], globals);

        Assert.Equal(target, resolved);
    }

    [Fact]
    public void OverlayTargetPersistence_ResolveSelection_ShouldRestoreByLegacyHandleAndMetadata()
    {
        var preview = new OverlayTarget("preview", "Preview + Logs", true);
        var byHandle = new OverlayTarget("hwnd:200", "Handle Match", false, NativeHandle: 0x200, ProcessId: 2, ProcessName: "emu-a", WindowTitle: "A");
        var byMetadata = new OverlayTarget("hwnd:300", "Metadata Match", false, NativeHandle: 0x300, ProcessId: 3, ProcessName: "emu-b", WindowTitle: "Arknights - MuMu");

        var globalsByHandle = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create("""{"TargetId":"missing","Hwnd":512,"ProcessId":99,"ProcessName":"other","Title":"other"}"""),
        };
        var globalsByMetadata = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create("""{"TargetId":"missing","Hwnd":9999,"ProcessId":3,"ProcessName":"emu-b","Title":"Arknights"}"""),
        };

        var resolvedByHandle = OverlayTargetPersistence.ResolveSelection([preview, byHandle, byMetadata], globalsByHandle);
        var resolvedByMetadata = OverlayTargetPersistence.ResolveSelection([preview, byHandle, byMetadata], globalsByMetadata);

        Assert.Equal(byHandle, resolvedByHandle);
        Assert.Equal(byMetadata, resolvedByMetadata);
    }

    [Fact]
    public void OverlayTargetPersistence_ResolveSelection_ShouldPreferNativeTarget_WhenLegacyPreviewSelectionWasImplicit()
    {
        var preview = new OverlayTarget("preview", "Preview + Logs", true);
        var target = new OverlayTarget("hwnd:400", "Game", false, NativeHandle: 0x400, ProcessId: 4, ProcessName: "emu", WindowTitle: "Title");
        var globals = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create("""{"TargetId":"missing","NativeHandle":9999,"ProcessId":77,"ProcessName":"other","WindowTitle":"other"}"""),
        };

        var resolved = OverlayTargetPersistence.ResolveSelection([preview, target], globals);

        Assert.Equal(target, resolved);
    }

    [Fact]
    public void OverlayTargetPersistence_ResolveSelection_ShouldRespectPreview_WhenPreviewPreferenceWasExplicitlyPinned()
    {
        var preview = new OverlayTarget("preview", "Preview + Logs", true);
        var target = new OverlayTarget("hwnd:401", "Game", false, NativeHandle: 0x401, ProcessId: 5, ProcessName: "emu", WindowTitle: "Title");
        var globals = new Dictionary<string, JsonNode?>
        {
            [LegacyConfigurationKeys.OverlayTarget] = JsonValue.Create(OverlayTargetPersistence.Serialize(preview)),
            [LegacyConfigurationKeys.OverlayPreviewPinned] = JsonValue.Create(bool.TrueString),
        };

        var resolved = OverlayTargetPersistence.ResolveSelection([preview, target], globals);

        Assert.Equal(preview, resolved);
    }

    [Fact]
    public async Task TaskQueuePage_AppendSystemLog_ShouldMirrorAndTrimOverlayLogs()
    {
        await using var fixture = await OverlayFixture.CreateAsync();
        var vm = fixture.Shell.TaskQueuePage;

        for (var index = 0; index < 205; index++)
        {
            vm.AppendSystemLog($"entry-{index}");
        }

        Assert.Equal(200, vm.OverlayLogs.Count);
        Assert.Contains("entry-5", vm.OverlayLogs[0].Content, StringComparison.Ordinal);
        Assert.Contains("entry-204", vm.OverlayLogs[^1].Content, StringComparison.Ordinal);
        Assert.NotEmpty(vm.LogCards);
        Assert.Contains("entry-204", vm.LogCards[^1].Items[^1].Content, StringComparison.Ordinal);
    }

    [Fact]
    public async Task OverlaySharedState_ShouldSyncVisibilityBetweenTaskQueueAndAdvancedPage()
    {
        await using var fixture = await OverlayFixture.CreateAsync();
        var taskQueue = fixture.Shell.TaskQueuePage;
        var advanced = new OverlayAdvancedPageViewModel(fixture.Runtime);

        Assert.False(taskQueue.OverlayVisible);
        Assert.False(advanced.Visible);

        await taskQueue.ToggleOverlayAsync();

        Assert.True(taskQueue.OverlayVisible);
        Assert.True(advanced.Visible);
    }

    [Fact]
    public async Task OverlayPresentation_ShouldPreferRunOwnerOverIdlePreference()
    {
        await using var fixture = await OverlayFixture.CreateAsync();
        var presentation = fixture.Shell.OverlayPresentation;

        Assert.Equal(OverlayLogSource.TaskQueue, presentation.ResolvedSource);
        Assert.Same(fixture.Shell.TaskQueuePage.OverlayLogs, presentation.CurrentLogs);

        presentation.PreferCopilot();
        Assert.Equal(OverlayLogSource.Copilot, presentation.ResolvedSource);
        Assert.Same(fixture.Shell.CopilotPage.Logs, presentation.CurrentLogs);

        Assert.True(fixture.Runtime.SessionService.TryBeginRun("TaskQueue", out _));
        presentation.RefreshResolvedSource();

        Assert.Equal(OverlayLogSource.TaskQueue, presentation.ResolvedSource);
        Assert.Same(fixture.Shell.TaskQueuePage.OverlayLogs, presentation.CurrentLogs);

        fixture.Runtime.SessionService.EndRun("TaskQueue");
        presentation.RefreshResolvedSource();

        Assert.Equal(OverlayLogSource.Copilot, presentation.ResolvedSource);
        Assert.Same(fixture.Shell.CopilotPage.Logs, presentation.CurrentLogs);
    }

    [Fact]
    public async Task MainShellOverlayContextMethods_ShouldUpdateIdlePreference()
    {
        await using var fixture = await OverlayFixture.CreateAsync();

        await fixture.Shell.ToggleOverlayFromCopilotAsync();
        Assert.Equal(OverlayLogSource.Copilot, fixture.Shell.OverlayPresentation.PreferredSource);

        await fixture.Shell.ToggleOverlayFromTaskQueueAsync();
        Assert.Equal(OverlayLogSource.TaskQueue, fixture.Shell.OverlayPresentation.PreferredSource);
    }

    [Fact]
    public async Task OverlayStateChangedEvent_ShouldSyncRuntimeModeStatusAndSharedVisibility()
    {
        await using var fixture = await OverlayFixture.CreateAsync();
        var taskQueue = fixture.Shell.TaskQueuePage;
        var advanced = new OverlayAdvancedPageViewModel(fixture.Runtime);
        var applyMethod = typeof(MainShellViewModel).GetMethod(
            "ApplyOverlayStateChanged",
            System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic);

        Assert.NotNull(applyMethod);
        var state = new OverlayStateChangedEvent(
            OverlayRuntimeMode.Preview,
            Visible: true,
            TargetId: "preview",
            Action: "fallback-enter",
            Message: "Overlay switched to Preview + Logs mode.",
            Timestamp: DateTimeOffset.UtcNow,
            Provider: "test-overlay",
            UsedFallback: true,
            ErrorCode: PlatformErrorCodes.OverlayPreviewMode);
        applyMethod!.Invoke(fixture.Shell, [state]);

        Assert.Equal(OverlayRuntimeMode.Preview, taskQueue.OverlayMode);
        Assert.True(taskQueue.OverlayVisible);
        Assert.True(advanced.Visible);
        Assert.Equal("Overlay switched to Preview + Logs mode.", taskQueue.OverlayStatusText);
        Assert.Equal("Preview + Logs", taskQueue.OverlayTargetSummaryText);
        Assert.Contains("Overlay switched to Preview + Logs mode.", taskQueue.OverlayButtonToolTip, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ToggleOverlayFromTrayAsync_ShouldUsePreviewModeGrowlWhenOverlayFallsBack()
    {
        var overlayService = new ScriptedOverlayService();
        overlayService.VisibleMode = OverlayRuntimeMode.Preview;
        overlayService.VisibleMessage = "Overlay switched to Preview + Logs mode.";
        overlayService.EmitStateOnVisibilityToggle = false;

        await using var fixture = await OverlayFixture.CreateAsync(overlayService: overlayService);
        var applyMethod = typeof(MainShellViewModel).GetMethod(
            "ApplyOverlayStateChanged",
            System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic);

        Assert.NotNull(applyMethod);
        applyMethod!.Invoke(fixture.Shell, [new OverlayStateChangedEvent(
            OverlayRuntimeMode.Preview,
            Visible: false,
            TargetId: "preview",
            Action: "fallback-enter",
            Message: overlayService.VisibleMessage,
            Timestamp: DateTimeOffset.UtcNow,
            Provider: overlayService.Capability.Provider,
            UsedFallback: true,
            ErrorCode: PlatformErrorCodes.OverlayPreviewMode)]);

        await fixture.Shell.ToggleOverlayFromTrayAsync();

        Assert.Contains(fixture.Shell.GrowlMessages, message => message.Contains("预览模式", StringComparison.Ordinal));
        Assert.Equal(OverlayRuntimeMode.Preview, fixture.Shell.TaskQueuePage.OverlayMode);
        Assert.True(fixture.Shell.TaskQueuePage.OverlayVisible);
    }

    [Fact]
    public async Task ToggleOverlayAsync_ShouldAutoMigrateLegacyPreviewSelectionToNativeTarget()
    {
        var overlayService = new ScriptedOverlayService();
        await using var fixture = await OverlayFixture.CreateAsync(overlayService: overlayService);

        fixture.Runtime.ConfigurationService.CurrentConfig.GlobalValues[LegacyConfigurationKeys.OverlayTarget] =
            JsonValue.Create(OverlayTargetPersistence.Serialize(new OverlayTarget("preview", "Preview + Logs", true)));
        fixture.Runtime.ConfigurationService.CurrentConfig.GlobalValues.Remove(LegacyConfigurationKeys.OverlayPreviewPinned);

        await fixture.Shell.TaskQueuePage.ReloadOverlayTargetsAsync();
        Assert.Equal("hwnd:123", fixture.Shell.TaskQueuePage.SelectedOverlayTarget?.Id);

        await fixture.Shell.TaskQueuePage.ToggleOverlayAsync();

        Assert.Equal("hwnd:123", overlayService.SelectedTargetId);
        Assert.True(fixture.Runtime.ConfigurationService.CurrentConfig.GlobalValues.TryGetValue(
            LegacyConfigurationKeys.OverlayPreviewPinned,
            out var previewPinnedNode));
        Assert.Equal(bool.FalseString, previewPinnedNode?.GetValue<string>());
    }

    private sealed class OverlayFixture : IAsyncDisposable
    {
        private OverlayFixture(string root, MAAUnifiedRuntime runtime, MainShellViewModel shell)
        {
            Root = root;
            Runtime = runtime;
            Shell = shell;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public MainShellViewModel Shell { get; }

        public static async Task<OverlayFixture> CreateAsync(
            IOverlayCapabilityService? overlayService = null,
            ITrayService? trayService = null)
        {
            var root = Path.Combine(Path.GetTempPath(), "maa-unified-overlay-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(root);

            var log = new UiLogService();
            var diagnostics = new UiDiagnosticsService(root, log);
            var config = new UnifiedConfigurationService(
                new AvaloniaJsonConfigStore(root),
                new GuiNewJsonConfigImporter(),
                new GuiJsonConfigImporter(),
                log,
                root);
            await config.LoadOrBootstrapAsync();

            var bridge = new FakeBridge();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var platform = new PlatformServiceBundle
            {
                TrayService = trayService ?? new NoOpTrayService(),
                NotificationService = new NoOpNotificationService(),
                HotkeyService = new NoOpGlobalHotkeyService(),
                AutostartService = new NoOpAutostartService(),
                FileDialogService = new NoOpFileDialogService(),
                OverlayService = overlayService ?? new NoOpOverlayCapabilityService(),
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
                ConfigurationProfileFeatureService = new ConfigurationProfileFeatureService(config),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
                AppLifecycleService = new NoOpAppLifecycleService(),
            };

            return new OverlayFixture(root, runtime, new MainShellViewModel(runtime, NoOpAppDialogService.Instance));
        }

        public async ValueTask DisposeAsync()
        {
            TestShellCleanup.StopTimerScheduler(Shell);
            await Runtime.DisposeAsync();

            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // Ignore cleanup failures for temp directories.
            }
        }
    }

    private sealed class ScriptedOverlayService : IOverlayCapabilityService
    {
        private string _selectedTargetId = "preview";

        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "scripted overlay",
            Provider: "scripted-overlay",
            HasFallback: true,
            FallbackMode: "preview");

        public OverlayRuntimeMode VisibleMode { get; set; } = OverlayRuntimeMode.Native;

        public string VisibleMessage { get; set; } = "Overlay attached to native target.";

        public bool EmitStateOnVisibilityToggle { get; set; } = true;

        public string SelectedTargetId => _selectedTargetId;

        public event EventHandler<OverlayStateChangedEvent>? OverlayStateChanged;

        public Task<PlatformOperationResult> BindHostWindowAsync(
            nint hostWindowHandle,
            bool clickThrough,
            double opacity,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "bound", "overlay.bind-host"));
        }

        public Task<PlatformOperationResult<IReadOnlyList<OverlayTarget>>> QueryTargetsAsync(CancellationToken cancellationToken = default)
        {
            IReadOnlyList<OverlayTarget> targets =
            [
                new OverlayTarget("preview", "Preview + Logs", true),
                new OverlayTarget("hwnd:123", "Arknights - 123", false, NativeHandle: 0x123, ProcessId: 123, ProcessName: "emu", WindowTitle: "Arknights"),
            ];
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, targets, "targets", "overlay.query-targets"));
        }

        public Task<PlatformOperationResult> SelectTargetAsync(string targetId, CancellationToken cancellationToken = default)
        {
            _selectedTargetId = string.IsNullOrWhiteSpace(targetId) ? "preview" : targetId;
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "selected", "overlay.select-target"));
        }

        public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
        {
            if (!visible)
            {
                if (EmitStateOnVisibilityToggle)
                {
                    Emit(new OverlayStateChangedEvent(
                        OverlayRuntimeMode.Hidden,
                        Visible: false,
                        TargetId: _selectedTargetId,
                        Action: "hide",
                        Message: "Overlay hidden.",
                        Timestamp: DateTimeOffset.UtcNow,
                        Provider: Capability.Provider,
                        UsedFallback: false,
                        ErrorCode: null));
                }

                return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "hidden", "overlay.set-visible"));
            }

            var usedFallback = VisibleMode == OverlayRuntimeMode.Preview;
            if (EmitStateOnVisibilityToggle)
            {
                Emit(new OverlayStateChangedEvent(
                    VisibleMode,
                    Visible: true,
                    TargetId: VisibleMode == OverlayRuntimeMode.Preview ? "preview" : _selectedTargetId,
                    Action: usedFallback ? "fallback-enter" : "show-native",
                    Message: VisibleMessage,
                    Timestamp: DateTimeOffset.UtcNow,
                    Provider: Capability.Provider,
                    UsedFallback: usedFallback,
                    ErrorCode: usedFallback ? PlatformErrorCodes.OverlayPreviewMode : null));
            }

            return Task.FromResult(
                usedFallback
                    ? PlatformOperation.FallbackSuccess(Capability.Provider, VisibleMessage, "overlay.set-visible", PlatformErrorCodes.OverlayPreviewMode)
                    : PlatformOperation.NativeSuccess(Capability.Provider, VisibleMessage, "overlay.set-visible"));
        }

        public void Emit(OverlayStateChangedEvent e)
        {
            if (!string.IsNullOrWhiteSpace(e.TargetId))
            {
                _selectedTargetId = e.TargetId;
            }

            OverlayStateChanged?.Invoke(this, e);
        }
    }

    private sealed class FakeBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _callbackChannel = Channel.CreateUnbounded<CoreCallbackEvent>();

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
            CoreInitializeRequest request,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(
                new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));
        }

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<int>.Ok(1));
        }

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));
        }

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));
        }

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));
        }

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await foreach (var callback in _callbackChannel.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public ValueTask DisposeAsync()
        {
            _callbackChannel.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }
    }
}
