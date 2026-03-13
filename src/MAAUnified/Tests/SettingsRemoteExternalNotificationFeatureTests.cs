using System.Runtime.CompilerServices;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class SettingsRemoteExternalNotificationFeatureTests
{
    [Fact]
    public async Task RemoteControlConnectivity_ValidEndpoints_SucceedsAndLogs()
    {
        var remote = new ScriptedRemoteControlFeatureService
        {
            TestHandler = static request =>
                UiOperationResult<RemoteControlConnectivityResult>.Ok(
                    new RemoteControlConnectivityResult(
                        request.PollIntervalMs,
                        new EndpointProbeResult("GetTask", request.GetTaskEndpoint, true, 200, "HTTP 200"),
                        new EndpointProbeResult("Report", request.ReportEndpoint, true, 204, "HTTP 204")),
                    "Remote connectivity passed."),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(remoteControlFeatureService: remote);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.RemoteGetTaskEndpoint = "https://example.com/get";
        vm.RemoteReportEndpoint = "https://example.com/report";
        vm.RemotePollInterval = 5000;

        await vm.TestRemoteControlConnectivityAsync();

        Assert.False(vm.HasRemoteControlErrorMessage);
        Assert.Contains("连通测试成功", vm.RemoteControlStatusMessage, StringComparison.Ordinal);
        var eventLog = await File.ReadAllTextAsync(fixture.Diagnostics.EventLogPath);
        Assert.Contains("Settings.RemoteControl.Test", eventLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task RemoteControlConnectivity_InvalidUri_ReturnsParameterError()
    {
        var remote = new ScriptedRemoteControlFeatureService
        {
            TestHandler = static _ => UiOperationResult<RemoteControlConnectivityResult>.Fail(
                UiErrorCode.RemoteControlInvalidParameters,
                "GetTask endpoint is invalid."),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(remoteControlFeatureService: remote);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        await vm.TestRemoteControlConnectivityAsync();

        Assert.True(vm.HasRemoteControlErrorMessage);
        Assert.Contains(UiErrorCode.RemoteControlInvalidParameters, vm.RemoteControlErrorMessage, StringComparison.Ordinal);
        var errorLog = await File.ReadAllTextAsync(fixture.Diagnostics.ErrorLogPath);
        Assert.Contains("Settings.RemoteControl.Test", errorLog, StringComparison.Ordinal);
    }

    [Fact]
    public async Task RemoteControlConnectivity_NetworkFailure_ReturnsNetworkError()
    {
        var details = new RemoteControlConnectivityResult(
            5000,
            new EndpointProbeResult("GetTask", "https://example.com/get", false, null, "timeout", UiErrorCode.RemoteControlNetworkFailure),
            new EndpointProbeResult("Report", "https://example.com/report", true, 200, "HTTP 200"));
        var remote = new ScriptedRemoteControlFeatureService
        {
            TestHandler = _ => UiOperationResult<RemoteControlConnectivityResult>.Fail(
                UiErrorCode.RemoteControlNetworkFailure,
                "Remote connectivity failed.",
                JsonSerializer.Serialize(details)),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(remoteControlFeatureService: remote);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        await vm.TestRemoteControlConnectivityAsync();

        Assert.True(vm.HasRemoteControlErrorMessage);
        Assert.Contains(UiErrorCode.RemoteControlNetworkFailure, vm.RemoteControlErrorMessage, StringComparison.Ordinal);
        Assert.Contains("GetTask=", vm.RemoteControlErrorMessage, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExternalNotification_LoadsAllProviders()
    {
        var notification = new ScriptedNotificationProviderFeatureService();

        await using var fixture = await RuntimeFixture.CreateAsync(notificationProviderFeatureService: notification);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        Assert.Equal(9, vm.AvailableNotificationProviders.Count);
        Assert.Contains("Smtp", vm.AvailableNotificationProviders, StringComparer.OrdinalIgnoreCase);
        Assert.Contains("DingTalk", vm.AvailableNotificationProviders, StringComparer.OrdinalIgnoreCase);
        Assert.Contains("CustomWebhook", vm.AvailableNotificationProviders, StringComparer.OrdinalIgnoreCase);
    }

    [Fact]
    public async Task ExternalNotification_ValidateParameters_AllProviders_DataDriven()
    {
        var service = new NotificationProviderFeatureService();
        var cases = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["Smtp"] = "server=smtp.example.com\nport=465\nfrom=noreply@example.com\nto=ops@example.com",
            ["ServerChan"] = "sendKey=abc123",
            ["Bark"] = "sendKey=abc123\nserver=https://api.day.app",
            ["Discord"] = "webhookUrl=https://discord.com/api/webhooks/test",
            ["DingTalk"] = "accessToken=ding-token\nsecret=ding-secret",
            ["Telegram"] = "botToken=token\nchatId=10001",
            ["Qmsg"] = "key=qmsg-key\nserver=https://qmsg.example.com",
            ["Gotify"] = "server=https://gotify.example.com\ntoken=test-token",
            ["CustomWebhook"] = "url=https://webhook.example.com/notify\nbody={\"ok\":true}",
        };

        foreach (var (provider, parameters) in cases)
        {
            var result = await service.ValidateProviderParametersAsync(
                new NotificationProviderRequest(provider, parameters));
            Assert.True(result.Success, $"Provider {provider} should pass validation but failed with: {result.Message}");
        }
    }

    [Fact]
    public async Task RemoteControlIdentity_SaveAndReload_RoundTripStable()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            await using (var first = await RuntimeFixture.CreateAsync(root, cleanupRoot: false))
            {
                var vm = new SettingsPageViewModel(first.Runtime, new ConnectionGameSharedStateViewModel());
                await vm.InitializeAsync();

                vm.RemoteGetTaskEndpoint = "https://example.com/get";
                vm.RemoteReportEndpoint = "https://example.com/report";
                vm.RemotePollInterval = 5200;
                vm.RemoteUserIdentity = "  user-A  ";
                vm.RemoteDeviceIdentity = "  device-B  ";

                await vm.SaveRemoteControlAsync();

                Assert.Equal("user-A", ReadCurrentProfileString(first.Config, ConfigurationKeys.RemoteControlUserIdentity));
                Assert.Equal("device-B", ReadCurrentProfileString(first.Config, ConfigurationKeys.RemoteControlDeviceIdentity));
            }

            await using var second = await RuntimeFixture.CreateAsync(root, cleanupRoot: false);
            var reloaded = new SettingsPageViewModel(second.Runtime, new ConnectionGameSharedStateViewModel());
            await reloaded.InitializeAsync();

            Assert.Equal("user-A", reloaded.RemoteUserIdentity);
            Assert.Equal("device-B", reloaded.RemoteDeviceIdentity);
        }
        finally
        {
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
    public async Task RemoteControlIdentity_InvalidControlCharacters_BlocksSaveAndPreservesConfig()
    {
        await using var fixture = await RuntimeFixture.CreateAsync();
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.RemoteControlUserIdentity] = JsonValue.Create("baseline-user");
        fixture.Config.CurrentConfig.Profiles[fixture.Config.CurrentConfig.CurrentProfile].Values[ConfigurationKeys.RemoteControlDeviceIdentity] = JsonValue.Create("baseline-device");
        await fixture.Config.SaveAsync();

        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();
        vm.RemoteUserIdentity = "bad\nuser";
        vm.RemoteDeviceIdentity = "good-device";

        await vm.SaveRemoteControlAsync();

        Assert.True(vm.HasRemoteControlErrorMessage);
        Assert.Contains(UiErrorCode.RemoteControlInvalidParameters, vm.RemoteControlErrorMessage, StringComparison.Ordinal);
        Assert.Equal("baseline-user", ReadCurrentProfileString(fixture.Config, ConfigurationKeys.RemoteControlUserIdentity));
        Assert.Equal("baseline-device", ReadCurrentProfileString(fixture.Config, ConfigurationKeys.RemoteControlDeviceIdentity));
    }

    [Fact]
    public async Task ExternalNotification_DingTalk_MissingAccessToken_FailsValidation()
    {
        var service = new NotificationProviderFeatureService();
        var result = await service.ValidateProviderParametersAsync(
            new NotificationProviderRequest("DingTalk", "secret=only-secret"));

        Assert.False(result.Success);
        Assert.Equal(UiErrorCode.NotificationProviderInvalidParameters, result.Error?.Code);
        Assert.Contains("accessToken", result.Message, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExternalNotification_DingTalk_SendTest_SuccessAndNetworkFailureAreClassified()
    {
        var successService = CreateNotificationProviderServiceForHttpProbe(
            static (_, _) => Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK)));
        var success = await successService.SendTestAsync(
            new NotificationProviderTestRequest(
                "DingTalk",
                "accessToken=test-token\nsecret=test-secret",
                "title",
                "message"));
        Assert.True(success.Success);

        var networkFailureService = CreateNotificationProviderServiceForHttpProbe(
            static (_, _) => throw new HttpRequestException("network down"));
        var failure = await networkFailureService.SendTestAsync(
            new NotificationProviderTestRequest(
                "DingTalk",
                "accessToken=test-token\nsecret=test-secret",
                "title",
                "message"));
        Assert.False(failure.Success);
        Assert.Equal(UiErrorCode.NotificationProviderNetworkFailure, failure.Error?.Code);
    }

    [Fact]
    public async Task ExternalNotification_TestSend_SelectedProviderOnly()
    {
        var notification = new ScriptedNotificationProviderFeatureService
        {
            SendHandler = static _ => UiOperationResult.Ok("sent"),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(notificationProviderFeatureService: notification);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.ExternalNotificationEnabled = true;
        vm.SelectedNotificationProvider = "Telegram";
        vm.NotificationProviderParametersText = "botToken=token\nchatId=12345";
        await vm.TestExternalNotificationAsync();

        var call = Assert.Single(notification.SendCalls);
        Assert.Equal("Telegram", call.Provider);
        Assert.Contains("测试发送成功", vm.ExternalNotificationStatusMessage, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExternalNotification_UnsupportedProvider_ReturnsUnsupportedError()
    {
        var notification = new ScriptedNotificationProviderFeatureService
        {
            SendHandler = static _ => UiOperationResult.Fail(
                UiErrorCode.NotificationProviderUnsupported,
                "provider unsupported"),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(notificationProviderFeatureService: notification);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.ExternalNotificationEnabled = true;
        vm.SelectedNotificationProvider = "Smtp";
        vm.NotificationProviderParametersText = "server=smtp.example.com\nport=465\nfrom=a@b.c\nto=d@e.f";
        await vm.TestExternalNotificationAsync();

        Assert.True(vm.HasExternalNotificationWarningMessage);
        Assert.Contains(UiErrorCode.NotificationProviderUnsupported, vm.ExternalNotificationWarningMessage, StringComparison.Ordinal);
    }

    [Fact]
    public async Task ExternalNotification_FailureMessages_AreDifferentiatedByCategory()
    {
        var invalidMessage = await RunExternalFailureCaseAsync(
            static request => UiOperationResult.Fail(UiErrorCode.NotificationProviderInvalidParameters, "bad params"),
            method: "validate");
        var networkMessage = await RunExternalFailureCaseAsync(
            static request => UiOperationResult.Fail(UiErrorCode.NotificationProviderNetworkFailure, "network down"),
            method: "send");
        var unsupportedMessage = await RunExternalFailureCaseAsync(
            static request => UiOperationResult.Fail(UiErrorCode.NotificationProviderUnsupported, "unsupported"),
            method: "send");

        Assert.Contains(UiErrorCode.NotificationProviderInvalidParameters, invalidMessage, StringComparison.Ordinal);
        Assert.Contains(UiErrorCode.NotificationProviderNetworkFailure, networkMessage, StringComparison.Ordinal);
        Assert.Contains(UiErrorCode.NotificationProviderUnsupported, unsupportedMessage, StringComparison.Ordinal);
        Assert.NotEqual(invalidMessage, networkMessage);
        Assert.NotEqual(networkMessage, unsupportedMessage);
    }

    [Fact]
    public async Task ExternalNotification_SaveAndReload_RoundTripStable()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        try
        {
            var notification = new ScriptedNotificationProviderFeatureService
            {
                ValidateHandler = static _ => UiOperationResult.Ok("valid"),
            };

            await using (var first = await RuntimeFixture.CreateAsync(
                root,
                cleanupRoot: false,
                notificationProviderFeatureService: notification))
            {
                var vm = new SettingsPageViewModel(first.Runtime, new ConnectionGameSharedStateViewModel());
                await vm.InitializeAsync();

                vm.ExternalNotificationEnabled = true;
                vm.ExternalNotificationSendWhenComplete = true;
                vm.ExternalNotificationSendWhenError = true;
                vm.ExternalNotificationSendWhenTimeout = false;
                vm.ExternalNotificationEnableDetails = true;

                vm.SelectedNotificationProvider = "Smtp";
                vm.NotificationProviderParametersText = "server=smtp.example.com\nport=587\nfrom=ops@example.com\nto=dev@example.com";
                vm.SelectedNotificationProvider = "Telegram";
                vm.NotificationProviderParametersText = "botToken=token-1\nchatId=10001";

                await vm.SaveExternalNotificationAsync();
            }

            await using var second = await RuntimeFixture.CreateAsync(
                root,
                cleanupRoot: false,
                notificationProviderFeatureService: new ScriptedNotificationProviderFeatureService
                {
                    ValidateHandler = static _ => UiOperationResult.Ok("valid"),
                });
            var reloaded = new SettingsPageViewModel(second.Runtime, new ConnectionGameSharedStateViewModel());
            await reloaded.InitializeAsync();

            Assert.True(reloaded.ExternalNotificationEnabled);
            Assert.True(reloaded.ExternalNotificationSendWhenComplete);
            Assert.True(reloaded.ExternalNotificationSendWhenError);
            Assert.False(reloaded.ExternalNotificationSendWhenTimeout);
            Assert.True(reloaded.ExternalNotificationEnableDetails);

            reloaded.SelectedNotificationProvider = "Smtp";
            Assert.Contains("server=smtp.example.com", reloaded.NotificationProviderParametersText, StringComparison.Ordinal);
            reloaded.SelectedNotificationProvider = "Telegram";
            Assert.Contains("chatId=10001", reloaded.NotificationProviderParametersText, StringComparison.Ordinal);
        }
        finally
        {
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
    public async Task ExternalNotification_Disabled_SkipsValidationAndSuppressesMessages()
    {
        var notification = new ScriptedNotificationProviderFeatureService
        {
            ValidateHandler = static _ => UiOperationResult.Fail(
                UiErrorCode.NotificationProviderInvalidParameters,
                "bad params"),
            SendHandler = static _ => UiOperationResult.Fail(
                UiErrorCode.NotificationProviderNetworkFailure,
                "network down"),
        };

        await using var fixture = await RuntimeFixture.CreateAsync(notificationProviderFeatureService: notification);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.ExternalNotificationEnabled = false;
        vm.SelectedNotificationProvider = "Smtp";
        vm.NotificationProviderParametersText = "server";

        await vm.ValidateExternalNotificationParametersAsync();
        await vm.TestExternalNotificationAsync();
        await vm.SaveExternalNotificationAsync();

        Assert.Empty(notification.ValidateCalls);
        Assert.Empty(notification.SendCalls);
        Assert.False(vm.HasExternalNotificationStatusMessage);
        Assert.False(vm.HasExternalNotificationWarningMessage);
        Assert.False(vm.HasExternalNotificationErrorMessage);
        Assert.Equal("False", ReadCurrentProfileString(fixture.Config, ConfigurationKeys.ExternalNotificationEnabled));
    }

    private static async Task<string> RunExternalFailureCaseAsync(
        Func<NotificationProviderTestRequest, UiOperationResult> handler,
        string method)
    {
        var notification = new ScriptedNotificationProviderFeatureService
        {
            ValidateHandler = static _ => UiOperationResult.Ok("valid"),
            SendHandler = handler,
        };

        await using var fixture = await RuntimeFixture.CreateAsync(notificationProviderFeatureService: notification);
        var vm = new SettingsPageViewModel(fixture.Runtime, new ConnectionGameSharedStateViewModel());
        await vm.InitializeAsync();

        vm.ExternalNotificationEnabled = true;
        vm.SelectedNotificationProvider = "CustomWebhook";
        vm.NotificationProviderParametersText = "url=https://webhook.example.com";
        if (string.Equals(method, "validate", StringComparison.OrdinalIgnoreCase))
        {
            notification.ValidateHandler = static _ =>
                UiOperationResult.Fail(UiErrorCode.NotificationProviderInvalidParameters, "bad params");
            await vm.ValidateExternalNotificationParametersAsync();
        }
        else
        {
            await vm.TestExternalNotificationAsync();
        }

        return vm.HasExternalNotificationWarningMessage
            ? vm.ExternalNotificationWarningMessage
            : vm.ExternalNotificationErrorMessage;
    }

    private static NotificationProviderFeatureService CreateNotificationProviderServiceForHttpProbe(
        Func<HttpRequestMessage, CancellationToken, Task<HttpResponseMessage>> sendHttpAsync)
    {
        var ctor = typeof(NotificationProviderFeatureService).GetConstructor(
            BindingFlags.Instance | BindingFlags.NonPublic,
            binder: null,
            [typeof(bool), typeof(Func<string, IReadOnlyDictionary<string, string>, string, string, CancellationToken, Task<UiOperationResult>>), typeof(Func<HttpRequestMessage, CancellationToken, Task<HttpResponseMessage>>)],
            modifiers: null);
        Assert.NotNull(ctor);
        return (NotificationProviderFeatureService)ctor!.Invoke([true, null, sendHttpAsync]);
    }

    private static string ReadCurrentProfileString(UnifiedConfigurationService config, string key)
    {
        if (!string.IsNullOrWhiteSpace(config.CurrentConfig.CurrentProfile)
            && config.CurrentConfig.Profiles.TryGetValue(config.CurrentConfig.CurrentProfile, out var profile)
            && profile.Values.TryGetValue(key, out var node))
        {
            return node?.ToString() ?? string.Empty;
        }

        return config.CurrentConfig.GlobalValues.TryGetValue(key, out var fallbackNode)
            ? fallbackNode?.ToString() ?? string.Empty
            : string.Empty;
    }

    private sealed class RuntimeFixture : IAsyncDisposable
    {
        private readonly bool _cleanupRoot;

        private RuntimeFixture(
            string root,
            MAAUnifiedRuntime runtime,
            UnifiedConfigurationService config,
            UiDiagnosticsService diagnostics,
            bool cleanupRoot)
        {
            Root = root;
            Runtime = runtime;
            Config = config;
            Diagnostics = diagnostics;
            _cleanupRoot = cleanupRoot;
        }

        public string Root { get; }

        public MAAUnifiedRuntime Runtime { get; }

        public UnifiedConfigurationService Config { get; }

        public UiDiagnosticsService Diagnostics { get; }

        public static async Task<RuntimeFixture> CreateAsync(
            string? root = null,
            bool cleanupRoot = true,
            IRemoteControlFeatureService? remoteControlFeatureService = null,
            INotificationProviderFeatureService? notificationProviderFeatureService = null)
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
                RemoteControlFeatureService = remoteControlFeatureService ?? new RemoteControlFeatureService(),
                PlatformCapabilityService = capability,
                OverlayFeatureService = new OverlayFeatureService(capability),
                NotificationProviderFeatureService = notificationProviderFeatureService ?? new NotificationProviderFeatureService(),
                SettingsFeatureService = new SettingsFeatureService(config, capability, diagnostics),
                DialogFeatureService = new DialogFeatureService(diagnostics),
                PostActionFeatureService = new PostActionFeatureService(
                    config,
                    diagnostics,
                    platform.PostActionExecutorService),
            };

            return new RuntimeFixture(root, runtime, config, diagnostics, cleanupRoot);
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

    private sealed class ScriptedRemoteControlFeatureService : IRemoteControlFeatureService
    {
        public Func<RemoteControlConnectivityRequest, UiOperationResult<RemoteControlConnectivityResult>>? TestHandler { get; set; }

        public List<RemoteControlConnectivityRequest> TestCalls { get; } = [];

        public Task<CoreResult<bool>> StartRemotePollingAsync(CancellationToken cancellationToken = default)
        {
            return Task.FromResult(CoreResult<bool>.Ok(true));
        }

        public Task<UiOperationResult<RemoteControlConnectivityResult>> TestConnectivityAsync(
            RemoteControlConnectivityRequest request,
            CancellationToken cancellationToken = default)
        {
            TestCalls.Add(request);
            var result = TestHandler?.Invoke(request)
                ?? UiOperationResult<RemoteControlConnectivityResult>.Ok(
                    new RemoteControlConnectivityResult(
                        request.PollIntervalMs,
                        new EndpointProbeResult("GetTask", request.GetTaskEndpoint, true, 200, "HTTP 200"),
                        new EndpointProbeResult("Report", request.ReportEndpoint, true, 200, "HTTP 200")),
                    "ok");
            return Task.FromResult(result);
        }
    }

    private sealed class ScriptedNotificationProviderFeatureService : INotificationProviderFeatureService
    {
        public string[] Providers { get; set; } =
        [
            "Smtp",
            "ServerChan",
            "Bark",
            "Discord",
            "DingTalk",
            "Telegram",
            "Qmsg",
            "Gotify",
            "CustomWebhook",
        ];

        public Func<NotificationProviderRequest, UiOperationResult>? ValidateHandler { get; set; }

        public Func<NotificationProviderTestRequest, UiOperationResult>? SendHandler { get; set; }

        public List<NotificationProviderRequest> ValidateCalls { get; } = [];

        public List<NotificationProviderTestRequest> SendCalls { get; } = [];

        public Task<string[]> GetAvailableProvidersAsync(CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.FromResult(Providers);
        }

        public Task<UiOperationResult> ValidateProviderParametersAsync(
            NotificationProviderRequest request,
            CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            ValidateCalls.Add(request);
            var result = ValidateHandler?.Invoke(request)
                ?? UiOperationResult.Ok("valid");
            return Task.FromResult(result);
        }

        public Task<UiOperationResult> SendTestAsync(
            NotificationProviderTestRequest request,
            CancellationToken cancellationToken = default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            SendCalls.Add(request);
            var result = SendHandler?.Invoke(request)
                ?? UiOperationResult.Ok("sent");
            return Task.FromResult(result);
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
}
