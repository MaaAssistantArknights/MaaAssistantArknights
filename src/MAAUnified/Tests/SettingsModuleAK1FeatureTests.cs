using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using System.Threading.Channels;
using MAAUnified.App.ViewModels;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SettingsModuleAK1FeatureTests
{
    private static readonly string[] _connectionKeys =
    [
        "ConnectAddress",
        "ConnectConfig",
        "AdbPath",
        "ClientType",
        "StartGame",
        "TouchMode",
        "AutoDetect",
    ];

    [Fact]
    public async Task SaveConnectionGameSettings_AndMainShellSyncConnectionToProfile_WriteSameFieldSet()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var shell = fixture.Shell;
        var state = shell.ConnectionGameSharedState;
        state.ConnectAddress = " 10.10.10.10:16384 ";
        state.ConnectConfig = " Mumu ";
        state.AdbPath = " /opt/adb ";
        state.ClientType = " YoStarEN ";
        state.StartGameEnabled = true;
        state.TouchMode = " maatouch ";
        state.AutoDetect = false;

        await shell.SettingsPage.SaveConnectionGameSettingsAsync();

        var profile = fixture.GetCurrentProfile();
        var expected = SnapshotConnectionValues(profile);

        ClearConnectionValues(profile);
        InvokePrivateMethod(shell, "SyncConnectionToProfile");
        var actual = SnapshotConnectionValues(profile);

        Assert.Equal(expected, actual);
    }

    [Fact]
    public async Task SaveConnectionGameSettings_ReflectsInBoundStartUpModuleImmediately_AndAfterRebind()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup")).Success);

        await fixture.Shell.TaskQueuePage.ReloadTasksAsync();
        await WaitUntilAsync(() => fixture.Shell.TaskQueuePage.StartUpModule.IsTaskBound);

        var state = fixture.Shell.SettingsPage.ConnectionGameSharedState;
        state.ConnectAddress = "192.168.0.9:5555";
        state.ConnectConfig = "LDPlayer";
        state.AdbPath = "/usr/bin/adb";
        state.ClientType = "YoStarJP";
        state.StartGameEnabled = true;
        state.TouchMode = "adb";
        state.AutoDetect = false;

        await fixture.Shell.SettingsPage.SaveConnectionGameSettingsAsync();
        AssertStartUpModuleConnectionValues(
            fixture.Shell,
            connectAddress: "192.168.0.9:5555",
            connectConfig: "LDPlayer",
            adbPath: "/usr/bin/adb",
            clientType: "YoStarJP",
            startGame: true,
            touchMode: "adb",
            autoDetect: false);

        await fixture.Shell.TaskQueuePage.ReloadTasksAsync();
        await WaitUntilAsync(() => fixture.Shell.TaskQueuePage.StartUpModule.IsTaskBound);
        AssertStartUpModuleConnectionValues(
            fixture.Shell,
            connectAddress: "192.168.0.9:5555",
            connectConfig: "LDPlayer",
            adbPath: "/usr/bin/adb",
            clientType: "YoStarJP",
            startGame: true,
            touchMode: "adb",
            autoDetect: false);
    }

    [Fact]
    public async Task StartUpBinding_DoesNotBackfillSharedStateFromStaleTaskParams()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup")).Success);

        var profile = fixture.GetCurrentProfile();
        profile.Values["ConnectAddress"] = JsonValue.Create("172.16.0.2:7000");
        profile.Values["ConnectConfig"] = JsonValue.Create("Mumu");
        profile.Values["AdbPath"] = JsonValue.Create("/profile/adb");
        profile.Values["ClientType"] = JsonValue.Create("YoStarEN");
        profile.Values["StartGame"] = JsonValue.Create(true);
        profile.Values["TouchMode"] = JsonValue.Create("maatouch");
        profile.Values["AutoDetect"] = JsonValue.Create(true);

        var staleParams = new JsonObject
        {
            ["client_type"] = "Txwy",
            ["start_game_enabled"] = false,
            ["account_name"] = "stale-account",
        };
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, staleParams, persistImmediately: false)).Success);

        InvokePrivateMethod(fixture.Shell, "SyncConnectionFromProfile");
        await fixture.Shell.TaskQueuePage.ReloadTasksAsync();
        await WaitUntilAsync(() => fixture.Shell.TaskQueuePage.StartUpModule.IsTaskBound);

        AssertStartUpModuleConnectionValues(
            fixture.Shell,
            connectAddress: "172.16.0.2:7000",
            connectConfig: "Mumu",
            adbPath: "/profile/adb",
            clientType: "YoStarEN",
            startGame: true,
            touchMode: "maatouch",
            autoDetect: true);
        Assert.Equal("stale-account", fixture.Shell.TaskQueuePage.StartUpModule.AccountName);
    }

    [Fact]
    public async Task QueueEnabledTasks_UsesProfileClientTypeAndStartGame_WhenTaskParamsAreStale()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup")).Success);

        var profile = fixture.GetCurrentProfile();
        profile.Values["ClientType"] = JsonValue.Create("YoStarKR");
        profile.Values["StartGame"] = JsonValue.Create(true);
        profile.Values["ConnectConfig"] = JsonValue.Create("General");

        var staleParams = new JsonObject
        {
            ["client_type"] = "Txwy",
            ["start_game_enabled"] = false,
            ["account_name"] = "stale-account",
        };
        Assert.True((await fixture.TaskQueue.UpdateTaskParamsAsync(0, staleParams, persistImmediately: false)).Success);

        var queueResult = await fixture.TaskQueue.QueueEnabledTasksAsync();
        Assert.True(queueResult.Success);
        var appended = Assert.Single(fixture.Bridge.AppendedTasks);
        var json = Assert.IsType<JsonObject>(JsonNode.Parse(appended.ParamsJson));

        Assert.Equal("YoStarKR", json["client_type"]?.GetValue<string>());
        Assert.True(json["start_game_enabled"]?.GetValue<bool>());
    }

    [Fact]
    public async Task ConnectConfigPc_ForcesStartGameFalse_InSettingsAndStartUp()
    {
        await using var fixture = await TestFixture.CreateAsync();
        Assert.True((await fixture.TaskQueue.AddTaskAsync("StartUp", "startup")).Success);
        await fixture.Shell.TaskQueuePage.ReloadTasksAsync();
        await WaitUntilAsync(() => fixture.Shell.TaskQueuePage.StartUpModule.IsTaskBound);

        var state = fixture.Shell.SettingsPage.ConnectionGameSharedState;
        state.ConnectConfig = "PC";
        state.StartGameEnabled = true;
        await fixture.Shell.SettingsPage.SaveConnectionGameSettingsAsync();

        Assert.False(state.CanStartGameEnabled);
        Assert.False(state.StartGameEnabled);
        Assert.False(fixture.Shell.TaskQueuePage.StartUpModule.CanEditStartGameEnabled);
        Assert.False(fixture.Shell.TaskQueuePage.StartUpModule.StartGameEnabled);
        Assert.False(fixture.GetCurrentProfile().Values["StartGame"]?.GetValue<bool>());
    }

    private static void AssertStartUpModuleConnectionValues(
        MainShellViewModel shell,
        string connectAddress,
        string connectConfig,
        string adbPath,
        string clientType,
        bool startGame,
        string touchMode,
        bool autoDetect)
    {
        var module = shell.TaskQueuePage.StartUpModule;
        Assert.Equal(connectAddress, module.ConnectAddress);
        Assert.Equal(connectConfig, module.ConnectConfig);
        Assert.Equal(adbPath, module.AdbPath);
        Assert.Equal(clientType, module.ClientType);
        Assert.Equal(startGame, module.StartGameEnabled);
        Assert.Equal(touchMode, module.TouchMode);
        Assert.Equal(autoDetect, module.AutoDetectConnection);
    }

    private static Dictionary<string, string> SnapshotConnectionValues(UnifiedProfile profile)
    {
        var result = new Dictionary<string, string>(StringComparer.Ordinal);
        foreach (var key in _connectionKeys)
        {
            result[key] = profile.Values.TryGetValue(key, out var node) && node is not null
                ? node.ToJsonString()
                : "<missing>";
        }

        return result;
    }

    private static void ClearConnectionValues(UnifiedProfile profile)
    {
        foreach (var key in _connectionKeys)
        {
            profile.Values.Remove(key);
        }
    }

    private static void InvokePrivateMethod(object target, string methodName)
    {
        var method = target.GetType().GetMethod(methodName, BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(method);
        method!.Invoke(target, null);
    }

    private static async Task WaitUntilAsync(Func<bool> condition, int timeoutMs = 2000)
    {
        var startedAt = Environment.TickCount64;
        while (!condition())
        {
            if (Environment.TickCount64 - startedAt > timeoutMs)
            {
                throw new TimeoutException("Condition not reached in expected time.");
            }

            await Task.Delay(20);
        }
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(
            string root,
            UnifiedConfigurationService config,
            TaskQueueFeatureService taskQueue,
            MAAUnifiedRuntime runtime,
            MainShellViewModel shell,
            FakeBridge bridge)
        {
            Root = root;
            Config = config;
            TaskQueue = taskQueue;
            Runtime = runtime;
            Shell = shell;
            Bridge = bridge;
        }

        public string Root { get; }

        public UnifiedConfigurationService Config { get; }

        public TaskQueueFeatureService TaskQueue { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public MainShellViewModel Shell { get; }

        public FakeBridge Bridge { get; }

        public static async Task<TestFixture> CreateAsync()
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
            var taskQueue = new TaskQueueFeatureService(session, config);

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
            var connectFeatureService = new ConnectFeatureService(session, config);
            var shellFeatureService = new ShellFeatureService(connectFeatureService);
            var runtime = new MAAUnifiedRuntime
            {
                CoreBridge = bridge,
                ConfigurationService = config,
                ResourceWorkflowService = new ResourceWorkflowService(root, bridge, log),
                SessionService = session,
                Platform = platform,
                LogService = log,
                DiagnosticsService = diagnostics,
                ConnectFeatureService = connectFeatureService,
                ShellFeatureService = shellFeatureService,
                TaskQueueFeatureService = taskQueue,
                CopilotFeatureService = new CopilotFeatureService(),
                ToolboxFeatureService = new ToolboxFeatureService(),
                RemoteControlFeatureService = new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(config, diagnostics, platform.PostActionExecutorService),
            };

            var shell = new MainShellViewModel(runtime);
            InvokePrivateMethod(shell, "SyncConnectionFromProfile");

            return new TestFixture(root, config, taskQueue, runtime, shell, bridge);
        }

        public UnifiedProfile GetCurrentProfile()
        {
            Assert.True(Config.TryGetCurrentProfile(out var profile));
            return profile;
        }

        public async ValueTask DisposeAsync()
        {
            await Runtime.DisposeAsync();
            await Bridge.DisposeAsync();
            try
            {
                Directory.Delete(Root, recursive: true);
            }
            catch
            {
                // ignore temporary folder cleanup failures
            }
        }
    }

    private sealed class FakeBridge : IMaaCoreBridge
    {
        private readonly Channel<CoreCallbackEvent> _callbackChannel = Channel.CreateUnbounded<CoreCallbackEvent>();
        private readonly List<CoreTaskRequest> _tasks = [];
        private int _taskId;

        public IReadOnlyList<CoreTaskRequest> AppendedTasks => _tasks;

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
        {
            _tasks.Add(task);
            return Task.FromResult(CoreResult<int>.Ok(Interlocked.Increment(ref _taskId)));
        }

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
            await foreach (var callback in _callbackChannel.Reader.ReadAllAsync(cancellationToken))
            {
                yield return callback;
            }
        }

        public ValueTask DisposeAsync()
        {
            _callbackChannel.Writer.TryComplete();
            return ValueTask.CompletedTask;
        }
    }
}
