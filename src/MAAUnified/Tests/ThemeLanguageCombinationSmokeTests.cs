using System.Reflection;
using System.Text.RegularExpressions;
using MAAUnified.App.ViewModels;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Application.Services.Localization;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class ThemeLanguageCombinationSmokeTests
{
    private static readonly string[] Themes = ["Light", "Dark"];

    private static readonly string[] Languages = ["zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr", "pallas"];

    [Fact]
    public void TargetViews_ShouldContainCoreStructures()
    {
        var root = GetMaaUnifiedRoot();
        var checks = new[]
        {
            new ViewStructureCheck("App/Views/MainWindow.axaml", "<TabControl", "<ListBox"),
            new ViewStructureCheck("App/Features/Root/TaskQueueView.axaml", "<ListBox", "<TabControl"),
            new ViewStructureCheck("App/Features/Root/SettingsView.axaml", "<ListBox", "<Expander"),
        };

        foreach (var check in checks)
        {
            var path = Path.Combine(root, check.RelativePath.Replace('/', Path.DirectorySeparatorChar));
            var xaml = File.ReadAllText(path);
            foreach (var required in check.RequiredElements)
            {
                Assert.Contains(required, xaml, StringComparison.Ordinal);
            }
        }
    }

    [Fact]
    public void TargetViews_LongTextBindings_ShouldUseWrappingOrTrimming()
    {
        var root = GetMaaUnifiedRoot();
        var controlStyles = File.ReadAllText(Path.Combine(root, "App", "Styles", "ControlStyles.axaml"));
        Assert.Contains("Style Selector=\"TextBlock.long-wrap\"", controlStyles, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"TextBlock.long-ellipsis\"", controlStyles, StringComparison.Ordinal);

        var mainWindow = File.ReadAllText(Path.Combine(root, "App", "Views", "MainWindow.axaml"));
        AssertTextBlockBindingHasLongTextConstraint(mainWindow, "GlobalStatus");
        AssertTextBlockBindingHasLongTextConstraint(mainWindow, "LastError");
        AssertTextBlockBindingHasLongTextConstraint(mainWindow, "CapabilitySummary");
        AssertTextBlockBindingHasLongTextConstraint(mainWindow, "ImportStatus");

        var taskQueue = File.ReadAllText(Path.Combine(root, "App", "Features", "Root", "TaskQueueView.axaml"));
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "DailyStageHint");
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "SelectedTaskValidationSummary");
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "OverlayStatusText");
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "StatusMessage");
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "LastErrorMessage");
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "Name");
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "Type");
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "Status");
        AssertTextBlockBindingHasLongTextConstraint(taskQueue, "DisplayName");

        var settings = File.ReadAllText(Path.Combine(root, "App", "Features", "Root", "SettingsView.axaml"));
        AssertTextBlockBindingHasLongTextConstraint(settings, "DisplayName");
        AssertTextBlockBindingHasLongTextConstraint(settings, "StatusMessage");
    }

    [Fact]
    public async Task ThemeLanguageMatrix_RuntimeSmoke_ShouldKeepLanguageThemeAndTrayTextsAligned()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        var vm = new MainShellViewModel(fixture.Runtime);
        await vm.InitializeAsync();

        var applyMethod = typeof(MainShellViewModel).GetMethod(
            "ApplyGuiSettingsAsync",
            BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(applyMethod);

        foreach (var theme in Themes)
        {
            foreach (var language in Languages)
            {
                var normalizedLanguage = UiLanguageCatalog.Normalize(language);

                vm.SettingsPage.Theme = theme;
                await vm.SwitchLanguageToAsync(language);

                var snapshot = vm.SettingsPage.CurrentGuiSnapshot with
                {
                    Theme = theme,
                    Language = normalizedLanguage,
                };

                var applyTask = applyMethod!.Invoke(vm, [snapshot, CancellationToken.None]) as Task;
                Assert.NotNull(applyTask);
                await applyTask!;

                Assert.Equal(theme, vm.SettingsPage.Theme);
                Assert.Equal(normalizedLanguage, vm.SettingsPage.Language);
                Assert.Equal(theme, vm.AppliedTheme);
                Assert.Equal(normalizedLanguage, vm.TaskQueuePage.Texts.Language);

                Assert.NotNull(fixture.TrayService.LastMenuText);
                Assert.False(string.IsNullOrWhiteSpace(fixture.TrayService.LastMenuText!.Start));
                Assert.False(string.IsNullOrWhiteSpace(fixture.TrayService.LastMenuText.SwitchLanguage));

                Assert.False(string.IsNullOrWhiteSpace(vm.CapabilitySummary));
                AssertLocalizationResolved(vm.CapabilitySummary);
            }
        }
    }

    private static void AssertTextBlockBindingHasLongTextConstraint(string xaml, string bindingPath)
    {
        var bindingLiteral = Regex.Escape($@"Text=""{{Binding {bindingPath}}}""");
        var pattern = $@"<TextBlock\b(?=[^>]*{bindingLiteral})(?=[^>]*(?:TextWrapping=""[^""]+""|TextTrimming=""[^""]+""|Classes=""[^""]*\blong-(?:wrap|ellipsis)\b[^""]*""))[^>]*>";
        Assert.Matches(new Regex(pattern, RegexOptions.Singleline), xaml);
    }

    private static void AssertLocalizationResolved(string value)
    {
        var unresolvedKeyPattern = new Regex(@"\b(?:CapabilityName|Status|TrayMenu|Error|Ui)\.[A-Za-z0-9_.-]+\b", RegexOptions.Compiled);
        Assert.DoesNotMatch(unresolvedKeyPattern, value);
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

    private sealed record ViewStructureCheck(string RelativePath, params string[] RequiredElements);

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private readonly bool _cleanupRoot;

        private RuntimeFixture(
            string root,
            MAAUnifiedRuntime runtime,
            CapturingTrayService trayService,
            bool cleanupRoot)
        {
            Root = root;
            Runtime = runtime;
            TrayService = trayService;
            _cleanupRoot = cleanupRoot;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public CapturingTrayService TrayService { get; }

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

            var bridge = new MaaCoreBridgeStub();
            var session = new UnifiedSessionService(bridge, config, log, new SessionStateMachine());
            var trayService = new CapturingTrayService();

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

            return new RuntimeFixture(root, runtime, trayService, cleanupRoot);
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
}
