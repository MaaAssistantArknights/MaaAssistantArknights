using System.IO.Compression;
using System.Reflection;
using System.Text.Json;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services.Localization;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class PlatformCapabilityContractTests
{
    [Fact]
    public void PlatformOperation_HelpersExposeConsistentExecutionSemantics()
    {
        var native = PlatformOperation.NativeSuccess("native-provider", "ok", "tray.show");
        Assert.True(native.Success);
        Assert.False(native.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Native, native.ExecutionMode);
        Assert.Equal("native-provider", native.Provider);
        Assert.Equal("tray.show", native.OperationId);

        var fallback = PlatformOperation.FallbackSuccess("fallback-provider", "fallback", "tray.setVisible", "TrayFallback");
        Assert.True(fallback.Success);
        Assert.True(fallback.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Fallback, fallback.ExecutionMode);
        Assert.Equal("TrayFallback", fallback.ErrorCode);

        var failed = PlatformOperation.Failed("provider", "failed", "E001", "hotkey.register", usedFallback: true);
        Assert.False(failed.Success);
        Assert.True(failed.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Failed, failed.ExecutionMode);
        Assert.Equal("E001", failed.ErrorCode);

        var nativeWithValue = PlatformOperation.NativeSuccess("provider", 7, "ok", "overlay.query-targets");
        Assert.True(nativeWithValue.Success);
        Assert.Equal(7, nativeWithValue.Value);

        var fallbackWithValue = PlatformOperation.FallbackSuccess("provider", 5, "fallback", "overlay.query-targets", PlatformErrorCodes.OverlayPreviewMode);
        Assert.True(fallbackWithValue.Success);
        Assert.True(fallbackWithValue.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Fallback, fallbackWithValue.ExecutionMode);
    }

    [Fact]
    public void PlatformCapabilitySnapshotFactory_BuildsSnapshotFromBundle()
    {
        var bundle = new PlatformServiceBundle
        {
            TrayService = new NoOpTrayService(),
            NotificationService = new NoOpNotificationService(),
            HotkeyService = new NoOpGlobalHotkeyService(),
            AutostartService = new NoOpAutostartService(),
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = new NoOpOverlayCapabilityService(),
            PostActionExecutorService = new NoOpPostActionExecutorService(),
        };

        var snapshot = PlatformCapabilitySnapshotFactory.FromBundle(bundle);
        Assert.Equal(bundle.TrayService.Capability, snapshot.Tray);
        Assert.Equal(bundle.NotificationService.Capability, snapshot.Notification);
        Assert.Equal(bundle.HotkeyService.Capability, snapshot.Hotkey);
        Assert.Equal(bundle.AutostartService.Capability, snapshot.Autostart);
        Assert.Equal(bundle.OverlayService.Capability, snapshot.Overlay);
    }

    [Fact]
    public async Task WindowScopedHotkeyService_HandlesConflictAndUnregister()
    {
        var service = new WindowScopedHotkeyService();

        var first = await service.RegisterAsync("ShowGui", "Ctrl+Shift+Alt+M");
        Assert.True(first.Success);
        Assert.True(first.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Fallback, first.ExecutionMode);

        var conflict = await service.RegisterAsync("LinkStart", "Alt+Ctrl+Shift+M");
        Assert.False(conflict.Success);
        Assert.Equal("HotkeyConflict", conflict.ErrorCode);
        Assert.Equal(PlatformExecutionMode.Failed, conflict.ExecutionMode);

        var removed = await service.UnregisterAsync("ShowGui");
        Assert.True(removed.Success);
        Assert.True(removed.UsedFallback);

        var missing = await service.UnregisterAsync("ShowGui");
        Assert.False(missing.Success);
        Assert.Equal(PlatformErrorCodes.HotkeyNotFound, missing.ErrorCode);

        var invalid = await service.RegisterAsync("Bad", "OnlyOneKey");
        Assert.False(invalid.Success);
        Assert.Equal(PlatformErrorCodes.HotkeyInvalidGesture, invalid.ErrorCode);
    }

    [Fact]
    public async Task WindowMenuTrayService_InitializeAndShutdown_ReturnFallbackSuccess()
    {
        var service = new WindowMenuTrayService();
        var init = await service.InitializeAsync("MAAUnified", TrayMenuText.Default);
        Assert.True(init.Success);
        Assert.True(init.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Fallback, init.ExecutionMode);

        var shutdown = await service.ShutdownAsync();
        Assert.True(shutdown.Success);
        Assert.True(shutdown.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Fallback, shutdown.ExecutionMode);
    }

    [Fact]
    public async Task NoOpOverlayService_ReturnsPreviewAndFallbackResult()
    {
        var service = new NoOpOverlayCapabilityService();
        var bindResult = await service.BindHostWindowAsync(nint.Zero, clickThrough: true, opacity: 0.8);
        Assert.True(bindResult.Success);
        Assert.True(bindResult.UsedFallback);

        var queryResult = await service.QueryTargetsAsync();
        Assert.True(queryResult.Success);
        Assert.True(queryResult.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Fallback, queryResult.ExecutionMode);
        Assert.NotNull(queryResult.Value);
        var preview = Assert.Single(queryResult.Value!);
        Assert.Equal("preview", preview.Id);
        Assert.True(preview.IsPrimary);

        var result = await service.SetVisibleAsync(true);
        Assert.True(result.Success);
        Assert.True(result.UsedFallback);
        Assert.Equal(PlatformExecutionMode.Fallback, result.ExecutionMode);
    }

    [Fact]
    public void PlatformServicesFactory_CreateDefaults_SelectsOverlayProviderByCurrentOs()
    {
        var bundle = PlatformServicesFactory.CreateDefaults();
        Assert.NotNull(bundle.TrayService);
        Assert.NotNull(bundle.NotificationService);
        Assert.NotNull(bundle.HotkeyService);
        Assert.NotNull(bundle.AutostartService);
        Assert.NotNull(bundle.OverlayService);
        Assert.True(bundle.TrayService.Capability.HasFallback);
        Assert.True(bundle.NotificationService.Capability.HasFallback);
        Assert.True(bundle.HotkeyService.Capability.HasFallback);

        if (OperatingSystem.IsWindows())
        {
            Assert.IsType<WindowsOverlayCapabilityService>(bundle.OverlayService);
        }
        else
        {
            Assert.IsType<NoOpOverlayCapabilityService>(bundle.OverlayService);
        }
    }

    [Fact]
    public void PlatformServicesFactory_CreateDefaults_UsesExpectedProviderFamilies()
    {
        var bundle = PlatformServicesFactory.CreateDefaults();
        Assert.True(bundle.TrayService is WindowsNotifyIconTrayService or AvaloniaTrayIconTrayService or WindowMenuTrayService or NoOpTrayService);
        Assert.True(bundle.NotificationService is DesktopNotificationService or CommandNotificationService or NoOpNotificationService);
        Assert.True(bundle.HotkeyService is SharpHookGlobalHotkeyService or WindowScopedHotkeyService or NoOpGlobalHotkeyService);
        Assert.True(bundle.AutostartService is CrossPlatformAutostartService or NoOpAutostartService);
        Assert.True(bundle.OverlayService is WindowsOverlayCapabilityService or NoOpOverlayCapabilityService);
    }

    [Fact]
    public void PlatformServicesFactory_WhenForcedFallback_UsesFallbackProviders()
    {
        var original = Environment.GetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK");
        try
        {
            Environment.SetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK", "1");
            var bundle = PlatformServicesFactory.CreateDefaults();
            Assert.IsType<WindowMenuTrayService>(bundle.TrayService);
            Assert.IsType<CommandNotificationService>(bundle.NotificationService);
            Assert.IsType<WindowScopedHotkeyService>(bundle.HotkeyService);
            Assert.IsType<NoOpOverlayCapabilityService>(bundle.OverlayService);
        }
        finally
        {
            Environment.SetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK", original);
        }
    }

    [Fact]
    public async Task PlatformServicesFactory_WhenForcedFallback_OperationsReturnFallbackResults()
    {
        var original = Environment.GetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK");
        try
        {
            Environment.SetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK", "1");
            var bundle = PlatformServicesFactory.CreateDefaults();
            var tray = await bundle.TrayService.SetVisibleAsync(false);
            Assert.True(tray.Success);
            Assert.True(tray.UsedFallback);

            var notify = await bundle.NotificationService.NotifyAsync("title", "msg");
            Assert.True(notify.Success);
            Assert.True(notify.UsedFallback);

            var hotkey = await bundle.HotkeyService.RegisterAsync("ShowGui", "Ctrl+Shift+Alt+M");
            Assert.True(hotkey.Success);
            Assert.True(hotkey.UsedFallback);
        }
        finally
        {
            Environment.SetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK", original);
        }
    }

    [Fact]
    public void CrossPlatformAutostartService_ContentTemplates_AreGenerated()
    {
        var type = typeof(CrossPlatformAutostartService);
        var linuxMethod = type.GetMethod("BuildLinuxDesktopEntry", BindingFlags.NonPublic | BindingFlags.Static);
        var macMethod = type.GetMethod("BuildMacLaunchAgentPlist", BindingFlags.NonPublic | BindingFlags.Static);
        Assert.NotNull(linuxMethod);
        Assert.NotNull(macMethod);

        var path = "/tmp/maa-unified";
        var linuxContent = Assert.IsType<string>(linuxMethod!.Invoke(null, new object[] { path }));
        var macContent = Assert.IsType<string>(macMethod!.Invoke(null, new object[] { path }));

        Assert.Contains("[Desktop Entry]", linuxContent, StringComparison.Ordinal);
        Assert.Contains("Exec=\"/tmp/maa-unified\"", linuxContent, StringComparison.Ordinal);

        Assert.Contains("<plist version=\"1.0\">", macContent, StringComparison.Ordinal);
        Assert.Contains("<string>/tmp/maa-unified</string>", macContent, StringComparison.Ordinal);
        Assert.Contains("<key>RunAtLoad</key>", macContent, StringComparison.Ordinal);
    }

    [Fact]
    public async Task UiDiagnosticsService_RecordsPlatformEvents_AndBundlesPlatformLog()
    {
        var root = CreateTempRoot();
        var diagnostics = new UiDiagnosticsService(root, new UiLogService());
        var result = PlatformOperation.FallbackSuccess("window-menu", "fallback", "tray.setVisible", "TrayFallback");

        await diagnostics.RecordPlatformEventAsync(PlatformCapabilityId.Tray, "set-visible", result);
        Assert.True(File.Exists(diagnostics.PlatformEventLogPath));

        var lines = await File.ReadAllLinesAsync(diagnostics.PlatformEventLogPath);
        Assert.Single(lines);

        using var json = JsonDocument.Parse(lines[0]);
        var payload = json.RootElement;
        Assert.Equal((int)PlatformCapabilityId.Tray, payload.GetProperty("Capability").GetInt32());
        Assert.Equal("set-visible", payload.GetProperty("Action").GetString());
        Assert.True(payload.GetProperty("UsedFallback").GetBoolean());
        Assert.Equal("window-menu", payload.GetProperty("Provider").GetString());
        Assert.Equal("tray.setVisible", payload.GetProperty("OperationId").GetString());

        await diagnostics.RecordFailedResultAsync("Platform.Fail", Application.Models.UiOperationResult.Fail("Code", "Failed operation"));
        Assert.True(File.Exists(diagnostics.ErrorLogPath));

        var bundlePath = await diagnostics.BuildIssueReportBundleAsync(root);
        Assert.True(File.Exists(bundlePath));
        using var zip = ZipFile.OpenRead(bundlePath);
        Assert.Contains(zip.Entries, entry => entry.FullName == "debug/avalonia-platform-events.log");
        Assert.Contains(zip.Entries, entry => entry.FullName == "debug/avalonia-ui-startup.log");
        Assert.Contains(zip.Entries, entry => entry.FullName == "debug/avalonia-ui-errors.log");
    }

    [Fact]
    public async Task PlatformCapabilityFeatureService_FailedOperation_WritesUiErrorLog()
    {
        var root = CreateTempRoot();
        var diagnostics = new UiDiagnosticsService(root, new UiLogService());
        var bundle = new PlatformServiceBundle
        {
            TrayService = new FailingTrayService(),
            NotificationService = new NoOpNotificationService(),
            HotkeyService = new NoOpGlobalHotkeyService(),
            AutostartService = new NoOpAutostartService(),
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = new NoOpOverlayCapabilityService(),
            PostActionExecutorService = new NoOpPostActionExecutorService(),
        };

        var service = new PlatformCapabilityFeatureService(bundle, diagnostics);
        var result = await service.SetTrayVisibleAsync(false);
        Assert.False(result.Success);
        Assert.True(File.Exists(diagnostics.ErrorLogPath));
        var content = await File.ReadAllTextAsync(diagnostics.ErrorLogPath);
        Assert.Contains("PlatformCapability.Tray.set-visible", content, StringComparison.Ordinal);
        Assert.Contains(PlatformErrorCodes.TrayInitFailed, content, StringComparison.Ordinal);
    }

    [Fact]
    public async Task PlatformCapabilityFeatureService_GetAutostartEnabledAsync_FailedQuery_WritesUiErrorLog()
    {
        var root = CreateTempRoot();
        var diagnostics = new UiDiagnosticsService(root, new UiLogService());
        var bundle = new PlatformServiceBundle
        {
            TrayService = new NoOpTrayService(),
            NotificationService = new NoOpNotificationService(),
            HotkeyService = new NoOpGlobalHotkeyService(),
            AutostartService = new FailingAutostartService(),
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = new NoOpOverlayCapabilityService(),
            PostActionExecutorService = new NoOpPostActionExecutorService(),
        };

        var service = new PlatformCapabilityFeatureService(bundle, diagnostics);
        var result = await service.GetAutostartEnabledAsync();

        Assert.False(result.Success);
        Assert.NotNull(result.Error);
        Assert.Equal(PlatformErrorCodes.AutostartQueryFailed, result.Error!.Code);
        Assert.True(File.Exists(diagnostics.ErrorLogPath));
        var content = await File.ReadAllTextAsync(diagnostics.ErrorLogPath);
        Assert.Contains("PlatformCapability.Autostart.query", content, StringComparison.Ordinal);
        Assert.Contains(PlatformErrorCodes.AutostartQueryFailed, content, StringComparison.Ordinal);
    }

    [Fact]
    public async Task PlatformCapabilityFeatureService_QueryOverlayTargetsAsync_ProviderThrows_ReturnsOverlayQueryFailed_AndWritesLogs()
    {
        var root = CreateTempRoot();
        var diagnostics = new UiDiagnosticsService(root, new UiLogService());
        var bundle = new PlatformServiceBundle
        {
            TrayService = new NoOpTrayService(),
            NotificationService = new NoOpNotificationService(),
            HotkeyService = new NoOpGlobalHotkeyService(),
            AutostartService = new NoOpAutostartService(),
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = new ThrowingOverlayService(),
            PostActionExecutorService = new NoOpPostActionExecutorService(),
        };

        var service = new PlatformCapabilityFeatureService(bundle, diagnostics);
        var result = await service.QueryOverlayTargetsAsync();

        Assert.False(result.Success);
        Assert.NotNull(result.Error);
        Assert.Equal(PlatformErrorCodes.OverlayQueryFailed, result.Error!.Code);

        Assert.True(File.Exists(diagnostics.PlatformEventLogPath));
        var platformLines = await File.ReadAllLinesAsync(diagnostics.PlatformEventLogPath);
        Assert.NotEmpty(platformLines);
        using (var doc = JsonDocument.Parse(platformLines[^1]))
        {
            var payload = doc.RootElement;
            Assert.Equal("query-targets", payload.GetProperty("Action").GetString());
            Assert.False(payload.GetProperty("Success").GetBoolean());
            Assert.Equal(PlatformErrorCodes.OverlayQueryFailed, payload.GetProperty("ErrorCode").GetString());
        }

        Assert.True(File.Exists(diagnostics.ErrorLogPath));
        var content = await File.ReadAllTextAsync(diagnostics.ErrorLogPath);
        Assert.Contains("PlatformCapability.Overlay.query-targets", content, StringComparison.Ordinal);
        Assert.Contains(PlatformErrorCodes.OverlayQueryFailed, content, StringComparison.Ordinal);
    }

    [Fact]
    public async Task PlatformCapabilityFeatureService_TrayCallbackSubscriberThrows_DoesNotBreakMainFlow_AndErrorIsLocatable()
    {
        var root = CreateTempRoot();
        var diagnostics = new UiDiagnosticsService(root, new UiLogService());
        var tray = new TriggerableTrayService();
        var bundle = new PlatformServiceBundle
        {
            TrayService = tray,
            NotificationService = new NoOpNotificationService(),
            HotkeyService = new NoOpGlobalHotkeyService(),
            AutostartService = new NoOpAutostartService(),
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = new NoOpOverlayCapabilityService(),
            PostActionExecutorService = new NoOpPostActionExecutorService(),
        };

        var service = new PlatformCapabilityFeatureService(bundle, diagnostics);
        service.TrayCommandInvoked += (_, _) => throw new InvalidOperationException("synthetic tray subscriber error");

        tray.Emit(TrayCommandId.Start, "test-tray");
        var operation = await service.SetTrayVisibleAsync(false);

        Assert.True(operation.Success);
        await WaitUntilAsync(async () =>
        {
            if (!File.Exists(diagnostics.ErrorLogPath))
            {
                return false;
            }

            var content = await File.ReadAllTextAsync(diagnostics.ErrorLogPath);
            return content.Contains("PlatformCapability.TrayCommand", StringComparison.Ordinal)
                   && content.Contains("Tray command callback failed.", StringComparison.Ordinal);
        });
    }

    [Fact]
    public async Task PlatformCapabilityFeatureService_HotkeyCallbackSubscriberThrows_DoesNotBreakMainFlow_AndErrorIsLocatable()
    {
        var root = CreateTempRoot();
        var diagnostics = new UiDiagnosticsService(root, new UiLogService());
        var hotkey = new TriggerableHotkeyService();
        var bundle = new PlatformServiceBundle
        {
            TrayService = new NoOpTrayService(),
            NotificationService = new NoOpNotificationService(),
            HotkeyService = hotkey,
            AutostartService = new NoOpAutostartService(),
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = new NoOpOverlayCapabilityService(),
            PostActionExecutorService = new NoOpPostActionExecutorService(),
        };

        var service = new PlatformCapabilityFeatureService(bundle, diagnostics);
        service.GlobalHotkeyTriggered += (_, _) => throw new InvalidOperationException("synthetic hotkey subscriber error");

        hotkey.Emit("ShowGui", "Ctrl+Shift+Alt+M");
        var operation = await service.RegisterGlobalHotkeyAsync("ShowGui", "Ctrl+Shift+Alt+M");

        Assert.True(operation.Success);
        await WaitUntilAsync(async () =>
        {
            if (!File.Exists(diagnostics.ErrorLogPath))
            {
                return false;
            }

            var content = await File.ReadAllTextAsync(diagnostics.ErrorLogPath);
            return content.Contains("PlatformCapability.HotkeyTriggered", StringComparison.Ordinal)
                   && content.Contains("Hotkey callback failed.", StringComparison.Ordinal);
        });
    }

    [Fact]
    public async Task PlatformCapabilityFeatureService_OverlayStateChanged_ShouldForwardAndWritePlatformLog()
    {
        var root = CreateTempRoot();
        var diagnostics = new UiDiagnosticsService(root, new UiLogService());
        var overlay = new TriggerableOverlayService();
        var bundle = new PlatformServiceBundle
        {
            TrayService = new NoOpTrayService(),
            NotificationService = new NoOpNotificationService(),
            HotkeyService = new NoOpGlobalHotkeyService(),
            AutostartService = new NoOpAutostartService(),
            FileDialogService = new NoOpFileDialogService(),
            OverlayService = overlay,
            PostActionExecutorService = new NoOpPostActionExecutorService(),
        };

        var service = new PlatformCapabilityFeatureService(bundle, diagnostics);
        OverlayStateChangedEvent? forwarded = null;
        service.OverlayStateChanged += (_, e) => forwarded = e;

        overlay.Emit(new OverlayStateChangedEvent(
            OverlayRuntimeMode.Preview,
            Visible: true,
            TargetId: "preview",
            Action: "target-lost",
            Message: "Overlay target was lost; switched to Preview + Logs mode.",
            Timestamp: DateTimeOffset.UtcNow,
            Provider: overlay.Capability.Provider,
            UsedFallback: true,
            ErrorCode: PlatformErrorCodes.OverlayTargetGone));

        await WaitUntilAsync(async () =>
        {
            if (forwarded is null || !File.Exists(diagnostics.PlatformEventLogPath))
            {
                return false;
            }

            var lines = await File.ReadAllLinesAsync(diagnostics.PlatformEventLogPath);
            return lines.Any(line => line.Contains("\"Action\":\"target-lost\"", StringComparison.Ordinal));
        });

        var forwardedEvent = Assert.IsType<OverlayStateChangedEvent>(forwarded);
        Assert.Equal("target-lost", forwardedEvent.Action);
        Assert.Equal(OverlayRuntimeMode.Preview, forwardedEvent.Mode);
        Assert.True(forwardedEvent.Visible);
        Assert.Equal("preview", forwardedEvent.TargetId);

        var platformLines = await File.ReadAllLinesAsync(diagnostics.PlatformEventLogPath);
        using var doc = JsonDocument.Parse(platformLines[^1]);
        var payload = doc.RootElement;
        Assert.Equal("target-lost", payload.GetProperty("Action").GetString());
        Assert.True(payload.GetProperty("UsedFallback").GetBoolean());
        Assert.Equal(PlatformErrorCodes.OverlayTargetGone, payload.GetProperty("ErrorCode").GetString());
    }

    [Fact]
    public void PlatformCapabilityTextMap_MissingKeyFallback_ShouldBeObservableViaFallbackReporter()
    {
        var fallbacks = new List<LocalizationFallbackInfo>();
        var text = PlatformCapabilityTextMap.GetUiText(
            "zh-cn",
            "Missing.Key.For.Q1",
            "fallback-value",
            info => fallbacks.Add(info));

        Assert.Equal("fallback-value", text);
        var fallback = Assert.Single(fallbacks);
        Assert.Equal("PlatformCapabilityText", fallback.Scope);
        Assert.Equal("zh-cn", fallback.Language);
        Assert.Equal("Missing.Key.For.Q1", fallback.Key);
        Assert.Equal("key", fallback.FallbackSource);
    }

    [Fact]
    public void PlatformCapabilityTextMap_ContainsAllErrorAndMenuKeys_ForAllSupportedLanguages()
    {
        var languages = new[] { "zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr", "pallas" };
        var requiredErrorCodes = typeof(PlatformErrorCodes)
            .GetFields(BindingFlags.Public | BindingFlags.Static)
            .Where(field => field.FieldType == typeof(string))
            .Select(field => field.GetValue(null))
            .OfType<string>()
            .ToArray();

        foreach (var language in languages)
        {
            foreach (var errorCode in requiredErrorCodes)
            {
                var fallback = $"MISSING-{errorCode}";
                var localized = PlatformCapabilityTextMap.FormatErrorCode(language, errorCode, fallback);
                Assert.NotEqual(fallback, localized);
            }

            var trayMenu = PlatformCapabilityTextMap.CreateTrayMenuText(language);
            Assert.False(string.IsNullOrWhiteSpace(trayMenu.Start));
            Assert.False(string.IsNullOrWhiteSpace(trayMenu.Stop));
            Assert.False(string.IsNullOrWhiteSpace(trayMenu.ForceShow));
            Assert.False(string.IsNullOrWhiteSpace(trayMenu.HideTray));
            Assert.False(string.IsNullOrWhiteSpace(trayMenu.ToggleOverlay));
            Assert.False(string.IsNullOrWhiteSpace(trayMenu.SwitchLanguage));
            Assert.False(string.IsNullOrWhiteSpace(trayMenu.Restart));
            Assert.False(string.IsNullOrWhiteSpace(trayMenu.Exit));

            Assert.False(string.IsNullOrWhiteSpace(PlatformCapabilityTextMap.GetCapabilityName(language, PlatformCapabilityId.Tray)));
            Assert.False(string.IsNullOrWhiteSpace(PlatformCapabilityTextMap.GetCapabilityName(language, PlatformCapabilityId.Notification)));
            Assert.False(string.IsNullOrWhiteSpace(PlatformCapabilityTextMap.GetCapabilityName(language, PlatformCapabilityId.Hotkey)));
            Assert.False(string.IsNullOrWhiteSpace(PlatformCapabilityTextMap.GetCapabilityName(language, PlatformCapabilityId.Autostart)));
            Assert.False(string.IsNullOrWhiteSpace(PlatformCapabilityTextMap.GetCapabilityName(language, PlatformCapabilityId.Overlay)));
        }
    }

    private static string CreateTempRoot()
    {
        var root = Path.Combine(Path.GetTempPath(), "maa-unified-tests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        return root;
    }

    private static async Task WaitUntilAsync(Func<Task<bool>> condition, int timeoutMs = 2000)
    {
        var startedAt = Environment.TickCount64;
        while (!await condition())
        {
            if (Environment.TickCount64 - startedAt > timeoutMs)
            {
                throw new TimeoutException("Condition not reached in expected time.");
            }

            await Task.Delay(25);
        }
    }

    private sealed class FailingTrayService : ITrayService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "failing tray",
            Provider: "failing-tray",
            HasFallback: true,
            FallbackMode: "window-menu");

        public event EventHandler<TrayCommandEvent>? CommandInvoked;

        public Task<PlatformOperationResult> InitializeAsync(string appTitle, TrayMenuText? menuText, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.Failed(Capability.Provider, "initialize failed", PlatformErrorCodes.TrayInitFailed, "tray.initialize"));

        public Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "shutdown", "tray.shutdown"));

        public Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.Failed(Capability.Provider, "show failed", PlatformErrorCodes.TrayInitFailed, "tray.show"));

        public Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.Failed(Capability.Provider, "menu failed", PlatformErrorCodes.TrayInitFailed, "tray.setMenuState"));

        public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.Failed(Capability.Provider, "visible failed", PlatformErrorCodes.TrayInitFailed, "tray.setVisible"));
    }

    private sealed class FailingAutostartService : IAutostartService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "failing autostart",
            Provider: "failing-autostart",
            HasFallback: true,
            FallbackMode: "noop");

        public Task<PlatformOperationResult<bool>> IsEnabledAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(
                PlatformOperation.Failed<bool>(
                    Capability.Provider,
                    "query failed",
                    PlatformErrorCodes.AutostartQueryFailed,
                    "autostart.query",
                    value: false));

        public Task<PlatformOperationResult> SetEnabledAsync(bool enabled, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "set", "autostart.set"));
    }

    private sealed class ThrowingOverlayService : IOverlayCapabilityService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "throwing overlay",
            Provider: "throwing-overlay",
            HasFallback: true,
            FallbackMode: "preview");

        public event EventHandler<OverlayStateChangedEvent>? OverlayStateChanged;

        public Task<PlatformOperationResult> BindHostWindowAsync(
            nint hostWindowHandle,
            bool clickThrough,
            double opacity,
            CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "bound", "overlay.bind-host"));

        public Task<PlatformOperationResult<IReadOnlyList<OverlayTarget>>> QueryTargetsAsync(CancellationToken cancellationToken = default)
            => throw new InvalidOperationException("synthetic overlay query failure");

        public Task<PlatformOperationResult> SelectTargetAsync(string targetId, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "selected", "overlay.select-target"));

        public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "visible", "overlay.set-visible"));
    }

    private sealed class TriggerableOverlayService : IOverlayCapabilityService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "trigger overlay",
            Provider: "trigger-overlay",
            HasFallback: true,
            FallbackMode: "preview");

        public event EventHandler<OverlayStateChangedEvent>? OverlayStateChanged;

        public Task<PlatformOperationResult> BindHostWindowAsync(
            nint hostWindowHandle,
            bool clickThrough,
            double opacity,
            CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "bound", "overlay.bind-host"));

        public Task<PlatformOperationResult<IReadOnlyList<OverlayTarget>>> QueryTargetsAsync(CancellationToken cancellationToken = default)
        {
            IReadOnlyList<OverlayTarget> targets = [new OverlayTarget("preview", "Preview + Logs", true)];
            return Task.FromResult(PlatformOperation.FallbackSuccess(
                Capability.Provider,
                targets,
                "preview only",
                "overlay.query-targets",
                PlatformErrorCodes.OverlayPreviewMode));
        }

        public Task<PlatformOperationResult> SelectTargetAsync(string targetId, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "selected", "overlay.select-target"));

        public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, visible ? "visible" : "hidden", "overlay.set-visible"));

        public void Emit(OverlayStateChangedEvent e)
            => OverlayStateChanged?.Invoke(this, e);
    }

    private sealed class TriggerableTrayService : ITrayService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "trigger tray",
            Provider: "trigger-tray",
            HasFallback: true,
            FallbackMode: "window-menu");

        public event EventHandler<TrayCommandEvent>? CommandInvoked;

        public Task<PlatformOperationResult> InitializeAsync(string appTitle, TrayMenuText? menuText, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "init", "tray.initialize"));

        public Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "shutdown", "tray.shutdown"));

        public Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "show", "tray.show"));

        public Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "menu", "tray.set-menu"));

        public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "visible", "tray.set-visible"));

        public void Emit(TrayCommandId command, string source)
            => CommandInvoked?.Invoke(this, new TrayCommandEvent(command, source, DateTimeOffset.UtcNow));
    }

    private sealed class TriggerableHotkeyService : IGlobalHotkeyService
    {
        public PlatformCapabilityStatus Capability { get; } = new(
            Supported: true,
            Message: "trigger hotkey",
            Provider: "trigger-hotkey",
            HasFallback: true,
            FallbackMode: "window-scoped");

        public event EventHandler<GlobalHotkeyTriggeredEvent>? Triggered;

        public Task<PlatformOperationResult> RegisterAsync(string name, string gesture, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "register", "hotkey.register"));

        public Task<PlatformOperationResult> UnregisterAsync(string name, CancellationToken cancellationToken = default)
            => Task.FromResult(PlatformOperation.NativeSuccess(Capability.Provider, "unregister", "hotkey.unregister"));

        public void Emit(string name, string gesture)
            => Triggered?.Invoke(this, new GlobalHotkeyTriggeredEvent(name, gesture, DateTimeOffset.UtcNow));
    }
}
