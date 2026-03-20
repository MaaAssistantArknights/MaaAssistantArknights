using System.Diagnostics;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using Avalonia;
using Avalonia.Controls;
using H.NotifyIcon.Core;
using Microsoft.Win32;
using SharpHook;
using SharpHook.Data;

namespace MAAUnified.Platform;

internal static class PlatformNativeDependencyProbe
{
    public static bool HasAssembly(string assemblyName)
    {
        try
        {
            _ = Assembly.Load(assemblyName);
            return true;
        }
        catch
        {
            return false;
        }
    }
}

internal static class HotkeyGestureNormalizer
{
    public static bool TryNormalize(string gesture, out string normalized)
    {
        normalized = string.Empty;
        if (string.IsNullOrWhiteSpace(gesture))
        {
            return false;
        }

        var parts = gesture.Split('+', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        if (parts.Length < 2)
        {
            return false;
        }

        var keyPart = parts[^1];
        if (keyPart.Length == 0)
        {
            return false;
        }

        var modifiers = parts[..^1]
            .Select(part => part.ToLowerInvariant())
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .OrderBy(part => part, StringComparer.OrdinalIgnoreCase)
            .ToList();

        if (modifiers.Any(mod => mod is not ("ctrl" or "control" or "shift" or "alt" or "meta" or "cmd" or "win")))
        {
            return false;
        }

        normalized = string.Join('+', modifiers.Append(keyPart.ToUpperInvariant()));
        return true;
    }
}

public sealed class WindowsNotifyIconTrayService : ITrayService
{
    private const nint DefaultAppIconId = 32512; // IDI_APPLICATION
    private readonly CommandNotificationService _notificationFallback = new();
    private readonly WindowMenuTrayService _fallbackService = new();
    private readonly Dictionary<TrayCommandId, PopupMenuItem> _menuItems = new();
    private nint _iconHandle;
    private bool _ownsIconHandle;
    private bool _visible = true;
    private bool _initialized;
    private bool _fallbackMode;
    private TrayMenuState _menuState = new(true, true, true, true, true);
    private TrayMenuText _menuText = TrayMenuText.Default;
    private TrayIconWithContextMenu? _trayIcon;

    public PlatformCapabilityStatus Capability => new(
        Supported: true,
        Message: "System tray integration is available via native notify icon backend.",
        Provider: "h-notifyicon",
        HasFallback: true,
        FallbackMode: "window-menu");

    public event EventHandler<TrayCommandEvent>? CommandInvoked;

    public WindowsNotifyIconTrayService()
    {
        _fallbackService.CommandInvoked += (_, e) => RaiseCommand(e.Command);
    }

    public static bool TryCreate([NotNullWhen(true)] out WindowsNotifyIconTrayService? service)
    {
        service = null;
        if (!OperatingSystem.IsWindows())
        {
            return false;
        }

        if (!PlatformNativeDependencyProbe.HasAssembly("H.NotifyIcon"))
        {
            return false;
        }

        service = new WindowsNotifyIconTrayService();
        return true;
    }

    public async Task<PlatformOperationResult> InitializeAsync(
        string appTitle,
        TrayMenuText? menuText,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _menuText = menuText ?? TrayMenuText.Default;

        if (_fallbackMode)
        {
            return await _fallbackService.InitializeAsync(appTitle, _menuText, cancellationToken);
        }

        if (_initialized)
        {
            RebuildMenu();
            ApplyMenuState();
            _trayIcon?.UpdateToolTip(string.IsNullOrWhiteSpace(appTitle) ? "MAAUnified" : appTitle.Trim());
            return PlatformOperation.NativeSuccess(
                Capability.Provider,
                "Tray service already initialized.",
                "tray.initialize");
        }

        try
        {
            _trayIcon = new TrayIconWithContextMenu("MAAUnified");
            RebuildMenu();
            _trayIcon.UpdateToolTip(string.IsNullOrWhiteSpace(appTitle) ? "MAAUnified" : appTitle.Trim());
            _iconHandle = LoadTrayIconHandle(out _ownsIconHandle);
            _trayIcon.UpdateIcon(_iconHandle);
            _trayIcon.UpdateVisibility(_visible ? IconVisibility.Visible : IconVisibility.Hidden);
            _trayIcon.Create();
            _initialized = true;
            _fallbackMode = false;
            ApplyMenuState();
            return PlatformOperation.NativeSuccess(
                Capability.Provider,
                "Tray service initialized.",
                "tray.initialize");
        }
        catch (Exception ex)
        {
            _trayIcon?.Dispose();
            _trayIcon = null;
            ReleaseIconHandle();
            _initialized = false;
            _fallbackMode = true;
            var fallbackResult = await _fallbackService.InitializeAsync(appTitle, _menuText, cancellationToken);
            if (fallbackResult.Success)
            {
                return PlatformOperation.FallbackSuccess(
                    Capability.Provider,
                    $"Native tray initialization failed and switched to fallback menu: {ex.Message}",
                    "tray.initialize",
                    PlatformErrorCodes.TrayFallback);
            }

            return PlatformOperation.Failed(
                Capability.Provider,
                $"Failed to initialize tray service: {ex.Message}",
                PlatformErrorCodes.TrayInitFailed,
                "tray.initialize");
        }
    }

    public Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_fallbackMode)
        {
            return _fallbackService.ShutdownAsync(cancellationToken);
        }

        try
        {
            _trayIcon?.TryRemove();
            _trayIcon?.Dispose();
            _trayIcon = null;
            ReleaseIconHandle();
            _initialized = false;
            _fallbackMode = false;
            return Task.FromResult(PlatformOperation.NativeSuccess(
                Capability.Provider,
                "Tray service shutdown completed.",
                "tray.shutdown"));
        }
        catch (Exception ex)
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                $"Tray shutdown failed: {ex.Message}",
                PlatformErrorCodes.TrayInitFailed,
                "tray.shutdown"));
        }
    }

    public async Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_fallbackMode)
        {
            return await _fallbackService.ShowAsync(title, message, cancellationToken);
        }

        if (_trayIcon is not null && _initialized)
        {
            try
            {
                _trayIcon.ShowNotification(
                    title,
                    message,
                    NotificationIcon.None,
                    customIconHandle: null,
                    largeIcon: false,
                    sound: true,
                    respectQuietTime: true,
                    realtime: false,
                    timeout: TimeSpan.FromSeconds(8));

                return PlatformOperation.NativeSuccess(
                    Capability.Provider,
                    "Tray notification dispatched.",
                    "tray.show");
            }
            catch
            {
                // fall through to notification fallback
            }
        }

        var fallback = await _notificationFallback.NotifyAsync(title, message, cancellationToken);
        if (!fallback.Success)
        {
            return PlatformOperation.Failed(
                Capability.Provider,
                "Tray notification failed and fallback could not be delivered.",
                PlatformErrorCodes.TrayFallback,
                "tray.show",
                usedFallback: true);
        }

        return PlatformOperation.FallbackSuccess(
            Capability.Provider,
            "Tray notification fallback delivered via notification service.",
            "tray.show",
            PlatformErrorCodes.TrayFallback);
    }

    public Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_fallbackMode)
        {
            return _fallbackService.SetMenuStateAsync(state, cancellationToken);
        }

        _menuState = state;
        if (_initialized)
        {
            ApplyMenuState();
        }

        return Task.FromResult(PlatformOperation.NativeSuccess(
            Capability.Provider,
            "Tray menu state updated.",
            "tray.setMenuState"));
    }

    public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_fallbackMode)
        {
            return _fallbackService.SetVisibleAsync(visible, cancellationToken);
        }

        _visible = visible;
        if (_trayIcon is null || !_initialized)
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Tray service is not initialized.",
                PlatformErrorCodes.TrayNotInitialized,
                "tray.setVisible"));
        }

        try
        {
            _trayIcon.UpdateVisibility(visible ? IconVisibility.Visible : IconVisibility.Hidden);
            if (visible)
            {
                _trayIcon.Show();
            }
            else
            {
                _trayIcon.Hide();
            }

            return Task.FromResult(PlatformOperation.NativeSuccess(
                Capability.Provider,
                $"Tray visibility set to {visible}.",
                "tray.setVisible"));
        }
        catch (Exception ex)
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                $"Tray visibility update failed: {ex.Message}",
                PlatformErrorCodes.TrayMenuDispatchFailed,
                "tray.setVisible"));
        }
    }

    private PopupMenu BuildMenu()
    {
        var menu = new PopupMenu();
        menu.Items.Add(CreateMenuItem(TrayCommandId.Start, _menuText.Start));
        menu.Items.Add(CreateMenuItem(TrayCommandId.Stop, _menuText.Stop));
        menu.Items.Add(new PopupMenuSeparator());
        menu.Items.Add(CreateMenuItem(TrayCommandId.ForceShow, _menuText.ForceShow));
        menu.Items.Add(CreateMenuItem(TrayCommandId.HideTray, _menuText.HideTray));
        menu.Items.Add(CreateMenuItem(TrayCommandId.ToggleOverlay, _menuText.ToggleOverlay));
        menu.Items.Add(CreateMenuItem(TrayCommandId.SwitchLanguage, _menuText.SwitchLanguage));
        menu.Items.Add(CreateMenuItem(TrayCommandId.Restart, _menuText.Restart));
        menu.Items.Add(new PopupMenuSeparator());
        menu.Items.Add(CreateMenuItem(TrayCommandId.Exit, _menuText.Exit));
        return menu;
    }

    private void RebuildMenu()
    {
        _menuItems.Clear();
        if (_trayIcon is not null)
        {
            _trayIcon.ContextMenu = BuildMenu();
        }
    }

    private PopupMenuItem CreateMenuItem(TrayCommandId command, string text)
    {
        var item = new PopupMenuItem(text, (_, _) => RaiseCommand(command));
        _menuItems[command] = item;
        return item;
    }

    private void ApplyMenuState()
    {
        SetEnabled(TrayCommandId.Start, _menuState.StartEnabled);
        SetEnabled(TrayCommandId.Stop, _menuState.StopEnabled);
        SetEnabled(TrayCommandId.ToggleOverlay, _menuState.OverlayEnabled);
        SetEnabled(TrayCommandId.ForceShow, _menuState.ForceShowEnabled);
        SetEnabled(TrayCommandId.HideTray, _menuState.HideTrayEnabled);
        SetEnabled(TrayCommandId.SwitchLanguage, true);
        SetEnabled(TrayCommandId.Restart, true);
        SetEnabled(TrayCommandId.Exit, true);
    }

    private void SetEnabled(TrayCommandId command, bool enabled)
    {
        if (_menuItems.TryGetValue(command, out var item))
        {
            item.Enabled = enabled;
        }
    }

    private void RaiseCommand(TrayCommandId command)
    {
        try
        {
            CommandInvoked?.Invoke(this, new TrayCommandEvent(command, Capability.Provider, DateTimeOffset.UtcNow));
        }
        catch
        {
            // Ignore user event exceptions to avoid destabilizing tray message loop.
        }
    }

    [DllImport("user32.dll")]
    private static extern nint LoadIcon(nint hInstance, nint lpIconName);

    [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
    private static extern uint ExtractIconEx(string lpszFile, int nIconIndex, nint[]? phiconLarge, nint[]? phiconSmall, uint nIcons);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool DestroyIcon(nint hIcon);

    private static nint LoadTrayIconHandle(out bool ownsHandle)
    {
        ownsHandle = false;
        var processPath = Environment.ProcessPath;
        if (!string.IsNullOrWhiteSpace(processPath))
        {
            var largeIcons = new nint[1];
            try
            {
                var extracted = ExtractIconEx(processPath, 0, largeIcons, null, 1);
                if (extracted > 0 && largeIcons[0] != nint.Zero)
                {
                    ownsHandle = true;
                    return largeIcons[0];
                }
            }
            catch
            {
                // Fall back to the default application icon when icon extraction fails.
            }
        }

        return LoadIcon(nint.Zero, DefaultAppIconId);
    }

    private void ReleaseIconHandle()
    {
        if (_ownsIconHandle && _iconHandle != nint.Zero)
        {
            _ = DestroyIcon(_iconHandle);
        }

        _iconHandle = nint.Zero;
        _ownsIconHandle = false;
    }
}

