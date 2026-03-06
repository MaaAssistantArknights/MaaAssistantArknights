using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Styling;
using Avalonia.Threading;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Advanced;
using MAAUnified.App.ViewModels.Copilot;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.App.ViewModels.Toolbox;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Orchestration;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Application.Services.Localization;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels;

public sealed class MainShellViewModel : ObservableObject
{
    private readonly MAAUnifiedRuntime _runtime;
    private readonly ConnectionGameSharedStateViewModel _connectionGameSharedState;
    private readonly SemaphoreSlim _guiApplySemaphore = new(1, 1);
    private readonly HashSet<string> _reportedLocalizationFallbacks = new(StringComparer.OrdinalIgnoreCase);
    private readonly object _localizationFallbackGate = new();
    private readonly DispatcherTimer _timerScheduleTimer;
    private readonly IAppDialogService _dialogService;
    private readonly Dictionary<int, string> _timerSlotMinuteDedup = [];
    private bool _syncingConnectionState;
    private int _timerScheduleProcessing;
    private int _selectedRootTabIndex;
    private bool _isWindowTopMost;
    private string _windowTitle = "MaaAssistantArknights";
    private string _windowVersionUpdateInfo = string.Empty;
    private string _windowResourceUpdateInfo = string.Empty;
    private string _importStatus = string.Empty;
    private string _capabilitySummary = string.Empty;
    private string _globalStatus = "Initializing...";
    private string _lastError = string.Empty;
    private string _selectedImportSource = "自动(gui.new + gui)";
    private bool _hasBlockingConfigIssues;
    private int _blockingConfigIssueCount;
    private SessionState _currentSessionState;
    private string _appliedTheme = "Light";
    private Bitmap? _shellBackgroundImage;
    private double _shellBackgroundOpacity = 0.45;
    private int _shellBackgroundBlur = 12;
    private Stretch _shellBackgroundStretch = Stretch.UniformToFill;
    private bool _schemaMigrationNoticeShown;

    public MainShellViewModel(MAAUnifiedRuntime runtime, IAppDialogService? dialogService = null)
    {
        _runtime = runtime;
        _dialogService = dialogService ??
            (Avalonia.Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime
                ? new AvaloniaDialogService(runtime)
                : NoOpAppDialogService.Instance);
        _connectionGameSharedState = new ConnectionGameSharedStateViewModel();
        _connectionGameSharedState.PropertyChanged += OnSharedConnectionStateChanged;

        RootTabs = new[] { "任务队列", "Copilot", "Toolbox", "高级", "设置" };
        GrowlMessages = new ObservableCollection<string>();
        RootLogs = new ObservableCollection<string>();
        ConfigIssueDetails = new ObservableCollection<ConfigIssueDetailItem>();

        ImportSourceOptions = new ObservableCollection<string>
        {
            "自动(gui.new + gui)",
            "仅 gui.new.json",
            "仅 gui.json",
        };

        TaskQueuePage = new TaskQueuePageViewModel(runtime, _connectionGameSharedState, ReportLocalizationFallback, _dialogService);
        CopilotPage = new CopilotPageViewModel(runtime);
        ToolboxPage = new ToolboxPageViewModel(runtime);
        RemoteControlCenterPage = new RemoteControlCenterPageViewModel(runtime);
        OverlayAdvancedPage = new OverlayAdvancedPageViewModel(runtime);
        TrayIntegrationPage = new TrayIntegrationPageViewModel(runtime);
        StageManagerPage = new StageManagerPageViewModel(runtime);
        WebApiPage = new WebApiPageViewModel(runtime);
        ExternalNotificationProvidersPage = new ExternalNotificationProvidersPageViewModel(runtime);
        SettingsPage = new SettingsPageViewModel(runtime, _connectionGameSharedState, ReportLocalizationFallback, dialogService: _dialogService);
        SettingsPage.GuiSettingsApplied += OnGuiSettingsApplied;
        TaskQueuePage.Texts.FallbackReported += OnTaskQueueLocalizationFallbackReported;
        _currentSessionState = runtime.SessionService.CurrentState;
        runtime.SessionService.SessionStateChanged += OnSessionStateChanged;
        _timerScheduleTimer = new DispatcherTimer
        {
            Interval = TimeSpan.FromSeconds(1),
        };
        _timerScheduleTimer.Tick += OnTimerScheduleTick;

        _runtime.LogService.LogReceived += log =>
        {
            Dispatcher.UIThread.Post(() =>
            {
                RootLogs.Add($"[{log.Timestamp:HH:mm:ss}] {log.Level} {log.Message}");
                const int maxCount = 400;
                while (RootLogs.Count > maxCount)
                {
                    RootLogs.RemoveAt(0);
                }
            });
        };

        _runtime.ConfigurationService.ConfigChanged += _ =>
        {
            Dispatcher.UIThread.Post(() =>
            {
                SyncConnectionFromProfile();
                RefreshConfigValidationState(_runtime.ConfigurationService.CurrentValidationIssues);
            });
        };
    }

    public IReadOnlyList<string> RootTabs { get; }

    public ObservableCollection<string> ImportSourceOptions { get; }

    public ObservableCollection<string> GrowlMessages { get; }

    public ObservableCollection<string> RootLogs { get; }

    public ObservableCollection<ConfigIssueDetailItem> ConfigIssueDetails { get; }

    public TaskQueuePageViewModel TaskQueuePage { get; }

    public CopilotPageViewModel CopilotPage { get; }

    public ToolboxPageViewModel ToolboxPage { get; }

    public RemoteControlCenterPageViewModel RemoteControlCenterPage { get; }

    public OverlayAdvancedPageViewModel OverlayAdvancedPage { get; }

    public TrayIntegrationPageViewModel TrayIntegrationPage { get; }

    public StageManagerPageViewModel StageManagerPage { get; }

    public WebApiPageViewModel WebApiPage { get; }

    public ExternalNotificationProvidersPageViewModel ExternalNotificationProvidersPage { get; }

    public SettingsPageViewModel SettingsPage { get; }

    public ConnectionGameSharedStateViewModel ConnectionGameSharedState => _connectionGameSharedState;

    public IPlatformCapabilityService PlatformCapabilityService => _runtime.PlatformCapabilityService;

