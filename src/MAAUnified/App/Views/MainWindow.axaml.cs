using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Platform;
using MAAUnified.App.ViewModels;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services.Localization;
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
            PlatformCapabilityTextMap.CreateTrayMenuText(VM.SettingsPage.Language, VM.ReportLocalizationFallback));
        await HandlePlatformResultAsync("PlatformCapability.Tray.Initialize", trayInit);

        await VM.RegisterHotkeysAtStartupAsync();

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
            await VM.ExecuteConnectAsync();
        }
    }

    private async void OnImportClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ExecuteManualImportAsync();
        }
    }

    private async void OnStartClick(object? sender, RoutedEventArgs e)
    {
        await DispatchTrayCommandAsync(TrayCommandId.Start, "window-shell-menu");
    }

    private async void OnStopClick(object? sender, RoutedEventArgs e)
    {
        await DispatchTrayCommandAsync(TrayCommandId.Stop, "window-shell-menu");
    }

    private async void OnSwitchLanguageToClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        var targetLanguage = (sender as MenuItem)?.Tag as string;
        if (string.IsNullOrWhiteSpace(targetLanguage))
        {
            return;
        }

        await VM.ExecuteTrayLanguageSwitchAsync(targetLanguage, "window-shell-menu");
    }

    private async void OnForceShowClick(object? sender, RoutedEventArgs e)
    {
        await DispatchTrayCommandAsync(TrayCommandId.ForceShow, "window-shell-menu");
    }

    private async void OnHideTrayClick(object? sender, RoutedEventArgs e)
    {
        await DispatchTrayCommandAsync(TrayCommandId.HideTray, "window-shell-menu");
    }

    private async void OnToggleOverlayClick(object? sender, RoutedEventArgs e)
    {
        await DispatchTrayCommandAsync(TrayCommandId.ToggleOverlay, "window-shell-menu");
    }

    private async void OnRestartClick(object? sender, RoutedEventArgs e)
    {
        await DispatchTrayCommandAsync(TrayCommandId.Restart, "window-shell-menu");
    }

    private async void OnExitClick(object? sender, RoutedEventArgs e)
    {
        await DispatchTrayCommandAsync(TrayCommandId.Exit, "window-shell-menu");
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
                "Overlay host handle unavailable.",
                VM.ReportLocalizationFallback));
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
        await DispatchTrayCommandAsync(e.Command, e.Source);
    }

    private async Task DispatchTrayCommandAsync(
        TrayCommandId command,
        string source,
        CancellationToken cancellationToken = default)
    {
        if (VM is null)
        {
            return;
        }

        try
        {
            var action = await VM.ExecuteTrayCommandAsync(command, source, cancellationToken);
            switch (action)
            {
                case ShellUiAction.None:
                    break;
                case ShellUiAction.ShowMainWindow:
                    Show();
                    Activate();
                    break;
                case ShellUiAction.CloseMainWindow:
                    Close();
                    break;
                default:
                    break;
            }
        }
        catch (Exception ex)
        {
            await App.Runtime.DiagnosticsService.RecordErrorAsync(
                "PlatformCapability.TrayCommand",
                $"Tray command execution failed. command={command} source={source}",
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
                await DispatchTrayCommandAsync(TrayCommandId.Start, "hotkey-linkstart");
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
        var vm = VM;
        var language = vm?.SettingsPage.Language ?? "en-us";
        Action<LocalizationFallbackInfo>? reporter = vm is null ? null : vm.ReportLocalizationFallback;
        return PlatformCapabilityTextMap.FormatErrorCode(
            language,
            result.Error?.Code,
            result.Message,
            reporter);
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