public sealed class AvaloniaTrayIconTrayService : ITrayService, IDisposable
{
    // 1x1 transparent PNG used when no app icon is available.
    private const string DefaultPngBase64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO7+N3EAAAAASUVORK5CYII=";

    private readonly CommandNotificationService _notificationFallback = new();
    private readonly WindowMenuTrayService _fallbackService = new();
    private readonly Dictionary<TrayCommandId, NativeMenuItem> _menuItems = new();
    private bool _visible = true;
    private bool _initialized;
    private bool _fallbackMode;
    private TrayMenuState _menuState = new(true, true, true, true, true);
    private TrayMenuText _menuText = TrayMenuText.Default;
    private Avalonia.Controls.TrayIcon? _trayIcon;
    private Avalonia.Controls.TrayIcons? _trayIcons;

    public PlatformCapabilityStatus Capability => new(
        Supported: true,
        Message: "System tray integration is available via Avalonia tray icon backend.",
        Provider: "avalonia-trayicon",
        HasFallback: true,
        FallbackMode: "window-menu");

    public event EventHandler<TrayCommandEvent>? CommandInvoked;

    public AvaloniaTrayIconTrayService()
    {
        _fallbackService.CommandInvoked += (_, e) => RaiseCommand(e.Command);
    }

    public static bool TryCreate([NotNullWhen(true)] out AvaloniaTrayIconTrayService? service)
    {
        service = null;
        if (!(OperatingSystem.IsLinux() || OperatingSystem.IsMacOS()))
        {
            return false;
        }

        if (!PlatformNativeDependencyProbe.HasAssembly("Avalonia.Controls"))
        {
            return false;
        }

        service = new AvaloniaTrayIconTrayService();
        return true;
    }

    public async Task<PlatformOperationResult> InitializeAsync(
        string appTitle,
        TrayMenuText? menuText,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _menuText = menuText ?? TrayMenuText.Default;

        if (_fallbackMode)
        {
            return await _fallbackService.InitializeAsync(appTitle, _menuText, cancellationToken);
        }

        if (_initialized && _trayIcon is not null)
        {
            _trayIcon.ToolTipText = string.IsNullOrWhiteSpace(appTitle) ? "MAAUnified" : appTitle.Trim();
            RebuildMenu();
            ApplyMenuState();
            _trayIcon.IsVisible = _visible;
            return PlatformOperation.NativeSuccess(
                Capability.Provider,
                "Tray service already initialized.",
                "tray.initialize");
        }

        try
        {
            if (Application.Current is null)
            {
                throw new InvalidOperationException("Avalonia application is not initialized.");
            }

            _trayIcon = new Avalonia.Controls.TrayIcon
            {
                ToolTipText = string.IsNullOrWhiteSpace(appTitle) ? "MAAUnified" : appTitle.Trim(),
                IsVisible = _visible,
                Icon = BuildDefaultIcon(),
            };
            RebuildMenu();
            _trayIcons = new Avalonia.Controls.TrayIcons
            {
                _trayIcon,
            };
            Application.Current.SetValue(Avalonia.Controls.TrayIcon.IconsProperty, _trayIcons);

            _initialized = true;
            _fallbackMode = false;
            ApplyMenuState();
            return PlatformOperation.NativeSuccess(
                Capability.Provider,
                "Tray service initialized.",
                "tray.initialize");
        }
        catch (Exception ex)
        {
            DisposeNative();
            _initialized = false;
            _fallbackMode = true;
            var fallbackResult = await _fallbackService.InitializeAsync(appTitle, _menuText, cancellationToken);
            if (fallbackResult.Success)
            {
                return PlatformOperation.FallbackSuccess(
                    Capability.Provider,
                    $"Native tray initialization failed and switched to fallback menu: {ex.Message}",
                    "tray.initialize",
                    PlatformErrorCodes.TrayFallback);
            }

            return PlatformOperation.Failed(
                Capability.Provider,
                $"Failed to initialize tray service: {ex.Message}",
                PlatformErrorCodes.TrayInitFailed,
                "tray.initialize");
        }
    }

    public Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_fallbackMode)
        {
            return _fallbackService.ShutdownAsync(cancellationToken);
        }

        try
        {
            DisposeNative();
            _initialized = false;
            _fallbackMode = false;
            return Task.FromResult(PlatformOperation.NativeSuccess(
                Capability.Provider,
                "Tray service shutdown completed.",
                "tray.shutdown"));
        }
        catch (Exception ex)
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                $"Tray shutdown failed: {ex.Message}",
                PlatformErrorCodes.TrayInitFailed,
                "tray.shutdown"));
        }
    }

    public async Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_fallbackMode)
        {
            return await _fallbackService.ShowAsync(title, message, cancellationToken);
        }

        var fallback = await _notificationFallback.NotifyAsync(title, message, cancellationToken);
        if (!fallback.Success)
        {
            return PlatformOperation.Failed(
                Capability.Provider,
                "Tray notification fallback could not be delivered.",
                PlatformErrorCodes.TrayFallback,
                "tray.show",
                usedFallback: true);
        }

        return PlatformOperation.FallbackSuccess(
            Capability.Provider,
            "Tray notification fallback delivered via notification service.",
            "tray.show",
            PlatformErrorCodes.TrayFallback);
    }

    public Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_fallbackMode)
        {
            return _fallbackService.SetMenuStateAsync(state, cancellationToken);
        }

        _menuState = state;
        if (_initialized)
        {
            ApplyMenuState();
        }

        return Task.FromResult(PlatformOperation.NativeSuccess(
            Capability.Provider,
            "Tray menu state updated.",
            "tray.setMenuState"));
    }

    public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_fallbackMode)
        {
            return _fallbackService.SetVisibleAsync(visible, cancellationToken);
        }

        _visible = visible;
        if (_trayIcon is null || !_initialized)
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Tray service is not initialized.",
                PlatformErrorCodes.TrayNotInitialized,
                "tray.setVisible"));
        }

        try
        {
            _trayIcon.IsVisible = visible;
            return Task.FromResult(PlatformOperation.NativeSuccess(
                Capability.Provider,
                $"Tray visibility set to {visible}.",
                "tray.setVisible"));
        }
        catch (Exception ex)
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                $"Tray visibility update failed: {ex.Message}",
                PlatformErrorCodes.TrayMenuDispatchFailed,
                "tray.setVisible"));
        }
    }

    public void Dispose()
    {
        DisposeNative();
    }

    private void RebuildMenu()
    {
        _menuItems.Clear();
        if (_trayIcon is null)
        {
            return;
        }

        var menu = new NativeMenu();
        menu.Items.Add(CreateMenuItem(TrayCommandId.Start, _menuText.Start));
        menu.Items.Add(CreateMenuItem(TrayCommandId.Stop, _menuText.Stop));
        menu.Items.Add(new NativeMenuItemSeparator());
        menu.Items.Add(CreateMenuItem(TrayCommandId.ForceShow, _menuText.ForceShow));
        menu.Items.Add(CreateMenuItem(TrayCommandId.HideTray, _menuText.HideTray));
        menu.Items.Add(CreateMenuItem(TrayCommandId.ToggleOverlay, _menuText.ToggleOverlay));
        menu.Items.Add(CreateMenuItem(TrayCommandId.SwitchLanguage, _menuText.SwitchLanguage));
        menu.Items.Add(CreateMenuItem(TrayCommandId.Restart, _menuText.Restart));
        menu.Items.Add(new NativeMenuItemSeparator());
        menu.Items.Add(CreateMenuItem(TrayCommandId.Exit, _menuText.Exit));
        _trayIcon.Menu = menu;
    }

    private NativeMenuItem CreateMenuItem(TrayCommandId command, string text)
    {
        var item = new NativeMenuItem(text);
        item.Click += (_, _) => RaiseCommand(command);
        _menuItems[command] = item;
        return item;
    }

    private void ApplyMenuState()
    {
        SetEnabled(TrayCommandId.Start, _menuState.StartEnabled);
        SetEnabled(TrayCommandId.Stop, _menuState.StopEnabled);
        SetEnabled(TrayCommandId.ToggleOverlay, _menuState.OverlayEnabled);
        SetEnabled(TrayCommandId.ForceShow, _menuState.ForceShowEnabled);
        SetEnabled(TrayCommandId.HideTray, _menuState.HideTrayEnabled);
        SetEnabled(TrayCommandId.SwitchLanguage, true);
        SetEnabled(TrayCommandId.Restart, true);
        SetEnabled(TrayCommandId.Exit, true);
    }

    private void SetEnabled(TrayCommandId command, bool enabled)
    {
        if (_menuItems.TryGetValue(command, out var item))
        {
            item.IsEnabled = enabled;
        }
    }

    private void RaiseCommand(TrayCommandId command)
    {
        try
        {
            CommandInvoked?.Invoke(this, new TrayCommandEvent(command, Capability.Provider, DateTimeOffset.UtcNow));
        }
        catch
        {
            // Ignore user event exceptions to avoid destabilizing tray message loop.
        }
    }

    private static WindowIcon BuildDefaultIcon()
    {
        var stream = new MemoryStream(Convert.FromBase64String(DefaultPngBase64));
        return new WindowIcon(stream);
    }

    private void DisposeNative()
    {
        if (Application.Current is not null)
        {
            Application.Current.SetValue(Avalonia.Controls.TrayIcon.IconsProperty, null);
        }

        _trayIcon?.Dispose();
        _trayIcon = null;
        _trayIcons?.Clear();
        _trayIcons = null;
    }
}

