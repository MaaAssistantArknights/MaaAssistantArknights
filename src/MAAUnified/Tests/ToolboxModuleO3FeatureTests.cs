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

public sealed class ToolboxModuleO3FeatureTests
{
    [Fact]
    public async Task ExecuteToolAsync_MissingHandler_ShouldFailWithToolNotSupported()
    {
        var handlers = new Dictionary<ToolboxToolKind, Func<ToolboxExecuteRequest, CancellationToken, Task<UiOperationResult<string>>>>
        {
            [ToolboxToolKind.Recruit] = static (_, _) => Task.FromResult(UiOperationResult<string>.Ok("ok", "ok")),
        };
        var timeouts = Enum.GetValues<ToolboxToolKind>()
            .ToDictionary(tool => tool, _ => TimeSpan.FromSeconds(5));
        var service = new ToolboxFeatureService(handlers, timeouts);

        var result = await service.ExecuteToolAsync(new ToolboxExecuteRequest(ToolboxToolKind.Depot, "format=summary;topN=50"));

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.ToolNotSupported, result.Error?.Code);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_DisclaimerNotAccepted_ShouldWriteFailureLogWithContext()
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = false,
        };

        await vm.InitializeAsync();
        vm.ApplySuccessPresetForCurrentTool();
        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(UiErrorCode.ToolboxDisclaimerNotAccepted, vm.LastExecutionErrorCode);
        Assert.Equal(ToolboxExecutionState.Failed, vm.ExecutionState);
        var errorLog = await File.ReadAllTextAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath);
        Assert.Contains("Toolbox.Recruit", errorLog, StringComparison.Ordinal);
        Assert.Contains($"code={UiErrorCode.ToolboxDisclaimerNotAccepted}", errorLog, StringComparison.Ordinal);
        Assert.Contains("stage", errorLog, StringComparison.Ordinal);
        Assert.Contains("parameterSummary", errorLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_InvalidParameters_ShouldWriteFailureLogWithContext()
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        await vm.InitializeAsync();
        vm.ApplyFailurePresetForCurrentTool();
        await vm.ExecuteCurrentToolAsync();

        Assert.Equal(UiErrorCode.ToolboxInvalidParameters, vm.LastExecutionErrorCode);
        Assert.Equal(ToolboxExecutionState.Failed, vm.ExecutionState);
        var errorLog = await File.ReadAllTextAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath);
        Assert.Contains("Toolbox.Recruit", errorLog, StringComparison.Ordinal);
        Assert.Contains($"code={UiErrorCode.ToolboxInvalidParameters}", errorLog, StringComparison.Ordinal);
        Assert.Contains("stage", errorLog, StringComparison.Ordinal);
        Assert.Contains("validation", errorLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_ShouldPersistHistoryAndReloadInNewViewModel()
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var first = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        await first.InitializeAsync();
        first.ApplySuccessPresetForCurrentTool();
        await first.ExecuteCurrentToolAsync();

        var second = new ToolboxPageViewModel(fixture.Runtime);
        await second.InitializeAsync();

        var record = Assert.Single(second.ExecutionHistory);
        Assert.Equal("Recruit", record.ToolName);
        Assert.True(record.Success);
        Assert.Contains("level3Time=540", record.ParameterSummary, StringComparison.Ordinal);
        Assert.Contains("ok:Recruit", record.ResultSummary, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_ShouldTrimPersistedHistoryToMaxCount()
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        await vm.InitializeAsync();
        for (var i = 0; i < 35; i++)
        {
            vm.SelectedTabIndex = i % vm.Tabs.Count;
            vm.ApplySuccessPresetForCurrentTool();
            await vm.ExecuteCurrentToolAsync();
        }

        Assert.Equal(30, vm.ExecutionHistory.Count);

        var payload = ReadGlobalString(fixture.Config, "Toolbox.ExecutionHistory");
        var array = Assert.IsType<JsonArray>(JsonNode.Parse(payload));
        Assert.Equal(30, array.Count);
    }

    [Fact]
    public async Task ExecuteCurrentToolAsync_OperBoxAndDepot_ShouldPersistLegacyResultKeys()
    {
        var service = new CapturingToolboxFeatureService();
        await using var fixture = await RuntimeFixture.CreateAsync(service);
        var vm = new ToolboxPageViewModel(fixture.Runtime)
        {
            DisclaimerAccepted = true,
        };

        await vm.InitializeAsync();

        vm.SelectedTabIndex = 1;
        vm.ApplySuccessPresetForCurrentTool();
        await vm.ExecuteCurrentToolAsync();

        vm.SelectedTabIndex = 2;
        vm.ApplySuccessPresetForCurrentTool();
        await vm.ExecuteCurrentToolAsync();

        Assert.Equal("ok:OperBox", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.OperBoxData));
        Assert.Equal("ok:Depot", ReadGlobalString(fixture.Config, LegacyConfigurationKeys.DepotResult));
    }

    private static string ReadGlobalString(UnifiedConfigurationService config, string key)
    {
        if (config.CurrentConfig.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            return node.ToString();
        }

        return string.Empty;
    }

    private sealed class CapturingToolboxFeatureService : IToolboxFeatureService
    {
        private readonly Func<ToolboxExecuteRequest, UiOperationResult<ToolboxExecuteResult>> _handler;

        public CapturingToolboxFeatureService(Func<ToolboxExecuteRequest, UiOperationResult<ToolboxExecuteResult>>? handler = null)
        {
            _handler = handler ?? (request => UiOperationResult<ToolboxExecuteResult>.Ok(
                new ToolboxExecuteResult(
                    request.Tool,
                    $"ok:{request.Tool}",
                    request.ParameterText,
                    DateTimeOffset.Now),
                "ok"));
        }

        public Task<UiOperationResult<ToolboxExecuteResult>> ExecuteToolAsync(ToolboxExecuteRequest request, CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(_handler(request));
        }
    }

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private RuntimeFixture(string root, UnifiedConfigurationService config, MAAUnifiedRuntime runtime)
        {
            Root = root;
            Config = config;
            Runtime = runtime;
        }

        public string Root { get; }

        public UnifiedConfigurationService Config { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public static async Task<RuntimeFixture> CreateAsync(IToolboxFeatureService toolboxFeatureService)
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
                ToolboxFeatureService = toolboxFeatureService,
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(config, diagnostics, platform.PostActionExecutorService),
            };

            return new RuntimeFixture(root, config, runtime);
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
