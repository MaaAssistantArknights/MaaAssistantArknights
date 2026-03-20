using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using MAAUnified.App.Features.Dialogs;
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
    public async Task SaveStartPerformanceSettings_WindowsGpuSelection_WritesExpectedKeys_AndReadBackMatchesVm()
    {
        await using var fixture = await RuntimeFixture.CreateAsync(gpuCapabilityService: new ScriptedWindowsGpuCapabilityService());
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
        vm.SelectedGpuOption = Assert.Single(
            vm.AvailableGpuOptions,
            option => option.Descriptor.InstancePath == "GPU-PATH");
        vm.DeploymentWithPause = true;
        vm.StartsWithScript = "  \"C:\\1.cmd\" -minimized  ";
        vm.EndsWithScript = "  \"C:\\2.cmd\" -noWindow  ";
        vm.CopilotWithScript = true;
        vm.ManualStopWithScript = true;
        vm.BlockSleep = true;
        vm.BlockSleepWithScreenOn = false;
        vm.EnablePenguin = false;
        vm.EnableYituliu = true;
        vm.PenguinId = "  penguin-001  ";
        vm.TaskTimeoutMinutes = 180;
        vm.ReminderIntervalMinutes = 45;

        await vm.SaveStartPerformanceSettingsAsync();

        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.RunDirectly));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.MinimizeDirectly));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.StartEmulator));
        Assert.Equal(emulatorPath, ReadScopedString(fixture.Config, ConfigurationKeys.EmulatorPath));
        Assert.Equal("--instance 2", ReadScopedString(fixture.Config, ConfigurationKeys.EmulatorAddCommand));
        Assert.Equal("135", ReadScopedString(fixture.Config, ConfigurationKeys.EmulatorWaitSeconds));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.PerformanceUseGpu));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.PerformanceAllowDeprecatedGpu));
        Assert.Equal("GPU-DESC", ReadScopedString(fixture.Config, ConfigurationKeys.PerformancePreferredGpuDescription));
        Assert.Equal("GPU-PATH", ReadScopedString(fixture.Config, ConfigurationKeys.PerformancePreferredGpuInstancePath));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.RoguelikeDeploymentWithPause));
        Assert.Equal("\"C:\\1.cmd\" -minimized", ReadScopedString(fixture.Config, ConfigurationKeys.StartsWithScript));
        Assert.Equal("\"C:\\2.cmd\" -noWindow", ReadScopedString(fixture.Config, ConfigurationKeys.EndsWithScript));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.CopilotWithScript));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.ManualStopWithScript));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.BlockSleep));
        Assert.Equal("False", ReadScopedString(fixture.Config, ConfigurationKeys.BlockSleepWithScreenOn));
        Assert.Equal("False", ReadScopedString(fixture.Config, ConfigurationKeys.EnablePenguin));
        Assert.Equal("True", ReadScopedString(fixture.Config, ConfigurationKeys.EnableYituliu));
        Assert.Equal("penguin-001", ReadScopedString(fixture.Config, ConfigurationKeys.PenguinId));
        Assert.Equal("180", ReadScopedString(fixture.Config, ConfigurationKeys.TaskTimeoutMinutes));
        Assert.Equal("45", ReadScopedString(fixture.Config, ConfigurationKeys.ReminderIntervalMinutes));

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
        Assert.True(vm.DeploymentWithPause);
        Assert.Equal("\"C:\\1.cmd\" -minimized", vm.StartsWithScript);
        Assert.Equal("\"C:\\2.cmd\" -noWindow", vm.EndsWithScript);
        Assert.True(vm.CopilotWithScript);
        Assert.True(vm.ManualStopWithScript);
        Assert.True(vm.BlockSleep);
        Assert.False(vm.BlockSleepWithScreenOn);
        Assert.False(vm.EnablePenguin);
        Assert.True(vm.EnableYituliu);
        Assert.Equal("penguin-001", vm.PenguinId);
        Assert.Equal(180, vm.TaskTimeoutMinutes);
        Assert.Equal(45, vm.ReminderIntervalMinutes);
        Assert.False(vm.HasPendingStartPerformanceChanges);
    }

    [Fact]
    public async Task SaveStartPerformanceSettings_GpuChangedAndRestartConfirmed_ShouldRestartAndExit()
    {
        await using var fixture = await RuntimeFixture.CreateAsync(gpuCapabilityService: new ScriptedWindowsGpuCapabilityService());
        fixture.Runtime.AppLifecycleService = new SpyAppLifecycleService(
            restartResult: UiOperationResult.Ok("Restart process launched."),
            exitResult: UiOperationResult.Ok("Application shutdown requested."));
        var dialogService = new ScriptedDialogService(DialogReturnSemantic.Confirm);
        var vm = new SettingsPageViewModel(
            fixture.Runtime,
            new ConnectionGameSharedStateViewModel(),
            dialogService: dialogService);
        await vm.InitializeAsync();

        vm.PerformanceUseGpu = true;
        vm.SelectedGpuOption = Assert.Single(
            vm.AvailableGpuOptions,
            option => option.Descriptor.InstancePath == "GPU-PATH");

        await vm.SaveStartPerformanceSettingsAsync();

        Assert.Equal(1, dialogService.WarningConfirmCallCount);
        Assert.Equal(1, Assert.IsType<SpyAppLifecycleService>(fixture.Runtime.AppLifecycleService).RestartCallCount);
        Assert.Equal(1, Assert.IsType<SpyAppLifecycleService>(fixture.Runtime.AppLifecycleService).ExitCallCount);
    }

    [Fact]
    public async Task SaveStartPerformanceSettings_GpuChangedAndRestartDeferred_ShouldSaveWithoutRestart()
    {
        await using var fixture = await RuntimeFixture.CreateAsync(gpuCapabilityService: new ScriptedWindowsGpuCapabilityService());
        fixture.Runtime.AppLifecycleService = new SpyAppLifecycleService(
            restartResult: UiOperationResult.Ok("Restart process launched."),
            exitResult: UiOperationResult.Ok("Application shutdown requested."));
        var dialogService = new ScriptedDialogService(DialogReturnSemantic.Cancel);
        var vm = new SettingsPageViewModel(
            fixture.Runtime,
            new ConnectionGameSharedStateViewModel(),
            dialogService: dialogService);
        await vm.InitializeAsync();

        vm.PerformanceUseGpu = true;
        vm.SelectedGpuOption = Assert.Single(
            vm.AvailableGpuOptions,
            option => option.Descriptor.InstancePath == "GPU-PATH");

        await vm.SaveStartPerformanceSettingsAsync();

        Assert.Equal(1, dialogService.WarningConfirmCallCount);
        Assert.Equal(0, Assert.IsType<SpyAppLifecycleService>(fixture.Runtime.AppLifecycleService).RestartCallCount);
        Assert.Equal(0, Assert.IsType<SpyAppLifecycleService>(fixture.Runtime.AppLifecycleService).ExitCallCount);
        Assert.Equal(vm.RootTexts["Settings.Performance.Gpu.RestartPending"], vm.StatusMessage);
    }

    [Fact]
    public async Task SaveStartPerformanceSettings_UnsupportedGpuConfig_IsClearedToSafeValues()
    {
        await using var fixture = await RuntimeFixture.CreateAsync(gpuCapabilityService: new UnsupportedGpuCapabilityService());
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformanceUseGpu] = JsonValue.Create("True");
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformancePreferredGpuDescription] = JsonValue.Create("LEGACY-DESC");
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformancePreferredGpuInstancePath] = JsonValue.Create("LEGACY-PATH");

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.RunDirectly = true;
        vm.PerformanceUseGpu = false;
        await vm.SaveStartPerformanceSettingsAsync();

        Assert.False(vm.IsGpuSelectionEnabled);
        Assert.Equal("False", ReadScopedString(fixture.Config, ConfigurationKeys.PerformanceUseGpu));
        Assert.Equal("False", ReadScopedString(fixture.Config, ConfigurationKeys.PerformanceAllowDeprecatedGpu));
        Assert.Equal(string.Empty, ReadScopedString(fixture.Config, ConfigurationKeys.PerformancePreferredGpuDescription));
        Assert.Equal(string.Empty, ReadScopedString(fixture.Config, ConfigurationKeys.PerformancePreferredGpuInstancePath));
    }

    [Fact]
    public async Task Initialize_UnsupportedGpuConfig_DisablesSelectionAndCollapsesLegacyState()
    {
        await using var fixture = await RuntimeFixture.CreateAsync(gpuCapabilityService: new UnsupportedGpuCapabilityService());
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformanceUseGpu] = JsonValue.Create("True");
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformancePreferredGpuDescription] = JsonValue.Create("Apple GPU");
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformancePreferredGpuInstancePath] = JsonValue.Create("Metal#0");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformanceUseGpu] = JsonValue.Create("True");

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.False(vm.PerformanceUseGpu);
        Assert.False(vm.IsGpuSelectionEnabled);
        Assert.False(vm.IsGpuDeprecatedToggleEnabled);
        Assert.False(vm.IsGpuCustomSelectionFieldsVisible);
        Assert.Single(vm.AvailableGpuOptions);
        Assert.Equal(GpuOptionKind.Disabled, vm.SelectedGpuOption?.Descriptor.Kind);
        Assert.Contains(
            "CPU OCR fallback",
            vm.StartPerformanceValidationMessage,
            StringComparison.Ordinal);
        Assert.False(fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values.ContainsKey(ConfigurationKeys.PerformanceUseGpu));
        Assert.False(fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values.ContainsKey(ConfigurationKeys.PerformancePreferredGpuDescription));
        Assert.False(fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values.ContainsKey(ConfigurationKeys.PerformancePreferredGpuInstancePath));
        Assert.False(fixture.Config.CurrentConfig.GlobalValues.ContainsKey(ConfigurationKeys.PerformanceUseGpu));
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

        Assert.Equal("60", ReadScopedString(fixture.Config, ConfigurationKeys.EmulatorWaitSeconds));
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

        Assert.Equal("False", ReadScopedString(fixture.Config, ConfigurationKeys.StartEmulator));
        Assert.Equal(string.Empty, ReadScopedString(fixture.Config, ConfigurationKeys.EmulatorPath));
        Assert.True(vm.HasPendingStartPerformanceChanges);
        Assert.Contains("does not exist", vm.StartPerformanceValidationMessage, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task Initialize_LoadStartPerformanceFromConfig_NormalizesRangeAndParsesBoolCompatibility()
    {
        await using var fixture = await RuntimeFixture.CreateAsync(gpuCapabilityService: new ScriptedWindowsGpuCapabilityService());

        var emulatorPath = CreateExistingFile(fixture.Root, "normalize-emulator.exe");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.RunDirectly] = JsonValue.Create("1");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.MinimizeDirectly] = JsonValue.Create(0);
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.StartEmulator] = JsonValue.Create("true");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EmulatorPath] = JsonValue.Create($"  {emulatorPath}  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EmulatorAddCommand] = JsonValue.Create("  --normalize  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EmulatorWaitSeconds] = JsonValue.Create("9999");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformanceUseGpu] = JsonValue.Create(1);
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformanceAllowDeprecatedGpu] = JsonValue.Create("false");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformancePreferredGpuDescription] = JsonValue.Create("  GPU-DESC  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PerformancePreferredGpuInstancePath] = JsonValue.Create("  GPU-PATH  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.RoguelikeDeploymentWithPause] = JsonValue.Create("1");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.StartsWithScript] = JsonValue.Create("  before.cmd  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EndsWithScript] = JsonValue.Create("  after.cmd  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.CopilotWithScript] = JsonValue.Create("true");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.ManualStopWithScript] = JsonValue.Create(0);
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.BlockSleep] = JsonValue.Create("1");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.BlockSleepWithScreenOn] = JsonValue.Create("0");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EnablePenguin] = JsonValue.Create("false");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.EnableYituliu] = JsonValue.Create(1);
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.PenguinId] = JsonValue.Create("  pid  ");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.TaskTimeoutMinutes] = JsonValue.Create("90");
        fixture.Config.CurrentConfig.GlobalValues[ConfigurationKeys.ReminderIntervalMinutes] = JsonValue.Create("0");

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
        Assert.Equal("GPU-DESC", vm.PerformancePreferredGpuDescription);
        Assert.Equal("GPU-PATH", vm.PerformancePreferredGpuInstancePath);
        Assert.Equal("GPU-DESC", vm.SelectedGpuOption?.Descriptor.Description);
        Assert.True(vm.DeploymentWithPause);
        Assert.Equal("before.cmd", vm.StartsWithScript);
        Assert.Equal("after.cmd", vm.EndsWithScript);
        Assert.True(vm.CopilotWithScript);
        Assert.False(vm.ManualStopWithScript);
        Assert.True(vm.BlockSleep);
        Assert.False(vm.BlockSleepWithScreenOn);
        Assert.False(vm.EnablePenguin);
        Assert.True(vm.EnableYituliu);
        Assert.Equal("pid", vm.PenguinId);
        Assert.Equal(90, vm.TaskTimeoutMinutes);
        Assert.Equal(1, vm.ReminderIntervalMinutes);
        Assert.Contains("clamped", vm.StartPerformanceValidationMessage, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task Initialize_WindowsGpuSelectionFallback_ShowsWarningAndClearsStaleSpecificGpu()
    {
        await using var fixture = await RuntimeFixture.CreateAsync(gpuCapabilityService: new ScriptedWindowsGpuCapabilityService());
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformanceUseGpu] = JsonValue.Create("True");
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformancePreferredGpuDescription] = JsonValue.Create("Missing GPU");
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.PerformancePreferredGpuInstancePath] = JsonValue.Create("Missing#0");

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.True(vm.PerformanceUseGpu);
        Assert.Equal(string.Empty, vm.PerformancePreferredGpuDescription);
        Assert.Equal(string.Empty, vm.PerformancePreferredGpuInstancePath);
        Assert.Equal(GpuOptionKind.SystemDefault, vm.SelectedGpuOption?.Descriptor.Kind);
        Assert.Contains(
            vm.RootTexts["Settings.Performance.Gpu.Warning.SelectionFallback"],
            vm.GpuWarningMessage,
            StringComparison.Ordinal);
    }

    [Fact]
    public async Task Initialize_WindowsGpuSelection_SystemDefaultDisplayShowsResolvedGpuName()
    {
        await using var fixture = await RuntimeFixture.CreateAsync(gpuCapabilityService: new ScriptedWindowsGpuCapabilityService());

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        var systemDefault = Assert.Single(
            vm.AvailableGpuOptions,
            option => option.Descriptor.Kind == GpuOptionKind.SystemDefault);
        Assert.Equal(
            $"{vm.RootTexts["Settings.Performance.Gpu.Option.SystemDefault"]} (RTX)",
            systemDefault.Display);
        Assert.Equal("RTX", systemDefault.Descriptor.Description);
    }

    [Fact]
    public async Task RestartRoundTrip_StartPerformanceFields_PersistAndReload_OnWindows()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            var emulatorPath = CreateExistingFile(root, "roundtrip-emulator.exe");

            await using (var first = await RuntimeFixture.CreateAsync(root, cleanupRoot: false, gpuCapabilityService: new ScriptedWindowsGpuCapabilityService()))
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
                vm.SelectedGpuOption = Assert.Single(
                    vm.AvailableGpuOptions,
                    option => option.Descriptor.InstancePath == "PCI#0");
                vm.DeploymentWithPause = true;
                vm.StartsWithScript = "before";
                vm.EndsWithScript = "after";
                vm.CopilotWithScript = true;
                vm.ManualStopWithScript = false;
                vm.BlockSleep = true;
                vm.BlockSleepWithScreenOn = true;
                vm.EnablePenguin = true;
                vm.EnableYituliu = false;
                vm.PenguinId = "penguin-roundtrip";
                vm.TaskTimeoutMinutes = 240;
                vm.ReminderIntervalMinutes = 60;

                await vm.SaveStartPerformanceSettingsAsync();
            }

            await using var second = await RuntimeFixture.CreateAsync(root, cleanupRoot: false, gpuCapabilityService: new ScriptedWindowsGpuCapabilityService());
            var reloaded = new SettingsPageViewModel(second.Runtime, new ConnectionGameSharedStateViewModel());
            await reloaded.InitializeAsync();

            Assert.True(reloaded.RunDirectly);
            Assert.False(reloaded.MinimizeDirectly);
            Assert.True(reloaded.OpenEmulatorAfterLaunch);
            Assert.Equal(emulatorPath, reloaded.EmulatorPath);
            Assert.Equal("--boot", reloaded.EmulatorAddCommand);
            Assert.Equal(210, reloaded.EmulatorWaitSeconds);
            Assert.True(reloaded.PerformanceUseGpu);
            Assert.Equal("RTX", reloaded.PerformancePreferredGpuDescription);
            Assert.Equal("PCI#0", reloaded.PerformancePreferredGpuInstancePath);
            Assert.True(reloaded.DeploymentWithPause);
            Assert.Equal("before", reloaded.StartsWithScript);
            Assert.Equal("after", reloaded.EndsWithScript);
            Assert.True(reloaded.CopilotWithScript);
            Assert.False(reloaded.ManualStopWithScript);
            Assert.True(reloaded.BlockSleep);
            Assert.True(reloaded.BlockSleepWithScreenOn);
            Assert.True(reloaded.EnablePenguin);
            Assert.False(reloaded.EnableYituliu);
            Assert.Equal("penguin-roundtrip", reloaded.PenguinId);
            Assert.Equal(240, reloaded.TaskTimeoutMinutes);
            Assert.Equal(60, reloaded.ReminderIntervalMinutes);
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

    private static string ReadScopedString(UnifiedConfigurationService config, string key)
    {
        if (config.CurrentConfig.Profiles.TryGetValue(config.CurrentConfig.CurrentProfile, out var profile)
            && profile.Values.TryGetValue(key, out var profileNode)
            && profileNode is not null)
        {
            if (profileNode is JsonValue profileValue && profileValue.TryGetValue(out string? text) && text is not null)
            {
                return text;
            }

            return profileNode.ToString();
        }

        if (!config.CurrentConfig.GlobalValues.TryGetValue(key, out var globalNode) || globalNode is null)
        {
            return string.Empty;
        }

        if (globalNode is JsonValue globalValue && globalValue.TryGetValue(out string? globalText) && globalText is not null)
        {
            return globalText;
        }

        return globalNode.ToString();
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

        public static async Task<RuntimeFixture> CreateAsync(
            string? root = null,
            bool cleanupRoot = true,
            IGpuCapabilityService? gpuCapabilityService = null)
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
                GpuCapabilityService = gpuCapabilityService ?? new UnsupportedGpuCapabilityService(),
            };

            var capability = new PlatformCapabilityFeatureService(platform, diagnostics);
            var connect = new ConnectFeatureService(session, config);
            var shell = new ShellFeatureService(connect);
            var runtime = new MAAUnifiedRuntime
            {
                CoreBridge = bridge,
                ConfigurationService = config,
                ResourceWorkflowService = new ResourceWorkflowService(root, bridge, log, platform.GpuCapabilityService),
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

    private sealed class ScriptedWindowsGpuCapabilityService : IGpuCapabilityService
    {
        public GpuSelectionResolution Resolve(GpuPreference preference)
        {
            var options = new List<GpuOptionDescriptor>
            {
                GpuOptionDescriptor.Disabled,
                GpuOptionDescriptor.SystemDefault("RTX"),
                new(
                    Id: "PCI#0",
                    Kind: GpuOptionKind.SpecificGpu,
                    DisplayName: "RTX",
                    Description: "RTX",
                    InstancePath: "PCI#0",
                    GpuIndex: 1),
                new(
                    Id: "GPU-PATH",
                    Kind: GpuOptionKind.SpecificGpu,
                    DisplayName: "GPU-DESC",
                    Description: "GPU-DESC",
                    InstancePath: "GPU-PATH",
                    GpuIndex: 2),
            };

            if (preference.AllowDeprecatedGpu)
            {
                options.Add(new GpuOptionDescriptor(
                    Id: "deprecated",
                    Kind: GpuOptionKind.SpecificGpu,
                    DisplayName: "Legacy GPU",
                    Description: "Legacy GPU",
                    InstancePath: "LEGACY#0",
                    GpuIndex: 3,
                    IsDeprecated: true,
                    DriverDate: new DateTime(2018, 1, 1)));
            }

            var selected = preference.UseGpu
                ? options.FirstOrDefault(option => option.Kind == GpuOptionKind.SystemDefault)
                : GpuOptionDescriptor.Disabled;
            var changed = false;

            if (preference.UseGpu)
            {
                if (!string.IsNullOrWhiteSpace(preference.PreferredGpuInstancePath))
                {
                    selected = options.FirstOrDefault(
                                   option => option.Kind == GpuOptionKind.SpecificGpu
                                       && string.Equals(option.InstancePath, preference.PreferredGpuInstancePath, StringComparison.Ordinal))
                               ?? selected;
                    changed = selected?.InstancePath != preference.PreferredGpuInstancePath && !string.IsNullOrWhiteSpace(preference.PreferredGpuInstancePath);
                }

                if ((selected is null || selected.Kind == GpuOptionKind.SystemDefault)
                    && !string.IsNullOrWhiteSpace(preference.PreferredGpuDescription))
                {
                    selected = options.FirstOrDefault(
                                   option => option.Kind == GpuOptionKind.SpecificGpu
                                       && string.Equals(option.Description, preference.PreferredGpuDescription, StringComparison.OrdinalIgnoreCase))
                               ?? selected;
                    changed = selected?.Description != preference.PreferredGpuDescription
                        && !string.IsNullOrWhiteSpace(preference.PreferredGpuDescription);
                }
            }

            selected ??= GpuOptionDescriptor.Disabled;

            return new GpuSelectionResolution(
                Snapshot: new GpuCapabilitySnapshot(
                    SupportMode: GpuPlatformSupportMode.WindowsSupported,
                    IsEditable: true,
                    AppliesToCore: true,
                    SupportsDeprecatedToggle: true,
                    Options: options,
                    StatusTextKey: "Settings.Performance.Gpu.Status.WindowsReady",
                    Provider: "scripted-windows"),
                SelectedOption: selected,
                SelectionChanged: changed,
                SelectionWarningTextKey: changed ? "Settings.Performance.Gpu.Warning.SelectionFallback" : null);
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

    private sealed class ScriptedDialogService : IAppDialogService
    {
        private readonly DialogReturnSemantic _warningConfirmReturn;

        public ScriptedDialogService(DialogReturnSemantic warningConfirmReturn)
        {
            _warningConfirmReturn = warningConfirmReturn;
        }

        public int WarningConfirmCallCount { get; private set; }

        public Task<DialogCompletion<AnnouncementDialogPayload>> ShowAnnouncementAsync(
            AnnouncementDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<AnnouncementDialogPayload>(DialogReturnSemantic.Close, null, "scripted"));

        public Task<DialogCompletion<VersionUpdateDialogPayload>> ShowVersionUpdateAsync(
            VersionUpdateDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<VersionUpdateDialogPayload>(DialogReturnSemantic.Close, null, "scripted"));

        public Task<DialogCompletion<ProcessPickerDialogPayload>> ShowProcessPickerAsync(
            ProcessPickerDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<ProcessPickerDialogPayload>(DialogReturnSemantic.Close, null, "scripted"));

        public Task<DialogCompletion<EmulatorPathDialogPayload>> ShowEmulatorPathAsync(
            EmulatorPathDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<EmulatorPathDialogPayload>(DialogReturnSemantic.Close, null, "scripted"));

        public Task<DialogCompletion<ErrorDialogPayload>> ShowErrorAsync(
            ErrorDialogRequest request,
            string sourceScope,
            Func<CancellationToken, Task<UiOperationResult>>? openIssueReportAsync = null,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<ErrorDialogPayload>(DialogReturnSemantic.Close, null, "scripted"));

        public Task<DialogCompletion<AchievementListDialogPayload>> ShowAchievementListAsync(
            AchievementListDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<AchievementListDialogPayload>(DialogReturnSemantic.Close, null, "scripted"));

        public Task<DialogCompletion<TextDialogPayload>> ShowTextAsync(
            TextDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
            => Task.FromResult(new DialogCompletion<TextDialogPayload>(DialogReturnSemantic.Close, null, "scripted"));

        public Task<DialogCompletion<WarningConfirmDialogPayload>> ShowWarningConfirmAsync(
            WarningConfirmDialogRequest request,
            string sourceScope,
            CancellationToken cancellationToken = default)
        {
            WarningConfirmCallCount++;
            return Task.FromResult(new DialogCompletion<WarningConfirmDialogPayload>(
                _warningConfirmReturn,
                _warningConfirmReturn == DialogReturnSemantic.Confirm ? new WarningConfirmDialogPayload(true) : null,
                "scripted"));
        }
    }

    private sealed class SpyAppLifecycleService : IAppLifecycleService
    {
        private readonly UiOperationResult _restartResult;
        private readonly UiOperationResult _exitResult;

        public SpyAppLifecycleService(UiOperationResult restartResult, UiOperationResult exitResult)
        {
            _restartResult = restartResult;
            _exitResult = exitResult;
        }

        public bool SupportsExit => true;

        public int RestartCallCount { get; private set; }

        public int ExitCallCount { get; private set; }

        public Task<UiOperationResult> RestartAsync(CancellationToken cancellationToken = default)
        {
            RestartCallCount++;
            return Task.FromResult(_restartResult);
        }

        public Task<UiOperationResult> ExitAsync(CancellationToken cancellationToken = default)
        {
            ExitCallCount++;
            return Task.FromResult(_exitResult);
        }
    }
}