public sealed class DesktopNotificationService : INotificationService
{
    private readonly CommandNotificationService _fallback = new();
    private readonly DesktopNotificationAdapter? _nativeAdapter;

    public DesktopNotificationService()
    {
        _nativeAdapter = DesktopNotificationAdapter.TryCreate("MaaAssistantArknights");
    }

    public PlatformCapabilityStatus Capability => _nativeAdapter is null
        ? new PlatformCapabilityStatus(
            Supported: false,
            Message: "DesktopNotifications backend is unavailable, using command/in-app fallback.",
            Provider: "desktop-notifications",
            HasFallback: true,
            FallbackMode: "command-or-in-app")
        : new PlatformCapabilityStatus(
            Supported: true,
            Message: "System notification uses DesktopNotifications backend.",
            Provider: "desktop-notifications",
            HasFallback: true,
            FallbackMode: "command-or-in-app");

    public static bool TryCreate([NotNullWhen(true)] out DesktopNotificationService? service)
    {
        service = null;
        if (!(OperatingSystem.IsWindows() || OperatingSystem.IsLinux() || OperatingSystem.IsMacOS()))
        {
            return false;
        }

        if (!PlatformNativeDependencyProbe.HasAssembly("DesktopNotifications"))
        {
            return false;
        }

        service = new DesktopNotificationService();
        return true;
    }

    public async Task<PlatformOperationResult> NotifyAsync(string title, string message, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        try
        {
            if (_nativeAdapter is null)
            {
                return await NotifyFallbackAsync(title, message, "Native notification backend is unavailable.", cancellationToken);
            }

            await _nativeAdapter.NotifyAsync(title, message, cancellationToken);
            return PlatformOperation.NativeSuccess(Capability.Provider, "System notification dispatched.", "notification.notify");
        }
        catch (Exception ex)
        {
            return await NotifyFallbackAsync(
                title,
                message,
                $"Native notification dispatch failed: {ex.Message}",
                cancellationToken);
        }
    }

    private async Task<PlatformOperationResult> NotifyFallbackAsync(
        string title,
        string message,
        string reason,
        CancellationToken cancellationToken)
    {
        var fallbackResult = await _fallback.NotifyAsync(title, message, cancellationToken);
        if (!fallbackResult.Success)
        {
            return PlatformOperation.Failed(
                Capability.Provider,
                $"{reason} Fallback notification also failed.",
                PlatformErrorCodes.NotificationSendFailed,
                "notification.notify",
                usedFallback: true);
        }

        return PlatformOperation.FallbackSuccess(
            Capability.Provider,
            $"{reason} Switched to fallback notification provider.",
            "notification.notify",
            PlatformErrorCodes.NotificationFallback);
    }

    private sealed class DesktopNotificationAdapter
    {
        private readonly object _manager;
        private readonly MethodInfo _showMethod;
        private readonly Type _notificationType;
        private readonly ConstructorInfo? _notificationCtor;

        private DesktopNotificationAdapter(
            object manager,
            MethodInfo showMethod,
            Type notificationType,
            ConstructorInfo? notificationCtor)
        {
            _manager = manager;
            _showMethod = showMethod;
            _notificationType = notificationType;
            _notificationCtor = notificationCtor;
        }

        public static DesktopNotificationAdapter? TryCreate(string appName)
        {
            try
            {
                var assembly = Assembly.Load("DesktopNotifications");
                var managerType = assembly.GetType("DesktopNotifications.NotificationManager");
                var notificationType = assembly.GetType("DesktopNotifications.Notification");
                if (managerType is null || notificationType is null)
                {
                    return null;
                }

                var manager = CreateManagerInstance(managerType, assembly, appName);
                if (manager is null)
                {
                    return null;
                }

                var showMethod = managerType
                    .GetMethods(BindingFlags.Instance | BindingFlags.Public)
                    .FirstOrDefault(m =>
                    {
                        if (!string.Equals(m.Name, "ShowNotification", StringComparison.Ordinal))
                        {
                            return false;
                        }

                        var parameters = m.GetParameters();
                        return parameters.Length == 1 && parameters[0].ParameterType.IsAssignableFrom(notificationType);
                    });

                if (showMethod is null)
                {
                    return null;
                }

                var ctor = notificationType.GetConstructors(BindingFlags.Instance | BindingFlags.Public)
                    .OrderBy(c => c.GetParameters().Length)
                    .FirstOrDefault();

                return new DesktopNotificationAdapter(manager, showMethod, notificationType, ctor);
            }
            catch
            {
                return null;
            }
        }

        public async Task NotifyAsync(string title, string message, CancellationToken cancellationToken)
        {
            var notification = CreateNotification(title, message);
            var result = _showMethod.Invoke(_manager, new[] { notification });
            if (result is Task task)
            {
                await task.WaitAsync(cancellationToken);
            }
        }

        private object CreateNotification(string title, string message)
        {
            object notification;
            if (_notificationCtor is not null)
            {
                var args = _notificationCtor
                    .GetParameters()
                    .Select(p => BuildConstructorArgument(p, title, message))
                    .ToArray();
                notification = _notificationCtor.Invoke(args);
            }
            else
            {
                notification = Activator.CreateInstance(_notificationType)
                    ?? throw new InvalidOperationException("Cannot create notification object.");
            }

            SetProperty(notification, "Title", title);
            SetProperty(notification, "Body", message);
            return notification;
        }

        private static object? CreateManagerInstance(Type managerType, Assembly assembly, string appName)
        {
            foreach (var ctor in managerType.GetConstructors(BindingFlags.Instance | BindingFlags.Public)
                         .OrderBy(c => c.GetParameters().Length))
            {
                try
                {
                    var args = ctor.GetParameters()
                        .Select(p => BuildManagerArgument(assembly, p, appName))
                        .ToArray();
                    return ctor.Invoke(args);
                }
                catch
                {
                    // try next constructor
                }
            }

            return null;
        }

        private static object? BuildManagerArgument(Assembly assembly, ParameterInfo parameter, string appName)
        {
            if (parameter.ParameterType == typeof(string))
            {
                return appName;
            }

            if (parameter.ParameterType == typeof(bool))
            {
                return false;
            }

            if (parameter.ParameterType == typeof(int))
            {
                return 0;
            }

            if (parameter.ParameterType == typeof(DateTimeOffset))
            {
                return DateTimeOffset.UtcNow;
            }

            var fullName = parameter.ParameterType.FullName ?? string.Empty;
            if (string.Equals(fullName, "DesktopNotifications.Application", StringComparison.Ordinal))
            {
                return CreateByName(assembly, fullName, appName);
            }

            if (string.Equals(fullName, "DesktopNotifications.ApplicationContext", StringComparison.Ordinal))
            {
                var app = CreateByName(assembly, "DesktopNotifications.Application", appName);
                return CreateByName(assembly, fullName, app);
            }

            return parameter.HasDefaultValue
                ? parameter.DefaultValue
                : parameter.ParameterType.IsValueType
                    ? Activator.CreateInstance(parameter.ParameterType)
                    : null;
        }

        private static object? BuildConstructorArgument(ParameterInfo parameter, string title, string message)
        {
            if (parameter.ParameterType == typeof(string))
            {
                if (parameter.Name?.Contains("title", StringComparison.OrdinalIgnoreCase) == true)
                {
                    return title;
                }

                return message;
            }

            if (parameter.ParameterType == typeof(DateTimeOffset))
            {
                return DateTimeOffset.UtcNow;
            }

            if (parameter.ParameterType == typeof(bool))
            {
                return false;
            }

            if (parameter.ParameterType == typeof(int))
            {
                return 0;
            }

            if (parameter.ParameterType == typeof(byte))
            {
                return (byte)0;
            }

            return parameter.HasDefaultValue
                ? parameter.DefaultValue
                : parameter.ParameterType.IsValueType
                    ? Activator.CreateInstance(parameter.ParameterType)
                    : null;
        }

        private static object? CreateByName(Assembly assembly, string typeName, params object?[] args)
        {
            var type = assembly.GetType(typeName);
            if (type is null)
            {
                return null;
            }

            foreach (var ctor in type.GetConstructors(BindingFlags.Instance | BindingFlags.Public)
                         .OrderBy(c => c.GetParameters().Length))
            {
                try
                {
                    var ctorParams = ctor.GetParameters();
                    if (ctorParams.Length == 0)
                    {
                        return ctor.Invoke(null);
                    }

                    var values = new object?[ctorParams.Length];
                    for (var i = 0; i < ctorParams.Length; i++)
                    {
                        values[i] = i < args.Length
                            ? args[i]
                            : ctorParams[i].HasDefaultValue
                                ? ctorParams[i].DefaultValue
                                : ctorParams[i].ParameterType.IsValueType
                                    ? Activator.CreateInstance(ctorParams[i].ParameterType)
                                    : null;
                    }

                    return ctor.Invoke(values);
                }
                catch
                {
                    // try next constructor
                }
            }

            return null;
        }

        private static void SetProperty(object target, string propertyName, string value)
        {
            var property = target.GetType().GetProperty(propertyName, BindingFlags.Instance | BindingFlags.Public);
            if (property?.CanWrite == true && property.PropertyType == typeof(string))
            {
                property.SetValue(target, value);
            }
        }
    }
}

public sealed class SharpHookGlobalHotkeyService : IGlobalHotkeyService, IDisposable
{
    private readonly object _syncRoot = new();
    private readonly Dictionary<string, RegisteredHotkey> _registeredByName = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, string> _registeredByChord = new(StringComparer.OrdinalIgnoreCase);
    private EventLoopGlobalHook? _hook;
    private Task? _hookTask;

    public PlatformCapabilityStatus Capability => new(
        Supported: true,
        Message: "Global hotkeys are available via SharpHook backend.",
        Provider: "sharp-hook",
        HasFallback: true,
        FallbackMode: "window-scoped");

    public event EventHandler<GlobalHotkeyTriggeredEvent>? Triggered;

    public static bool TryCreate([NotNullWhen(true)] out SharpHookGlobalHotkeyService? service)
    {
        service = null;
        if (!(OperatingSystem.IsWindows() || OperatingSystem.IsLinux() || OperatingSystem.IsMacOS()))
        {
            return false;
        }

        if (!PlatformNativeDependencyProbe.HasAssembly("SharpHook"))
        {
            return false;
        }

        service = new SharpHookGlobalHotkeyService();
        return true;
    }

