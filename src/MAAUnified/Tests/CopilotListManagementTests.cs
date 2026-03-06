using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Copilot;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;

namespace MAAUnified.Tests;

public sealed class CopilotListManagementTests
{
    [Fact]
    public async Task AddEmptyTaskAsync_ShouldPersistListAndUpdateFeedback()
    {
        await using var fixture = await CopilotFixture.CreateAsync();
        var vm = fixture.ViewModel;

        await vm.AddEmptyTaskAsync();

        Assert.Single(vm.Items);
        Assert.NotNull(vm.SelectedItem);
        Assert.Contains("新增", vm.StatusMessage, StringComparison.Ordinal);
        Assert.True(string.IsNullOrWhiteSpace(vm.LastErrorMessage));

        var payload = GetPersistedTaskListPayload(fixture.Config);
        Assert.NotNull(payload);
        var node = JsonNode.Parse(payload!);
        var array = Assert.IsType<JsonArray>(node);
        Assert.Single(array);
    }

    [Fact]
    public async Task RemoveSelectedAsync_WithoutSelection_ShouldSetFailureFeedbackAndLog()
    {
        await using var fixture = await CopilotFixture.CreateAsync();
        var vm = fixture.ViewModel;

        await vm.RemoveSelectedAsync();

        Assert.Contains("删除作业失败", vm.StatusMessage, StringComparison.Ordinal);
        Assert.Contains("请选择", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath, "[Copilot.Remove]"));
    }

    [Fact]
    public async Task MoveSelectedUpAndDown_ShouldReorderItemsAndPersist()
    {
        await using var fixture = await CopilotFixture.CreateAsync();
        var vm = fixture.ViewModel;

        await vm.AddEmptyTaskAsync();
        await vm.AddEmptyTaskAsync();
        await vm.AddEmptyTaskAsync();
        vm.Items[0].Name = "First";
        vm.Items[1].Name = "Second";
        vm.Items[2].Name = "Third";

        vm.SelectedItem = vm.Items[1];
        await vm.MoveSelectedUpAsync();

        Assert.Equal(["Second", "First", "Third"], vm.Items.Select(i => i.Name).ToArray());
        Assert.Equal("Second", vm.SelectedItem?.Name);

        await vm.MoveSelectedDownAsync();

        Assert.Equal(["First", "Second", "Third"], vm.Items.Select(i => i.Name).ToArray());
        Assert.Equal("Second", vm.SelectedItem?.Name);

        var payload = GetPersistedTaskListPayload(fixture.Config);
        Assert.NotNull(payload);
        var persistedArray = Assert.IsType<JsonArray>(JsonNode.Parse(payload!));
        Assert.Equal("First", persistedArray[0]?["Name"]?.GetValue<string>());
        Assert.Equal("Second", persistedArray[1]?["Name"]?.GetValue<string>());
        Assert.Equal("Third", persistedArray[2]?["Name"]?.GetValue<string>());
    }

    [Fact]
    public async Task SendLikeAsync_WithoutSelection_ShouldSetFailureFeedbackAndLog()
    {
        await using var fixture = await CopilotFixture.CreateAsync();
        var vm = fixture.ViewModel;

        await vm.SendLikeAsync(true);

        Assert.Contains("反馈失败", vm.StatusMessage, StringComparison.Ordinal);
        Assert.Contains("请选择", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath, "[Copilot.Feedback]"));
    }

    [Fact]
    public async Task AddEmptyTaskAsync_WhenPersistenceFails_ShouldRollbackListState()
    {
        await using var fixture = await CopilotFixture.CreateAsync(failPersistence: true);
        var vm = fixture.ViewModel;

        await vm.AddEmptyTaskAsync();

        Assert.Empty(vm.Items);
        Assert.Contains("失败", vm.StatusMessage, StringComparison.Ordinal);
        Assert.Contains("持久化", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath, "[Copilot.Add]"));
    }

    [Fact]
    public async Task Constructor_ShouldLoadPersistedTaskList_AndMapLegacyTabIndex()
    {
        var payload = new JsonArray
        {
            new JsonObject
            {
                ["name"] = "Legacy-SSS",
                ["tab_index"] = 1,
            },
            new JsonObject
            {
                ["name"] = "Typed-Main",
                ["type"] = "主线",
            },
        }.ToJsonString();

        await using var fixture = await CopilotFixture.CreateAsync(persistedPayload: payload);
        var vm = fixture.ViewModel;

        Assert.Equal(2, vm.Items.Count);
        Assert.Equal("Legacy-SSS", vm.Items[0].Name);
        Assert.Equal("SSS", vm.Items[0].Type);
        Assert.Equal("Typed-Main", vm.Items[1].Name);
        Assert.Equal("主线", vm.Items[1].Type);
        Assert.Equal("Legacy-SSS", vm.SelectedItem?.Name);
    }

    private static string? GetPersistedTaskListPayload(UnifiedConfigurationService config)
    {
        if (!config.CurrentConfig.GlobalValues.TryGetValue(LegacyConfigurationKeys.CopilotTaskList, out var node)
            || node is not JsonValue value
            || !value.TryGetValue(out string? payload))
        {
            return null;
        }

        return payload;
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

    private sealed class CopilotFixture : IAsyncDisposable
    {
        private CopilotFixture(
            string root,
            UnifiedConfigurationService config,
            MAAUnifiedRuntime runtime,
            CopilotPageViewModel viewModel)
        {
            Root = root;
            Config = config;
            Runtime = runtime;
            ViewModel = viewModel;
        }

        public string Root { get; }

        public UnifiedConfigurationService Config { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public CopilotPageViewModel ViewModel { get; }

        public static async Task<CopilotFixture> CreateAsync(
            bool failPersistence = false,
            string? persistedPayload = null)
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

            if (!string.IsNullOrWhiteSpace(persistedPayload))
            {
                config.CurrentConfig.GlobalValues[LegacyConfigurationKeys.CopilotTaskList] = JsonValue.Create(persistedPayload);
            }

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
            var settings = new SettingsFeatureService(config, capability, diagnostics);
            ISettingsFeatureService settingsFeature = failPersistence
                ? new FailingSettingsFeatureService(settings)
                : settings;

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
                SettingsFeatureService = settingsFeature,
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
            };

            var vm = new CopilotPageViewModel(runtime);
            return new CopilotFixture(root, config, runtime, vm);
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

    private sealed class FailingSettingsFeatureService : ISettingsFeatureService
    {
        private readonly ISettingsFeatureService _inner;

        public FailingSettingsFeatureService(ISettingsFeatureService inner)
        {
            _inner = inner;
        }

        public Task<UiOperationResult> SaveGlobalSettingAsync(string key, string value, CancellationToken cancellationToken = default)
            => Task.FromResult(UiOperationResult.Fail(
                UiErrorCode.CopilotListPersistenceFailed,
                "Copilot 列表持久化失败（模拟）。"));

        public Task<UiOperationResult> SaveGlobalSettingsAsync(
            IReadOnlyDictionary<string, string> updates,
            CancellationToken cancellationToken = default)
            => Task.FromResult(UiOperationResult.Fail(
                UiErrorCode.CopilotListPersistenceFailed,
                "Copilot 列表持久化失败（模拟）。"));

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

    private sealed class FakeBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _channel = Channel.CreateUnbounded<CoreCallbackEvent>();

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
            await foreach (var callback in _channel.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public ValueTask DisposeAsync()
        {
            _channel.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }
    }
}
