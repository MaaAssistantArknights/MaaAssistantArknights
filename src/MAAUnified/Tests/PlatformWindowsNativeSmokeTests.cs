using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class PlatformWindowsNativeSmokeTests
{
    [Fact]
    public void WindowsHealthyEnvironment_ShouldDefaultToNativeProviders()
    {
        if (!OperatingSystem.IsWindows())
        {
            return;
        }

        var original = Environment.GetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK");
        try
        {
            Environment.SetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK", null);
            var bundle = PlatformServicesFactory.CreateDefaults();

            Assert.IsType<WindowsNotifyIconTrayService>(bundle.TrayService);
            Assert.IsType<DesktopNotificationService>(bundle.NotificationService);
            Assert.IsType<SharpHookGlobalHotkeyService>(bundle.HotkeyService);
            Assert.IsType<CrossPlatformAutostartService>(bundle.AutostartService);
            Assert.IsType<WindowsOverlayCapabilityService>(bundle.OverlayService);

            var snapshot = PlatformCapabilitySnapshotFactory.FromBundle(bundle);
            Assert.True(snapshot.Tray.Supported);
            Assert.Equal("desktop-notifications", snapshot.Notification.Provider);
            Assert.True(snapshot.Notification.HasFallback);
            Assert.True(snapshot.Hotkey.Supported);
            Assert.True(snapshot.Autostart.Supported);
            Assert.True(snapshot.Overlay.Supported);
        }
        finally
        {
            Environment.SetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK", original);
        }
    }

    [Fact]
    public void WindowsForcedFallback_ShouldStillExposeFallbackCapabilities_ExceptAutostart()
    {
        if (!OperatingSystem.IsWindows())
        {
            return;
        }

        var original = Environment.GetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK");
        try
        {
            Environment.SetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK", "1");
            var bundle = PlatformServicesFactory.CreateDefaults();

            Assert.IsType<WindowMenuTrayService>(bundle.TrayService);
            Assert.IsType<CommandNotificationService>(bundle.NotificationService);
            Assert.IsType<WindowScopedHotkeyService>(bundle.HotkeyService);
            Assert.IsType<CrossPlatformAutostartService>(bundle.AutostartService);
            Assert.IsType<NoOpOverlayCapabilityService>(bundle.OverlayService);

            var snapshot = PlatformCapabilitySnapshotFactory.FromBundle(bundle);
            Assert.True(snapshot.Tray.HasFallback);
            Assert.True(snapshot.Notification.HasFallback);
            Assert.True(snapshot.Hotkey.HasFallback);
            Assert.True(snapshot.Autostart.Supported);
            Assert.False(snapshot.Autostart.HasFallback);
            Assert.Equal("registry-run", snapshot.Autostart.Provider);
            Assert.True(snapshot.Overlay.HasFallback);
        }
        finally
        {
            Environment.SetEnvironmentVariable("MAA_PLATFORM_FORCE_FALLBACK", original);
        }
    }
}