    public Task<PlatformOperationResult> RegisterAsync(string name, string gesture, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(name))
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Hotkey name cannot be empty.",
                PlatformErrorCodes.HotkeyNameMissing,
                "hotkey.register"));
        }

        if (!TryParseGesture(gesture, out var binding))
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Invalid hotkey gesture format.",
                PlatformErrorCodes.HotkeyInvalidGesture,
                "hotkey.register"));
        }

        lock (_syncRoot)
        {
            if (_registeredByName.TryGetValue(name, out var previous))
            {
                _registeredByChord.Remove(previous.ChordKey);
            }

            if (_registeredByChord.TryGetValue(binding.ChordKey, out var existingName)
                && !string.Equals(existingName, name, StringComparison.OrdinalIgnoreCase))
            {
                return Task.FromResult(PlatformOperation.Failed(
                    Capability.Provider,
                    "Hotkey gesture already in use.",
                    PlatformErrorCodes.HotkeyConflict,
                    "hotkey.register"));
            }

            _registeredByName[name] = binding with { Name = name };
            _registeredByChord[binding.ChordKey] = name;
        }

        var hookResult = EnsureHookStarted();
        if (!hookResult.Success)
        {
            lock (_syncRoot)
            {
                if (_registeredByName.TryGetValue(name, out var rollback) && rollback.ChordKey == binding.ChordKey)
                {
                    _registeredByName.Remove(name);
                    _registeredByChord.Remove(binding.ChordKey);
                }
            }

            return Task.FromResult(hookResult);
        }

        return Task.FromResult(PlatformOperation.NativeSuccess(
            Capability.Provider,
            $"Global hotkey registered: {name} => {binding.NormalizedGesture}",
            "hotkey.register"));
    }

    public Task<PlatformOperationResult> UnregisterAsync(string name, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(name))
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Hotkey name cannot be empty.",
                PlatformErrorCodes.HotkeyNameMissing,
                "hotkey.unregister"));
        }

        bool removed;
        lock (_syncRoot)
        {
            removed = _registeredByName.Remove(name, out var existing);
            if (removed)
            {
                _registeredByChord.Remove(existing.ChordKey);
            }
        }

        if (!removed)
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                $"Hotkey `{name}` was not registered.",
                PlatformErrorCodes.HotkeyNotFound,
                "hotkey.unregister"));
        }

        StopHookIfIdle();
        return Task.FromResult(PlatformOperation.NativeSuccess(
            Capability.Provider,
            $"Global hotkey unregistered: {name}",
            "hotkey.unregister"));
    }

    public void Dispose()
    {
        lock (_syncRoot)
        {
            StopHookUnsafe();
            _registeredByName.Clear();
            _registeredByChord.Clear();
        }
    }

    private PlatformOperationResult EnsureHookStarted()
    {
        lock (_syncRoot)
        {
            if (_hook is not null)
            {
                return PlatformOperation.NativeSuccess(Capability.Provider, "Global hook already running.", "hotkey.hook");
            }

            try
            {
                _hook = new EventLoopGlobalHook(GlobalHookType.Keyboard);
                _hook.KeyPressed += OnHookKeyPressed;
                _hookTask = _hook.RunAsync();
                _hookTask.ContinueWith(OnHookCompleted, TaskScheduler.Default);
                return PlatformOperation.NativeSuccess(Capability.Provider, "Global hook started.", "hotkey.hook");
            }
            catch (Exception ex) when (ex is HookException || ex is DllNotFoundException || ex is PlatformNotSupportedException)
            {
                StopHookUnsafe();
                var errorCode = ex.Message.Contains("permission", StringComparison.OrdinalIgnoreCase)
                                || ex.Message.Contains("access", StringComparison.OrdinalIgnoreCase)
                    ? PlatformErrorCodes.HotkeyPermissionDenied
                    : PlatformErrorCodes.HotkeyHookStartFailed;
                return PlatformOperation.Failed(
                    Capability.Provider,
                    $"Failed to start global hotkey hook: {ex.Message}",
                    errorCode,
                    "hotkey.hook");
            }
        }
    }

    private void StopHookIfIdle()
    {
        lock (_syncRoot)
        {
            if (_registeredByName.Count == 0)
            {
                StopHookUnsafe();
            }
        }
    }

    private void StopHookUnsafe()
    {
        try
        {
            if (_hook is not null)
            {
                _hook.KeyPressed -= OnHookKeyPressed;
                _hook.Stop();
                _hook.Dispose();
            }
        }
        catch
        {
            // ignored
        }
        finally
        {
            _hook = null;
            _hookTask = null;
        }
    }

    private void OnHookCompleted(Task hookTask)
    {
        if (hookTask.IsFaulted)
        {
            lock (_syncRoot)
            {
                StopHookUnsafe();
            }
        }
    }

    private void OnHookKeyPressed(object? sender, KeyboardHookEventArgs e)
    {
        RegisteredHotkey? matched = null;
        lock (_syncRoot)
        {
            foreach (var item in _registeredByName.Values)
            {
                if (IsMatch(item, e))
                {
                    matched = item;
                    break;
                }
            }
        }

        if (!matched.HasValue)
        {
            return;
        }

        var hotkey = matched.Value;
        try
        {
            Triggered?.Invoke(this, new GlobalHotkeyTriggeredEvent(
                hotkey.Name,
                hotkey.NormalizedGesture,
                DateTimeOffset.UtcNow));
        }
        catch
        {
            // Ignore callback errors to keep hook thread alive.
        }
    }

    private static bool IsMatch(RegisteredHotkey hotkey, KeyboardHookEventArgs args)
    {
        if (args.Data.KeyCode != hotkey.KeyCode)
        {
            return false;
        }

        var mask = args.RawEvent.Mask;
        var ctrl = mask.HasFlag(EventMask.Ctrl);
        var shift = mask.HasFlag(EventMask.Shift);
        var alt = mask.HasFlag(EventMask.Alt);
        var meta = mask.HasFlag(EventMask.Meta);
        return ctrl == hotkey.Ctrl
               && shift == hotkey.Shift
               && alt == hotkey.Alt
               && meta == hotkey.Meta;
    }

    private static bool TryParseGesture(string gesture, out RegisteredHotkey hotkey)
    {
        hotkey = default;
        if (!HotkeyGestureNormalizer.TryNormalize(gesture, out var normalized))
        {
            return false;
        }

        var tokens = normalized.Split('+', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        if (tokens.Length < 2)
        {
            return false;
        }

        var keyToken = tokens[^1];
        if (!TryParseKeyCode(keyToken, out var keyCode))
        {
            return false;
        }

        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        bool meta = false;

        foreach (var modifier in tokens[..^1])
        {
            switch (modifier.ToLowerInvariant())
            {
                case "ctrl":
                case "control":
                    ctrl = true;
                    break;
                case "shift":
                    shift = true;
                    break;
                case "alt":
                    alt = true;
                    break;
                case "meta":
                case "cmd":
                case "win":
                    meta = true;
                    break;
                default:
                    return false;
            }
        }

        var chord = $"{(int)keyCode}:{ctrl}:{shift}:{alt}:{meta}";
        hotkey = new RegisteredHotkey(string.Empty, normalized, keyCode, ctrl, shift, alt, meta, chord);
        return true;
    }

    private static bool TryParseKeyCode(string token, out KeyCode keyCode)
    {
        keyCode = KeyCode.VcUndefined;
        if (string.IsNullOrWhiteSpace(token))
        {
            return false;
        }

        var upper = token.Trim().ToUpperInvariant();
        if (upper.Length == 1 && upper[0] is >= 'A' and <= 'Z')
        {
            return Enum.TryParse($"Vc{upper}", out keyCode);
        }

        if (upper.Length == 1 && upper[0] is >= '0' and <= '9')
        {
            return Enum.TryParse($"Vc{upper}", out keyCode);
        }

        if (upper.StartsWith("F", StringComparison.Ordinal)
            && int.TryParse(upper[1..], out var fn)
            && fn is >= 1 and <= 24)
        {
            return Enum.TryParse($"VcF{fn}", out keyCode);
        }

        return upper switch
        {
            "ENTER" => Enum.TryParse("VcEnter", out keyCode),
            "TAB" => Enum.TryParse("VcTab", out keyCode),
            "SPACE" => Enum.TryParse("VcSpace", out keyCode),
            "ESC" or "ESCAPE" => Enum.TryParse("VcEscape", out keyCode),
            _ => false,
        };
    }

    private readonly record struct RegisteredHotkey(
        string Name,
        string NormalizedGesture,
        KeyCode KeyCode,
        bool Ctrl,
        bool Shift,
        bool Alt,
        bool Meta,
        string ChordKey);
}

public sealed class WindowMenuTrayService : ITrayService
{
    private bool _visible = true;
    private TrayMenuState _menuState = new(true, true, true, true, true);

    public event EventHandler<TrayCommandEvent>? CommandInvoked;

    public PlatformCapabilityStatus Capability => new(
        Supported: false,
        Message: "System tray native integration is unavailable, fallback to window menu.",
        Provider: "window-menu",
        HasFallback: true,
        FallbackMode: "window-menu");

    public Task<PlatformOperationResult> InitializeAsync(string appTitle, TrayMenuText? menuText, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(PlatformOperation.FallbackSuccess(
            Capability.Provider,
            "Window-menu tray fallback initialized.",
            operationId: "tray.initialize",
            errorCode: PlatformErrorCodes.TrayFallback));
    }

    public Task<PlatformOperationResult> ShutdownAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(PlatformOperation.FallbackSuccess(
            Capability.Provider,
            "Window-menu tray fallback shutdown completed.",
            operationId: "tray.shutdown",
            errorCode: PlatformErrorCodes.TrayFallback));
    }

    public Task<PlatformOperationResult> ShowAsync(string title, string message, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(PlatformOperation.FallbackSuccess(
            Capability.Provider,
            $"Window menu fallback message: {title} - {message}",
            operationId: "tray.show",
            errorCode: PlatformErrorCodes.TrayFallback));
    }

    public Task<PlatformOperationResult> SetMenuStateAsync(TrayMenuState state, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _menuState = state;
        return Task.FromResult(PlatformOperation.FallbackSuccess(
            Capability.Provider,
            "Window menu tray state updated.",
            operationId: "tray.setMenuState",
            errorCode: PlatformErrorCodes.TrayFallback));
    }

    public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _visible = visible;
        return Task.FromResult(PlatformOperation.FallbackSuccess(
            Capability.Provider,
            $"Window menu tray visibility set to {visible}.",
            operationId: "tray.setVisible",
            errorCode: PlatformErrorCodes.TrayFallback));
    }
}

public sealed class CommandNotificationService : INotificationService
{
    private readonly PlatformCapabilityStatus _capability;