    public int SelectedRootTabIndex
    {
        get => _selectedRootTabIndex;
        set => SetProperty(ref _selectedRootTabIndex, Math.Clamp(value, 0, RootTabs.Count - 1));
    }

    public bool IsWindowTopMost
    {
        get => _isWindowTopMost;
        set => SetProperty(ref _isWindowTopMost, value);
    }

    public string WindowTitle
    {
        get => _windowTitle;
        set => SetProperty(ref _windowTitle, value);
    }

    public string WindowVersionUpdateInfo
    {
        get => _windowVersionUpdateInfo;
        set
        {
            if (SetProperty(ref _windowVersionUpdateInfo, value))
            {
                OnPropertyChanged(nameof(HasWindowVersionUpdateInfo));
            }
        }
    }

    public string WindowResourceUpdateInfo
    {
        get => _windowResourceUpdateInfo;
        set
        {
            if (SetProperty(ref _windowResourceUpdateInfo, value))
            {
                OnPropertyChanged(nameof(HasWindowResourceUpdateInfo));
            }
        }
    }

    public string ImportStatus
    {
        get => _importStatus;
        set => SetProperty(ref _importStatus, value);
    }

    public string CapabilitySummary
    {
        get => _capabilitySummary;
        set => SetProperty(ref _capabilitySummary, value);
    }

    public string SelectedImportSource
    {
        get => _selectedImportSource;
        set => SetProperty(ref _selectedImportSource, value);
    }

    public string GlobalStatus
    {
        get => _globalStatus;
        set => SetProperty(ref _globalStatus, value);
    }

    public string LastError
    {
        get => _lastError;
        set
        {
            if (SetProperty(ref _lastError, value))
            {
                OnPropertyChanged(nameof(HasLastError));
            }
        }
    }

    public string AppliedTheme
    {
        get => _appliedTheme;
        private set => SetProperty(ref _appliedTheme, value);
    }

    public Bitmap? ShellBackgroundImage
    {
        get => _shellBackgroundImage;
        private set
        {
            if (ReferenceEquals(_shellBackgroundImage, value))
            {
                return;
            }

            var old = _shellBackgroundImage;
            _shellBackgroundImage = value;
            OnPropertyChanged();
            OnPropertyChanged(nameof(HasShellBackgroundImage));
            old?.Dispose();
        }
    }

    public bool HasShellBackgroundImage => ShellBackgroundImage is not null;

    public double ShellBackgroundOpacity
    {
        get => _shellBackgroundOpacity;
        private set => SetProperty(ref _shellBackgroundOpacity, Math.Clamp(value, 0, 1));
    }

    public int ShellBackgroundBlur
    {
        get => _shellBackgroundBlur;
        private set => SetProperty(ref _shellBackgroundBlur, Math.Clamp(value, 0, 80));
    }

    public Stretch ShellBackgroundStretch
    {
        get => _shellBackgroundStretch;
        private set => SetProperty(ref _shellBackgroundStretch, value);
    }

    public bool HasWindowVersionUpdateInfo => !string.IsNullOrWhiteSpace(WindowVersionUpdateInfo);

    public bool HasWindowResourceUpdateInfo => !string.IsNullOrWhiteSpace(WindowResourceUpdateInfo);

    public bool HasLastError => !string.IsNullOrWhiteSpace(LastError);

    public bool HasBlockingConfigIssues
    {
        get => _hasBlockingConfigIssues;
        private set
        {
            if (SetProperty(ref _hasBlockingConfigIssues, value))
            {
                OnPropertyChanged(nameof(CanStartExecution));
            }
        }
    }

    public int BlockingConfigIssueCount
    {
        get => _blockingConfigIssueCount;
        private set => SetProperty(ref _blockingConfigIssueCount, value);
    }

    public SessionState CurrentSessionState
    {
        get => _currentSessionState;
        private set
        {
            if (SetProperty(ref _currentSessionState, value))
            {
                OnPropertyChanged(nameof(CanStartExecution));
                OnPropertyChanged(nameof(CanStopExecution));
            }
        }
    }

    public bool CanStartExecution => CurrentSessionState == SessionState.Connected && !HasBlockingConfigIssues;

    public bool CanStopExecution => CurrentSessionState == SessionState.Running;

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var loadResult = await _runtime.ConfigurationService.LoadOrBootstrapAsync(cancellationToken);
            if (loadResult.LoadedFromExistingConfig)
            {
                GlobalStatus = "已加载 config/avalonia.json";
            }
            else if (loadResult.ImportReport is not null)
            {
                GlobalStatus = $"首次导入完成: {loadResult.ImportReport.Summary}";
            }

