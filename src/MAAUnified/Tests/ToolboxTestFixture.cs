using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

internal sealed class ToolboxTestFixture : IAsyncDisposable
{
    private ToolboxTestFixture(
        string root,
        UnifiedConfigurationService config,
        RecordingBridge bridge,
        MAAUnifiedRuntime runtime,
        ConnectionGameSharedStateViewModel connectionState)
    {
        Root = root;
        Config = config;
        Bridge = bridge;
        Runtime = runtime;
        ConnectionState = connectionState;
    }

    public string Root { get; }

    public UnifiedConfigurationService Config { get; }

    public RecordingBridge Bridge { get; }

    public MAAUnifiedRuntime Runtime { get; }

    public ConnectionGameSharedStateViewModel ConnectionState { get; }

    public static async Task<ToolboxTestFixture> CreateAsync(
        IReadOnlyDictionary<string, string>? globalSeeds = null,
        IReadOnlyDictionary<string, JsonNode?>? profileSeeds = null)
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-toolbox-tests", Guid.NewGuid().ToString("N"));
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

        if (globalSeeds is not null)
        {
            foreach (var pair in globalSeeds)
            {
                config.CurrentConfig.GlobalValues[pair.Key] = JsonValue.Create(pair.Value);
            }
        }

        if (config.TryGetCurrentProfile(out var profile))
        {
            profile.Values["ClientType"] = JsonValue.Create("Official");
            profile.Values["ServerType"] = JsonValue.Create("CN");
            profile.Values["ConnectAddress"] = JsonValue.Create("127.0.0.1:5555");
            profile.Values["ConnectConfig"] = JsonValue.Create("General");
            profile.Values["AutoDetect"] = JsonValue.Create(true);
            if (profileSeeds is not null)
            {
                foreach (var pair in profileSeeds)
                {
                    profile.Values[pair.Key] = pair.Value?.DeepClone();
                }
            }
        }

        var bridge = new RecordingBridge();
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
            ToolboxFeatureService = new ToolboxFeatureService(bridge, connect),
            RemoteControlFeatureService = new RemoteControlFeatureService(),
            PlatformCapabilityService = capability,
            OverlayFeatureService = new OverlayFeatureService(capability),
            NotificationProviderFeatureService = new NotificationProviderFeatureService(),
            SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
            DialogFeatureService = new DialogFeatureService(diagnostics),
            PostActionFeatureService = new PostActionFeatureService(config, diagnostics, platform.PostActionExecutorService),
        };

        var connectionState = new ConnectionGameSharedStateViewModel
        {
            ConnectAddress = "127.0.0.1:5555",
            ConnectConfig = "General",
            ClientType = "Official",
            AutoDetect = true,
        };

        return new ToolboxTestFixture(root, config, bridge, runtime, connectionState);
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

    internal sealed class RecordingBridge : IMaaCoreBridge
    {
        private static readonly byte[] TinyPng =
            Convert.FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO7+W0cAAAAASUVORK5CYII=");

        private readonly Channel<CoreCallbackEvent> _callbacks = Channel.CreateUnbounded<CoreCallbackEvent>();
        private int _nextTaskId = 1;

        public List<CoreTaskRequest> AppendedTasks { get; } = [];

        public int ConnectCallCount { get; private set; }

        public int StartCallCount { get; private set; }

        public int StopCallCount { get; private set; }

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
        {
            ConnectCallCount++;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
        {
            AppendedTasks.Add(task);
            return Task.FromResult(CoreResult<int>.Ok(_nextTaskId++));
        }

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            StartCallCount++;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            StopCallCount++;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Ok(TinyPng));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await foreach (var callback in _callbacks.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public void EnqueueCallback(CoreCallbackEvent callback)
        {
            _callbacks.Writer.TryWrite(callback);
        }

        public ValueTask DisposeAsync()
        {
            _callbacks.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }
    }
}
