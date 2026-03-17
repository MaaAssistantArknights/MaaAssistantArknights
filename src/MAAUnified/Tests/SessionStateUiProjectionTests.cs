using System.Runtime.CompilerServices;
using MAAUnified.App.ViewModels.Copilot;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SessionStateUiProjectionTests
{
    [Fact]
    public async Task TaskQueuePage_ShouldProjectSessionState_ToRunControls()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());

        Assert.Equal(SessionState.Idle, vm.CurrentSessionState);
        Assert.False(vm.IsRunning);
        Assert.True(vm.CanToggleRun);
        Assert.Equal(vm.RootTexts.GetOrDefault("TaskQueue.Root.LinkStart", "Link Start!"), vm.RunButtonText);

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.False(vm.IsRunning);
        Assert.True(vm.CanToggleRun);
        Assert.Equal(vm.RootTexts.GetOrDefault("TaskQueue.Root.LinkStart", "Link Start!"), vm.RunButtonText);

        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        Assert.True(vm.IsRunning);
        Assert.True(vm.CanToggleRun);
        Assert.Equal(vm.RootTexts.GetOrDefault("TaskQueue.Root.Stop", "Stop"), vm.RunButtonText);

        Assert.True((await fixture.Runtime.ConnectFeatureService.StopAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.False(vm.IsRunning);
        Assert.True(vm.CanToggleRun);
        Assert.Equal(vm.RootTexts.GetOrDefault("TaskQueue.Root.LinkStart", "Link Start!"), vm.RunButtonText);
    }

    [Fact]
    public async Task TaskQueuePage_ShouldKeepLinkStartClickable_WhenConfigHasBlockingIssues()
    {
        await using var fixture = await TestFixture.CreateAsync(existingAvaloniaJson: CreateBlockingConfigJson());
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.True(vm.HasBlockingConfigIssues);
        Assert.True(vm.CanToggleRun);
        Assert.True(vm.BlockingConfigIssueCount > 0);
    }

    [Fact]
    public async Task TaskQueuePage_ToggleRun_WhenNotConnected_ShouldAutoConnectAndStart()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.Equal(SessionState.Idle, vm.CurrentSessionState);

        await vm.ToggleRunAsync();
        await WaitUntilAsync(() => vm.CurrentSessionState is SessionState.Running or SessionState.Connected);

        Assert.NotEqual(SessionState.Idle, vm.CurrentSessionState);
        Assert.DoesNotContain("Session state", vm.LastErrorMessage ?? string.Empty, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task TaskQueuePage_Start_ShouldClearVisibleLogsFromPreviousRun()
    {
        await using var fixture = await TestFixture.CreateAsync(existingAvaloniaJson: CreateRunnableConfigJson());
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.AppendSystemLog("stale system log");
        typeof(TaskQueuePageViewModel)
            .GetMethod("UpdateDownloadLog", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic)!
            .Invoke(vm, [DateTimeOffset.UtcNow, "INFO", "download old package"]);

        Assert.Contains("stale system log", FlattenLogs(vm), StringComparison.Ordinal);
        Assert.True(vm.HasDownloadLog);

        await vm.ToggleRunAsync();
        await WaitUntilAsync(() => vm.CurrentSessionState is SessionState.Running or SessionState.Connected);

        Assert.DoesNotContain("stale system log", FlattenLogs(vm), StringComparison.Ordinal);
        Assert.False(vm.HasDownloadLog);
    }

    [Fact]
    public async Task TaskQueuePage_Start_WhenConnectFails_ShouldShowConciseGuidanceWithoutIdleNoise()
    {
        await using var fixture = await TestFixture.CreateAsync(bridge: new FailingConnectBridge());
        var vm = new TaskQueuePageViewModel(
            fixture.Runtime,
            new ConnectionGameSharedStateViewModel
            {
                ConnectAddress = "127.0.0.1:5555",
                ConnectConfig = "MuMuEmulator12",
                AdbPath = @"D:\Program Files\Netease\MuMuPlayer-12.0\shell\adb.exe",
            });
        await vm.InitializeAsync();

        await vm.StartAsync();
        await WaitUntilAsync(() => HasLinkStartFailureLog(vm));

        Assert.Contains("连接失败", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.Contains("当前系统是 Linux", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.Contains("连接失败，请检查当前配置的 ADB 路径。", vm.StartPrecheckWarningMessage, StringComparison.Ordinal);
        Assert.Contains("Connection failed. Check the ADB path of the current profile.", vm.StartPrecheckWarningMessage, StringComparison.Ordinal);
        Assert.DoesNotContain("会话状态", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.DoesNotContain("Session state", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.DoesNotContain("错误详情", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.DoesNotContain(@"D:\Program Files", vm.StartPrecheckWarningMessage, StringComparison.Ordinal);
        Assert.DoesNotContain("ADB restart failed:", FlattenLogs(vm), StringComparison.Ordinal);
    }

    [Fact]
    public async Task TaskQueuePage_Start_WhenValidationBlocks_ShouldSelectFirstBlockingTaskAndAppendFailureLog()
    {
        await using var fixture = await TestFixture.CreateAsync(existingAvaloniaJson: CreateValidationBlockingConfigJson());
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.Equal("reclamation-ok", vm.SelectedTask?.Name);
        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        await vm.StartAsync();

        Assert.Equal(SessionState.Connected, vm.CurrentSessionState);
        Assert.Equal("recruit-blocking", vm.SelectedTask?.Name);
        Assert.True(vm.HasStartPrecheckWarningMessage);
        Assert.Contains("recruit-blocking", vm.StartPrecheckWarningMessage, StringComparison.OrdinalIgnoreCase);
        await WaitUntilAsync(() => HasLinkStartFailureLog(vm));
        Assert.True(HasLinkStartFailureLog(vm));
    }

    [Fact]
    public async Task CopilotPage_ShouldProjectSessionState_ToRunControls()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new CopilotPageViewModel(fixture.Runtime);

        Assert.Equal(SessionState.Idle, vm.CurrentSessionState);
        Assert.False(vm.CanStart);
        Assert.False(vm.CanStop);
        Assert.False(vm.IsRunning);

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.True(vm.CanStart);
        Assert.False(vm.CanStop);
        Assert.False(vm.IsRunning);

        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        Assert.False(vm.CanStart);
        Assert.True(vm.CanStop);
        Assert.True(vm.IsRunning);
    }

    [Fact]
    public async Task CopilotPage_Start_WhenSessionNotConnected_ShouldExposeBilingualErrorMessage()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new CopilotPageViewModel(fixture.Runtime);
        vm.Items.Add(new CopilotItemViewModel("sample", "Copilot", inlinePayload: "{}"));
        vm.SelectedItem = vm.Items[0];

        await vm.StartAsync();

        Assert.Contains("会话状态", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.Contains("Session state", vm.LastErrorMessage, StringComparison.Ordinal);
        Assert.Contains("start", vm.LastErrorMessage, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task TaskQueuePage_WaitAndStop_ShouldDisableControlsAndRecoverWhenSessionStops()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        var waitTask = vm.WaitAndStopAsync();
        await WaitUntilAsync(() => vm.IsWaitingForStop);

        Assert.False(vm.CanToggleRun);
        Assert.False(vm.CanWaitAndStop);
        Assert.Equal("等待中...", vm.WaitAndStopButtonText);

        Assert.True((await fixture.Runtime.ConnectFeatureService.StopAsync()).Success);
        await waitTask;
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Connected);

        Assert.False(vm.IsWaitingForStop);
        Assert.True(vm.CanToggleRun);
        Assert.False(vm.CanWaitAndStop);
        Assert.Equal("等待并停止", vm.WaitAndStopButtonText);
        Assert.Equal(1, Assert.IsType<FakeBridge>(fixture.Bridge).StopCallCount);
    }

    [Fact]
    public async Task TaskQueuePage_WaitAndStop_Canceled_ShouldRecoverPendingState()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        using var cts = new CancellationTokenSource();
        var waitTask = vm.WaitAndStopAsync(cts.Token);
        await WaitUntilAsync(() => vm.IsWaitingForStop);
        cts.Cancel();

        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await waitTask);

        Assert.False(vm.IsWaitingForStop);
        Assert.True(vm.CanToggleRun);
        Assert.True(vm.CanWaitAndStop);
        Assert.Equal("等待并停止", vm.WaitAndStopButtonText);
    }

    [Fact]
    public async Task TaskQueuePage_WaitAndStop_ShouldDisableControlsDuringWait_AndRecoverAfterCancel()
    {
        await using var fixture = await TestFixture.CreateAsync();
        var vm = new TaskQueuePageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());

        Assert.True((await fixture.Runtime.ConnectFeatureService.ConnectAsync("127.0.0.1:5555", "General", null)).Success);
        Assert.True((await fixture.Runtime.ConnectFeatureService.StartAsync()).Success);
        await WaitUntilAsync(() => vm.CurrentSessionState == SessionState.Running);

        using var waitCts = new CancellationTokenSource();
        var waitTask = vm.WaitAndStopAsync(waitCts.Token);

        Assert.False(vm.CanWaitAndStop);
        Assert.False(vm.CanToggleRun);

        waitCts.Cancel();
        await Assert.ThrowsAnyAsync<OperationCanceledException>(async () => await waitTask);

        Assert.True(vm.CanWaitAndStop);
        Assert.True(vm.CanToggleRun);
    }

    private static async Task WaitUntilAsync(Func<bool> condition, int retry = 80, int delayMs = 25)
    {
        for (var i = 0; i < retry; i++)
        {
            if (condition())
            {
                return;
            }

            await Task.Delay(delayMs);
        }

        throw new TimeoutException("Condition not reached in expected time.");
    }

    private static bool HasLinkStartFailureLog(TaskQueuePageViewModel vm)
    {
        try
        {
            return vm.LogCards
                .ToArray()
                .SelectMany(card => card.Items.ToArray())
                .Any(entry => entry.Level == "ERROR" && entry.Content.Contains("Link Start failed:", StringComparison.Ordinal));
        }
        catch (InvalidOperationException)
        {
            return false;
        }
    }

    private static string FlattenLogs(TaskQueuePageViewModel vm)
    {
        try
        {
            return string.Join(
                "\n",
                vm.LogCards
                    .ToArray()
                    .SelectMany(card => card.Items.ToArray())
                    .Select(entry => entry.Content));
        }
        catch (InvalidOperationException)
        {
            return string.Empty;
        }
    }

    private static string CreateBlockingConfigJson()
    {
        return
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": {
                  "Values": {},
                  "TaskQueue": [
                    {
                      "Type": "Recruit",
                      "Name": "Recruit",
                      "IsEnabled": true,
                      "Params": {
                        "times": 4
                      }
                    }
                  ]
                }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """;
    }

    private static string CreateRunnableConfigJson()
    {
        return
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": {
                  "Values": {
                    "ConnectAddress": "127.0.0.1:5555",
                    "ConnectConfig": "General"
                  },
                  "TaskQueue": [
                    {
                      "Type": "Recruit",
                      "Name": "Recruit",
                      "IsEnabled": true,
                      "Params": {
                        "times": 4
                      }
                    }
                  ]
                }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """;
    }

    private static string CreateValidationBlockingConfigJson()
    {
        return
            """
            {
              "SchemaVersion": 2,
              "CurrentProfile": "Default",
              "Profiles": {
                "Default": {
                  "Values": {},
                  "TaskQueue": [
                    {
                      "Type": "Reclamation",
                      "Name": "reclamation-ok",
                      "IsEnabled": true,
                      "Params": {
                        "theme": "Tales",
                        "mode": 1,
                        "increment_mode": 0,
                        "num_craft_batches": 16,
                        "tools_to_craft": [],
                        "clear_store": false
                      }
                    },
                    {
                      "Type": "Recruit",
                      "Name": "recruit-blocking",
                      "IsEnabled": true,
                      "Params": {
                        "times": -1
                      }
                    }
                  ]
                }
              },
              "GlobalValues": {},
              "Migration": {}
            }
            """;
    }

    private sealed class TestFixture : IAsyncDisposable
    {
        private TestFixture(string root, MAAUnifiedRuntime runtime, IMaaCoreBridge bridge)
        {
            Root = root;
            Runtime = runtime;
            Bridge = bridge;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public IMaaCoreBridge Bridge { get; }

        public static async Task<TestFixture> CreateAsync(
            string? existingAvaloniaJson = null,
            IMaaCoreBridge? bridge = null)
        {
            var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(Path.Combine(root, "config"));
            if (!string.IsNullOrWhiteSpace(existingAvaloniaJson))
            {
                await File.WriteAllTextAsync(
                    Path.Combine(root, "config", "avalonia.json"),
                    existingAvaloniaJson);
            }

            var log = new UiLogService();
            var diagnostics = new UiDiagnosticsService(root, log);
            var config = new UnifiedConfigurationService(
                new AvaloniaJsonConfigStore(root),
                new GuiNewJsonConfigImporter(),
                new GuiJsonConfigImporter(),
                log,
                root);
            await config.LoadOrBootstrapAsync();

            bridge ??= new FakeBridge();
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

            return new TestFixture(root, runtime, bridge);
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

    private sealed class FakeBridge : IMaaCoreBridge
    {
        private bool _connected;
        private bool _running;

        public int StopCallCount { get; private set; }

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
        {
            _connected = !string.IsNullOrWhiteSpace(connectionInfo.Address);
            return Task.FromResult(_connected
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.ConnectFailed, "connect failed")));
        }

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(1));

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
        {
            if (!_connected)
            {
                return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StartFailed, "not connected")));
            }

            _running = true;
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
        {
            StopCallCount++;
            var wasRunning = _running;
            _running = false;
            return Task.FromResult(wasRunning
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StopFailed, "not running")));
        }

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, _connected, _running)));

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await Task.CompletedTask;
            yield break;
        }

        public ValueTask DisposeAsync() => ValueTask.CompletedTask;
    }

    private sealed class FailingConnectBridge : IMaaCoreBridge
    {
        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(CoreInitializeRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreInitializeInfo>.Ok(new CoreInitializeInfo(request.BaseDirectory, "fake", "fake", request.ClientType)));

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.ConnectFailed, "Connection callback reported `ConnectFailed`.")));

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(1));

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StartFailed, "not connected")));

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StopFailed, "not running")));

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, false, false)));

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "not supported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "not supported")));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await Task.CompletedTask;
            yield break;
        }

        public ValueTask DisposeAsync() => ValueTask.CompletedTask;
    }
}
