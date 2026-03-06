using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Toolbox;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.Tests;

public sealed class ToolboxModuleO2FeatureTests
{
    [Fact]
    public async Task InitializeAsync_ShouldLoadToolboxBridgeSettings()
    {
        var seeds = new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [LegacyConfigurationKeys.ToolBoxChooseLevel3Time] = "510",
            [LegacyConfigurationKeys.ToolBoxChooseLevel4Time] = "520",
            [LegacyConfigurationKeys.ToolBoxChooseLevel5Time] = "530",
            [LegacyConfigurationKeys.GachaShowDisclaimerNoMore] = "true",
            [LegacyConfigurationKeys.PeepTargetFps] = "37",
            [LegacyConfigurationKeys.MiniGameTaskName] = "MiniGame@SecretFront",
            [LegacyConfigurationKeys.MiniGameSecretFrontEnding] = "D",
            [LegacyConfigurationKeys.MiniGameSecretFrontEvent] = "游侠",
        };

        await using var fixture = await RuntimeFixture.CreateAsync(new CapturingToolboxFeatureService(), seeds: seeds);
        var vm = new ToolboxPageViewModel(fixture.Runtime);

        await vm.InitializeAsync();

        Assert.Equal("510", vm.RecruitLevel3TimeInput);
        Assert.Equal("520", vm.RecruitLevel4TimeInput);
        Assert.Equal("530", vm.RecruitLevel5TimeInput);
        Assert.True(vm.GachaShowDisclaimerNoMore);
        Assert.Equal("37", vm.VideoRecognitionTargetFpsInput);
        Assert.Equal("MiniGame@SecretFront", vm.MiniGameTaskName);
        Assert.Equal("D", vm.MiniGameSecretFrontEnding);
        Assert.Equal("游侠", vm.MiniGameSecretFrontEvent);
    }

    [Theory]
    [InlineData(0, ToolboxToolKind.Recruit, "level3Time=")]
    [InlineData(1, ToolboxToolKind.OperBox, "mode=")]
    [InlineData(2, ToolboxToolKind.Depot, "topN=")]
    [InlineData(3, ToolboxToolKind.Gacha, "drawCount=")]
    [InlineData(4, ToolboxToolKind.VideoRecognition, "targetFps=")]
    [InlineData(5, ToolboxToolKind.MiniGame, "taskName=")]
    public async Task ExecuteCurrentToolAsync_SuccessPreset_ShouldSucceedAndBuildToolSpecificParameterText(
        int tabIndex,
        ToolboxToolKind expectedTool,
        string expectedToken)
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        await vm.InitializeAsync();
        vm.SelectedTabIndex = tabIndex;
        vm.ApplySuccessPresetForCurrentTool();

        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(ToolboxExecutionState.Succeeded, vm.ExecutionState);
        Assert.Equal(1, service.CallCount);
        var request = Assert.Single(service.Requests);
        Assert.Equal(expectedTool, request.Tool);
        Assert.Contains(expectedToken, request.ParameterText, StringComparison.Ordinal);
        Assert.Contains(expectedToken, vm.CurrentToolParameters, StringComparison.Ordinal);
    }

    [Theory]
    [InlineData(0)]
    [InlineData(1)]
    [InlineData(2)]
    [InlineData(3)]
    [InlineData(4)]
    [InlineData(5)]
    public async Task ExecuteCurrentToolAsync_FailurePreset_ShouldFailWithoutCallingService(int tabIndex)
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        await vm.InitializeAsync();
        vm.SelectedTabIndex = tabIndex;
        vm.ApplyFailurePresetForCurrentTool();

        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(0, service.CallCount);
        Assert.Equal(ToolboxExecutionState.Failed, vm.ExecutionState);
        Assert.Equal(UiErrorCode.ToolboxInvalidParameters, vm.LastExecutionErrorCode);
        Assert.Contains(UiErrorCode.ToolboxInvalidParameters, vm.ResultText, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_DisclaimerNotAccepted_AllTabs_ShouldUseUnifiedPrompt()
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = false,
        };

        await vm.InitializeAsync();

        var messages = new HashSet<string>(StringComparer.Ordinal);
        for (var tab = 0; tab < vm.Tabs.Count; tab++)
        {
            vm.SelectedTabIndex = tab;
            vm.ApplySuccessPresetForCurrentTool();
            await vm.ExecuteCurrentToolAsync();
            messages.Add(vm.ResultText);
            Assert.Equal(UiErrorCode.ToolboxDisclaimerNotAccepted, vm.LastExecutionErrorCode);
        }

        Assert.Equal(0, service.CallCount);
        var message = Assert.Single(messages);
        Assert.Equal($"请先确认免责声明。 ({UiErrorCode.ToolboxDisclaimerNotAccepted})", message);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_Recruit_ShouldPersistRecruitBridgeSettings()
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        await vm.InitializeAsync();
        vm.SelectedTabIndex = 0;
        vm.RecruitLevel3TimeInput = "500";
        vm.RecruitLevel4TimeInput = "510";
        vm.RecruitLevel5TimeInput = "520";

        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(ToolboxExecutionState.Succeeded, vm.ExecutionState);
        Assert.Equal("500", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.ToolBoxChooseLevel3Time));
        Assert.Equal("510", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.ToolBoxChooseLevel4Time));
        Assert.Equal("520", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.ToolBoxChooseLevel5Time));
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_GachaPeepMiniGame_ShouldPersistBridgeSettings()
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        await vm.InitializeAsync();

        vm.SelectedTabIndex = 3;
        vm.GachaDrawCountInput = "10";
        vm.GachaShowDisclaimerNoMore = true;
        await vm.ExecuteCurrentToolAsync();

        vm.SelectedTabIndex = 4;
        vm.VideoRecognitionTargetFpsInput = "45";
        await vm.ExecuteCurrentToolAsync();

        vm.SelectedTabIndex = 5;
        vm.MiniGameTaskName = "MiniGame@SecretFront";
        vm.MiniGameSecretFrontEnding = "C";
        vm.MiniGameSecretFrontEvent = "支援作战平台";
        await vm.ExecuteCurrentToolAsync();

        Assert.Equal("True", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.GachaShowDisclaimerNoMore));
        Assert.Equal("45", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.PeepTargetFps));
        Assert.Equal("MiniGame@SecretFront", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.MiniGameTaskName));
        Assert.Equal("C", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.MiniGameSecretFrontEnding));
        Assert.Equal("支援作战平台", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.MiniGameSecretFrontEvent));
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_BridgeSaveFailure_ShouldContinueExecutionAndLogFailure()
    {
        var service = new CapturingToolboxFeatureService();
        await using var baseFixture = await RuntimeFixture.CreateAsync(service);
        var failingSettings = new FailingSettingsFeatureService(baseFixture.SettingsFeatureService);

        await using var fixture = await RuntimeFixture.CreateAsync(
            service,
            settingsFeatureService: failingSettings,
            root: baseFixture.Root + "-failing");

        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
            SelectedTabIndex = 0,
            RecruitLevel3TimeInput = "500",
            RecruitLevel4TimeInput = "510",
            RecruitLevel5TimeInput = "520",
        };

        await vm.InitializeAsync();
        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(ToolboxExecutionState.Succeeded, vm.ExecutionState);
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath, "Toolbox.ConfigBridge"));
    }

    [Fact]
    public void ToolboxView_ShouldContainParameterizedBindingsAndPresetButtons()
    {
        var root = GetMaaUnifiedRoot();
        var path = Path.Combine(root, "App", "Features", "Advanced", "ToolboxView.axaml");
        var text = File.ReadAllText(path);

        Assert.Contains("RecruitLevel3TimeInput", text, StringComparison.Ordinal);
        Assert.Contains("OperBoxMode", text, StringComparison.Ordinal);
        Assert.Contains("DepotTopNInput", text, StringComparison.Ordinal);
        Assert.Contains("GachaDrawCountInput", text, StringComparison.Ordinal);
        Assert.Contains("VideoRecognitionTargetFpsInput", text, StringComparison.Ordinal);
        Assert.Contains("MiniGameTaskName", text, StringComparison.Ordinal);
        Assert.Contains("执行成功示例", text, StringComparison.Ordinal);
        Assert.Contains("执行失败示例", text, StringComparison.Ordinal);
    }

    private static async Task<bool> WaitForLogContainsAsync(string path, string expected, int retry = 30, int delayMs = 25)
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

    private static string ReadGlobalString(UnifiedConfigurationService config, string key)
    {
        if (config.CurrentConfig.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            return node.ToString();
        }

        return string.Empty;
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

    private sealed class CapturingToolboxFeatureService : IToolboxFeatureService
    {
        private readonly Func<ToolboxExecuteRequest, UiOperationResult<ToolboxExecuteResult>> _handler;

        public CapturingToolboxFeatureService(Func<ToolboxExecuteRequest, UiOperationResult<ToolboxExecuteResult>>? handler = null)
        {
            _handler = handler ?? (request => UiOperationResult<ToolboxExecuteResult>.Ok(
                new ToolboxExecuteResult(request.Tool, $"ok:{request.Tool}", request.ParameterText, DateTimeOffset.Now),
                "ok"));
        }

        public int CallCount => Requests.Count;

        public List<ToolboxExecuteRequest> Requests { get; } = [];

        public Task<UiOperationResult<ToolboxExecuteResult>> ExecuteToolAsync(ToolboxExecuteRequest request, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            Requests.Add(request);
            return Task.FromResult(_handler(request));
        }
    }

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private RuntimeFixture(string root, UnifiedConfigurationService config, MAAUnifiedRuntime runtime, ISettingsFeatureService settingsFeatureService)
        {
            Root = root;
            Config = config;
            Runtime = runtime;
            SettingsFeatureService = settingsFeatureService;
        }

        public string Root { get; }

        public UnifiedConfigurationService Config { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public ISettingsFeatureService SettingsFeatureService { get; }

        public static async Task<RuntimeFixture> CreateAsync(
            IToolboxFeatureService toolboxFeatureService,
            IReadOnlyDictionary<string, string>? seeds = null,
            ISettingsFeatureService? settingsFeatureService = null,
            string? root = null)
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

            if (seeds is not null)
            {
                foreach (var (key, value) in seeds)
                {
                    config.CurrentConfig.GlobalValues[key] = JsonValue.Create(value);
                }
            }

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
            var resolvedSettings = settingsFeatureService ?? new SettingsFeatureService(config, capability, diagnostics);

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
                SettingsFeatureService = resolvedSettings,
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(config, diagnostics, platform.PostActionExecutorService),
            };

            return new RuntimeFixture(root, config, runtime, resolvedSettings);
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

    private sealed class FailingSettingsFeatureService : ISettingsFeatureService
    {
        private readonly ISettingsFeatureService _inner;

        public FailingSettingsFeatureService(ISettingsFeatureService inner)
        {
            _inner = inner;
        }

        public Task<UiOperationResult> SaveGlobalSettingAsync(string key, string value, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.SettingsSaveFailed, "forced save failure"));
        }

        public Task<UiOperationResult> SaveGlobalSettingsAsync(IReadOnlyDictionary<string, string> updates, CancellationToken cancellationToken = default)
        {
            return Task.FromResult(UiOperationResult.Fail(UiErrorCode.SettingsSaveFailed, "forced save failure"));
        }

        public Task<UiOperationResult> TestNotificationAsync(string title, string message, CancellationToken cancellationToken = default)
            => _inner.TestNotificationAsync(title, message, cancellationToken);

        public Task<UiOperationResult> RegisterHotkeyAsync(string name, string gesture, CancellationToken cancellationToken = default)
            => _inner.RegisterHotkeyAsync(name, gesture, cancellationToken);

        public Task<UiOperationResult<bool>> GetAutostartStatusAsync(CancellationToken cancellationToken = default)
            => _inner.GetAutostartStatusAsync(cancellationToken);

        public Task<UiOperationResult> SetAutostartAsync(bool enabled, CancellationToken cancellationToken = default)
            => _inner.SetAutostartAsync(enabled, cancellationToken);

        public Task<UiOperationResult<string>> BuildIssueReportAsync(CancellationToken cancellationToken = default)
            => _inner.BuildIssueReportAsync(cancellationToken);
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
