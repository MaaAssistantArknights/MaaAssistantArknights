using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels;
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

public sealed class SettingsGuiBackgroundFeatureTests
{
    [Fact]
    public async Task SaveGuiSettingsAsync_WritesAllKeysInSingleBatch()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var path = CreateExistingFile(fixture.Root, "bg-save-ok.txt");
        vm.Theme = "Dark";
        vm.Language = "en-us";
        vm.UseTray = false;
        vm.MinimizeToTray = true;
        vm.WindowTitleScrollable = false;
        vm.BackgroundImagePath = path;
        vm.BackgroundOpacity = 61;
        vm.BackgroundBlur = 27;
        vm.BackgroundStretchMode = "Uniform";

        await vm.SaveGuiSettingsAsync();

        Assert.Equal("Dark", ReadGlobalString(fixture.Config, "Theme.Mode"));
        Assert.Equal("en-us", ReadGlobalString(fixture.Config, ConfigurationKeys.Localization));
        Assert.Equal("False", ReadGlobalString(fixture.Config, ConfigurationKeys.UseTray));
        Assert.Equal("True", ReadGlobalString(fixture.Config, ConfigurationKeys.MinimizeToTray));
        Assert.Equal("False", ReadGlobalString(fixture.Config, ConfigurationKeys.WindowTitleScrollable));
        Assert.Equal(path, ReadGlobalString(fixture.Config, ConfigurationKeys.BackgroundImagePath));
        Assert.Equal("61", ReadGlobalString(fixture.Config, ConfigurationKeys.BackgroundOpacity));
        Assert.Equal("27", ReadGlobalString(fixture.Config, ConfigurationKeys.BackgroundBlurEffectRadius));
        Assert.Equal("Uniform", ReadGlobalString(fixture.Config, ConfigurationKeys.BackgroundImageStretchMode));

        var eventLog = await File.ReadAllTextAsync(fixture.Diagnostics.EventLogPath);
        Assert.Contains("Saved settings batch:", eventLog, StringComparison.Ordinal);
        Assert.DoesNotContain("Saved setting:", eventLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task SaveGuiSettingsAsync_InvalidBackgroundPath_BlocksEntireSave()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var baselinePath = CreateExistingFile(fixture.Root, "bg-baseline.txt");
        vm.Language = "en-us";
        vm.UseTray = true;
        vm.MinimizeToTray = false;
        vm.WindowTitleScrollable = true;
        vm.BackgroundImagePath = baselinePath;
        vm.BackgroundOpacity = 45;
        vm.BackgroundBlur = 12;
        vm.BackgroundStretchMode = "UniformToFill";
        await vm.SaveGuiSettingsAsync();

        var before = CaptureGuiSnapshot(fixture.Config);

        var missingPath = Path.Combine(fixture.Root, "missing.png");
        vm.BackgroundImagePath = missingPath;
        vm.Language = "ja-jp";
        vm.UseTray = false;
        await vm.SaveGuiSettingsAsync();

        var after = CaptureGuiSnapshot(fixture.Config);
        Assert.Equal(before, after);
        Assert.True(vm.HasPendingGuiChanges);
        Assert.Equal(missingPath, vm.BackgroundImagePath);
    }

    [Fact]
    public async Task AutoSave_OnCheckboxAndNumeric_TriggersSave()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.UseTray = false;
        vm.BackgroundOpacity = 73;

