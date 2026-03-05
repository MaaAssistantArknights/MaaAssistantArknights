using System.Reflection;
using System.Runtime.CompilerServices;
using MAAUnified.App.ViewModels;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Application.Services.Localization;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class MainShellViewModelTests
{
    [Theory]
    [InlineData(false, false)]
    [InlineData(false, true)]
    [InlineData(true, false)]
    [InlineData(true, true)]
    public async Task CanStartExecution_ShouldMatchTrayStartState(bool isRunning, bool hasBlockingIssue)
    {
        await using var fixture = await TestFixture.CreateAsync();
        SetTaskQueueRunning(fixture.ViewModel.TaskQueuePage, isRunning);
        InvokeRefreshConfigValidationState(
            fixture.ViewModel,
            hasBlockingIssue ? [CreateBlockingIssue()] : []);
        await InvokeSyncTrayMenuStateAsync(fixture.ViewModel);

        var expected = !isRunning && !hasBlockingIssue;
        Assert.Equal(expected, fixture.ViewModel.CanStartExecution);
        Assert.NotNull(fixture.TrayService.LastMenuState);
        Assert.Equal(expected, fixture.TrayService.LastMenuState!.StartEnabled);
    }

    [Fact]
    public void ConfigIssueDetails_ShouldOnlyKeepBlockingIssues_WithCompleteFields()
    {
        using var fixture = TestFixture.CreateSync();
        var issues = new[]
        {
            new ConfigValidationIssue
            {
                Scope = string.Empty,
                Code = "BlockingCode",
                Field = string.Empty,
                Message = "Need fix",
                Blocking = true,
                ProfileName = null,
                TaskIndex = null,
                TaskName = string.Empty,
                SuggestedAction = null,
            },
            new ConfigValidationIssue
            {
                Scope = "TaskValidation",
                Code = "WarnOnly",
                Field = "times",
                Message = "Warning",
                Blocking = false,
            },
        };

        InvokeRefreshConfigValidationState(fixture.ViewModel, issues);

        var detail = Assert.Single(fixture.ViewModel.ConfigIssueDetails);
        Assert.Equal("-", detail.Scope);
        Assert.Equal("BlockingCode", detail.Code);
        Assert.Equal("-", detail.Field);
        Assert.True(detail.Blocking);
        Assert.Equal("-", detail.ProfileName);
        Assert.Equal("-", detail.TaskIndex);
        Assert.Equal("-", detail.TaskName);
        Assert.Equal("Need fix", detail.Message);
        Assert.Equal("-", detail.SuggestedAction);
    }

    [Fact]
    public async Task ExecuteSwitchLanguageAsync_ShouldUseShellFeatureService()
    {
        var shellSpy = new SpyShellFeatureService("ja-jp");
        await using var fixture = await TestFixture.CreateAsync(shellSpy);

        await fixture.ViewModel.ExecuteSwitchLanguageAsync("ja-jp");

        Assert.Equal(1, shellSpy.SwitchLanguageCallCount);
        Assert.Equal("zh-cn", shellSpy.LastCurrentLanguage);
        Assert.Equal("ja-jp", shellSpy.LastTargetLanguage);
        Assert.Equal("ja-jp", fixture.ViewModel.SettingsPage.Language);
        Assert.True(fixture.TrayService.InitializeCallCount > 0);
    }

    [Fact]
    public async Task SwitchLanguageToAsync_ShouldSyncSettingsTaskQueueAndTrayText()
    {
        var shellSpy = new SpyShellFeatureService("pallas");
        await using var fixture = await TestFixture.CreateAsync(shellSpy);

        await fixture.ViewModel.SwitchLanguageToAsync("pallas");

        Assert.Equal(1, shellSpy.SwitchLanguageCallCount);
        Assert.Equal("zh-cn", shellSpy.LastCurrentLanguage);
        Assert.Equal("pallas", shellSpy.LastTargetLanguage);
        Assert.Equal("pallas", fixture.ViewModel.SettingsPage.Language);
        Assert.Equal("pallas", fixture.ViewModel.TaskQueuePage.Texts.Language);
        Assert.NotNull(fixture.TrayService.LastMenuText);
        Assert.False(string.IsNullOrWhiteSpace(fixture.TrayService.LastMenuText!.SwitchLanguage));
    }

    [Fact]
    public async Task SwitchLanguageCycleAsync_ShouldSyncSettingsTaskQueueAndTrayText()
    {
        var shellSpy = new SpyShellFeatureService("en-us");
        await using var fixture = await TestFixture.CreateAsync(shellSpy);

        await fixture.ViewModel.SwitchLanguageCycleAsync();

        Assert.Equal(1, shellSpy.SwitchLanguageCallCount);
        Assert.Equal("zh-cn", shellSpy.LastCurrentLanguage);
        Assert.Null(shellSpy.LastTargetLanguage);
        Assert.Equal("en-us", fixture.ViewModel.SettingsPage.Language);
        Assert.Equal("en-us", fixture.ViewModel.TaskQueuePage.Texts.Language);
        Assert.NotNull(fixture.TrayService.LastMenuText);
        Assert.False(string.IsNullOrWhiteSpace(fixture.TrayService.LastMenuText!.SwitchLanguage));
    }

    [Fact]
    public async Task ExecuteTrayCommandAsync_SwitchLanguageCycle_ShouldUseShellFeatureService()
    {
        var shellSpy = new SpyShellFeatureService("en-us");
        await using var fixture = await TestFixture.CreateAsync(shellSpy);

        var action = await fixture.ViewModel.ExecuteTrayCommandAsync(TrayCommandId.SwitchLanguage, "test-tray");

        Assert.Equal(ShellUiAction.None, action);
        Assert.Equal(1, shellSpy.SwitchLanguageCallCount);
        Assert.Equal("zh-cn", shellSpy.LastCurrentLanguage);
        Assert.Null(shellSpy.LastTargetLanguage);
        Assert.Contains(fixture.ViewModel.GrowlMessages, msg => msg.Contains("语言切换为: en-us", StringComparison.Ordinal));
    }

    [Fact]
    public async Task ExecuteTrayLanguageSwitchAsync_TargetLanguage_UsesTrayScopeAndGrowl()
    {
        var shellSpy = new SpyShellFeatureService("ja-jp");
        await using var fixture = await TestFixture.CreateAsync(shellSpy);

        await fixture.ViewModel.ExecuteTrayLanguageSwitchAsync("ja-jp", "window-shell-menu");

        Assert.Equal(1, shellSpy.SwitchLanguageCallCount);
        Assert.Equal("ja-jp", shellSpy.LastTargetLanguage);
        Assert.Equal("ja-jp", fixture.ViewModel.SettingsPage.Language);
        Assert.Contains(fixture.ViewModel.GrowlMessages, msg => msg.Contains("语言切换为: ja-jp", StringComparison.Ordinal));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.EventLogPath,
            "App.Shell.Tray.SwitchLanguage"));
    }

    [Fact]
    public async Task ReportLocalizationFallback_ShouldDeduplicateByScopeLanguageAndKey()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var first = new LocalizationFallbackInfo("TaskQueue.Localization", "ja-jp", "TaskQueue.Unknown.Key", "en-us");
        var second = new LocalizationFallbackInfo("TaskQueue.Localization", "ja-jp", "TaskQueue.Unknown.Key.2", "en-us");

        fixture.ViewModel.ReportLocalizationFallback(first);
        fixture.ViewModel.ReportLocalizationFallback(first);
        fixture.ViewModel.ReportLocalizationFallback(second);

        var lines = await WaitForEventLinesAsync(
            fixture.Runtime.DiagnosticsService.EventLogPath,
            "Localization.Fallback",
            expectedCount: 2);
        Assert.Equal(2, lines.Count);
        Assert.Single(lines.Where(line => line.Contains("key=TaskQueue.Unknown.Key; fallback=", StringComparison.Ordinal)));
        Assert.Single(lines.Where(line => line.Contains("key=TaskQueue.Unknown.Key.2; fallback=", StringComparison.Ordinal)));
    }

    [Fact]
    public async Task SettingsHotkeyError_WithUnknownCode_ShouldReportLocalizationFallback()
    {
        await using var fixture = await TestFixture.CreateAsync(hotkeyService: new UnknownErrorHotkeyService());
        await fixture.ViewModel.SettingsPage.InitializeAsync();

        await fixture.ViewModel.SettingsPage.RegisterHotkeysAsync();

        Assert.True(fixture.ViewModel.SettingsPage.HasHotkeyErrorMessage);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.EventLogPath,
            "Localization.Fallback"));
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.EventLogPath,
            "key=Error.HotkeyErrorNotMapped"));
    }

    [Fact]
    public async Task ExecuteTrayCommandAsync_ForceShowAndExit_ShouldReturnUiAction()
    {
        await using var fixture = await TestFixture.CreateAsync();

        var showAction = await fixture.ViewModel.ExecuteTrayCommandAsync(TrayCommandId.ForceShow, "test-tray");
        var closeAction = await fixture.ViewModel.ExecuteTrayCommandAsync(TrayCommandId.Exit, "test-tray");

        Assert.Equal(ShellUiAction.ShowMainWindow, showAction);
        Assert.Equal(ShellUiAction.CloseMainWindow, closeAction);
    }

    [Fact]
    public async Task ExecuteTrayCommandAsync_Restart_ShouldUseLifecycleServiceAndCloseWindow()
    {
        var lifecycle = new SpyAppLifecycleService(UiOperationResult.Ok("Restart process launched."));
        await using var fixture = await TestFixture.CreateAsync(appLifecycleService: lifecycle);

        var action = await fixture.ViewModel.ExecuteTrayCommandAsync(TrayCommandId.Restart, "test-tray");

        Assert.Equal(1, lifecycle.RestartCallCount);
        Assert.Equal(ShellUiAction.CloseMainWindow, action);
        Assert.Contains(fixture.ViewModel.GrowlMessages, msg => msg.Contains("重启命令已触发", StringComparison.Ordinal));
    }

    [Fact]
    public async Task SetTrayVisibleAsync_ShouldRecordDiagnosticsAndPushGrowl()
    {
        await using var fixture = await TestFixture.CreateAsync();

        await fixture.ViewModel.SetTrayVisibleAsync(false);

        Assert.Contains(fixture.ViewModel.GrowlMessages, msg => msg.Contains("set-visible", StringComparison.Ordinal));
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.EventLogPath, "App.Shell.Tray.SetVisible"));
    }

    [Fact]
    public async Task ToggleOverlayFromTrayAsync_ShouldRecordDiagnosticsAndPushGrowl()
    {
        await using var fixture = await TestFixture.CreateAsync();

        await fixture.ViewModel.ToggleOverlayFromTrayAsync();

        Assert.Contains(fixture.ViewModel.GrowlMessages, msg => msg.Contains("Overlay 已", StringComparison.Ordinal));
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.EventLogPath, "App.Shell.Tray.ToggleOverlay"));
    }

    [Fact]
    public async Task RuntimeFactory_ShouldInjectShellFeatureService()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        var runtime = MAAUnifiedRuntimeFactory.Create(root);
        try
        {
            Assert.IsType<ShellFeatureService>(runtime.ShellFeatureService);
        }
        finally
        {
            await runtime.DisposeAsync();
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
    public void LegacyMainViewModelFile_ShouldNotExist()
    {
        var root = GetMaaUnifiedRoot();
        var legacyVmPath = Path.Combine(root, "App", "ViewModels", "MainViewModel.cs");
        Assert.False(File.Exists(legacyVmPath), "MainViewModel.cs should not exist after main shell unification.");
    }

    [Fact]
    public void MainWindow_OnSwitchLanguageToClick_ShouldUseTrayLanguageEntryPoint()
    {
        var root = GetMaaUnifiedRoot();
        var path = Path.Combine(root, "App", "Views", "MainWindow.axaml.cs");
        var content = File.ReadAllText(path);
        Assert.DoesNotContain(
            "await VM.SwitchLanguageToAsync(targetLanguage);",
            content,
            StringComparison.Ordinal);
        Assert.Contains(
            "await VM.ExecuteTrayLanguageSwitchAsync(targetLanguage, \"window-shell-menu\");",
            content,
            StringComparison.Ordinal);
    }

    private static ConfigValidationIssue CreateBlockingIssue()
    {
        return new ConfigValidationIssue
        {
            Scope = "TaskValidation",
            Code = "Issue",
            Field = "field",
            Message = "blocked",
            Blocking = true,
            ProfileName = "Default",
            TaskIndex = 0,
            TaskName = "TaskA",
            SuggestedAction = "Fix it",
        };
    }

    private static void SetTaskQueueRunning(TaskQueuePageViewModel vm, bool value)
    {
        var property = typeof(TaskQueuePageViewModel).GetProperty(nameof(TaskQueuePageViewModel.IsRunning));
        Assert.NotNull(property);
        var setter = property!.GetSetMethod(nonPublic: true);
        Assert.NotNull(setter);
        setter!.Invoke(vm, [value]);
    }

    private static void InvokeRefreshConfigValidationState(MainShellViewModel vm, IReadOnlyList<ConfigValidationIssue> issues)
    {
        var method = typeof(MainShellViewModel).GetMethod(
            "RefreshConfigValidationState",
            BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(method);
        method!.Invoke(vm, [issues]);
    }

    private static async Task InvokeSyncTrayMenuStateAsync(MainShellViewModel vm)
    {
        var method = typeof(MainShellViewModel).GetMethod(
            "SyncTrayMenuStateAsync",
            BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(method);
        var task = method!.Invoke(vm, [CancellationToken.None]) as Task;
        Assert.NotNull(task);
        await task!;
    }

    private static string GetMaaUnifiedRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            var appDir = Path.Combine(current.FullName, "App");
            var testsDir = Path.Combine(current.FullName, "Tests");
            if (Directory.Exists(appDir) && Directory.Exists(testsDir))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new DirectoryNotFoundException("Cannot locate src/MAAUnified root from test runtime path.");
    }

    private static async Task<bool> WaitForLogContainsAsync(string path, string expected, int retry = 20, int delayMs = 25)
    {
        for (var i = 0; i < retry; i++)
        {
            if (File.Exists(path))
            {
                var content = await File.ReadAllTextAsync(path);
                if (content.Contains(expected, StringComparison.Ordinal))
                {
                    return true;
                }
            }

            await Task.Delay(delayMs);
        }

        return false;
    }

    private static async Task<IReadOnlyList<string>> WaitForEventLinesAsync(
        string path,
        string expected,
        int expectedCount,
        int retry = 40,
        int delayMs = 25)
    {
        for (var i = 0; i < retry; i++)
        {
            if (File.Exists(path))
            {
                var lines = await File.ReadAllLinesAsync(path);
                var matched = lines
                    .Where(line => line.Contains(expected, StringComparison.Ordinal))
                    .ToArray();
                if (matched.Length >= expectedCount)
                {
                    return matched;
                }
            }

            await Task.Delay(delayMs);
        }

        return Array.Empty<string>();
    }

    private sealed class TestFixture : IAsyncDisposable, IDisposable
    {
        private TestFixture(
            string root,
            MAAUnifiedRuntime runtime,
            MainShellViewModel viewModel,
            CapturingTrayService trayService)
        {
            Root = root;
            Runtime = runtime;
            ViewModel = viewModel;
            TrayService = trayService;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public MainShellViewModel ViewModel { get; }

        public CapturingTrayService TrayService { get; }

        public static TestFixture CreateSync()
        {
            return CreateAsync().GetAwaiter().GetResult();
        }

        public static async Task<TestFixture> CreateAsync(
            IShellFeatureService? shellService = null,
            IAppLifecycleService? appLifecycleService = null,
            IGlobalHotkeyService? hotkeyService = null)
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

            var bridge = new FakeBridge();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var tray = new CapturingTrayService();
            var platform = new PlatformServiceBundle
            {
                TrayService = tray,
                NotificationService = new NoOpNotificationService(),
                HotkeyService = hotkeyService ?? new NoOpGlobalHotkeyService(),
                AutostartService = new NoOpAutostartService(),
                FileDialogService = new NoOpFileDialogService(),
                OverlayService = new NoOpOverlayCapabilityService(),
                PostActionExecutorService = new NoOpPostActionExecutorService(),
            };

            var capability = new PlatformCapabilityFeatureService(platform, diagnostics);
            var connect = new ConnectFeatureService(session, config);
            shellService ??= new ShellFeatureService(connect);

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
                ShellFeatureService = shellService,
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
                AppLifecycleService = appLifecycleService ?? new NoOpAppLifecycleService(),
            };

            return new TestFixture(root, runtime, new MainShellViewModel(runtime), tray);
        }

        public void Dispose()
        {
            DisposeAsync().AsTask().GetAwaiter().GetResult();
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
                // ignore cleanup failures in temporary test directories
            }
        }
    }

    private sealed class SpyAppLifecycleService : IAppLifecycleService
    {
        private readonly UiOperationResult _result;

        public SpyAppLifecycleService(UiOperationResult result)
        {
            _result = result;
        }

        public int RestartCallCount { get; private set; }

        public Task<UiOperationResult> RestartAsync(CancellationToken cancellationToken = default)
        {
            RestartCallCount++;
            return Task.FromResult(_result);
        }
    }

    private sealed class SpyShellFeatureService : IShellFeatureService
    {
        private readonly string _nextLanguage;

        public SpyShellFeatureService(string nextLanguage)
        {
            _nextLanguage = nextLanguage;
        }

        public int SwitchLanguageCallCount { get; private set; }

        public string LastCurrentLanguage { get; private set; } = string.Empty;

        public string? LastTargetLanguage { get; private set; }

        public Task<UiOperationResult> ConnectAsync(
            string address,
            string config,
            string? adbPath,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(UiOperationResult.Ok("Connected."));
        }

        public Task<UiOperationResult<ImportReport>> ImportLegacyConfigAsync(
            ImportSource source,
            bool manualImport,
            CancellationToken cancellationToken = default)
        {
            return Task.FromResult(UiOperationResult<ImportReport>.Ok(new ImportReport(), "Imported."));
        }

        public Task<UiOperationResult<string>> SwitchLanguageAsync(
            string currentLanguage,
            string? targetLanguage = null,
            CancellationToken cancellationToken = default)
        {
            SwitchLanguageCallCount++;
            LastCurrentLanguage = currentLanguage;
            LastTargetLanguage = targetLanguage;
            return Task.FromResult(UiOperationResult<string>.Ok(_nextLanguage, $"Language switched to {_nextLanguage}."));
        }

        public IReadOnlyList<string> GetSupportedLanguages()
        {
            return ["zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr", "pallas"];
        }
    }

    private sealed class CapturingTrayService : ITrayService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "tray test service",
            Provider: "test");

        public event EventHandler<TrayCommandEvent>? CommandInvoked;

        public int InitializeCallCount { get; private set; }

        public TrayMenuState? LastMenuState { get; private set; }

        public TrayMenuText? LastMenuText { get; private set; }

        public Task<PlatformOperationResult> InitializeAsync(
            string appTitle,
            TrayMenuText? menuText,
            CancellationToken cancellationToken = default)
        {
            InitializeCallCount++;
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
            LastMenuState = state;
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

    private sealed class UnknownErrorHotkeyService : IGlobalHotkeyService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "test hotkey service",
            Provider: "test-hotkey");

        public event EventHandler<GlobalHotkeyTriggeredEvent>? Triggered;

        public Task<PlatformOperationResult> RegisterAsync(
            string name,
            string gesture,
            CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "unknown hotkey registration failure",
                "HotkeyErrorNotMapped",
                "hotkey.register"));
        }

        public Task<PlatformOperationResult> UnregisterAsync(string name, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(PlatformOperation.NativeSuccess(
                Capability.Provider,
                "unregistered",
                "hotkey.unregister"));
        }
    }
}
