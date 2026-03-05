using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SettingsHotkeyFeedbackFeatureTests
{
    [Fact]
    public async Task RegisterHotkeysAsync_ShowGuiFails_StopsAndSkipsLinkStart()
    {
        var hotkeyService = new ScriptedHotkeyService();
        hotkeyService.EnqueueRegisterResult(
            "ShowGui",
            PlatformOperation.Failed("test", "conflict", PlatformErrorCodes.HotkeyConflict, "hotkey.register"));

        await using var fixture = await RuntimeFixture.CreateAsync(hotkeyService: hotkeyService);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var before = ReadGlobalString(fixture.Config, ConfigurationKeys.HotKeys);
        vm.HotkeyShowGui = "Ctrl+Alt+S";
        vm.HotkeyLinkStart = "Ctrl+Alt+L";

        await vm.RegisterHotkeysAsync();

        var only = Assert.Single(hotkeyService.RegisterCalls);
        Assert.Equal("ShowGui", only.Name);
        Assert.Equal(before, ReadGlobalString(fixture.Config, ConfigurationKeys.HotKeys));
        Assert.Contains("热键冲突", vm.HotkeyErrorMessage, StringComparison.Ordinal);
        Assert.Contains("未继续注册", vm.HotkeyStatusMessage, StringComparison.Ordinal);
    }

    [Fact]
    public async Task RegisterHotkeysAsync_ShowGuiSuccess_LinkStartFails_KeepsShowGuiAndPersistsOnlyShowGui()
    {
        var hotkeyService = new ScriptedHotkeyService();
        hotkeyService.EnqueueRegisterResult(
            "ShowGui",
            PlatformOperation.NativeSuccess("test", "show registered", "hotkey.register"));
        hotkeyService.EnqueueRegisterResult(
            "LinkStart",
            PlatformOperation.Failed("test", "invalid", PlatformErrorCodes.HotkeyInvalidGesture, "hotkey.register"));

        await using var fixture = await RuntimeFixture.CreateAsync(hotkeyService: hotkeyService);
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.HotKeys] = JsonValue.Create("ShowGui=Ctrl+1;LinkStart=Ctrl+2");
        await fixture.Config.SaveAsync();

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.HotkeyShowGui = "Ctrl+Shift+Alt+M";
        vm.HotkeyLinkStart = "Ctrl+Shift+Alt+L";

        await vm.RegisterHotkeysAsync();

        Assert.Equal(2, hotkeyService.RegisterCalls.Count);
        Assert.Equal("ShowGui", hotkeyService.RegisterCalls[0].Name);
        Assert.Equal("LinkStart", hotkeyService.RegisterCalls[1].Name);

        var hotkeys = ParseHotkeys(ReadGlobalString(fixture.Config, ConfigurationKeys.HotKeys));
        Assert.Equal("Ctrl+Shift+Alt+M", hotkeys["ShowGui"]);
        Assert.Equal("Ctrl+2", hotkeys["LinkStart"]);
        Assert.Contains("热键格式非法", vm.HotkeyErrorMessage, StringComparison.Ordinal);
        Assert.Contains("ShowGui 注册成功", vm.HotkeyStatusMessage, StringComparison.Ordinal);
    }

    [Fact]
    public async Task RegisterHotkeysAsync_MapsHotkeyConflictMessage()
    {
        await ValidateMappedHotkeyErrorAsync(PlatformErrorCodes.HotkeyConflict, "热键冲突");
    }

    [Fact]
    public async Task RegisterHotkeysAsync_MapsHotkeyInvalidGestureMessage()
    {
        await ValidateMappedHotkeyErrorAsync(PlatformErrorCodes.HotkeyInvalidGesture, "热键格式非法");
    }

    [Fact]
    public async Task RegisterHotkeysAsync_MapsHotkeyNotFoundMessage()
    {
        await ValidateMappedHotkeyErrorAsync(PlatformErrorCodes.HotkeyNotFound, "热键不存在");
    }

    [Fact]
    public async Task RegisterHotkeysAsync_FallbackPlatform_ShowsVisibleWarningAndLogsFallbackScope()
    {
        var fallbackCapability = new PlatformCapabilityStatus(
            Supported: false,
            Message: "Global hotkeys are unavailable, fallback to window-scoped hotkeys.",
            Provider: "window-scoped",
            HasFallback: true,
            FallbackMode: "window-scoped");
        var hotkeyService = new ScriptedHotkeyService(fallbackCapability);
        hotkeyService.EnqueueRegisterResult(
            "ShowGui",
            PlatformOperation.FallbackSuccess(
                "window-scoped",
                "Window-scoped hotkey registered: ShowGui",
                "hotkey.register",
                PlatformErrorCodes.HotkeyFallback));
        hotkeyService.EnqueueRegisterResult(
            "LinkStart",
            PlatformOperation.FallbackSuccess(
                "window-scoped",
                "Window-scoped hotkey registered: LinkStart",
                "hotkey.register",
                PlatformErrorCodes.HotkeyFallback));

        await using var fixture = await RuntimeFixture.CreateAsync(hotkeyService: hotkeyService);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        await vm.RegisterHotkeysAsync();

        Assert.True(vm.HasHotkeyWarningMessage);
        Assert.Contains("热键已降级为窗口级", vm.HotkeyWarningMessage, StringComparison.Ordinal);
        var eventLog = await File.ReadAllTextAsync(fixture.Diagnostics.EventLogPath);
        Assert.Contains("Settings.Hotkey.Fallback", eventLog, StringComparison.Ordinal);
        var platformLog = await File.ReadAllTextAsync(fixture.Diagnostics.PlatformEventLogPath);
        Assert.Contains("hotkey.register", platformLog, StringComparison.OrdinalIgnoreCase);
        Assert.Contains("window-scoped", platformLog, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task LoadHotkeysFromConfig_RoundTripStable()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            var firstHotkeyService = new ScriptedHotkeyService();
            await using (var first = await RuntimeFixture.CreateAsync(root, cleanupRoot: false, hotkeyService: firstHotkeyService))
            {
                first.Config.CurrentConfig.GlobalValues[ConfigurationKeys.HotKeys] = JsonValue.Create("ShowGui=Ctrl+1;LinkStart=Ctrl+2");
                await first.Config.SaveAsync();

                var vm = new SettingsPageViewModel(first.Runtime, new ConnectionGameSharedStateViewModel());
                await vm.InitializeAsync();
                Assert.Equal("Ctrl+1", vm.HotkeyShowGui);
                Assert.Equal("Ctrl+2", vm.HotkeyLinkStart);

                vm.HotkeyShowGui = "Ctrl+Shift+Alt+M";
                vm.HotkeyLinkStart = "Ctrl+Shift+Alt+L";
                await vm.RegisterHotkeysAsync();

                var serialized = ReadGlobalString(first.Config, ConfigurationKeys.HotKeys);
                Assert.Equal("ShowGui=Ctrl+Shift+Alt+M;LinkStart=Ctrl+Shift+Alt+L", serialized);
            }

            await using var second = await RuntimeFixture.CreateAsync(root, cleanupRoot: false, hotkeyService: new ScriptedHotkeyService());
            var reloadedVm = new SettingsPageViewModel(second.Runtime, new ConnectionGameSharedStateViewModel());
            await reloadedVm.InitializeAsync();
            Assert.Equal("Ctrl+Shift+Alt+M", reloadedVm.HotkeyShowGui);
            Assert.Equal("Ctrl+Shift+Alt+L", reloadedVm.HotkeyLinkStart);
        }
        finally
        {
            try
            {
                Directory.Delete(root, recursive: true);
            }
            catch
            {
                // ignore cleanup failures in temporary test directories
            }
        }
    }

    private static async Task ValidateMappedHotkeyErrorAsync(string errorCode, string expectedLocalized)
    {
        var hotkeyService = new ScriptedHotkeyService();
        hotkeyService.EnqueueRegisterResult(
            "ShowGui",
            PlatformOperation.Failed("test", "failed", errorCode, "hotkey.register"));

        await using var fixture = await RuntimeFixture.CreateAsync(hotkeyService: hotkeyService);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        await vm.RegisterHotkeysAsync();

        Assert.Contains(expectedLocalized, vm.HotkeyErrorMessage, StringComparison.Ordinal);
    }

    private static IReadOnlyDictionary<string, string> ParseHotkeys(string raw)
    {
        var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        if (string.IsNullOrWhiteSpace(raw))
        {
            return result;
        }

        foreach (var segment in raw.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            var index = segment.IndexOf('=');
            if (index <= 0 || index >= segment.Length - 1)
            {
                continue;
            }

            var key = segment[..index].Trim();
            var value = segment[(index + 1)..].Trim();
            result[key] = value;
        }

        return result;
    }

    private static string ReadGlobalString(UnifiedConfigurationService config, string key)
    {
        if (!config.CurrentConfig.GlobalValues.TryGetValue(key, out var node) || node is null)
        {
            return string.Empty;
        }

        if (node is JsonValue value && value.TryGetValue(out string? text) && text is not null)
        {
            return text;
        }

        return node.ToString();
    }

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private readonly bool _cleanupRoot;

        private RuntimeFixture(
            string root,
            MAAUnifiedRuntime runtime,
            UnifiedConfigurationService config,
            UiDiagnosticsService diagnostics,
            bool cleanupRoot)
        {
            Root = root;
            Runtime = runtime;
            Config = config;
            Diagnostics = diagnostics;
            _cleanupRoot = cleanupRoot;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public UnifiedConfigurationService Config { get; }

        public UiDiagnosticsService Diagnostics { get; }

        public static async Task<RuntimeFixture> CreateAsync(
            string? root = null,
            bool cleanupRoot = true,
            IGlobalHotkeyService? hotkeyService = null)
        {
            root ??= Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
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

            var bridge = new FakeBridge();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            hotkeyService ??= new ScriptedHotkeyService();
            var platform = new PlatformServiceBundle
            {
                TrayService = new NoOpTrayService(),
                NotificationService = new NoOpNotificationService(),
                HotkeyService = hotkeyService,
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
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
            };

            return new RuntimeFixture(root, runtime, config, diagnostics, cleanupRoot);
        }

        public async ValueTask DisposeAsync()
        {
            await Runtime.DisposeAsync();
            if (!_cleanupRoot)
            {
                return;
            }

            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // ignore cleanup failures in temporary test directories
            }
        }
    }

    private sealed class ScriptedHotkeyService : IGlobalHotkeyService
    {
        private readonly Dictionary<string, Queue<PlatformOperationResult>> _registerResults = new(StringComparer.OrdinalIgnoreCase);

        public ScriptedHotkeyService(PlatformCapabilityStatus? capability = null)
        {
            Capability = capability ?? new PlatformCapabilityStatus(
                Supported: true,
                Message: "hotkey test service",
                Provider: "test");
        }

        public PlatformCapabilityStatus Capability { get; }

        public event EventHandler<GlobalHotkeyTriggeredEvent>? Triggered;

        public List<(string Name, string Gesture)> RegisterCalls { get; } = [];

        public void EnqueueRegisterResult(string name, PlatformOperationResult result)
        {
            if (!_registerResults.TryGetValue(name, out var queue))
            {
                queue = new Queue<PlatformOperationResult>();
                _registerResults[name] = queue;
            }

            queue.Enqueue(result);
        }

        public Task<PlatformOperationResult> RegisterAsync(string name, string gesture, CancellationToken cancellationToken = default)
        {
            RegisterCalls.Add((name, gesture));
            if (_registerResults.TryGetValue(name, out var queue) && queue.Count > 0)
            {
                return Task.FromResult(queue.Dequeue());
            }

            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, $"Registered {name}", "hotkey.register"));
        }

        public Task<PlatformOperationResult> UnregisterAsync(string name, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, $"Unregistered {name}", "hotkey.unregister"));
        }
    }

    private sealed class FakeBridge : IMaaCoreBridge
    {
        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
            CoreInitializeRequest request,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));
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
            await Task.CompletedTask;
            yield break;
        }

        public ValueTask DisposeAsync()
        {
            return ValueTask.CompletedTask;
        }
    }
}