    public CommandNotificationService()
    {
        _capability = BuildCapability();
    }

    public PlatformCapabilityStatus Capability => _capability;

    public async Task<PlatformOperationResult> NotifyAsync(string title, string message, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (!_capability.Supported)
        {
            return PlatformOperation.FallbackSuccess(
                _capability.Provider,
                _capability.Message,
                operationId: "notification.notify",
                errorCode: PlatformErrorCodes.NotificationFallback);
        }

        var command = BuildCommand(title, message);
        if (command is null)
        {
            return PlatformOperation.FallbackSuccess(
                _capability.Provider,
                "Notification command is not available, switched to in-app fallback.",
                operationId: "notification.notify",
                errorCode: PlatformErrorCodes.NotificationFallback);
        }

        var result = await ExecuteCommandAsync(command.Value.fileName, command.Value.arguments, cancellationToken);
        if (result.exitCode == 0)
        {
            return PlatformOperation.FallbackSuccess(
                _capability.Provider,
                "System notification dispatched by fallback command provider.",
                operationId: "notification.notify",
                errorCode: PlatformErrorCodes.NotificationFallback);
        }

        return PlatformOperation.FallbackSuccess(
            _capability.Provider,
            $"System notification failed (exit={result.exitCode}), switched to in-app fallback.",
            operationId: "notification.notify",
            errorCode: PlatformErrorCodes.NotificationFallback);
    }

    private static PlatformCapabilityStatus BuildCapability()
    {
        if (OperatingSystem.IsLinux())
        {
            return new PlatformCapabilityStatus(
                Supported: IsCommandAvailable("which", "notify-send"),
                Message: "Linux notifications use notify-send.",
                Provider: "notify-send",
                HasFallback: true,
                FallbackMode: "in-app");
        }

        if (OperatingSystem.IsMacOS())
        {
            return new PlatformCapabilityStatus(
                Supported: IsCommandAvailable("which", "osascript"),
                Message: "macOS notifications use osascript.",
                Provider: "osascript",
                HasFallback: true,
                FallbackMode: "in-app");
        }

        if (OperatingSystem.IsWindows())
        {
            return new PlatformCapabilityStatus(
                Supported: IsCommandAvailable("where", "powershell"),
                Message: "Windows notifications use PowerShell toast command.",
                Provider: "powershell-toast",
                HasFallback: true,
                FallbackMode: "in-app");
        }

        return new PlatformCapabilityStatus(
            Supported: false,
            Message: "No supported system notification backend for current platform.",
            Provider: "unknown",
            HasFallback: true,
            FallbackMode: "in-app");
    }

    private static (string fileName, string arguments)? BuildCommand(string title, string message)
    {
        var safeTitle = EscapeSingleLine(title);
        var safeMessage = EscapeSingleLine(message);

        if (OperatingSystem.IsLinux())
        {
            return ("notify-send", $"--app-name MAAUnified \"{safeTitle}\" \"{safeMessage}\"");
        }

        if (OperatingSystem.IsMacOS())
        {
            var script = $"display notification \"{EscapeAppleScript(safeMessage)}\" with title \"{EscapeAppleScript(safeTitle)}\"";
            return ("osascript", $"-e \"{script}\"");
        }

        if (OperatingSystem.IsWindows())
        {
            var escapedTitle = EscapePowerShellLiteral(safeTitle);
            var escapedMessage = EscapePowerShellLiteral(safeMessage);
            var script =
                "$ErrorActionPreference='Stop';" +
                "[Windows.UI.Notifications.ToastNotificationManager,Windows.UI.Notifications,ContentType=WindowsRuntime] > $null;" +
                "$template=[Windows.UI.Notifications.ToastTemplateType]::ToastText02;" +
                "$xml=[Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent($template);" +
                "$texts=$xml.GetElementsByTagName('text');" +
                $"$texts.Item(0).AppendChild($xml.CreateTextNode('{escapedTitle}')) > $null;" +
                $"$texts.Item(1).AppendChild($xml.CreateTextNode('{escapedMessage}')) > $null;" +
                "$toast=[Windows.UI.Notifications.ToastNotification]::new($xml);" +
                "$notifier=[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('MAAUnified');" +
                "$notifier.Show($toast);";
            return ("powershell", $"-NoProfile -NonInteractive -WindowStyle Hidden -Command \"{script}\"");
        }

        return null;
    }

    private static bool IsCommandAvailable(string command, string args)
    {
        try
        {
            var psi = new ProcessStartInfo(command, args)
            {
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            using var process = Process.Start(psi);
            if (process is null)
            {
                return false;
            }

            process.WaitForExit(1500);
            return process.ExitCode == 0;
        }
        catch
        {
            return false;
        }
    }

    private static async Task<(int exitCode, string output, string error)> ExecuteCommandAsync(
        string fileName,
        string args,
        CancellationToken cancellationToken)
    {
        try
        {
            var psi = new ProcessStartInfo(fileName, args)
            {
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true,
            };

            using var process = Process.Start(psi);
            if (process is null)
            {
                return (-1, string.Empty, "Process start failed.");
            }

            using var registration = cancellationToken.Register(() =>
            {
                try
                {
                    if (!process.HasExited)
                    {
                        process.Kill(entireProcessTree: true);
                    }
                }
                catch
                {
                    // ignored
                }
            });

            await process.WaitForExitAsync(cancellationToken);
            var output = await process.StandardOutput.ReadToEndAsync(cancellationToken);
            var error = await process.StandardError.ReadToEndAsync(cancellationToken);
            return (process.ExitCode, output, error);
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            return (-1, string.Empty, ex.Message);
        }
    }

    private static string EscapeSingleLine(string text)
    {
        return (text ?? string.Empty)
            .Replace("\r", " ", StringComparison.Ordinal)
            .Replace("\n", " ", StringComparison.Ordinal)
            .Trim();
    }

    private static string EscapeAppleScript(string text)
    {
        return text.Replace("\\", "\\\\", StringComparison.Ordinal)
            .Replace("\"", "\\\"", StringComparison.Ordinal);
    }

    private static string EscapePowerShellLiteral(string text)
    {
        return text.Replace("'", "''", StringComparison.Ordinal);
    }
}

public sealed class WindowScopedHotkeyService : IGlobalHotkeyService
{
    private readonly Dictionary<string, string> _registeredByName = new(StringComparer.OrdinalIgnoreCase);
    private readonly HashSet<string> _registeredGestures = new(StringComparer.OrdinalIgnoreCase);

    public event EventHandler<GlobalHotkeyTriggeredEvent>? Triggered;

    public PlatformCapabilityStatus Capability => new(
        Supported: false,
        Message: "Global hotkeys are unavailable, fallback to window-scoped hotkeys.",
        Provider: "window-scoped",
        HasFallback: true,
        FallbackMode: "window-scoped");

    public Task<PlatformOperationResult> RegisterAsync(string name, string gesture, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (string.IsNullOrWhiteSpace(name))
        {
            return Task.FromResult(PlatformOperation.Failed(Capability.Provider, "Hotkey name cannot be empty.", PlatformErrorCodes.HotkeyNameMissing, "hotkey.register"));
        }

        if (!HotkeyGestureNormalizer.TryNormalize(gesture, out var normalizedGesture))
        {
            return Task.FromResult(PlatformOperation.Failed(Capability.Provider, "Invalid hotkey gesture format.", PlatformErrorCodes.HotkeyInvalidGesture, "hotkey.register"));
        }

        if (_registeredByName.TryGetValue(name, out var existingGesture))
        {
            _registeredGestures.Remove(existingGesture);
        }

        if (_registeredGestures.Contains(normalizedGesture))
        {
            return Task.FromResult(PlatformOperation.Failed(Capability.Provider, "Hotkey gesture already in use.", PlatformErrorCodes.HotkeyConflict, "hotkey.register", usedFallback: true));
        }

        _registeredByName[name] = normalizedGesture;
        _registeredGestures.Add(normalizedGesture);

        return Task.FromResult(PlatformOperation.FallbackSuccess(
            Capability.Provider,
            $"Window-scoped hotkey registered: {name} => {normalizedGesture}",
            operationId: "hotkey.register",
            errorCode: PlatformErrorCodes.HotkeyFallback));
    }

    public Task<PlatformOperationResult> UnregisterAsync(string name, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (string.IsNullOrWhiteSpace(name))
        {
            return Task.FromResult(PlatformOperation.Failed(Capability.Provider, "Hotkey name cannot be empty.", PlatformErrorCodes.HotkeyNameMissing, "hotkey.unregister"));
        }

        if (_registeredByName.TryGetValue(name, out var existingGesture))
        {
            _registeredByName.Remove(name);
            _registeredGestures.Remove(existingGesture);
            return Task.FromResult(PlatformOperation.FallbackSuccess(
                Capability.Provider,
                $"Window-scoped hotkey unregistered: {name}",
                operationId: "hotkey.unregister",
                errorCode: PlatformErrorCodes.HotkeyFallback));
        }

        return Task.FromResult(PlatformOperation.Failed(
            Capability.Provider,
            $"Hotkey `{name}` was not registered.",
            PlatformErrorCodes.HotkeyNotFound,
            operationId: "hotkey.unregister",
            usedFallback: true));
    }
}

public sealed class CrossPlatformAutostartService : IAutostartService
{
    private const string WindowsRunKey = @"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    private readonly PlatformCapabilityStatus _capability;
    private readonly string _processPath;
    private readonly string _windowsValueName;
    private readonly string _linuxAutostartFile;
    private readonly string _macLaunchAgentFile;

    public CrossPlatformAutostartService()
    {
        _processPath = Environment.ProcessPath ?? string.Empty;

        _windowsValueName = string.IsNullOrWhiteSpace(_processPath)
            ? "MAA_UNIFIED"
            : "MAA_" + ComputeStableHash(_processPath);

        var configDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
        _linuxAutostartFile = Path.Combine(configDir, "autostart", "maa-unified.desktop");

        var home = Environment.GetFolderPath(Environment.SpecialFolder.Personal);
        _macLaunchAgentFile = Path.Combine(home, "Library", "LaunchAgents", "io.maa.unified.autostart.plist");

        _capability = BuildCapability();
    }

    public PlatformCapabilityStatus Capability => _capability;

    public Task<PlatformOperationResult<bool>> IsEnabledAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (!_capability.Supported)
        {
            return Task.FromResult(new PlatformOperationResult<bool>(
                false,
                false,
                _capability.Message,
                PlatformErrorCodes.AutostartUnsupported,
                false,
                _capability.Provider,
                "autostart.query",
                PlatformExecutionMode.Failed));
        }

