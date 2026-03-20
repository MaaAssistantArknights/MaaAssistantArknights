using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Platform;
using Avalonia.Threading;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.Infrastructure;
using MAAUnified.App.Services;
using MAAUnified.App.ViewModels;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services.Localization;
using MAAUnified.Platform;
using System.ComponentModel;

namespace MAAUnified.App.Views;

public partial class MainWindow : Window
{
    private bool _platformBound;
    private bool _dialogErrorBound;
    private bool _processingDialogErrors;
    private bool _allowLifecycleClose;
    private bool _closeRequestPending;
    private readonly object _dialogErrorGate = new();
    private readonly Queue<DialogErrorRaisedEvent> _pendingDialogErrors = [];
    private readonly HashSet<string> _pendingDialogErrorKeys = new(StringComparer.Ordinal);
    private readonly IAppDialogService _dialogService;
    private readonly ShellCloseConfirmationService _closeConfirmationService;
    private OverlayHostWindow? _overlayHostWindow;
    private RuntimeLogWindow? _runtimeLogWindow;

    public MainWindow()
    {
        InitializeComponent();
        WindowVisuals.ApplyDefaultIcon(this);
        _dialogService = Avalonia.Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime
            ? new AvaloniaDialogService(App.Runtime)
            : NoOpAppDialogService.Instance;
        _closeConfirmationService = new ShellCloseConfirmationService(_dialogService);
        BindDialogErrorEvents();
        Opened += OnWindowOpened;
        Closing += OnWindowClosing;
        Closed += OnWindowClosed;
    }

    private MainShellViewModel? VM => DataContext as MainShellViewModel;

    private async void OnWindowClosing(object? sender, CancelEventArgs e)
    {
        if (_allowLifecycleClose || VM is null)
        {
            return;
        }

        e.Cancel = true;
        if (_closeRequestPending)
        {
            return;
        }

        _closeRequestPending = true;
        try
        {
            if (!await ConfirmCloseAsync("App.Shell.Window.Close.Confirm"))
            {
                await App.Runtime.DiagnosticsService.RecordEventAsync(
                    "App.Shell.Window.Close",
                    "source=window-chrome; cancelled");
                return;
            }

            await App.Runtime.DiagnosticsService.RecordEventAsync(
                "App.Shell.Window.Close",
                "source=window-chrome; confirmed");
            _ = await ExitApplicationAsync("App.Shell.Window.Close.Exit");
        }
        finally
        {
            if (!_allowLifecycleClose)
            {
                _closeRequestPending = false;
            }
        }
    }

    private async void OnWindowOpened(object? sender, EventArgs e)
    {
        Program.RecordStartupStage("MainWindow.Opened", "Main window opened.");
        StartDialogErrorPumpIfNeeded();
        var vm = VM;
        if (vm is null || _platformBound)
        {
            return;
        }

        Program.RecordStartupStage("MainWindow.PlatformInit.WaitFirstScreen.Begin", "Waiting for first screen before platform initialization.");
        await vm.WaitForFirstScreenReadyAsync();
        Program.RecordStartupStage("MainWindow.PlatformInit.WaitFirstScreen.End", "First screen ready; continuing platform initialization.");

        vm = VM;
        if (vm is null || _platformBound || !IsVisible)
        {
            return;
        }

        Program.RecordStartupStage("MainWindow.PlatformInit.Begin", "Initializing tray, hotkeys, and overlay host.");
        vm.PlatformCapabilityService.TrayCommandInvoked += OnTrayCommandInvoked;
        vm.PlatformCapabilityService.GlobalHotkeyTriggered += OnGlobalHotkeyTriggered;
        vm.PlatformCapabilityService.OverlayStateChanged += OnPlatformOverlayStateChanged;
        _platformBound = true;

        var trayInit = await vm.PlatformCapabilityService.InitializeTrayAsync(
            "MaaAssistantArknights",
            PlatformCapabilityTextMap.CreateTrayMenuText(vm.SettingsPage.Language, vm.ReportLocalizationFallback));
        await HandlePlatformResultAsync("PlatformCapability.Tray.Initialize", trayInit);

        await vm.RegisterHotkeysAtStartupAsync();
        try
        {
            await EnsureOverlayHostBoundAsync();
        }
        catch (Exception ex)
        {
            await App.Runtime.DiagnosticsService.RecordErrorAsync(
                "PlatformCapability.Overlay.BindHost",
                "Overlay host initialization failed during window startup.",
                ex);
        }

        Program.RecordStartupStage("MainWindow.PlatformInit.End", "Platform initialization completed.");
    }

