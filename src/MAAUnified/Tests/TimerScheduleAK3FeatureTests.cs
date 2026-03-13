using System.Globalization;
using System.Reflection;
using System.Runtime.CompilerServices;
using MAAUnified.App.ViewModels;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

[Collection("MainShellSerial")]
public sealed class TimerScheduleAK3FeatureTests
{
    [Fact]
    public async Task Timer_Schedule_Idle_TriggersStart()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var scheduledAt = BuildScheduleTime(6, 30);
        await fixture.ConfigureSingleTimerAsync(scheduledAt, force: false, custom: false, profile: "Default");

        await InvokeEvaluateTimerScheduleAsync(fixture.Shell, scheduledAt);

        Assert.True(fixture.Shell.TaskQueuePage.IsRunning);
        Assert.Equal(1, fixture.Bridge.StartCallCount);
        Assert.Equal(0, fixture.Bridge.StopCallCount);
    }

    [Fact]
    public async Task Timer_Schedule_RunningAndNoForce_Skips()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var scheduledAt = BuildScheduleTime(7, 15);
        await fixture.ConfigureSingleTimerAsync(scheduledAt, force: false, custom: false, profile: "Default");
        await fixture.Shell.StartAsync();
        Assert.True(fixture.Shell.TaskQueuePage.IsRunning);
        Assert.Equal(1, fixture.Bridge.StartCallCount);

        await InvokeEvaluateTimerScheduleAsync(fixture.Shell, scheduledAt);

        Assert.Equal(1, fixture.Bridge.StartCallCount);
        Assert.Equal(0, fixture.Bridge.StopCallCount);
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.EventLogPath, "Timer.Schedule.Skip"));
    }

    [Fact]
    public async Task Timer_Schedule_RunningAndForce_StopThenStart()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var scheduledAt = BuildScheduleTime(8, 20);
        await fixture.ConfigureSingleTimerAsync(scheduledAt, force: true, custom: false, profile: "Default");
        await fixture.Shell.StartAsync();
        Assert.True(fixture.Shell.TaskQueuePage.IsRunning);
        Assert.Equal(1, fixture.Bridge.StartCallCount);

        await InvokeEvaluateTimerScheduleAsync(fixture.Shell, scheduledAt);

        Assert.True(fixture.Shell.TaskQueuePage.IsRunning);
        Assert.Equal(2, fixture.Bridge.StartCallCount);
        Assert.Equal(1, fixture.Bridge.StopCallCount);
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.EventLogPath, "Timer.Schedule.StopAndStart"));
    }

    [Fact]
    public async Task Timer_Schedule_SameMinute_Deduplicated()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var scheduledAt = BuildScheduleTime(9, 10);
        await fixture.ConfigureSingleTimerAsync(scheduledAt, force: true, custom: false, profile: "Default");

        await InvokeEvaluateTimerScheduleAsync(fixture.Shell, scheduledAt);
        await InvokeEvaluateTimerScheduleAsync(fixture.Shell, scheduledAt);

        Assert.True(fixture.Shell.TaskQueuePage.IsRunning);
        Assert.Equal(1, fixture.Bridge.StartCallCount);
        Assert.Equal(0, fixture.Bridge.StopCallCount);
    }

    [Fact]
    public async Task Timer_Schedule_LogsScopes()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var scheduledAt = BuildScheduleTime(10, 40);
        await fixture.ConfigureSingleTimerAsync(scheduledAt, force: false, custom: true, profile: "Night");

        await InvokeEvaluateTimerScheduleAsync(fixture.Shell, scheduledAt);

        Assert.True(await WaitForConditionAsync(() => fixture.Shell.TaskQueuePage.IsRunning, retry: 80, delayMs: 20));
        Assert.Equal("Night", fixture.Runtime.ConfigurationService.CurrentConfig.CurrentProfile);
        Assert.True(await WaitForLogCountAtLeastAsync(fixture.Runtime.DiagnosticsService.EventLogPath, "Timer.Schedule.Trigger", 1, retry: 80, delayMs: 30));
        Assert.True(await WaitForLogCountAtLeastAsync(fixture.Runtime.DiagnosticsService.EventLogPath, "Timer.Schedule.SwitchProfile", 1, retry: 80, delayMs: 30));
        Assert.True(await WaitForLogCountAtLeastAsync(fixture.Runtime.DiagnosticsService.EventLogPath, "Timer.Schedule.Start", 1, retry: 80, delayMs: 30));
    }

    [Fact]
    public async Task Timer_Schedule_LogsScopes_RepeatedRuns_StayStable()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var expectedSwitchProfileCount = 0;
        var expectedStartCount = 0;

        for (var i = 0; i < 3; i++)
        {
            var scheduledAt = BuildScheduleTime(12, 10 + i);
            await fixture.ConfigureSingleTimerAsync(scheduledAt, force: false, custom: true, profile: "Night");

            await InvokeEvaluateTimerScheduleAsync(fixture.Shell, scheduledAt);

            expectedSwitchProfileCount++;
            expectedStartCount++;
            Assert.True(await WaitForConditionAsync(() => fixture.Shell.TaskQueuePage.IsRunning, retry: 80, delayMs: 20));
            Assert.Equal(expectedStartCount, fixture.Bridge.StartCallCount);
            Assert.Equal("Night", fixture.Runtime.ConfigurationService.CurrentConfig.CurrentProfile);
            Assert.True(await WaitForLogCountAtLeastAsync(
                fixture.Runtime.DiagnosticsService.EventLogPath,
                "Timer.Schedule.SwitchProfile",
                expectedSwitchProfileCount,
                retry: 80,
                delayMs: 30));
            Assert.True(await WaitForLogCountAtLeastAsync(
                fixture.Runtime.DiagnosticsService.EventLogPath,
                "Timer.Schedule.Start",
                expectedStartCount,
                retry: 80,
                delayMs: 30));

            await fixture.Shell.StopAsync();
            fixture.Runtime.ConfigurationService.CurrentConfig.CurrentProfile = "Default";
            await fixture.Runtime.ConfigurationService.SaveAsync();
            await fixture.Shell.TaskQueuePage.ReloadTasksAsync();
            await fixture.Shell.TaskQueuePage.WaitForPendingBindingAsync();
        }
    }

    [Fact]
    public async Task Timer_Schedule_InvalidProfile_LogsError()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var scheduledAt = BuildScheduleTime(11, 11);
        var settings = fixture.Shell.SettingsPage;
        foreach (var slot in settings.Timers)
        {
            slot.Enabled = false;
            slot.Time = "07:00";
            slot.Profile = "Default";
        }

        settings.CustomTimerConfig = true;
        settings.ForceScheduledStart = false;
        settings.Timers[0].Enabled = true;
        settings.Timers[0].Time = scheduledAt.ToString("HH:mm", CultureInfo.InvariantCulture);
        settings.Timers[0].Profile = "MissingProfile";

        await InvokeEvaluateTimerScheduleAsync(fixture.Shell, scheduledAt);

        Assert.False(fixture.Shell.TaskQueuePage.IsRunning);
        Assert.True(await WaitForLogContainsAsync(fixture.Runtime.DiagnosticsService.ErrorLogPath, "Timer.Schedule.Error"));
    }

    private static DateTimeOffset BuildScheduleTime(int hour, int minute)
    {
        return new DateTimeOffset(2026, 1, 1, hour, minute, 20, TimeSpan.FromHours(8));
    }

    private static async Task InvokeEvaluateTimerScheduleAsync(MainShellViewModel shell, DateTimeOffset now)
    {
        var method = typeof(MainShellViewModel).GetMethod(
            "EvaluateTimerScheduleAsync",
            BindingFlags.Instance | BindingFlags.NonPublic);
        Assert.NotNull(method);
        var task = method!.Invoke(shell, [now, CancellationToken.None]) as Task;
        Assert.NotNull(task);
        await task!;
    }

    private static async Task<bool> WaitForLogContainsAsync(string path, string expected, int retry = 20, int delayMs = 30)
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

    private static async Task<bool> WaitForLogCountAtLeastAsync(
        string path,
        string expected,
        int expectedCount,
        int retry = 20,
        int delayMs = 30)
    {
        for (var i = 0; i < retry; i++)
        {
            if (File.Exists(path))
            {
                var content = await File.ReadAllTextAsync(path);
                var count = CountOccurrences(content, expected);
                if (count >= expectedCount)
                {
                    return true;
                }
            }

            await Task.Delay(delayMs);
        }

        return false;
    }

    private static async Task<bool> WaitForConditionAsync(
        Func<bool> predicate,
        int retry = 20,
        int delayMs = 30)
    {
        for (var i = 0; i < retry; i++)
        {
            if (predicate())
            {
                return true;
            }

            await Task.Delay(delayMs);
        }

        return false;
    }

    private static int CountOccurrences(string text, string token)
    {
        if (string.IsNullOrEmpty(text) || string.IsNullOrEmpty(token))
        {
            return 0;
        }

        var count = 0;
        var index = 0;
        while (true)
        {
            index = text.IndexOf(token, index, StringComparison.Ordinal);
            if (index < 0)
            {
                return count;
            }

            count++;
            index += token.Length;
        }
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(
            string root,
            MAAUnifiedRuntime runtime,
            MainShellViewModel shell,
            CountingBridge bridge)
        {
            Root = root;
            Runtime = runtime;
            Shell = shell;
            Bridge = bridge;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public MainShellViewModel Shell { get; }

        public CountingBridge Bridge { get; }

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

            config.CurrentConfig.Profiles["Night"] = new UnifiedProfile();
            await config.SaveAsync();

            var bridge = new CountingBridge();
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

            config.CurrentConfig.CurrentProfile = "Default";
            Assert.True((await runtime.TaskQueueFeatureService.AddTaskAsync("StartUp", "startup-default")).Success);
            config.CurrentConfig.CurrentProfile = "Night";
            Assert.True((await runtime.TaskQueueFeatureService.AddTaskAsync("StartUp", "startup-night")).Success);
            config.CurrentConfig.CurrentProfile = "Default";
            await config.SaveAsync();

            var shell = new MainShellViewModel(runtime);
            await shell.InitializeAsync();
            var connectResult = await runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null);
            Assert.True(connectResult.Success, connectResult.Message);
            TestShellCleanup.StopTimerScheduler(shell);
            await shell.TaskQueuePage.ReloadTasksAsync();

            return new TestFixture(root, runtime, shell, bridge);
        }

        public async Task ConfigureSingleTimerAsync(
            DateTimeOffset scheduledAt,
            bool force,
            bool custom,
            string profile)
        {
            var settings = Shell.SettingsPage;
            foreach (var slot in settings.Timers)
            {
                slot.Enabled = false;
                slot.Time = "07:00";
                slot.Profile = "Default";
            }

            settings.ForceScheduledStart = force;
            settings.ShowWindowBeforeForceScheduledStart = false;
            settings.CustomTimerConfig = custom;

            settings.Timers[0].Enabled = true;
            settings.Timers[0].Time = scheduledAt.ToString("HH:mm", CultureInfo.InvariantCulture);
            settings.Timers[0].Profile = profile;

            await settings.SaveTimerSettingsAsync();
            Assert.False(settings.HasPendingTimerChanges, settings.TimerValidationMessage);
        }

        public async ValueTask DisposeAsync()
        {
            TestShellCleanup.StopTimerScheduler(Shell);
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

    private sealed class CountingBridge : IMaaCoreBridge
    {
        public int StartCallCount { get; private set; }

        public int StopCallCount { get; private set; }

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
            StartCallCount++;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            StopCallCount++;
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