        try
        {
            var enabled = QueryAutostartEnabled();
            return Task.FromResult(new PlatformOperationResult<bool>(
                true,
                enabled,
                enabled ? "Autostart is enabled." : "Autostart is disabled.",
                null,
                false,
                _capability.Provider,
                "autostart.query",
                PlatformExecutionMode.Native));
        }
        catch (Exception ex)
        {
            return Task.FromResult(new PlatformOperationResult<bool>(
                false,
                false,
                $"Failed to query autostart: {ex.Message}",
                PlatformErrorCodes.AutostartQueryFailed,
                false,
                _capability.Provider,
                "autostart.query",
                PlatformExecutionMode.Failed));
        }
    }

    public async Task<PlatformOperationResult> SetEnabledAsync(bool enabled, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (!_capability.Supported)
        {
            return PlatformOperation.Failed(_capability.Provider, _capability.Message, PlatformErrorCodes.AutostartUnsupported, "autostart.set");
        }

        if (string.IsNullOrWhiteSpace(_processPath) || !File.Exists(_processPath))
        {
            return PlatformOperation.Failed(_capability.Provider, "Executable path is unavailable for autostart.", PlatformErrorCodes.AutostartExecutableMissing, "autostart.set");
        }

        try
        {
            ApplyAutostart(enabled);
            var current = await IsEnabledAsync(cancellationToken);
            if (!current.Success || current.Value != enabled)
            {
                return PlatformOperation.Failed(_capability.Provider, "Autostart verification failed after update.", PlatformErrorCodes.AutostartVerificationFailed, "autostart.set");
            }

            return PlatformOperation.NativeSuccess(_capability.Provider, $"Autostart set to {enabled}.", "autostart.set");
        }
        catch (Exception ex)
        {
            return PlatformOperation.Failed(_capability.Provider, $"Failed to set autostart: {ex.Message}", PlatformErrorCodes.AutostartSetFailed, "autostart.set");
        }
    }

    private PlatformCapabilityStatus BuildCapability()
    {
        if (OperatingSystem.IsWindows())
        {
            return new PlatformCapabilityStatus(true, "Autostart uses HKCU Run registry entry.", "registry-run", false, null);
        }

        if (OperatingSystem.IsLinux())
        {
            return new PlatformCapabilityStatus(true, "Autostart uses XDG desktop autostart file.", "xdg-autostart", false, null);
        }

        if (OperatingSystem.IsMacOS())
        {
            return new PlatformCapabilityStatus(true, "Autostart uses LaunchAgents plist.", "launch-agent", false, null);
        }

        return new PlatformCapabilityStatus(false, "Autostart is not supported on current platform.", "unsupported", false, null);
    }

    private bool QueryAutostartEnabled()
    {
        if (OperatingSystem.IsWindows())
        {
            using var key = Registry.CurrentUser.OpenSubKey(WindowsRunKey, false);
            var value = key?.GetValue(_windowsValueName) as string;
            if (string.IsNullOrWhiteSpace(value))
            {
                return false;
            }

            return value.Contains(_processPath, StringComparison.OrdinalIgnoreCase);
        }

        if (OperatingSystem.IsLinux())
        {
            return File.Exists(_linuxAutostartFile);
        }

        if (OperatingSystem.IsMacOS())
        {
            return File.Exists(_macLaunchAgentFile);
        }

        return false;
    }

    private void ApplyAutostart(bool enabled)
    {
        if (OperatingSystem.IsWindows())
        {
            using var key = Registry.CurrentUser.OpenSubKey(WindowsRunKey, writable: true)
                ?? throw new InvalidOperationException("Cannot open HKCU Run registry key.");

            if (enabled)
            {
                key.SetValue(_windowsValueName, $"\"{_processPath}\"");
            }
            else
            {
                key.DeleteValue(_windowsValueName, throwOnMissingValue: false);
            }

            return;
        }

        if (OperatingSystem.IsLinux())
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_linuxAutostartFile) ?? string.Empty);
            if (enabled)
            {
                var content = BuildLinuxDesktopEntry(_processPath);
                File.WriteAllText(_linuxAutostartFile, content, Encoding.UTF8);
            }
            else
            {
                File.Delete(_linuxAutostartFile);
            }

            return;
        }

        if (OperatingSystem.IsMacOS())
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_macLaunchAgentFile) ?? string.Empty);
            if (enabled)
            {
                var plist = BuildMacLaunchAgentPlist(_processPath);
                File.WriteAllText(_macLaunchAgentFile, plist, Encoding.UTF8);
            }
            else
            {
                File.Delete(_macLaunchAgentFile);
            }
        }
    }

    private static string ComputeStableHash(string input)
    {
        var hash1 = (5381 << 16) + 5381;
        var hash2 = hash1;

        for (var i = 0; i < input.Length; i += 2)
        {
            hash1 = ((hash1 << 5) + hash1) ^ input[i];
            if (i == input.Length - 1)
            {
                break;
            }

            hash2 = ((hash2 << 5) + hash2) ^ input[i + 1];
        }

        return (hash1 + (hash2 * 1566083941)).ToString("X");
    }

    private static string BuildLinuxDesktopEntry(string processPath)
    {
        return new StringBuilder()
            .AppendLine("[Desktop Entry]")
            .AppendLine("Type=Application")
            .AppendLine("Version=1.0")
            .AppendLine("Name=MAAUnified")
            .AppendLine("Comment=Start MAAUnified at login")
            .AppendLine($"Exec=\"{processPath}\"")
            .AppendLine("Terminal=false")
            .AppendLine("X-GNOME-Autostart-enabled=true")
            .ToString();
    }

    private static string BuildMacLaunchAgentPlist(string processPath)
    {
        var escapedPath = processPath
            .Replace("&", "&amp;", StringComparison.Ordinal)
            .Replace("<", "&lt;", StringComparison.Ordinal)
            .Replace(">", "&gt;", StringComparison.Ordinal)
            .Replace("\"", "&quot;", StringComparison.Ordinal)
            .Replace("'", "&apos;", StringComparison.Ordinal);

        return new StringBuilder()
            .AppendLine("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")
            .AppendLine("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">")
            .AppendLine("<plist version=\"1.0\">")
            .AppendLine("<dict>")
            .AppendLine("  <key>Label</key>")
            .AppendLine("  <string>io.maa.unified.autostart</string>")
            .AppendLine("  <key>ProgramArguments</key>")
            .AppendLine("  <array>")
            .AppendLine($"    <string>{escapedPath}</string>")
            .AppendLine("  </array>")
            .AppendLine("  <key>RunAtLoad</key>")
            .AppendLine("  <true/>")
            .AppendLine("</dict>")
            .AppendLine("</plist>")
            .ToString();
    }
}

public sealed class WindowsOverlayCapabilityService : IOverlayCapabilityService, IDisposable
{
    private static readonly nint HwndTopMost = new(-1);
    private const int GwlExStyle = -20;
    private const int MaxConsecutiveSyncFailures = 3;
    private const nint WsExLayered = 0x00080000;
    private const nint WsExTransparent = 0x00000020;
    private const uint LwaAlpha = 0x2;
    private const uint SwpNoActivate = 0x0010;
    private const uint SwpShowWindow = 0x0040;
    private const int SwHide = 0;
    private const int SwShowNoActivate = 4;

    internal readonly record struct OverlayRect(int Left, int Top, int Right, int Bottom);

    internal interface INativeApi
    {
        IEnumerable<nint> EnumerateWindows();

        bool IsWindow(nint hWnd);

        bool IsWindowVisible(nint hWnd);

        int GetWindowTextLength(nint hWnd);

        string GetWindowText(nint hWnd);

        uint GetWindowThreadProcessId(nint hWnd);

        bool TryGetWindowRect(nint hWnd, out OverlayRect rect);

        bool SetWindowPos(nint hWnd, nint hWndInsertAfter, int x, int y, int cx, int cy, uint uFlags);

        bool ShowWindow(nint hWnd, int nCmdShow);

        nint GetWindowLongPtr(nint hWnd, int nIndex);

        int GetLastWin32Error();

        nint SetWindowLongPtr(nint hWnd, int nIndex, nint dwNewLong);

        bool SetLayeredWindowAttributes(nint hwnd, uint crKey, byte bAlpha, uint dwFlags);
    }

    private readonly object _syncRoot = new();
    private readonly INativeApi _api;
    private nint _selectedTarget;
    private nint _hostWindow;
    private bool _clickThrough = true;
    private double _opacity = 0.85d;
    private bool _visible;
    private Timer? _attachSyncTimer;
    private OverlayRect _lastTargetRect;
    private DateTimeOffset _lastSyncAt;
    private string? _lastSyncIssue;
    private int _consecutiveSyncFailures;

    public WindowsOverlayCapabilityService()
        : this(new Win32NativeApi())
    {
    }

    internal WindowsOverlayCapabilityService(INativeApi api)
    {
        _api = api;
    }

    public PlatformCapabilityStatus Capability => new(
        Supported: true,
        Message: "Overlay attachment is available via Win32 window targets.",
        Provider: "win32-overlay",
        HasFallback: true,
        FallbackMode: "preview-and-log");

    public event EventHandler<OverlayStateChangedEvent>? OverlayStateChanged;