    private async void OnWindowClosed(object? sender, EventArgs e)
    {
        if (_dialogErrorBound)
        {
            App.Runtime.DialogFeatureService.ErrorRaised -= OnDialogErrorRaised;
            lock (_dialogErrorGate)
            {
                _pendingDialogErrors.Clear();
                _pendingDialogErrorKeys.Clear();
                _processingDialogErrors = false;
            }

            _dialogErrorBound = false;
        }

        VM?.CancelStartupInitialization();

        if (VM is not null && _platformBound)
        {
            VM.PlatformCapabilityService.TrayCommandInvoked -= OnTrayCommandInvoked;
            VM.PlatformCapabilityService.GlobalHotkeyTriggered -= OnGlobalHotkeyTriggered;
            VM.PlatformCapabilityService.OverlayStateChanged -= OnPlatformOverlayStateChanged;
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

        if (_runtimeLogWindow is not null)
        {
            _runtimeLogWindow.Closed -= OnRuntimeLogWindowClosed;
            _runtimeLogWindow.Close();
            _runtimeLogWindow = null;
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

    private async void OnManualUpdateClick(object? sender, PointerPressedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        e.Handled = true;
        await VM.SettingsPage.CheckVersionUpdateWithDialogAsync();
    }

    private void OnManualUpdateResourceClick(object? sender, PointerPressedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        e.Handled = true;
        _ = VM.SettingsPage.ManualUpdateResourceAsync();
    }

    private void OnToggleTopMostClick(object? sender, RoutedEventArgs e)
    {
        if (VM is null)
        {
            return;
        }

        VM.IsWindowTopMost = !VM.IsWindowTopMost;
    }

    public void OpenRuntimeLogWindow()
    {
        if (VM is null)
        {
            return;
        }

        if (_runtimeLogWindow is null)
        {
            _runtimeLogWindow = new RuntimeLogWindow
            {
                DataContext = VM,
            };
            _runtimeLogWindow.Closed += OnRuntimeLogWindowClosed;
            _runtimeLogWindow.Show(this);
            return;
        }

        if (_runtimeLogWindow.WindowState == WindowState.Minimized)
        {
            _runtimeLogWindow.WindowState = WindowState.Normal;
        }

        _runtimeLogWindow.Activate();
    }

    private void OnRuntimeLogWindowClosed(object? sender, EventArgs e)
    {
        if (_runtimeLogWindow is not null)
        {
            _runtimeLogWindow.Closed -= OnRuntimeLogWindowClosed;
            _runtimeLogWindow = null;
        }
    }

    private async Task EnsureOverlayHostBoundAsync(CancellationToken cancellationToken = default)
    {
        if (VM is null || _overlayHostWindow is not null)
        {
            return;
        }

        _overlayHostWindow = new OverlayHostWindow
        {
            DataContext = VM.OverlayPresentation,
        };
        _overlayHostWindow.Show();
        var handle = _overlayHostWindow.TryGetPlatformHandle()?.Handle ?? nint.Zero;
        if (handle == nint.Zero)
        {
            await Dispatcher.UIThread.InvokeAsync(static () => { }, DispatcherPriority.Loaded);
            handle = _overlayHostWindow.TryGetPlatformHandle()?.Handle ?? nint.Zero;
        }

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

    private void OnPlatformOverlayStateChanged(object? sender, OverlayStateChangedEvent e)
    {
        if (Dispatcher.UIThread.CheckAccess())
        {
            ApplyOverlayHostState(e);
            return;
        }

        Dispatcher.UIThread.Post(() => ApplyOverlayHostState(e));
    }

    private void ApplyOverlayHostState(OverlayStateChangedEvent e)
    {
        if (_overlayHostWindow is null)
        {
            return;
        }

        _overlayHostWindow.SetOverlayActive(e.Visible);
        if (!e.Visible || e.Mode != OverlayRuntimeMode.Preview)
        {
            return;
        }

        try
        {
            var screens = _overlayHostWindow.Screens;
            var screen = screens.ScreenFromWindow(this)
                ?? screens.ScreenFromWindow(_overlayHostWindow)
                ?? screens.Primary;
            if (screen is null)
            {
                return;
            }

            _overlayHostWindow.ApplyPreviewBounds(screen.WorkingArea);
        }
        catch (ObjectDisposedException)
        {
            // Ignore late overlay events while the shell is shutting down.
        }
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
            if (command is TrayCommandId.Exit or TrayCommandId.Restart)
            {
                var confirmScope = command == TrayCommandId.Restart
                    ? "App.Shell.Tray.Restart.Confirm"
                    : "App.Shell.Tray.Exit.Confirm";
                if (!await ConfirmCloseAsync(confirmScope, cancellationToken))
                {
                    await App.Runtime.DiagnosticsService.RecordEventAsync(
                        confirmScope,
                        $"source={source}; cancelled",
                        cancellationToken);
                    return;
                }
            }

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
                    var exitScope = command == TrayCommandId.Restart
                        ? "App.Shell.Tray.Restart.Exit"
                        : "App.Shell.Tray.Exit";
                    _ = await ExitApplicationAsync(exitScope, cancellationToken);
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

    private Task<bool> ConfirmCloseAsync(string sourceScope, CancellationToken cancellationToken = default)
    {
        var vm = VM;
        if (vm is null)
        {
            return Task.FromResult(true);
        }

        return _closeConfirmationService.ConfirmCloseAsync(
            vm.RootTexts,
            vm.SettingsPage.Language,
            vm.TaskQueuePage.IsRunning,
            vm.SettingsPage.IsVersionUpdateActionRunning,
            sourceScope,
            cancellationToken);
    }

    private async Task<bool> ExitApplicationAsync(string scope, CancellationToken cancellationToken = default)
    {
        var vm = VM;
        _allowLifecycleClose = true;
        var result = await App.Runtime.AppLifecycleService.ExitAsync(cancellationToken);
        if (result.Success)
        {
            return true;
        }

        _allowLifecycleClose = false;
        _closeRequestPending = false;
        if (vm is not null)
        {
            vm.PushGrowl(result.Message);
        }

        await App.Runtime.DiagnosticsService.RecordFailedResultAsync(scope, result, cancellationToken);
        return false;
    }

    private void OnDialogErrorRaised(object? sender, DialogErrorRaisedEvent e)
    {
        Dispatcher.UIThread.Post(() => EnqueueDialogError(e));
    }

    private void EnqueueDialogError(DialogErrorRaisedEvent dialogError)
    {
        var key = BuildDialogErrorKey(dialogError);
        var shouldPump = false;
        lock (_dialogErrorGate)
        {
            if (!_pendingDialogErrorKeys.Add(key))
            {
                return;
            }

            _pendingDialogErrors.Enqueue(dialogError);
            if (!_processingDialogErrors && IsVisible)
            {
                _processingDialogErrors = true;
                shouldPump = true;
            }
        }

        if (shouldPump)
        {
            _ = ProcessDialogErrorQueueAsync();
        }
    }

    private void BindDialogErrorEvents()
    {
        if (_dialogErrorBound)
        {
            return;
        }

        App.Runtime.DialogFeatureService.ErrorRaised += OnDialogErrorRaised;
        _dialogErrorBound = true;
    }

    private void StartDialogErrorPumpIfNeeded()
    {
        var shouldPump = false;
        lock (_dialogErrorGate)
        {
            if (_processingDialogErrors || _pendingDialogErrors.Count == 0 || !IsVisible)
            {
                return;
            }

            _processingDialogErrors = true;
            shouldPump = true;
        }

        if (shouldPump)
        {
            _ = ProcessDialogErrorQueueAsync();
        }
    }

    private async Task ProcessDialogErrorQueueAsync()
    {
        while (true)
        {
            DialogErrorRaisedEvent dialogError;
            string key;
            lock (_dialogErrorGate)
            {
                if (_pendingDialogErrors.Count == 0)
                {
                    _processingDialogErrors = false;
                    return;
                }

                dialogError = _pendingDialogErrors.Dequeue();
                key = BuildDialogErrorKey(dialogError);
            }

            try
            {
                await ShowErrorDialogAsync(dialogError);
            }
            catch (Exception ex)
            {
                await App.Runtime.DiagnosticsService.RecordErrorAsync(
                    "Dialog.ErrorPopup",
                    $"Failed to show error dialog. context={dialogError.Context} code={dialogError.Result.Error?.Code ?? UiErrorCode.UiOperationFailed}",
                    ex);
            }
            finally
            {
                lock (_dialogErrorGate)
                {
                    _pendingDialogErrorKeys.Remove(key);
                }
            }
        }
    }

    private async Task ShowErrorDialogAsync(DialogErrorRaisedEvent dialogError)
    {
        var language = VM?.SettingsPage.Language ?? UiLanguageCatalog.FallbackLanguage;
        var localizedResult = DialogTextCatalog.LocalizeErrorResult(language, dialogError.Result);
        var request = new ErrorDialogRequest(
            Title: DialogTextCatalog.ErrorDialogTitle(language),
            Context: dialogError.Context,
            Result: localizedResult,
            Suggestion: DialogTextCatalog.BuildErrorSuggestion(language, dialogError.Result),
            ConfirmText: DialogTextCatalog.ErrorDialogCloseButton(language),
            CancelText: DialogTextCatalog.ErrorDialogIgnoreButton(language),
            Language: language);
        Func<CancellationToken, Task<UiOperationResult>>? openIssueReportAsync = VM is null
            ? null
            : VM.SettingsPage.OpenIssueReportEntryForDialogAsync;
        await _dialogService.ShowErrorAsync(
            request,
            "MainWindow.DialogFeature.ErrorPopup",
            openIssueReportAsync,
            CancellationToken.None);
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

    private static string BuildDialogErrorKey(DialogErrorRaisedEvent dialogError)
    {
        var code = dialogError.Result.Error?.Code ?? UiErrorCode.UiOperationFailed;
        return $"{dialogError.Context}|{code}|{dialogError.Result.Message}";
    }
}
