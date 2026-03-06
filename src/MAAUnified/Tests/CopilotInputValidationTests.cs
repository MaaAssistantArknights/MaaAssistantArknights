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

namespace MAAUnified.Tests;

public sealed class CopilotInputValidationTests
{
    [Fact]
    public async Task ImportFromFileAsync_EmptyPath_ReturnsActionableFailure()
    {
        var service = new CopilotFeatureService();

        var result = await service.ImportFromFileAsync(string.Empty);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.CopilotFileMissing, result.Error?.Code);
        Assert.Contains("路径", result.Message, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ImportFromFileAsync_FileNotFound_ReturnsActionableFailure()
    {
        var service = new CopilotFeatureService();
        var missing = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"), "missing.json");

        var result = await service.ImportFromFileAsync(missing);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.CopilotFileNotFound, result.Error?.Code);
        Assert.Contains("不存在", result.Message, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ImportFromFileAsync_InvalidJson_ReturnsFormatFailure()
    {
        var service = new CopilotFeatureService();
        var file = CreateTempFile("{invalid json");

        var result = await service.ImportFromFileAsync(file);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.CopilotPayloadInvalidJson, result.Error?.Code);
        Assert.Contains("JSON", result.Message, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task ImportFromFileAsync_ArrayItemMissingField_ReturnsIndexedFailure()
    {
        var service = new CopilotFeatureService();
        var payload = new JsonArray
        {
            new JsonObject
            {
                ["stage_name"] = "1-7",
                ["minimum_required"] = "v4.0",
                ["actions"] = new JsonArray(new JsonObject()),
            },
            new JsonObject
            {
                ["stage_name"] = "2-3",
                ["actions"] = new JsonArray(new JsonObject()),
            },
        };
        var file = CreateTempFile(payload.ToJsonString());

        var result = await service.ImportFromFileAsync(file);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.CopilotPayloadMissingFields, result.Error?.Code);
        Assert.Contains("第2个作业", result.Message, StringComparison.Ordinal);
        Assert.Contains("minimum_required", result.Message, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ImportFromFileAsync_ValidRegularPayload_Succeeds()
    {
        var service = new CopilotFeatureService();
        var payload = new JsonObject
        {
            ["stage_name"] = "1-7",
            ["minimum_required"] = "v4.0",
            ["actions"] = new JsonArray(new JsonObject { ["type"] = "Deploy" }),
        };
        var file = CreateTempFile(payload.ToJsonString());

        var result = await service.ImportFromFileAsync(file);

        Assert.True(result.Success);
        Assert.Contains("已导入", result.Message, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ImportFromFileAsync_ValidSssPayloadWithoutActions_Succeeds()
    {
        var service = new CopilotFeatureService();
        var payload = new JsonObject
        {
            ["type"] = "SSS",
            ["stage_name"] = "多索雷斯在建地块",
            ["minimum_required"] = "v4.9.0",
        };
        var file = CreateTempFile(payload.ToJsonString());

        var result = await service.ImportFromFileAsync(file);

        Assert.True(result.Success);
    }

    [Fact]
    public async Task ImportFromClipboardAsync_EmptyPayload_ReturnsActionableFailure()
    {
        var service = new CopilotFeatureService();

        var result = await service.ImportFromClipboardAsync("   ");

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.CopilotClipboardEmpty, result.Error?.Code);
    }

    [Fact]
    public async Task ImportFromClipboardAsync_PathPayload_UsesFileValidation()
    {
        var service = new CopilotFeatureService();
        var payload = new JsonObject
        {
            ["stage_name"] = "1-7",
            ["minimum_required"] = "v4.0",
            ["actions"] = new JsonArray(new JsonObject()),
        };
        var file = CreateTempFile(payload.ToJsonString());

        var result = await service.ImportFromClipboardAsync(file);

        Assert.True(result.Success);
        Assert.Contains("已导入作业文件", result.Message, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ImportFromClipboardAsync_MissingPath_ReturnsNotFound()
    {
        var service = new CopilotFeatureService();
        var missing = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"), "missing.json");

        var result = await service.ImportFromClipboardAsync(missing);

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.CopilotFileNotFound, result.Error?.Code);
    }

    [Fact]
    public async Task CopilotPage_ImportFailure_DoesNotMutateList_AndWritesScopedFailureLog()
    {
        await using var fixture = await CopilotPageFixture.CreateAsync();
        var vm = fixture.ViewModel;
        await vm.AddEmptyTaskAsync();
        var selectedBefore = vm.SelectedItem;
        var countBefore = vm.Items.Count;
        vm.FilePath = Path.Combine(fixture.Root, "does-not-exist.json");

        await vm.ImportFromFileAsync();

        Assert.Equal(countBefore, vm.Items.Count);
        Assert.Same(selectedBefore, vm.SelectedItem);
        Assert.Contains("不存在", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "[Copilot.ImportFile]",
            retry: 40));
    }

    [Fact]
    public async Task CopilotPage_ClipboardImportFailure_DoesNotMutateList_AndWritesScopedFailureLog()
    {
        await using var fixture = await CopilotPageFixture.CreateAsync();
        var vm = fixture.ViewModel;
        await vm.AddEmptyTaskAsync();
        var selectedBefore = vm.SelectedItem;
        var countBefore = vm.Items.Count;

        await vm.ImportFromClipboardAsync("{\"stage_name\":\"1-7\"}");

        Assert.Equal(countBefore, vm.Items.Count);
        Assert.Same(selectedBefore, vm.SelectedItem);
        Assert.Contains("minimum_required", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.True(await WaitForLogContainsAsync(
            fixture.Runtime.DiagnosticsService.ErrorLogPath,
            "[Copilot.ImportClipboard]",
            retry: 40));
    }

    private static string CreateTempFile(string contents)
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        var file = Path.Combine(root, "copilot.json");
        File.WriteAllText(file, contents);
        return file;
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

    private sealed class CopilotPageFixture : IAsyncDisposable
    {
        private CopilotPageFixture(string root, MAAUnifiedRuntime runtime)
        {
            Root = root;
            Runtime = runtime;
            ViewModel = new CopilotPageViewModel(runtime);
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public CopilotPageViewModel ViewModel { get; }

        public static async Task<CopilotPageFixture> CreateAsync()
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

            var bridge = new TestBridge();
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

            return new CopilotPageFixture(root, runtime);
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

    private sealed class TestBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _channel = Channel.CreateUnbounded<CoreCallbackEvent>();

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
            CoreInitializeRequest request,
            CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(
                new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(
            CoreConnectionInfo connectionInfo,
            CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<int>> AppendTaskAsync(
            CoreTaskRequest task,
            CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(1));

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));

        public Task<CoreResult<bool>> AttachWindowAsync(
            CoreAttachWindowRequest request,
            CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync(
            [EnumeratorCancellation] CancellationToken cancellationToken = default)
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