        await WaitUntilAsync(() =>
            string.Equals(ReadGlobalString(fixture.Config, ConfigurationKeys.UseTray), "False", StringComparison.OrdinalIgnoreCase) &&
            string.Equals(ReadGlobalString(fixture.Config, ConfigurationKeys.BackgroundOpacity), "73", StringComparison.Ordinal));
    }

    [Fact]
    public async Task AutoSave_OnTextCommit_TriggersSave()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var path = CreateExistingFile(fixture.Root, "bg-text-commit.txt");
        vm.BackgroundImagePath = path;

        await Task.Delay(200);
        Assert.NotEqual(path, ReadGlobalString(fixture.Config, ConfigurationKeys.BackgroundImagePath));

        await vm.SaveGuiSettingsAsync();

        await WaitUntilAsync(() =>
            string.Equals(ReadGlobalString(fixture.Config, ConfigurationKeys.BackgroundImagePath), path, StringComparison.Ordinal));
    }

    [Fact]
    public async Task SaveFailure_KeepsInputAndMarksPending()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var baselinePath = CreateExistingFile(fixture.Root, "bg-before-fail.txt");
        vm.BackgroundImagePath = baselinePath;
        await vm.SaveGuiSettingsAsync();

        var missingPath = Path.Combine(fixture.Root, "not-exist.png");
        vm.BackgroundImagePath = missingPath;
        await vm.SaveGuiSettingsAsync();

        Assert.True(vm.HasPendingGuiChanges);
        Assert.Equal(missingPath, vm.BackgroundImagePath);
        Assert.Contains("does not exist", vm.GuiValidationMessage, StringComparison.OrdinalIgnoreCase);
        Assert.Equal(baselinePath, ReadGlobalString(fixture.Config, ConfigurationKeys.BackgroundImagePath));
    }

    [Fact]
    public async Task LoadFromConfig_OutOfRangeBackground_ClampsAndWarns()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.BackgroundOpacity] = JsonValue.Create("-8");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.BackgroundBlurEffectRadius] = JsonValue.Create("999");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.BackgroundImageStretchMode] = JsonValue.Create("BadStretch");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.BackgroundImagePath] = JsonValue.Create(Path.Combine(fixture.Root, "missing.jpg"));
        fixture.Config.CurrentConfig.GlobalValues["Theme.Mode"] = JsonValue.Create("UnknownTheme");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.Localization] = JsonValue.Create("No-Such-Language");

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.Equal("Light", vm.Theme);
        Assert.Equal("zh-cn", vm.Language);
        Assert.Equal(0, vm.BackgroundOpacity);
        Assert.Equal(80, vm.BackgroundBlur);
        Assert.Equal("UniformToFill", vm.BackgroundStretchMode);
        Assert.Equal(string.Empty, vm.BackgroundImagePath);
        Assert.True(vm.HasGuiValidationMessage);

        var eventLog = await File.ReadAllTextAsync(fixture.Diagnostics.EventLogPath);
        Assert.Contains("Settings.Gui.Normalize", eventLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task RestartRoundTrip_ThemeLanguageBackground_PersistAndReapply()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            await using (var first = await RuntimeFixture.CreateAsync(root, cleanupRoot: false))
            {
                var vm = new SettingsPageViewModel(first.Runtime, new ConnectionGameSharedStateViewModel());
                await vm.InitializeAsync();

                var path = CreateExistingFile(root, "bg-restart.txt");
                vm.Theme = "Dark";
                vm.Language = "ko-kr";
                vm.BackgroundImagePath = path;
                vm.BackgroundOpacity = 52;
                vm.BackgroundBlur = 18;
                vm.BackgroundStretchMode = "Fill";
                await vm.SaveGuiSettingsAsync();
            }

            await using var second = await RuntimeFixture.CreateAsync(root, cleanupRoot: false);
            var reloaded = new SettingsPageViewModel(second.Runtime, new ConnectionGameSharedStateViewModel());
            await reloaded.InitializeAsync();

            Assert.Equal("Dark", reloaded.Theme);
            Assert.Equal("ko-kr", reloaded.Language);
            Assert.Equal(Path.Combine(root, "bg-restart.txt"), reloaded.BackgroundImagePath);
            Assert.Equal(52, reloaded.BackgroundOpacity);
            Assert.Equal(18, reloaded.BackgroundBlur);
            Assert.Equal("Fill", reloaded.BackgroundStretchMode);
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

    [Fact]
    public async Task LanguageApply_RefreshesTaskQueueAndTrayText()
    {
        var tray = new CapturingTrayService();
        await using var fixture = await RuntimeFixture.CreateAsync(trayService: tray);
        var vm = new MainShellViewModel(fixture.Runtime);

        var applyMethod = typeof(MainShellViewModel).GetMethod(
            "ApplyGuiSettingsAsync",
            BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(applyMethod);

        var snapshot = new GuiSettingsSnapshot(
            Theme: "Light",
            Language: "ja-jp",
            UseTray: true,
            MinimizeToTray: false,
            WindowTitleScrollable: true,
            BackgroundImagePath: string.Empty,
            BackgroundOpacity: 45,
            BackgroundBlur: 12,
            BackgroundStretchMode: "UniformToFill");

        var task = applyMethod!.Invoke(vm, [snapshot, CancellationToken.None]) as Task;
        Assert.NotNull(task);
        await task!;

        Assert.Equal("ja-jp", vm.TaskQueuePage.Texts.Language);
        Assert.NotNull(tray.LastMenuText);
        Assert.Equal("開始", tray.LastMenuText!.Start);
    }

    private static Dictionary<string, string> CaptureGuiSnapshot(UnifiedConfigurationService config)
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            ["Theme.Mode"] = ReadGlobalString(config, "Theme.Mode"),
            [ConfigurationKeys.Localization] = ReadGlobalString(config, ConfigurationKeys.Localization),
            [ConfigurationKeys.UseTray] = ReadGlobalString(config, ConfigurationKeys.UseTray),
            [ConfigurationKeys.MinimizeToTray] = ReadGlobalString(config, ConfigurationKeys.MinimizeToTray),
            [ConfigurationKeys.WindowTitleScrollable] = ReadGlobalString(config, ConfigurationKeys.WindowTitleScrollable),
            [ConfigurationKeys.BackgroundImagePath] = ReadGlobalString(config, ConfigurationKeys.BackgroundImagePath),
            [ConfigurationKeys.BackgroundOpacity] = ReadGlobalString(config, ConfigurationKeys.BackgroundOpacity),
            [ConfigurationKeys.BackgroundBlurEffectRadius] = ReadGlobalString(config, ConfigurationKeys.BackgroundBlurEffectRadius),
            [ConfigurationKeys.BackgroundImageStretchMode] = ReadGlobalString(config, ConfigurationKeys.BackgroundImageStretchMode),
        };
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

    private static string CreateExistingFile(string root, string name)
    {
        var path = Path.Combine(root, name);
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        File.WriteAllText(path, "dummy");
        return path;
    }

    private static async Task WaitUntilAsync(Func<bool> condition, int timeoutMs = 2000)
    {
        var start = Environment.TickCount64;
        while (!condition())
        {
            if (Environment.TickCount64 - start > timeoutMs)
            {
                throw new TimeoutException("Expected condition was not met.");
            }

            await Task.Delay(25);
        }
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
            ITrayService? trayService = null)
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
            trayService ??= new CapturingTrayService();
            var platform = new PlatformServiceBundle
            {
                TrayService = trayService,
                NotificationService = new NoOpNotificationService(),
                HotkeyService = new NoOpGlobalHotkeyService(),
                AutostartService = new NoOpAutostartService(),
                FileDialogService = new NoOpFileDialogService(),
                OverlayService = new NoOpOverlayCapabilityService(),
                PostActionExecutorService = new NoOpPostActionExecutorService(),
            };

            var capability = new PlatformCapabilityFeatureService(platform, diagnostics);
            var connect = new ConnectFeatureService(session, config);
            var shell = new ShellFeatureService(connect);
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
                ShellFeatureService = shell,
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

    private sealed class CapturingTrayService : ITrayService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "tray test service",
            Provider: "test");

        public event EventHandler<TrayCommandEvent>? CommandInvoked;

        public TrayMenuText? LastMenuText { get; private set; }

        public Task<PlatformOperationResult> InitializeAsync(
            string appTitle,
            TrayMenuText? menuText,
            CancellationToken cancellationToken = default)
        {
            LastMenuText = menuText;
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "initialized", "tray.initialize"));
        }

        public Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "shutdown", "tray.shutdown"));
        }

        public Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "show", "tray.show"));
        }

        public Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "set-menu", "tray.setMenuState"));
        }

        public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "set-visible", "tray.setVisible"));
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
