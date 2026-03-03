using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Platform;
using MAAUnified.App.ViewModels;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Platform;

namespace MAAUnified.App.Views;

public partial class MainWindow : Window
{
    private bool _platformBound;
    private OverlayHostWindow? _overlayHostWindow;

    public MainWindow()
    {
        InitializeComponent();
        Opened += OnWindowOpened;
        Closed += OnWindowClosed;
    }

    private MainShellViewModel? VM => DataContext as MainShellViewModel;

    private async void OnWindowOpened(object? sender, EventArgs e)
    {
        if (VM is null || _platformBound)
        {
            return;
        }

        VM.PlatformCapabilityService.TrayCommandInvoked += OnTrayCommandInvoked;
        VM.PlatformCapabilityService.GlobalHotkeyTriggered += OnGlobalHotkeyTriggered;
        _platformBound = true;

        var trayInit = await VM.PlatformCapabilityService.InitializeTrayAsync(
            "MaaAssistantArknights",
            PlatformCapabilityTextMap.CreateTrayMenuText(VM.SettingsPage.Language));
        await HandlePlatformResultAsync("PlatformCapability.Tray.Initialize", trayInit);

        var registerShow = await VM.PlatformCapabilityService.RegisterGlobalHotkeyAsync("ShowGui", VM.SettingsPage.HotkeyShowGui);
        await HandlePlatformResultAsync("PlatformCapability.Hotkey.Register.ShowGui", registerShow);

        var registerLink = await VM.PlatformCapabilityService.RegisterGlobalHotkeyAsync("LinkStart", VM.SettingsPage.HotkeyLinkStart);
        await HandlePlatformResultAsync("PlatformCapability.Hotkey.Register.LinkStart", registerLink);

        await EnsureOverlayHostBoundAsync();
    }

    private async void OnWindowClosed(object? sender, EventArgs e)
    {
        if (VM is not null && _platformBound)
        {
            VM.PlatformCapabilityService.TrayCommandInvoked -= OnTrayCommandInvoked;
            VM.PlatformCapabilityService.GlobalHotkeyTriggered -= OnGlobalHotkeyTriggered;
            _platformBound = false;
            _ = await VM.PlatformCapabilityService.UnregisterGlobalHotkeyAsync("ShowGui");
            _ = await VM.PlatformCapabilityService.UnregisterGlobalHotkeyAsync("LinkStart");
            _ = await VM.PlatformCapabilityService.ShutdownTrayAsync();
        }

        if (_overlayHostWindow is not null)
        {
            _overlayHostWindow.Close();
            _overlayHostWindow = null;
        }
    }

