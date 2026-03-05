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

public sealed class SettingsModuleAK2FeatureTests
{
    [Fact]
    public async Task SaveStartPerformanceSettings_WritesExpectedKeys_AndReadBackMatchesVm()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var emulatorPath = CreateExistingFile(fixture.Root, "emulator.exe");
        vm.RunDirectly = true;
        vm.MinimizeDirectly = true;
        vm.OpenEmulatorAfterLaunch = true;
        vm.EmulatorPath = $"  {emulatorPath}  ";
        vm.EmulatorAddCommand = " --instance 2 ";
        vm.EmulatorWaitSeconds = 135;
        vm.PerformanceUseGpu = true;
        vm.PerformanceAllowDeprecatedGpu = true;
        vm.PerformancePreferredGpuDescription = "  GPU-DESC  ";
        vm.PerformancePreferredGpuInstancePath = "  GPU-PATH  ";

        await vm.SaveStartPerformanceSettingsAsync();

        Assert.Equal("True", ReadGlobalString(fixture.Config, ConfigurationKeys.RunDirectly));
        Assert.Equal("True", ReadGlobalString(fixture.Config, ConfigurationKeys.MinimizeDirectly));
        Assert.Equal("True", ReadGlobalString(fixture.Config, ConfigurationKeys.StartEmulator));
        Assert.Equal(emulatorPath, ReadGlobalString(fixture.Config, ConfigurationKeys.EmulatorPath));
        Assert.Equal("--instance 2", ReadGlobalString(fixture.Config, ConfigurationKeys.EmulatorAddCommand));
        Assert.Equal("135", ReadGlobalString(fixture.Config, ConfigurationKeys.EmulatorWaitSeconds));
        Assert.Equal("True", ReadGlobalString(fixture.Config, ConfigurationKeys.PerformanceUseGpu));
        Assert.Equal("True", ReadGlobalString(fixture.Config, ConfigurationKeys.PerformanceAllowDeprecatedGpu));
        Assert.Equal("GPU-DESC", ReadGlobalString(fixture.Config, ConfigurationKeys.PerformancePreferredGpuDescription));
        Assert.Equal("GPU-PATH", ReadGlobalString(fixture.Config, ConfigurationKeys.PerformancePreferredGpuInstancePath));