    public Task<PlatformOperationResult> BindHostWindowAsync(
        nint hostWindowHandle,
        bool clickThrough,
        double opacity,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (hostWindowHandle == nint.Zero || !_api.IsWindow(hostWindowHandle))
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Overlay host window handle is invalid.",
                PlatformErrorCodes.OverlayHostNotBound,
                "overlay.bindHost"));
        }

        _hostWindow = hostWindowHandle;
        _clickThrough = clickThrough;
        _opacity = Math.Clamp(opacity, 0.05d, 1.0d);

        if (!ConfigureHostWindow())
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Failed to configure overlay host window styles.",
                PlatformErrorCodes.OverlayAttachFailed,
                "overlay.bindHost"));
        }

        return Task.FromResult(PlatformOperation.NativeSuccess(
            Capability.Provider,
            "Overlay host window bound.",
            "overlay.bindHost"));
    }

    public Task<PlatformOperationResult<IReadOnlyList<OverlayTarget>>> QueryTargetsAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        try
        {
            var targets = new List<OverlayTarget>();
            var seen = new HashSet<nint>();

            foreach (var hWnd in _api.EnumerateWindows())
            {
                if (!_api.IsWindowVisible(hWnd))
                {
                    continue;
                }

                var len = _api.GetWindowTextLength(hWnd);
                if (len <= 0)
                {
                    continue;
                }

                var title = _api.GetWindowText(hWnd).Trim();
                if (string.IsNullOrWhiteSpace(title))
                {
                    continue;
                }

                var pid = _api.GetWindowThreadProcessId(hWnd);
                if (pid == (uint)Environment.ProcessId)
                {
                    continue;
                }

                if (!seen.Add(hWnd))
                {
                    continue;
                }

                string? processName = null;
                try
                {
                    processName = Process.GetProcessById((int)pid).ProcessName;
                }
                catch
                {
                    // Best-effort metadata for target restore.
                }

                var display = string.IsNullOrWhiteSpace(processName)
                    ? $"{title} (pid:{pid})"
                    : $"{title} - {processName} - {pid}";
                targets.Add(new OverlayTarget(
                    $"hwnd:{hWnd:X}",
                    display,
                    false,
                    NativeHandle: (long)hWnd,
                    ProcessId: (int)pid,
                    ProcessName: processName,
                    WindowTitle: title));
                if (targets.Count >= 80)
                {
                    break;
                }
            }

            targets.Insert(0, new OverlayTarget("preview", "Preview + Logs", true));
            IReadOnlyList<OverlayTarget> resultTargets = targets;
            if (targets.Count <= 1)
            {
                return Task.FromResult(PlatformOperation.FallbackSuccess(
                    Capability.Provider,
                    resultTargets,
                    "No native overlay target is available, switched to preview mode.",
                    "overlay.query-targets",
                    PlatformErrorCodes.OverlayPreviewMode));
            }

            return Task.FromResult(PlatformOperation.NativeSuccess(
                Capability.Provider,
                resultTargets,
                $"Found {targets.Count - 1} native overlay target(s).",
                "overlay.query-targets"));
        }
        catch (Exception ex)
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                $"Overlay target query failed: {ex.Message}",
                PlatformErrorCodes.OverlayQueryFailed,
                "overlay.query-targets",
                value: (IReadOnlyList<OverlayTarget>)Array.Empty<OverlayTarget>()));
        }
    }

    public Task<PlatformOperationResult> SelectTargetAsync(string targetId, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (string.Equals(targetId, "preview", StringComparison.OrdinalIgnoreCase))
        {
            _selectedTarget = nint.Zero;
            _consecutiveSyncFailures = 0;
            StopAttachSync();
            HideHostWindow();
            if (_visible)
            {
                EmitStateChanged(
                    OverlayRuntimeMode.Preview,
                    visible: true,
                    targetId: "preview",
                    action: "fallback-enter",
                    message: "Overlay switched to Preview + Logs mode.",
                    usedFallback: true,
                    errorCode: PlatformErrorCodes.OverlayPreviewMode);
            }

            return Task.FromResult(PlatformOperation.FallbackSuccess(
                Capability.Provider,
                "Overlay target switched to preview mode.",
                operationId: "overlay.selectTarget",
                errorCode: PlatformErrorCodes.OverlayPreviewMode));
        }

        if (!TryParseHandle(targetId, out var handle))
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                $"Invalid overlay target id: {targetId}",
                PlatformErrorCodes.OverlayTargetInvalid,
                operationId: "overlay.selectTarget"));
        }

        if (!_api.IsWindow(handle))
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                $"Overlay target is unavailable: 0x{handle:X}",
                PlatformErrorCodes.OverlayTargetGone,
                operationId: "overlay.selectTarget"));
        }

        _selectedTarget = handle;
        _consecutiveSyncFailures = 0;
        if (_visible)
        {
            if (_hostWindow == nint.Zero)
            {
                EnterPreviewMode(
                    "Overlay host is unavailable; switched to Preview + Logs mode.",
                    "fallback-enter",
                    PlatformErrorCodes.OverlayHostNotBound);
                return Task.FromResult(PlatformOperation.Failed(
                    Capability.Provider,
                    "Overlay host is unavailable; switched to Preview + Logs mode.",
                    PlatformErrorCodes.OverlayHostNotBound,
                    operationId: "overlay.selectTarget"));
            }

            if (!ConfigureHostWindow())
            {
                EnterPreviewMode(
                    "Overlay target attach failed; switched to Preview + Logs mode.",
                    "fallback-enter",
                    PlatformErrorCodes.OverlayAttachFailed);
                return Task.FromResult(PlatformOperation.Failed(
                    Capability.Provider,
                    "Overlay target attach failed; switched to Preview + Logs mode.",
                    PlatformErrorCodes.OverlayAttachFailed,
                    operationId: "overlay.selectTarget"));
            }

            if (!TryAttachSelectedTarget(out var attachMessage, out var attachErrorCode))
            {
                return Task.FromResult(PlatformOperation.Failed(
                    Capability.Provider,
                    attachMessage,
                    attachErrorCode,
                    operationId: "overlay.selectTarget"));
            }

            EmitStateChanged(
                OverlayRuntimeMode.Native,
                visible: true,
                targetId: GetSelectedTargetId(),
                action: "target-change",
                message: $"Overlay target changed to 0x{handle:X}.",
                usedFallback: false,
                errorCode: null);
        }

        return Task.FromResult(PlatformOperation.NativeSuccess(
            Capability.Provider,
            $"Overlay target selected: 0x{handle:X}",
            operationId: "overlay.selectTarget"));
    }

    public Task<PlatformOperationResult> SetVisibleAsync(bool visible, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        _visible = visible;

        if (!visible)
        {
            _consecutiveSyncFailures = 0;
            StopAttachSync();
            HideHostWindow();
            EmitStateChanged(
                OverlayRuntimeMode.Hidden,
                visible: false,
                targetId: GetSelectedTargetId(),
                action: "hide",
                message: "Overlay hidden.",
                usedFallback: false,
                errorCode: null);
            return Task.FromResult(PlatformOperation.NativeSuccess(
                Capability.Provider,
                "Overlay visibility set to false.",
                operationId: "overlay.setVisible"));
        }

        if (_selectedTarget == nint.Zero)
        {
            _consecutiveSyncFailures = 0;
            StopAttachSync();
            HideHostWindow();
            EmitStateChanged(
                OverlayRuntimeMode.Preview,
                visible: true,
                targetId: "preview",
                action: "fallback-enter",
                message: "Overlay switched to Preview + Logs mode because no native target is selected.",
                usedFallback: true,
                errorCode: PlatformErrorCodes.OverlayPreviewMode);
            return Task.FromResult(PlatformOperation.FallbackSuccess(
                Capability.Provider,
                "Overlay switched to preview mode because no native target is selected.",
                operationId: "overlay.setVisible",
                errorCode: PlatformErrorCodes.OverlayPreviewMode));
        }

        if (_hostWindow == nint.Zero)
        {
            EnterPreviewMode(
                "Overlay host is unavailable; switched to Preview + Logs mode.",
                "fallback-enter",
                PlatformErrorCodes.OverlayHostNotBound);
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Overlay host is unavailable; switched to Preview + Logs mode.",
                PlatformErrorCodes.OverlayHostNotBound,
                operationId: "overlay.setVisible"));
        }

        if (!ConfigureHostWindow())
        {
            EnterPreviewMode(
                "Overlay host configuration failed; switched to Preview + Logs mode.",
                "fallback-enter",
                PlatformErrorCodes.OverlayAttachFailed);
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                "Overlay host configuration failed; switched to Preview + Logs mode.",
                PlatformErrorCodes.OverlayAttachFailed,
                operationId: "overlay.setVisible"));
        }

        if (!TryAttachSelectedTarget(out var attachMessage, out var attachErrorCode))
        {
            return Task.FromResult(PlatformOperation.Failed(
                Capability.Provider,
                attachMessage,
                attachErrorCode,
                operationId: "overlay.setVisible"));
        }

        EmitStateChanged(
            OverlayRuntimeMode.Native,
            visible: true,
            targetId: GetSelectedTargetId(),
            action: "show-native",
            message: $"Overlay attached to native target: 0x{_selectedTarget:X}.",
            usedFallback: false,
            errorCode: null);

        var extra = _lastSyncAt == default
            ? string.Empty
            : $" last-sync={_lastSyncAt:O}";
        var issue = string.IsNullOrWhiteSpace(_lastSyncIssue)
            ? string.Empty
            : $" issue={_lastSyncIssue}";
        return Task.FromResult(PlatformOperation.NativeSuccess(
            Capability.Provider,
            $"Overlay visibility set to {visible}.{extra}{issue}",
            operationId: "overlay.setVisible"));
    }

    public void Dispose()
    {
        StopAttachSync();
    }

    internal void SyncNowForTesting()
    {
        SyncSelectedTarget();
    }

    private void StartAttachSync()
    {
        lock (_syncRoot)
        {
            _attachSyncTimer ??= new Timer(_ => SyncSelectedTarget(), null, TimeSpan.Zero, TimeSpan.FromMilliseconds(300));
        }
    }

    private void StopAttachSync()
    {
        lock (_syncRoot)
        {
            _attachSyncTimer?.Dispose();
            _attachSyncTimer = null;
        }
    }

    private void SyncSelectedTarget()
    {
        var handle = _selectedTarget;
        if (handle == nint.Zero || _hostWindow == nint.Zero || !_visible)
        {
            return;
        }

        if (SyncSelectedTargetCore(handle, out var failureMessage, out var failureCode))
        {
            return;
        }

        if (string.Equals(failureCode, PlatformErrorCodes.OverlayTargetGone, StringComparison.Ordinal))
        {
            EnterPreviewMode(
                "Overlay target was lost; switched to Preview + Logs mode.",
                "target-lost",
                PlatformErrorCodes.OverlayTargetGone);
            return;
        }

        _consecutiveSyncFailures++;
        _lastSyncIssue = failureMessage;
        if (_consecutiveSyncFailures >= MaxConsecutiveSyncFailures)
        {
            EnterPreviewMode(
                "Overlay sync failed repeatedly; switched to Preview + Logs mode.",
                "fallback-enter",
                failureCode);
        }
    }

    private bool TryAttachSelectedTarget(out string failureMessage, out string failureCode)
    {
        if (!SyncSelectedTargetCore(_selectedTarget, out failureMessage, out failureCode))
        {
            EnterPreviewMode(
                string.Equals(failureCode, PlatformErrorCodes.OverlayTargetGone, StringComparison.Ordinal)
                    ? "Overlay target was lost; switched to Preview + Logs mode."
                    : "Overlay target attach failed; switched to Preview + Logs mode.",
                string.Equals(failureCode, PlatformErrorCodes.OverlayTargetGone, StringComparison.Ordinal)
                    ? "target-lost"
                    : "fallback-enter",
                failureCode);
            return false;
        }

        _consecutiveSyncFailures = 0;
        StartAttachSync();
        return true;
    }

    private bool SyncSelectedTargetCore(nint handle, out string failureMessage, out string failureCode)
    {
        if (handle == nint.Zero || !_api.IsWindow(handle))
        {
            failureMessage = $"Overlay target is unavailable: 0x{handle:X}";
            failureCode = PlatformErrorCodes.OverlayTargetGone;
            return false;
        }

        if (!_api.TryGetWindowRect(handle, out var rect))
        {
            failureMessage = "GetWindowRect failed.";
            failureCode = PlatformErrorCodes.OverlayAttachFailed;
            return false;
        }

        if (!SetHostBounds(rect))
        {
            failureMessage = "SetWindowPos failed.";
            failureCode = PlatformErrorCodes.OverlayAttachFailed;
            return false;
        }

        _lastTargetRect = rect;
        _lastSyncAt = DateTimeOffset.UtcNow;
        _lastSyncIssue = null;
        _consecutiveSyncFailures = 0;
        failureMessage = string.Empty;
        failureCode = string.Empty;
        return true;
    }

    private void EnterPreviewMode(string message, string action, string? errorCode)
    {
        _selectedTarget = nint.Zero;
        _consecutiveSyncFailures = 0;
        _lastSyncIssue = message;
        StopAttachSync();
        HideHostWindow();
        EmitStateChanged(
            OverlayRuntimeMode.Preview,
            visible: _visible,
            targetId: "preview",
            action: action,
            message: message,
            usedFallback: true,
            errorCode: errorCode);
    }

    private void EmitStateChanged(
        OverlayRuntimeMode mode,
        bool visible,
        string targetId,
        string action,
        string message,
        bool usedFallback,
        string? errorCode)
    {
        OverlayStateChanged?.Invoke(
            this,
            new OverlayStateChangedEvent(
                mode,
                visible,
                targetId,
                action,
                message,
                DateTimeOffset.UtcNow,
                Capability.Provider,
                usedFallback,
                errorCode));
    }

    private string GetSelectedTargetId()
    {
        return _selectedTarget == nint.Zero
            ? "preview"
            : $"hwnd:{_selectedTarget:X}";
    }

    private bool ConfigureHostWindow()
    {
        if (_hostWindow == nint.Zero || !_api.IsWindow(_hostWindow))
        {
            return false;
        }

        var style = _api.GetWindowLongPtr(_hostWindow, GwlExStyle);
        if (style == nint.Zero && _api.GetLastWin32Error() != 0)
        {
            return false;
        }

        style |= WsExLayered;
        if (_clickThrough)
        {
            style |= WsExTransparent;
        }
        else
        {
            style &= ~WsExTransparent;
        }

        _ = _api.SetWindowLongPtr(_hostWindow, GwlExStyle, style);
        var alpha = (byte)Math.Round(_opacity * 255d, MidpointRounding.AwayFromZero);
        return _api.SetLayeredWindowAttributes(_hostWindow, 0, alpha, LwaAlpha);
    }

    private bool SetHostBounds(OverlayRect targetRect)
    {
        if (_hostWindow == nint.Zero)
        {
            return false;
        }

        var width = Math.Max(1, targetRect.Right - targetRect.Left);
        var height = Math.Max(1, targetRect.Bottom - targetRect.Top);
        var ok = _api.SetWindowPos(
            _hostWindow,
            HwndTopMost,
            targetRect.Left,
            targetRect.Top,
            width,
            height,
            SwpNoActivate | SwpShowWindow);
        if (ok)
        {
            _ = _api.ShowWindow(_hostWindow, SwShowNoActivate);
        }

        return ok;
    }

    private void HideHostWindow()
    {
        if (_hostWindow != nint.Zero && _api.IsWindow(_hostWindow))
        {
            _ = _api.ShowWindow(_hostWindow, SwHide);
        }
    }

    private static bool TryParseHandle(string targetId, out nint handle)
    {
        handle = nint.Zero;
        if (string.IsNullOrWhiteSpace(targetId))
        {
            return false;
        }

        var payload = targetId.StartsWith("hwnd:", StringComparison.OrdinalIgnoreCase)
            ? targetId[5..]
            : targetId;

        if (!long.TryParse(payload, System.Globalization.NumberStyles.HexNumber, null, out var value)
            && !long.TryParse(payload, out value))
        {
            return false;
        }

        handle = (nint)value;
        return handle != nint.Zero;
    }

    private delegate bool EnumWindowsProc(nint hWnd, nint lParam);

    private sealed class Win32NativeApi : INativeApi
    {
        public IEnumerable<nint> EnumerateWindows()
        {
            var windows = new List<nint>();
            EnumWindows((hWnd, lParam) =>
            {
                _ = lParam;
                windows.Add(hWnd);
                return true;
            }, nint.Zero);
            return windows;
        }

        public bool IsWindow(nint hWnd) => WindowsOverlayCapabilityService.IsWindow(hWnd);

        public bool IsWindowVisible(nint hWnd) => WindowsOverlayCapabilityService.IsWindowVisible(hWnd);

        public int GetWindowTextLength(nint hWnd) => WindowsOverlayCapabilityService.GetWindowTextLength(hWnd);

        public string GetWindowText(nint hWnd)
        {
            var len = GetWindowTextLength(hWnd);
            if (len <= 0)
            {
                return string.Empty;
            }

            var titleBuilder = new StringBuilder(len + 1);
            _ = WindowsOverlayCapabilityService.GetWindowText(hWnd, titleBuilder, titleBuilder.Capacity);
            return titleBuilder.ToString();
        }

        public uint GetWindowThreadProcessId(nint hWnd)
        {
            WindowsOverlayCapabilityService.GetWindowThreadProcessId(hWnd, out var pid);
            return pid;
        }

        public bool TryGetWindowRect(nint hWnd, out OverlayRect rect)
        {
            if (WindowsOverlayCapabilityService.GetWindowRect(hWnd, out var winRect))
            {
                rect = new OverlayRect(winRect.Left, winRect.Top, winRect.Right, winRect.Bottom);
                return true;
            }

            rect = default;
            return false;
        }

        public bool SetWindowPos(nint hWnd, nint hWndInsertAfter, int x, int y, int cx, int cy, uint uFlags)
            => WindowsOverlayCapabilityService.SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);

        public bool ShowWindow(nint hWnd, int nCmdShow)
            => WindowsOverlayCapabilityService.ShowWindow(hWnd, nCmdShow);

        public nint GetWindowLongPtr(nint hWnd, int nIndex)
            => GetWindowLongPtrCompat(hWnd, nIndex);

        public int GetLastWin32Error()
            => Marshal.GetLastWin32Error();

        public nint SetWindowLongPtr(nint hWnd, int nIndex, nint dwNewLong)
            => SetWindowLongPtrCompat(hWnd, nIndex, dwNewLong);

        public bool SetLayeredWindowAttributes(nint hwnd, uint crKey, byte bAlpha, uint dwFlags)
            => WindowsOverlayCapabilityService.SetLayeredWindowAttributes(hwnd, crKey, bAlpha, dwFlags);
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct WinRect
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll")]
    private static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, nint lParam);

    [DllImport("user32.dll")]
    private static extern bool IsWindow(nint hWnd);

    [DllImport("user32.dll")]
    private static extern bool IsWindowVisible(nint hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetWindowText(nint hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    private static extern int GetWindowTextLength(nint hWnd);

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(nint hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    private static extern bool GetWindowRect(nint hWnd, out WinRect lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool SetWindowPos(
        nint hWnd,
        nint hWndInsertAfter,
        int x,
        int y,
        int cx,
        int cy,
        uint uFlags);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool ShowWindow(nint hWnd, int nCmdShow);

    [DllImport("user32.dll", EntryPoint = "GetWindowLongPtrW", SetLastError = true)]
    private static extern nint GetWindowLongPtr64(nint hWnd, int nIndex);

    [DllImport("user32.dll", EntryPoint = "GetWindowLongW", SetLastError = true)]
    private static extern int GetWindowLong32(nint hWnd, int nIndex);

    [DllImport("user32.dll", EntryPoint = "SetWindowLongPtrW", SetLastError = true)]
    private static extern nint SetWindowLongPtr64(nint hWnd, int nIndex, nint dwNewLong);

    [DllImport("user32.dll", EntryPoint = "SetWindowLongW", SetLastError = true)]
    private static extern int SetWindowLong32(nint hWnd, int nIndex, int dwNewLong);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool SetLayeredWindowAttributes(nint hwnd, uint crKey, byte bAlpha, uint dwFlags);

    private static nint GetWindowLongPtrCompat(nint hWnd, int nIndex)
    {
        return IntPtr.Size == 8
            ? GetWindowLongPtr64(hWnd, nIndex)
            : new nint(GetWindowLong32(hWnd, nIndex));
    }

    private static nint SetWindowLongPtrCompat(nint hWnd, int nIndex, nint dwNewLong)
    {
        return IntPtr.Size == 8
            ? SetWindowLongPtr64(hWnd, nIndex, dwNewLong)
            : new nint(SetWindowLong32(hWnd, nIndex, dwNewLong.ToInt32()));
    }
}

public sealed class CommandPostActionExecutorService : IPostActionExecutorService
{
    public CommandPostActionExecutorService()
    {
        CapabilityMatrix = GetCapabilityMatrix();
    }

    public PostActionCapabilityMatrix CapabilityMatrix { get; }

    public PostActionCapabilityMatrix GetCapabilityMatrix(PostActionExecutorRequest? request = null)
        => PostActionExecutorSupport.BuildCapabilityMatrix(request);

    public async Task<PlatformOperationResult> ExecuteAsync(
        PostActionType action,
        PostActionExecutorRequest? request = null,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var capability = GetCapabilityMatrix(request).Get(action);
        if (!capability.Supported)
        {
            return PlatformOperation.Failed(
                capability.Provider,
                capability.Message,
                PlatformErrorCodes.PostActionUnsupported,
                operationId: $"post-action.{action}",
                usedFallback: capability.HasFallback);
        }

        return action switch
        {
            PostActionType.ExitArknights => await PostActionExecutorSupport.ExecuteLegacyCommandAsync(action, request, cancellationToken),
            PostActionType.BackToAndroidHome => await PostActionExecutorSupport.ExecuteLegacyCommandAsync(action, request, cancellationToken),
            PostActionType.ExitEmulator => await PostActionExecutorSupport.ExecuteExitEmulatorAsync(request, cancellationToken),
            PostActionType.ExitSelf => await PostActionExecutorSupport.ExecuteLegacyCommandAsync(action, request, cancellationToken),
            PostActionType.Hibernate => await PostActionExecutorSupport.ExecutePowerActionAsync(action, cancellationToken),
            PostActionType.Shutdown => await PostActionExecutorSupport.ExecutePowerActionAsync(action, cancellationToken),
            PostActionType.Sleep => await PostActionExecutorSupport.ExecutePowerActionAsync(action, cancellationToken),
            _ => PlatformOperation.Failed("post-action", "Unknown post action.", PlatformErrorCodes.PostActionExecutionFailed, $"post-action.{action}"),
        };
    }
}