            if (loadResult.ValidationIssues.Count > 0)
            {
                var blockingCount = loadResult.ValidationIssues.Count(i => i.Blocking);
                var warningCount = loadResult.ValidationIssues.Count - blockingCount;
                var summary = $"配置校验异常: 阻断 {blockingCount} / 预警 {warningCount}";
                GlobalStatus = summary;
                LastError = string.Join(
                    "; ",
                    loadResult.ValidationIssues
                        .Take(3)
                        .Select(i => $"{i.Code}:{i.Field}:{i.Message}"));
                await RecordFailedResultAsync(
                    "Config.LoadValidation",
                    UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, $"{summary} | {LastError}"),
                    cancellationToken: cancellationToken);
            }

            SyncConnectionFromProfile();
            RefreshConfigValidationState(loadResult.ValidationIssues);
            await ShowSchemaMigrationNoticeIfNeededAsync(loadResult, cancellationToken);

            var initResult = await _runtime.ResourceWorkflowService.InitializeCoreAsync(_runtime.ConfigurationService.CurrentConfig, cancellationToken);
            if (!initResult.Success)
            {
                LastError = $"Core 初始化失败: {initResult.Error?.Code} {initResult.Error?.Message}";
                await RecordFailedResultAsync(
                    "App.Initialize",
                    UiOperationResult.Fail(
                        initResult.Error?.Code.ToString() ?? UiErrorCode.CoreUnknown,
                        LastError,
                        initResult.Error?.Exception),
                    cancellationToken);
            }

            await TaskQueuePage.InitializeAsync(cancellationToken);
            await CopilotPage.InitializeAsync(cancellationToken);
            await ToolboxPage.InitializeAsync(cancellationToken);
            await RemoteControlCenterPage.InitializeAsync(cancellationToken);
            await OverlayAdvancedPage.InitializeAsync(cancellationToken);
            await TrayIntegrationPage.InitializeAsync(cancellationToken);
            await StageManagerPage.InitializeAsync(cancellationToken);
            await WebApiPage.InitializeAsync(cancellationToken);
            await ExternalNotificationProvidersPage.InitializeAsync(cancellationToken);
            await SettingsPage.InitializeAsync(cancellationToken);
            TaskQueuePage.SetLanguage(SettingsPage.Language);
            await ApplyGuiSettingsAsync(SettingsPage.CurrentGuiSnapshot, cancellationToken);

            _ = Task.Run(() => _runtime.SessionService.StartCallbackPumpAsync(
                callback =>
                {
                    Dispatcher.UIThread.Post(() =>
                    {
                        RootLogs.Add($"[{callback.Timestamp:HH:mm:ss}] CORE {callback.MsgName}({callback.MsgId}) {callback.PayloadJson}");
                    });
                    return Task.CompletedTask;
                },
                cancellationToken), cancellationToken);

            await RefreshCapabilitySummaryAsync(cancellationToken);
            WindowVersionUpdateInfo = "版本更新可用，点击设置 > Version Update";
            WindowResourceUpdateInfo = "资源更新可用，点击检查资源";
            await SyncTrayMenuStateAsync(cancellationToken);
            StartTimerScheduler();
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "App.Initialize",
                ex,
                UiErrorCode.UiError,
                $"初始化异常: {ex.Message}",
                cancellationToken);
        }
    }

    public async Task RegisterHotkeysAtStartupAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            await SettingsPage.RegisterHotkeysAsync(HotkeyRegistrationSource.Startup, cancellationToken);

            if (!string.IsNullOrWhiteSpace(SettingsPage.HotkeyWarningMessage))
            {
                PushGrowl(SettingsPage.HotkeyWarningMessage);
            }

            if (!string.IsNullOrWhiteSpace(SettingsPage.HotkeyErrorMessage))
            {
                LastError = SettingsPage.HotkeyErrorMessage;
                PushGrowl(SettingsPage.HotkeyErrorMessage);
            }
        }
        catch (Exception ex)
        {
            var message = $"Startup hotkey registration failed: {ex.Message}";
            await RecordUnhandledExceptionAsync(
                "App.Shell.Hotkey.Startup",
                ex,
                UiErrorCode.HotkeyRegistrationFailed,
                message,
                cancellationToken);
        }
    }

    public Task ConnectAsync(CancellationToken cancellationToken = default)
        => ExecuteConnectAsync(cancellationToken);

    public async Task ExecuteConnectAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var result = await _runtime.ShellFeatureService.ConnectAsync(
                _connectionGameSharedState.ConnectAddress,
                _connectionGameSharedState.ConnectConfig,
                string.IsNullOrWhiteSpace(_connectionGameSharedState.AdbPath) ? null : _connectionGameSharedState.AdbPath,
                cancellationToken);

            CurrentSessionState = _runtime.SessionService.CurrentState;
            if (!await ApplyResultAsync(result, "App.Shell.Connect", cancellationToken))
            {
                PushGrowl(result.Message);
                return;
            }

            GlobalStatus = result.Message;
            PushGrowl(result.Message);
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "App.Shell.Connect",
                ex,
                UiErrorCode.CoreUnknown,
                "Connection failed unexpectedly.",
                cancellationToken);
            PushGrowl(LastError);
        }
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        CurrentSessionState = _runtime.SessionService.CurrentState;
        var latestIssues = _runtime.ConfigurationService.RevalidateCurrentConfig();
        RefreshConfigValidationState(latestIssues);

        if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
        {
            await ApplyResultAsync(
                UiOperationResult.Fail(UiErrorCode.OperationAlreadyRunning, "Execution is already running."),
                "App.Shell.Start",
                cancellationToken);
            return;
        }

        if (!CanStartExecution)
        {
            if (CurrentSessionState != SessionState.Connected)
            {
                await ApplyResultAsync(
                    UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, $"Session state `{CurrentSessionState}` does not allow Start."),
                    "App.Shell.Start",
                    cancellationToken);
                return;
            }

            var first = _runtime.ConfigurationService.CurrentValidationIssues.FirstOrDefault(i => i.Blocking);
            var message = first is null
                ? "Config validation has blocking issues."
                : $"{first.Scope}:{first.Code}:{first.Field}:{first.Message}";
            await ApplyResultAsync(
                UiOperationResult.Fail(UiErrorCode.TaskValidationFailed, message),
                "App.Shell.Start",
                cancellationToken);
            await RecordConfigValidationFailureAsync(first, cancellationToken);
            return;
        }

        await TaskQueuePage.StartAsync(cancellationToken);
        CurrentSessionState = _runtime.SessionService.CurrentState;
        await SyncTrayMenuStateAsync(cancellationToken);
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        CurrentSessionState = _runtime.SessionService.CurrentState;
        if (!CanStopExecution)
        {
            await ApplyResultAsync(
                UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, $"Session state `{CurrentSessionState}` does not allow Stop."),
                "App.Shell.Stop",
                cancellationToken);
            return;
        }

        await TaskQueuePage.StopAsync(cancellationToken);
        CurrentSessionState = _runtime.SessionService.CurrentState;
        await SyncTrayMenuStateAsync(cancellationToken);
    }

    public Task ManualImportAsync(CancellationToken cancellationToken = default)
        => ExecuteManualImportAsync(cancellationToken);

    public async Task ExecuteManualImportAsync(CancellationToken cancellationToken = default)
    {
        var source = SelectedImportSource switch
        {
            "仅 gui.new.json" => ImportSource.GuiNewOnly,
            "仅 gui.json" => ImportSource.GuiOnly,
            _ => ImportSource.Auto,
        };

        var result = await _runtime.ShellFeatureService.ImportLegacyConfigAsync(source, manualImport: true, cancellationToken);
        var report = await ApplyResultAsync(result, "App.Shell.ImportLegacy", cancellationToken);
        if (report is null)
        {
            ImportStatus = result.Message;
            PushGrowl(result.Message);
            return;
        }

        ImportStatus = $"导入完成: {report.Summary}";
        GlobalStatus = ImportStatus;
        await TaskQueuePage.ReloadTasksAsync(cancellationToken);
        await SettingsPage.InitializeAsync(cancellationToken);
        TaskQueuePage.SetLanguage(SettingsPage.Language);
        SyncConnectionFromProfile();
        RefreshConfigValidationState(_runtime.ConfigurationService.CurrentValidationIssues);
        PushGrowl(ImportStatus);
        await RecordEventAsync("App.Shell.ImportLegacy.Refresh", ImportStatus, cancellationToken);
    }

    public Task SwitchLanguageCycleAsync(CancellationToken cancellationToken = default)
    {
        return SwitchLanguageCoreAsync(
            targetLanguage: null,
            successScope: "App.Shell.SwitchLanguage",
            source: null,
            cancellationToken);
    }

    public Task SwitchLanguageToAsync(string targetLanguage, CancellationToken cancellationToken = default)
    {
        return SwitchLanguageCoreAsync(
            targetLanguage,
            successScope: "App.Shell.SwitchLanguage",
            source: null,
            cancellationToken);
    }

    public async Task ExecuteSwitchLanguageAsync(string? targetLanguage = null, CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(targetLanguage))
        {
            await SwitchLanguageCycleAsync(cancellationToken);
            return;
        }

        await SwitchLanguageToAsync(targetLanguage, cancellationToken);
    }

    public async Task ExecuteTrayLanguageSwitchAsync(
        string? targetLanguage,
        string source,
        CancellationToken cancellationToken = default)
    {
        await SwitchLanguageCoreAsync(
            targetLanguage,
            successScope: "App.Shell.Tray.SwitchLanguage",
            source,
            cancellationToken);
    }

    public async Task<ShellUiAction> ExecuteTrayCommandAsync(
        TrayCommandId command,
        string source,
        CancellationToken cancellationToken = default)
    {
        var scope = GetTrayCommandScope(command);
        try
        {
            switch (command)
            {
                case TrayCommandId.Start:
                    if (!CanStartExecution)
                    {
                        CurrentSessionState = _runtime.SessionService.CurrentState;
                        var blockedMessage = CurrentSessionState switch
                        {
                            SessionState.Running or SessionState.Stopping => "任务正在执行中，Start 已禁用。",
                            SessionState.Connected => "存在阻断级配置错误，Start/LinkStart 已禁用。",
                            _ => $"当前会话状态为 {CurrentSessionState}，Start 已禁用。",
                        };
                        PushGrowl(blockedMessage);
                        await RecordEventAsync(
                            scope,
                            $"source={source}; blocked",
                            cancellationToken);
                        return ShellUiAction.None;
                    }

                    await StartAsync(cancellationToken);
                    CurrentSessionState = _runtime.SessionService.CurrentState;
                    PushGrowl(CurrentSessionState == SessionState.Running ? "开始执行" : "启动被阻断，请先修复错误。");
                    await RecordEventAsync(
                        scope,
                        $"source={source}; session={CurrentSessionState}",
                        cancellationToken);
                    return ShellUiAction.None;

                case TrayCommandId.Stop:
                    CurrentSessionState = _runtime.SessionService.CurrentState;
                    if (!CanStopExecution)
                    {
                        var blockedStopMessage = $"当前会话状态为 {CurrentSessionState}，Stop 已禁用。";
                        PushGrowl(blockedStopMessage);
                        await RecordEventAsync(
                            scope,
                            $"source={source}; blocked",
                            cancellationToken);
                        return ShellUiAction.None;
                    }

                    await StopAsync(cancellationToken);
                    PushGrowl("停止执行");
                    await RecordEventAsync(
                        scope,
                        $"source={source}; stopped",
                        cancellationToken);
                    return ShellUiAction.None;

                case TrayCommandId.ForceShow:
                    PushGrowl("主窗口已强制显示");
                    await RecordEventAsync(
                        scope,
                        $"source={source}; show",
                        cancellationToken);
                    return ShellUiAction.ShowMainWindow;

                case TrayCommandId.HideTray:
                    await SetTrayVisibleAsync(false, cancellationToken);
                    await RecordEventAsync(
                        scope,
                        $"source={source}; hide-requested",
                        cancellationToken);
                    return ShellUiAction.None;

                case TrayCommandId.ToggleOverlay:
                    await ToggleOverlayFromTrayAsync(cancellationToken);
                    await RecordEventAsync(
                        scope,
                        $"source={source}; toggled",
                        cancellationToken);
                    return ShellUiAction.None;

                case TrayCommandId.SwitchLanguage:
                    await ExecuteTrayLanguageSwitchAsync(
                        targetLanguage: null,
                        source,
                        cancellationToken);
                    return ShellUiAction.None;

                case TrayCommandId.Exit:
                    await RecordEventAsync(
                        scope,
                        $"source={source}; close",
                        cancellationToken);
                    return ShellUiAction.CloseMainWindow;

                case TrayCommandId.Restart:
                    var restartResult = await _runtime.AppLifecycleService.RestartAsync(cancellationToken);
                    if (!await ApplyResultAsync(restartResult, scope, cancellationToken))
                    {
                        PushGrowl(restartResult.Message);
                        return ShellUiAction.None;
                    }

                    PushGrowl("重启命令已触发。");
                    await RecordEventAsync(
                        scope,
                        $"source={source}; restart-launched",
                        cancellationToken);
                    return ShellUiAction.CloseMainWindow;

                default:
                    var unknownMessage = $"未知托盘命令: {command}";
                    PushGrowl(unknownMessage);
                    _ = await ApplyResultAsync(
                        UiOperationResult.Fail(UiErrorCode.UnknownTrayCommand, unknownMessage),
                        scope,
                        cancellationToken);
                    return ShellUiAction.None;
            }
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                scope,
                ex,
                UiErrorCode.UiOperationFailed,
                $"Tray command execution failed. source={source}",
                cancellationToken);
            PushGrowl(ex.Message);
            return ShellUiAction.None;
        }
    }

    public void PushGrowl(string message)
    {
        GrowlMessages.Add($"{DateTime.Now:HH:mm:ss} {message}");
        const int max = 8;
        while (GrowlMessages.Count > max)
        {
            GrowlMessages.RemoveAt(0);
        }
    }

    private void SyncConnectionToProfile(string? changedPropertyName = null)
    {
        if (_syncingConnectionState)
        {
            return;
        }

        if (!_runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return;
        }

        ConnectionGameProfileSync.WritePropertyToProfile(profile, _connectionGameSharedState, changedPropertyName);
    }

    private void SyncConnectionFromProfile()
    {
        if (!_runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return;
        }

        _syncingConnectionState = true;
        try
        {
            ConnectionGameProfileSync.ReadFromProfile(profile, _connectionGameSharedState);
        }
        finally
        {
            _syncingConnectionState = false;
        }
    }

    private void OnSharedConnectionStateChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_syncingConnectionState)
        {
            return;
        }

        if (ConnectionGameProfileSync.ShouldSyncProperty(e.PropertyName))
        {
            SyncConnectionToProfile(e.PropertyName);
        }
    }

    private void StartTimerScheduler()
    {
        if (_timerScheduleTimer.IsEnabled)
        {
            return;
        }

        _timerScheduleTimer.Start();
    }

    private void OnTimerScheduleTick(object? sender, EventArgs e)
    {
        _ = EvaluateTimerScheduleAsync(DateTimeOffset.Now);
    }

    private async Task EvaluateTimerScheduleAsync(DateTimeOffset now, CancellationToken cancellationToken = default)
    {
        if (Interlocked.Exchange(ref _timerScheduleProcessing, 1) == 1)
        {
            return;
        }

        try
        {
            var minuteKey = now.ToString("yyyyMMddHHmm", CultureInfo.InvariantCulture);
            var nowHour = now.Hour;
            var nowMinute = now.Minute;

            foreach (var slot in SettingsPage.Timers.OrderBy(static slot => slot.Index))
            {
                if (!slot.Enabled)
                {
                    continue;
                }

                if (!TryParseTimerTime(slot.Time, out var slotHour, out var slotMinute))
                {
                    continue;
                }

                if (slotHour != nowHour || slotMinute != nowMinute)
                {
                    continue;
                }

                if (_timerSlotMinuteDedup.TryGetValue(slot.Index, out var lastMinute)
                    && string.Equals(lastMinute, minuteKey, StringComparison.Ordinal))
                {
                    continue;
                }

                _timerSlotMinuteDedup[slot.Index] = minuteKey;
                await TriggerScheduledSlotAsync(slot, minuteKey, cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            // no-op
        }
        catch (Exception ex)
        {
            await RecordTimerScheduleErrorAsync($"Timer scheduler tick failed: {ex.Message}", ex, cancellationToken);
        }
        finally
        {
            Volatile.Write(ref _timerScheduleProcessing, 0);
        }
    }

    private async Task TriggerScheduledSlotAsync(
        TimerSlotViewModel slot,
        string minuteKey,
        CancellationToken cancellationToken)
    {
        CurrentSessionState = _runtime.SessionService.CurrentState;
        var sessionRunning = CurrentSessionState is SessionState.Running or SessionState.Stopping;
        var triggerMessage =
            $"slot={slot.Index}; time={slot.Time}; minute={minuteKey}; session={CurrentSessionState}; force={SettingsPage.ForceScheduledStart}";
        await RecordEventAsync("Timer.Schedule.Trigger", triggerMessage, cancellationToken);

        if (sessionRunning && !SettingsPage.ForceScheduledStart)
        {
            await RecordEventAsync(
                "Timer.Schedule.Skip",
                $"slot={slot.Index}; reason=running-without-force",
                cancellationToken);
            return;
        }

        if (sessionRunning && SettingsPage.ForceScheduledStart)
        {
            if (SettingsPage.ShowWindowBeforeForceScheduledStart)
            {
                PushGrowl("定时触发：强制执行前显示窗口。");
            }

            await StopAsync(cancellationToken);
            CurrentSessionState = _runtime.SessionService.CurrentState;
            if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
            {
                await RecordTimerScheduleErrorAsync(
                    $"slot={slot.Index}; stop failed before force scheduled restart; lastError={LastError}",
                    cancellationToken: cancellationToken);
                return;
            }

            if (!await SwitchTimerProfileIfNeededAsync(slot, cancellationToken))
            {
                return;
            }

            await StartAsync(cancellationToken);
            CurrentSessionState = _runtime.SessionService.CurrentState;
            if (CurrentSessionState == SessionState.Running)
            {
                await RecordEventAsync(
                    "Timer.Schedule.StopAndStart",
                    $"slot={slot.Index}; profile={_runtime.ConfigurationService.CurrentConfig.CurrentProfile}",
                    cancellationToken);
            }
            else
            {
                await RecordTimerScheduleErrorAsync(
                    $"slot={slot.Index}; start failed after forced stop; lastError={LastError}",
                    cancellationToken: cancellationToken);
            }

            return;
        }

        if (!await SwitchTimerProfileIfNeededAsync(slot, cancellationToken))
        {
            return;
        }

        await StartAsync(cancellationToken);
        CurrentSessionState = _runtime.SessionService.CurrentState;
        if (CurrentSessionState == SessionState.Running)
        {
            await RecordEventAsync(
                "Timer.Schedule.Start",
                $"slot={slot.Index}; profile={_runtime.ConfigurationService.CurrentConfig.CurrentProfile}",
                cancellationToken);
            return;
        }

        await RecordTimerScheduleErrorAsync(
            $"slot={slot.Index}; start failed; lastError={LastError}",
            cancellationToken: cancellationToken);
    }

    private async Task<bool> SwitchTimerProfileIfNeededAsync(TimerSlotViewModel slot, CancellationToken cancellationToken)
    {
        if (!SettingsPage.CustomTimerConfig)
        {
            return true;
        }

        var targetProfile = slot.Profile?.Trim() ?? string.Empty;
        if (string.IsNullOrWhiteSpace(targetProfile))
        {
            await RecordTimerScheduleErrorAsync(
                $"slot={slot.Index}; custom config enabled but profile is empty.",
                cancellationToken: cancellationToken);
            return false;
        }

        var config = _runtime.ConfigurationService.CurrentConfig;
        if (!config.Profiles.ContainsKey(targetProfile))
        {
            await RecordTimerScheduleErrorAsync(
                $"slot={slot.Index}; profile `{targetProfile}` does not exist.",
                cancellationToken: cancellationToken);
            return false;
        }

        if (string.Equals(config.CurrentProfile, targetProfile, StringComparison.OrdinalIgnoreCase))
        {
            return true;
        }

        try
        {
            config.CurrentProfile = targetProfile;
            await _runtime.ConfigurationService.SaveAsync(cancellationToken);
            await TaskQueuePage.ReloadTasksAsync(cancellationToken);
            await TaskQueuePage.WaitForPendingBindingAsync(cancellationToken);
            SyncConnectionFromProfile();
            await RecordEventAsync(
                "Timer.Schedule.SwitchProfile",
                $"slot={slot.Index}; profile={targetProfile}",
                cancellationToken);
            return true;
        }
        catch (Exception ex)
        {
            await RecordTimerScheduleErrorAsync(
                $"slot={slot.Index}; failed to switch profile to `{targetProfile}`.",
                ex,
                cancellationToken);
            return false;
        }
    }

    private async Task RecordTimerScheduleErrorAsync(
        string message,
        Exception? exception = null,
        CancellationToken cancellationToken = default)
    {
        LastError = message;
        await RecordErrorAsync(
            "Timer.Schedule.Error",
            message,
            exception,
            cancellationToken);
        await RecordFailedResultAsync(
            "Timer.Schedule.Error",
            UiOperationResult.Fail(UiErrorCode.UiOperationFailed, message, exception?.ToString()),
            cancellationToken);
    }

    private static bool TryParseTimerTime(string? value, out int hour, out int minute)
    {
        hour = default;
        minute = default;
        if (string.IsNullOrWhiteSpace(value))
        {
            return false;
        }

        var normalized = value.Trim();
        if (normalized.Length != 5 || normalized[2] != ':')
        {
            return false;
        }

        if (!int.TryParse(normalized.AsSpan(0, 2), NumberStyles.None, CultureInfo.InvariantCulture, out hour))
        {
            return false;
        }

        if (!int.TryParse(normalized.AsSpan(3, 2), NumberStyles.None, CultureInfo.InvariantCulture, out minute))
        {
            return false;
        }

        if (hour is < 0 or > 23)
        {
            return false;
        }

        if (minute is < 0 or > 59)
        {
            return false;
        }

        return true;
    }

    private void OnGuiSettingsApplied(object? sender, GuiSettingsAppliedEventArgs e)
    {
        Dispatcher.UIThread.Post(() => _ = ApplyGuiSettingsAsync(e.Snapshot));
    }

    private async Task ApplyGuiSettingsAsync(GuiSettingsSnapshot snapshot, CancellationToken cancellationToken = default)
    {
        var lockAcquired = false;
        try
        {
            await _guiApplySemaphore.WaitAsync(cancellationToken);
            lockAcquired = true;

            AppliedTheme = snapshot.Theme;
            if (Avalonia.Application.Current is not null)
            {
                Avalonia.Application.Current.RequestedThemeVariant = string.Equals(snapshot.Theme, "Dark", StringComparison.OrdinalIgnoreCase)
                    ? ThemeVariant.Dark
                    : ThemeVariant.Light;
            }

            TaskQueuePage.SetLanguage(snapshot.Language);
            SettingsPage.AutostartStatus = PlatformCapabilityTextMap.FormatAutostartStatus(
                snapshot.Language,
                SettingsPage.StartSelf,
                ReportLocalizationFallback);

            ShellBackgroundOpacity = snapshot.BackgroundOpacity / 100d;
            ShellBackgroundBlur = snapshot.BackgroundBlur;
            ShellBackgroundStretch = ParseStretch(snapshot.BackgroundStretchMode);
            ApplyShellBackgroundImage(snapshot.BackgroundImagePath);

            await RefreshCapabilitySummaryAsync(cancellationToken);

            var trayRefresh = await _runtime.PlatformCapabilityService.InitializeTrayAsync(
                "MaaAssistantArknights",
                PlatformCapabilityTextMap.CreateTrayMenuText(snapshot.Language, ReportLocalizationFallback),
                cancellationToken);
            if (!trayRefresh.Success)
            {
                LastError = trayRefresh.Message;
                await RecordFailedResultAsync(
                    "App.Gui.Apply.TrayRefresh",
                    trayRefresh,
                    cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            // No-op for canceled apply requests.
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "App.Gui.Apply",
                ex,
                UiErrorCode.SettingsSaveFailed,
                $"GUI apply failed: {ex.Message}",
                cancellationToken);
        }
        finally
        {
            if (lockAcquired)
            {
                _guiApplySemaphore.Release();
            }
        }
    }

    private void ApplyShellBackgroundImage(string path)
    {
        if (string.IsNullOrWhiteSpace(path))
        {
            ShellBackgroundImage = null;
            return;
        }

        try
        {
            ShellBackgroundImage = new Bitmap(path);
        }
        catch (Exception ex)
        {
            ShellBackgroundImage = null;
            LastError = $"Background image load failed: {ex.Message}";
            _ = RecordErrorAsync(
                "App.Gui.Apply.Background",
                LastError,
                ex);
            _ = RecordFailedResultAsync(
                "App.Gui.Apply.Background",
                UiOperationResult.Fail(UiErrorCode.BackgroundImagePathNotFound, LastError, ex.ToString()));
        }
    }

    private static Stretch ParseStretch(string stretch)
    {
        return Enum.TryParse<Stretch>(stretch, ignoreCase: true, out var parsed)
            ? parsed
            : Stretch.UniformToFill;
    }

    private void RefreshConfigValidationState(IReadOnlyList<ConfigValidationIssue> issues)
    {
        var blockingIssues = issues.Where(i => i.Blocking).ToArray();
        ConfigIssueDetails.Clear();
        foreach (var issue in blockingIssues)
        {
            ConfigIssueDetails.Add(new ConfigIssueDetailItem
            {
                Scope = NormalizeIssueText(issue.Scope),
                Code = NormalizeIssueText(issue.Code),
                Field = NormalizeIssueText(issue.Field),
                Blocking = issue.Blocking,
                ProfileName = NormalizeIssueText(issue.ProfileName),
                TaskIndex = issue.TaskIndex?.ToString(CultureInfo.InvariantCulture) ?? "-",
                TaskName = NormalizeIssueText(issue.TaskName),
                Message = NormalizeIssueText(issue.Message),
                SuggestedAction = NormalizeIssueText(issue.SuggestedAction),
            });
        }

        BlockingConfigIssueCount = blockingIssues.Length;
        HasBlockingConfigIssues = BlockingConfigIssueCount > 0;
        _ = SyncTrayMenuStateAsync();
    }

    private async Task ShowSchemaMigrationNoticeIfNeededAsync(
        ConfigLoadResult loadResult,
        CancellationToken cancellationToken = default)
    {
        var notice = loadResult.SchemaMigrationNotice;
        if (_schemaMigrationNoticeShown || notice is null)
        {
            return;
        }

        _schemaMigrationNoticeShown = true;
        var prompt = string.Join(
            Environment.NewLine,
            "检测到配置版本落后于最新 schema。",
            $"当前版本: v{notice.CurrentSchemaVersion}",
            $"最新版本: v{notice.LatestSchemaVersion}",
            string.Empty,
            notice.Message,
            $"建议动作: {notice.SuggestedAction}",
            $"保存后会先备份为 avalonia.json.schema-v{notice.CurrentSchemaVersion}.bak.<timestamp> 再写入最新 schema。");
        try
        {
            var completion = await _dialogService.ShowTextAsync(
                new TextDialogRequest(
                    Title: "[工作包D] 配置版本迁移提示",
                    Prompt: prompt,
                    DefaultText: string.Empty,
                    MultiLine: true,
                    ConfirmText: "我知道了",
                    CancelText: "关闭"),
                "App.Shell.Config.SchemaMigration",
                cancellationToken);
            await RecordEventAsync(
                "Config.SchemaMigration.Notice",
                $"schema={notice.CurrentSchemaVersion}->{notice.LatestSchemaVersion}; return={completion.Return}; summary={completion.Summary}",
                cancellationToken);
            if (string.Equals(completion.Summary, "dialog-service-unavailable", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(completion.Summary, "owner-unavailable", StringComparison.OrdinalIgnoreCase))
            {
                await RecordEventAsync(
                    "Config.SchemaMigration.DialogUnavailable",
                    $"schema={notice.CurrentSchemaVersion}->{notice.LatestSchemaVersion}",
                    cancellationToken);
            }
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception ex)
        {
            await RecordErrorAsync(
                "Config.SchemaMigration.DialogError",
                "Failed to show schema migration notice dialog.",
                ex,
                cancellationToken);
        }
    }

    private async Task RefreshCapabilitySummaryAsync(CancellationToken cancellationToken = default)
    {
        var snapshotResult = await _runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
        if (!snapshotResult.Success || snapshotResult.Value is null)
        {
            CapabilitySummary = PlatformCapabilityTextMap.FormatSnapshotUnavailable(
                SettingsPage.Language,
                snapshotResult.Message,
                ReportLocalizationFallback);
            return;
        }

        var snapshot = snapshotResult.Value;
        var lang = SettingsPage.Language;
        CapabilitySummary = string.Join(
            Environment.NewLine,
            BuildCapabilityLine(lang, PlatformCapabilityId.Tray, snapshot.Tray),
            BuildCapabilityLine(lang, PlatformCapabilityId.Notification, snapshot.Notification),
            BuildCapabilityLine(lang, PlatformCapabilityId.Hotkey, snapshot.Hotkey),
            BuildCapabilityLine(lang, PlatformCapabilityId.Autostart, snapshot.Autostart),
            BuildCapabilityLine(lang, PlatformCapabilityId.Overlay, snapshot.Overlay));
    }

    public async Task SetTrayVisibleAsync(bool visible, CancellationToken cancellationToken = default)
    {
        var result = await _runtime.PlatformCapabilityService.SetTrayVisibleAsync(visible, cancellationToken);
        if (!await ApplyResultAsync(result, "App.Shell.Tray.SetVisible", cancellationToken))
        {
            PushGrowl(result.Message);
            return;
        }

        PushGrowl(result.Message);
    }

    public async Task ToggleOverlayFromTrayAsync(CancellationToken cancellationToken = default)
    {
        var scope = "App.Shell.Tray.ToggleOverlay";
        try
        {
            await TaskQueuePage.ToggleOverlayAsync(cancellationToken);
            await SyncTrayMenuStateAsync(cancellationToken);

            var message = TaskQueuePage.OverlayVisible
                ? "Overlay 已开启。"
                : "Overlay 已关闭。";
            PushGrowl(message);
            await RecordEventAsync(
                scope,
                $"visible={TaskQueuePage.OverlayVisible}",
                cancellationToken);
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                scope,
                ex,
                UiErrorCode.PlatformOperationFailed,
                "Toggle overlay from tray failed.",
                cancellationToken);
            PushGrowl(ex.Message);
        }
    }

    private async Task ApplyLanguageChangeAsync(string next, CancellationToken cancellationToken = default)
    {
        SettingsPage.Language = next;
        SettingsPage.AutostartStatus = PlatformCapabilityTextMap.FormatAutostartStatus(
            next,
            SettingsPage.StartSelf,
            ReportLocalizationFallback);
        TaskQueuePage.SetLanguage(next);
        await RefreshCapabilitySummaryAsync(cancellationToken);

        var trayRefresh = await _runtime.PlatformCapabilityService.InitializeTrayAsync(
            "MaaAssistantArknights",
            PlatformCapabilityTextMap.CreateTrayMenuText(next, ReportLocalizationFallback),
            cancellationToken);
        if (!await ApplyResultAsync(trayRefresh, "App.Shell.SwitchLanguage.TrayRefresh", cancellationToken))
        {
            return;
        }
    }

    private async Task SwitchLanguageCoreAsync(
        string? targetLanguage,
        string successScope,
        string? source,
        CancellationToken cancellationToken)
    {
        var switchResult = await _runtime.ShellFeatureService.SwitchLanguageAsync(
            SettingsPage.Language,
            targetLanguage,
            cancellationToken);
        if (!switchResult.Success || string.IsNullOrWhiteSpace(switchResult.Value))
        {
            PushGrowl(switchResult.Message);
            _ = await ApplyResultAsync(
                UiOperationResult.Fail(
                    switchResult.Error?.Code ?? UiErrorCode.LanguageSwitchFailed,
                    switchResult.Message,
                    switchResult.Error?.Details),
                successScope,
                cancellationToken);
            return;
        }

        var next = switchResult.Value;
        await ApplyLanguageChangeAsync(next, cancellationToken);
        PushGrowl($"语言切换为: {next}");

        var message = source is null
            ? $"Language switched to {next}."
            : $"source={source}; target={(string.IsNullOrWhiteSpace(targetLanguage) ? "cycle" : targetLanguage)}; result={next}";
        await RecordEventAsync(
            successScope,
            message,
            cancellationToken);
    }

    public void ReportLocalizationFallback(LocalizationFallbackInfo info)
    {
        var language = UiLanguageCatalog.Normalize(info.Language);
        var key = info.Key?.Trim() ?? string.Empty;
        if (string.IsNullOrWhiteSpace(key))
        {
            return;
        }

        var dedupeKey = $"{info.Scope}|{language}|{key}";
        lock (_localizationFallbackGate)
        {
            if (!_reportedLocalizationFallbacks.Add(dedupeKey))
            {
                return;
            }
        }

        _ = RecordEventAsync(
            "Localization.Fallback",
            $"scope={info.Scope}; language={language}; key={key}; fallback={info.FallbackSource}");
    }

    private void OnTaskQueueLocalizationFallbackReported(LocalizationFallbackInfo info)
    {
        ReportLocalizationFallback(info);
    }

    private void OnSessionStateChanged(SessionState state)
    {
        if (Dispatcher.UIThread.CheckAccess())
        {
            CurrentSessionState = state;
            _ = SyncTrayMenuStateAsync();
            return;
        }

        Dispatcher.UIThread.Post(() =>
        {
            CurrentSessionState = state;
            _ = SyncTrayMenuStateAsync();
        });
    }

    private static string NormalizeIssueText(string? value)
    {
        return string.IsNullOrWhiteSpace(value) ? "-" : value.Trim();
    }

    private async Task SyncTrayMenuStateAsync(CancellationToken cancellationToken = default)
    {
        CurrentSessionState = _runtime.SessionService.CurrentState;
        var state = new TrayMenuState(
            StartEnabled: CanStartExecution,
            StopEnabled: CanStopExecution,
            OverlayEnabled: true,
            ForceShowEnabled: true,
            HideTrayEnabled: true);
        var result = await _runtime.PlatformCapabilityService.SetTrayMenuStateAsync(state, cancellationToken);
        if (!result.Success)
        {
            LastError = result.Message;
            await RecordFailedResultAsync("App.Shell.Tray.SyncState", result, cancellationToken);
        }
    }

    private static string GetTrayCommandScope(TrayCommandId command)
    {
        return command switch
        {
            TrayCommandId.Start => "App.Shell.Tray.Start",
            TrayCommandId.Stop => "App.Shell.Tray.Stop",
            TrayCommandId.ForceShow => "App.Shell.Tray.ForceShow",
            TrayCommandId.HideTray => "App.Shell.Tray.HideTray",
            TrayCommandId.ToggleOverlay => "App.Shell.Tray.ToggleOverlay",
            TrayCommandId.SwitchLanguage => "App.Shell.Tray.SwitchLanguage",
            TrayCommandId.Restart => "App.Shell.Tray.Restart",
            TrayCommandId.Exit => "App.Shell.Tray.Exit",
            _ => "App.Shell.Tray.Unknown",
        };
    }

    private string BuildCapabilityLine(string language, PlatformCapabilityId capability, PlatformCapabilityStatus status)
    {
        return PlatformCapabilityTextMap.FormatCapabilityLine(
            language,
            capability,
            status,
            ReportLocalizationFallback);
    }

    private Task RecordEventAsync(string scope, string message, CancellationToken cancellationToken = default)
    {
        return _runtime.DiagnosticsService.RecordEventAsync(scope, message, cancellationToken);
    }

    private Task RecordFailedResultAsync(string scope, UiOperationResult result, CancellationToken cancellationToken = default)
    {
        return _runtime.DiagnosticsService.RecordFailedResultAsync(scope, result, cancellationToken);
    }

    private Task RecordErrorAsync(string scope, string message, Exception? ex = null, CancellationToken cancellationToken = default)
    {
        return _runtime.DiagnosticsService.RecordErrorAsync(scope, message, ex, cancellationToken);
    }

    private Task RecordConfigValidationFailureAsync(ConfigValidationIssue? issue, CancellationToken cancellationToken = default)
    {
        return _runtime.DiagnosticsService.RecordConfigValidationFailureAsync(issue, cancellationToken);
    }

    private async Task<bool> ApplyResultAsync(UiOperationResult result, string scope, CancellationToken cancellationToken = default)
    {
        if (result.Success)
        {
            LastError = string.Empty;
            await RecordEventAsync(scope, result.Message, cancellationToken);
            return true;
        }

        LastError = result.Message;
        await RecordFailedResultAsync(scope, result, cancellationToken);
        return false;
    }

    private async Task<T?> ApplyResultAsync<T>(UiOperationResult<T> result, string scope, CancellationToken cancellationToken = default)
    {
        if (result.Success)
        {
            LastError = string.Empty;
            await RecordEventAsync(scope, result.Message, cancellationToken);
            return result.Value;
        }

        LastError = result.Message;
        await RecordFailedResultAsync(
            scope,
            UiOperationResult.Fail(
                result.Error?.Code ?? UiErrorCode.UiOperationFailed,
                result.Message,
                result.Error?.Details),
            cancellationToken);
        return default;
    }

    private async Task RecordUnhandledExceptionAsync(
        string scope,
        Exception ex,
        string code,
        string contextMessage,
        CancellationToken cancellationToken = default)
    {
        LastError = contextMessage;
        await RecordErrorAsync(scope, contextMessage, ex, cancellationToken);
        await RecordFailedResultAsync(
            scope,
            UiOperationResult.Fail(code, ex.Message, ex.ToString()),
            cancellationToken);
    }
}

public enum ShellUiAction
{
    None = 0,
    ShowMainWindow = 1,
    CloseMainWindow = 2,
}

public sealed class ConfigIssueDetailItem
{
    public required string Scope { get; init; }

    public required string Code { get; init; }

    public required string Field { get; init; }

    public required bool Blocking { get; init; }

    public required string ProfileName { get; init; }

    public required string TaskIndex { get; init; }

    public required string TaskName { get; init; }

    public required string Message { get; init; }

    public required string SuggestedAction { get; init; }
}