    private async void OnConnectClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ConnectAsync();
            VM.PushGrowl("连接命令已执行");
        }
    }

    private async void OnImportClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ManualImportAsync();
            VM.PushGrowl("旧配置导入已执行");
        }
    }

    private async void OnStartClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            if (!VM.CanStartExecution)
            {
                VM.PushGrowl("存在阻断级配置错误，Start/LinkStart 已禁用。");
                return;
            }

            await VM.StartAsync();
            VM.PushGrowl(VM.TaskQueuePage.IsRunning ? "开始执行" : "启动被阻断，请先修复错误。");
        }
    }

    private async void OnStopClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StopAsync();
            VM.PushGrowl("停止执行");
        }
    }

    private async void OnSwitchLanguageClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        var next = VM.SwitchLanguageCycle();
        VM.PushGrowl($"语言切换为: {next}");
        var refreshResult = await VM.PlatformCapabilityService.InitializeTrayAsync(
            "MaaAssistantArknights",
            PlatformCapabilityTextMap.CreateTrayMenuText(next));
        await HandlePlatformResultAsync("PlatformCapability.Tray.Initialize", refreshResult);
    }

    private void OnForceShowClick(object? sender, RoutedEventArgs e)
    {
        Show();
        Activate();
        VM?.PushGrowl("主窗口已强制显示");
    }

    private async void OnHideTrayClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SetTrayVisibleAsync(false);
        }
    }

    private async void OnToggleOverlayClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ToggleOverlayFromTrayAsync();
            VM.PushGrowl("Overlay 切换已触发");
        }
    }

    private void OnRestartClick(object? sender, RoutedEventArgs e)
    {
        VM?.PushGrowl("重启命令已记录。请手动重启进程（当前为跨平台占位实现）。");
    }

    private void OnExitClick(object? sender, RoutedEventArgs e)
    {
        Close();
    }

    private void OnManualUpdateClick(object? sender, PointerPressedEventArgs e)
    {
        VM?.PushGrowl("手动更新入口：设置 > Version Update");
    }

    private void OnManualUpdateResourceClick(object? sender, PointerPressedEventArgs e)
    {
        VM?.PushGrowl("手动资源更新入口：设置 > Version Update");
    }

    private async Task EnsureOverlayHostBoundAsync(CancellationToken cancellationToken = default)
    {
        if (VM is null || _overlayHostWindow is not null)
        {
            return;
        }

        _overlayHostWindow = new OverlayHostWindow();
        _overlayHostWindow.Show();
        _overlayHostWindow.Hide();

        var handle = _overlayHostWindow.TryGetPlatformHandle()?.Handle ?? nint.Zero;
        if (handle == nint.Zero)
        {
            VM.PushGrowl(PlatformCapabilityTextMap.GetUiText(
                VM.SettingsPage.Language,
                "Ui.OverlayHostUnavailable",
                "Overlay host handle unavailable."));
            await App.Runtime.DiagnosticsService.RecordFailedResultAsync(
                "PlatformCapability.Overlay.BindHost",
                UiOperationResult.Fail(PlatformErrorCodes.OverlayHostNotBound, "Overlay host handle unavailable."),
                cancellationToken);
            return;
        }

        var result = await VM.PlatformCapabilityService.BindOverlayHostAsync(
            handle,
            clickThrough: true,
            opacity: 0.85,
            cancellationToken);
        await HandlePlatformResultAsync("PlatformCapability.Overlay.BindHost", result, cancellationToken);
    }

    private async void OnTrayCommandInvoked(object? sender, TrayCommandEvent e)
    {
        if (VM is null)
        {
            return;
        }

        try
        {
            switch (e.Command)
            {
                case TrayCommandId.Start:
                    await VM.StartAsync();
                    break;
                case TrayCommandId.Stop:
                    await VM.StopAsync();
                    break;
                case TrayCommandId.ForceShow:
                    Show();
                    Activate();
                    break;
                case TrayCommandId.HideTray:
                    await VM.SetTrayVisibleAsync(false);
                    break;
                case TrayCommandId.ToggleOverlay:
                    await VM.ToggleOverlayFromTrayAsync();
                    break;
                case TrayCommandId.Exit:
                    Close();
                    break;
                default:
                    VM.PushGrowl(string.Format(
                        PlatformCapabilityTextMap.GetUiText(
                            VM.SettingsPage.Language,
                            "Ui.UnknownTrayCommand",
                            "Unknown tray command: {0}"),
                        e.Command));
                    break;
            }
        }
        catch (Exception ex)
        {
            await App.Runtime.DiagnosticsService.RecordErrorAsync(
                "PlatformCapability.TrayCommand",
                "Tray command execution failed.",
                ex);
        }
    }

    private async void OnGlobalHotkeyTriggered(object? sender, GlobalHotkeyTriggeredEvent e)
    {
        if (VM is null)
        {
            return;
        }

        try
        {
            if (string.Equals(e.Name, "ShowGui", StringComparison.OrdinalIgnoreCase))
            {
                Show();
                Activate();
                return;
            }

            if (string.Equals(e.Name, "LinkStart", StringComparison.OrdinalIgnoreCase))
            {
                if (VM.CanStartExecution)
                {
                    await VM.StartAsync();
                }
            }
        }
        catch (Exception ex)
        {
            await App.Runtime.DiagnosticsService.RecordErrorAsync(
                "PlatformCapability.HotkeyTriggered",
                "Global hotkey execution failed.",
                ex);
        }
    }

    private string Localize(UiOperationResult result)
    {
        var language = VM?.SettingsPage.Language ?? "en-us";
        return PlatformCapabilityTextMap.FormatErrorCode(language, result.Error?.Code, result.Message);
    }

    private async Task HandlePlatformResultAsync(
        string scope,
        UiOperationResult result,
        CancellationToken cancellationToken = default)
    {
        if (VM is null)
        {
            return;
        }

        if (!result.Success)
        {
            VM.PushGrowl(Localize(result));
            await App.Runtime.DiagnosticsService.RecordFailedResultAsync(scope, result, cancellationToken);
            return;
        }

        if (IsFallbackMessage(result.Message))
        {
            VM.PushGrowl(result.Message);
        }
    }

    private static bool IsFallbackMessage(string message)
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return false;
        }

        return message.Contains("fallback", StringComparison.OrdinalIgnoreCase)
               || message.Contains("降级", StringComparison.OrdinalIgnoreCase)
               || message.Contains("preview", StringComparison.OrdinalIgnoreCase);
    }
}