        Assert.True(vm.RunDirectly);
        Assert.True(vm.MinimizeDirectly);
        Assert.True(vm.OpenEmulatorAfterLaunch);
        Assert.Equal(emulatorPath, vm.EmulatorPath);
        Assert.Equal("--instance 2", vm.EmulatorAddCommand);
        Assert.Equal(135, vm.EmulatorWaitSeconds);
        Assert.True(vm.PerformanceUseGpu);
        Assert.True(vm.PerformanceAllowDeprecatedGpu);
        Assert.Equal("GPU-DESC", vm.PerformancePreferredGpuDescription);
        Assert.Equal("GPU-PATH", vm.PerformancePreferredGpuInstancePath);
        Assert.False(vm.HasPendingStartPerformanceChanges);
    }

    [Fact]
    public async Task SaveStartPerformanceSettings_InvalidEmulatorWaitSeconds_BlocksSaveAndKeepsPreviousConfig()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.EmulatorWaitSeconds = 60;
        await vm.SaveStartPerformanceSettingsAsync();

        vm.EmulatorWaitSeconds = 900;
        await vm.SaveStartPerformanceSettingsAsync();

        Assert.Equal("60", ReadGlobalString(fixture.Config, ConfigurationKeys.EmulatorWaitSeconds));
        Assert.True(vm.HasPendingStartPerformanceChanges);
        Assert.Contains("0-600", vm.StartPerformanceValidationMessage, StringComparison.Ordinal);
    }

    [Fact]
    public async Task SaveStartPerformanceSettings_OpenEmulatorWithoutExistingPath_BlocksSaveAndKeepsPreviousConfig()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.OpenEmulatorAfterLaunch = false;
        vm.EmulatorPath = string.Empty;
        vm.EmulatorWaitSeconds = 30;
        await vm.SaveStartPerformanceSettingsAsync();

        var missingPath = Path.Combine(fixture.Root, "missing-emulator.exe");
        vm.OpenEmulatorAfterLaunch = true;
        vm.EmulatorPath = missingPath;
        await vm.SaveStartPerformanceSettingsAsync();

        Assert.Equal("False", ReadGlobalString(fixture.Config, ConfigurationKeys.StartEmulator));
        Assert.Equal(string.Empty, ReadGlobalString(fixture.Config, ConfigurationKeys.EmulatorPath));
        Assert.True(vm.HasPendingStartPerformanceChanges);
        Assert.Contains("does not exist", vm.StartPerformanceValidationMessage, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task Initialize_LoadStartPerformanceFromConfig_NormalizesRangeAndParsesBoolCompatibility()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();

        var emulatorPath = CreateExistingFile(fixture.Root, "normalize-emulator.exe");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.RunDirectly] = JsonValue.Create("1");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.MinimizeDirectly] = JsonValue.Create(0);
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.StartEmulator] = JsonValue.Create("true");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EmulatorPath] = JsonValue.Create($"  {emulatorPath}  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EmulatorAddCommand] = JsonValue.Create("  --normalize  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EmulatorWaitSeconds] = JsonValue.Create("9999");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformanceUseGpu] = JsonValue.Create(1);
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformanceAllowDeprecatedGpu] = JsonValue.Create("false");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformancePreferredGpuDescription] = JsonValue.Create("  DESC  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformancePreferredGpuInstancePath] = JsonValue.Create("  PATH  ");

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.True(vm.RunDirectly);
        Assert.False(vm.MinimizeDirectly);
        Assert.True(vm.OpenEmulatorAfterLaunch);
        Assert.Equal(emulatorPath, vm.EmulatorPath);
        Assert.Equal("--normalize", vm.EmulatorAddCommand);
        Assert.Equal(600, vm.EmulatorWaitSeconds);
        Assert.True(vm.PerformanceUseGpu);
        Assert.False(vm.PerformanceAllowDeprecatedGpu);
        Assert.Equal("DESC", vm.PerformancePreferredGpuDescription);
        Assert.Equal("PATH", vm.PerformancePreferredGpuInstancePath);
        Assert.Contains("clamped", vm.StartPerformanceValidationMessage, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task RestartRoundTrip_StartPerformanceFields_PersistAndReload()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            var emulatorPath = CreateExistingFile(root, "roundtrip-emulator.exe");

            await using (var first = await RuntimeFixture.CreateAsync(root, cleanupRoot: false))
            {
                var vm = new SettingsPageViewModel(first.Runtime, new ConnectionGameSharedStateViewModel());
                await vm.InitializeAsync();

                vm.RunDirectly = true;
                vm.MinimizeDirectly = false;
                vm.OpenEmulatorAfterLaunch = true;
                vm.EmulatorPath = emulatorPath;
                vm.EmulatorAddCommand = "--boot";
                vm.EmulatorWaitSeconds = 210;
                vm.PerformanceUseGpu = true;
                vm.PerformanceAllowDeprecatedGpu = false;
                vm.PerformancePreferredGpuDescription = "RTX";
                vm.PerformancePreferredGpuInstancePath = "PCI#0";

                await vm.SaveStartPerformanceSettingsAsync();
            }

            await using var second = await RuntimeFixture.CreateAsync(root, cleanupRoot: false);
            var reloaded = new SettingsPageViewModel(second.Runtime, new ConnectionGameSharedStateViewModel());
            await reloaded.InitializeAsync();

            Assert.True(reloaded.RunDirectly);
            Assert.False(reloaded.MinimizeDirectly);
            Assert.True(reloaded.OpenEmulatorAfterLaunch);
            Assert.Equal(emulatorPath, reloaded.EmulatorPath);
            Assert.Equal("--boot", reloaded.EmulatorAddCommand);
            Assert.Equal(210, reloaded.EmulatorWaitSeconds);
            Assert.True(reloaded.PerformanceUseGpu);
            Assert.False(reloaded.PerformanceAllowDeprecatedGpu);
            Assert.Equal("RTX", reloaded.PerformancePreferredGpuDescription);
            Assert.Equal("PCI#0", reloaded.PerformancePreferredGpuInstancePath);
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

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private readonly bool _cleanupRoot;

        private RuntimeFixture(
            string root,
            MAAUnifiedRuntime runtime,
            UnifiedConfigurationService config,
            bool cleanupRoot)
        {
            Root = root;
            Runtime = runtime;
            Config = config;
            _cleanupRoot = cleanupRoot;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public UnifiedConfigurationService Config { get; }

        public static async Task<RuntimeFixture> CreateAsync(string? root = null, bool cleanupRoot = true)
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

            return new RuntimeFixture(root, runtime, config, cleanupRoot);
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
