using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Text.Json.Nodes;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Styling;
using Avalonia.Threading;
using MAAUnified.App.Features.Dialogs;
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
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels;

public sealed class MainShellViewModel : ObservableObject
{
    private const string AppDisplayName = "MaaAssistantArknights";
    private const string DeveloperModeConfigKey = "GUI.DeveloperMode";
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
    private string _windowTitle = AppDisplayName;
    private string _windowVersionUpdateInfo = string.Empty;
    private string _windowResourceUpdateInfo = string.Empty;
    private string _importStatus = string.Empty;
    private string _capabilitySummary = string.Empty;
    private string _globalStatus = "Initializing...";
    private string _lastError = string.Empty;
    private ImportSource _selectedImportSource = ImportSource.Auto;
    private ImportSourceOptionItem? _selectedImportSourceOption;
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

        RootTexts = new RootLocalizationTextMap("Root.Localization.MainShell");
        RootTexts.FallbackReported += ReportLocalizationFallback;
        RootTabs = new[] { "TaskQueue", "Copilot", "Toolbox", "Settings" };
        GrowlMessages = new ObservableCollection<string>();
        RootLogs = new ObservableCollection<string>();
        ConfigIssueDetails = new ObservableCollection<ConfigIssueDetailItem>();

        ImportSourceOptions = new ObservableCollection<ImportSourceOptionItem>();
        RefreshRootTextState();

        TaskQueuePage = new TaskQueuePageViewModel(
            runtime,
            _connectionGameSharedState,
            ReportLocalizationFallback,
            _dialogService,
            NavigateToSettingsSection);
        CopilotPage = new CopilotPageViewModel(runtime);
        ToolboxPage = new ToolboxPageViewModel(runtime, _connectionGameSharedState);
        SettingsPage = new SettingsPageViewModel(runtime, _connectionGameSharedState, ReportLocalizationFallback, dialogService: _dialogService);
        SettingsPage.GuiSettingsApplied += OnGuiSettingsApplied;
        SettingsPage.ResourceVersionUpdated += OnSettingsResourceVersionUpdated;
        SettingsPage.ConfigurationContextChanged += OnSettingsConfigurationContextChanged;
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
                AppendRootLogEntry($"[{log.Timestamp:HH:mm:ss}] {log.Level} {log.Message}");
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

    public RootLocalizationTextMap RootTexts { get; }

    public ObservableCollection<ImportSourceOptionItem> ImportSourceOptions { get; }

    public ObservableCollection<string> GrowlMessages { get; }

    public ObservableCollection<string> RootLogs { get; }

    public ObservableCollection<ConfigIssueDetailItem> ConfigIssueDetails { get; }

    public TaskQueuePageViewModel TaskQueuePage { get; }

    public CopilotPageViewModel CopilotPage { get; }

    public ToolboxPageViewModel ToolboxPage { get; }

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
                RefreshWindowTitle();
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
                RefreshWindowTitle();
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

    public ImportSource SelectedImportSource
    {
        get => _selectedImportSource;
        set => SetProperty(ref _selectedImportSource, value);
    }

    public ImportSourceOptionItem? SelectedImportSourceOption
    {
        get => _selectedImportSourceOption;
        set
        {
            if (!SetProperty(ref _selectedImportSourceOption, value))
            {
                return;
            }

            SelectedImportSource = value?.Source ?? ImportSource.Auto;
        }
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
        private set
        {
            if (SetProperty(ref _blockingConfigIssueCount, value))
            {
                OnPropertyChanged(nameof(BlockingConfigIssueSummary));
            }
        }
    }

    public string BlockingConfigIssueSummary
        => string.Format(
            CultureInfo.CurrentCulture,
            RootTexts["Main.Blocking.Title"],
            BlockingConfigIssueCount);

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
                GlobalStatus = ImportReportTextFormatter.BuildStatusMessage(loadResult.ImportReport, manualImport: false);
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

            ApplyDeveloperModeFromConfig();
            _runtime.LogService.Debug(
                $"App init start: profile={_runtime.ConfigurationService.CurrentConfig.CurrentProfile}, state={_runtime.SessionService.CurrentState}");

            SyncConnectionFromProfile();
            RefreshConfigValidationState(loadResult.ValidationIssues);
            await ShowSchemaMigrationNoticeIfNeededAsync(loadResult, cancellationToken);

            _runtime.LogService.Debug("Begin core initialization from startup pipeline.");
            var initResult = await _runtime.ResourceWorkflowService.InitializeCoreAsync(_runtime.ConfigurationService.CurrentConfig, cancellationToken);
            if (!initResult.Success)
            {
                var initCode = initResult.Error?.Code.ToString() ?? UiErrorCode.CoreUnknown;
                var initMessage = initResult.Error?.Message ?? "Core initialize failed.";
                LastError = BuildBilingualMessage(
                    $"Core 初始化失败: {initCode} {initMessage}",
                    $"Core initialize failed: {initCode} {initMessage}");
                await ApplyResultAsync(
                    UiOperationResult.Fail(
                        initCode,
                        LastError,
                        initResult.Error?.Exception),
                    "App.Initialize",
                    cancellationToken);
            }
            else
            {
                _runtime.LogService.Debug($"Core initialization succeeded: version={initResult.Value?.CoreVersion}");
            }

            await TaskQueuePage.InitializeAsync(cancellationToken);
            await CopilotPage.InitializeAsync(cancellationToken);
            await ToolboxPage.InitializeAsync(cancellationToken);
            await SettingsPage.InitializeAsync(cancellationToken);
            TaskQueuePage.SetLanguage(SettingsPage.Language);
            await ApplyGuiSettingsAsync(SettingsPage.CurrentGuiSnapshot, cancellationToken);
            AppendImportReportToTaskQueue(loadResult.ImportReport, manualImport: false);

            _ = Task.Run(
                () => _runtime.SessionService.StartCallbackPumpAsync(
                    callback => Dispatcher.UIThread.InvokeAsync(() => ApplySessionCallback(callback)).GetTask(),
                    cancellationToken),
                cancellationToken);

            await RefreshCapabilitySummaryAsync(cancellationToken);
            RefreshRootTextState();
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
            var result = await ConnectWithCurrentSettingsAsync(cancellationToken);
            if (!result.Success)
            {
                result = UiOperationResult.Fail(
                    result.Error?.Code ?? UiErrorCode.UiOperationFailed,
                    BuildConnectionFailureMessage(result),
                    result.Error?.Details);
            }

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
        UiOperationResult? startConnectResult = null;

        if (CurrentSessionState is SessionState.Running or SessionState.Stopping)
        {
            await ApplyResultAsync(
                UiOperationResult.Fail(UiErrorCode.OperationAlreadyRunning, "Execution is already running."),
                "App.Shell.Start",
                cancellationToken);
            return;
        }

        if (CurrentSessionState != SessionState.Connected)
        {
            var connectResult = await ConnectWithCurrentSettingsAsync(cancellationToken);
            startConnectResult = connectResult;
            CurrentSessionState = _runtime.SessionService.CurrentState;
            if (connectResult.Success)
            {
                GlobalStatus = connectResult.Message;
                PushGrowl(connectResult.Message);
            }
        }

        if (!CanStartExecution)
        {
            if (CurrentSessionState != SessionState.Connected)
            {
                var stateMessage = startConnectResult is { Success: false } failedConnect
                    ? BuildConnectionFailureMessage(failedConnect)
                    : BuildLinkStartStateNotAllowedMessage(CurrentSessionState);

                await ApplyResultAsync(
                    UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, stateMessage),
                    "App.Shell.Start",
                    cancellationToken);
                NavigateToSettingsSection("Connect");
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

    private async Task<UiOperationResult> ConnectWithCurrentSettingsAsync(CancellationToken cancellationToken)
    {
        var effectiveAdbPath = _connectionGameSharedState.ResolveEffectiveAdbPath(updateStateWhenResolved: true);
        var adbPath = string.IsNullOrWhiteSpace(effectiveAdbPath) ? null : effectiveAdbPath;
        var candidates = _connectionGameSharedState.BuildConnectAddressCandidates(includeConfiguredAddress: true);
        _runtime.LogService.Debug(
            $"Connect candidates prepared: count={candidates.Count}, config={_connectionGameSharedState.ConnectConfig}, adb={adbPath ?? "<null>"}");
        UiOperationResult? lastFailure = null;

        foreach (var candidate in candidates)
        {
            _runtime.LogService.Debug($"Trying connect candidate: {candidate}");
            var result = await _runtime.ShellFeatureService.ConnectAsync(
                candidate,
                _connectionGameSharedState.ConnectConfig,
                adbPath,
                cancellationToken);
            if (result.Success)
            {
                _runtime.LogService.Debug($"Connect candidate succeeded: {candidate}");
                _connectionGameSharedState.ConnectAddress = candidate;
                return result;
            }

            _runtime.LogService.Debug(
                $"Connect candidate failed: {candidate}, code={result.Error?.Code}, message={result.Message}");
            lastFailure = result;
        }

        return lastFailure ?? UiOperationResult.Fail(UiErrorCode.UiOperationFailed, "Connection failed.");
    }

    private string BuildConnectionFailureMessage(UiOperationResult connectResult)
    {
        var segments = new List<string>
        {
            BuildBilingualMessage(
                "连接失败。请“检查连接设置” -> “尝试重启模拟器与 ADB” -> “重启电脑”。",
                "Connection failed. Check connection settings -> try restarting the emulator and ADB -> reboot the computer."),
        };

        var settingsHint = _connectionGameSharedState.BuildConnectionSettingsHintMessage();
        if (!string.IsNullOrWhiteSpace(settingsHint))
        {
            segments.Add(settingsHint);
        }

        if (!string.IsNullOrWhiteSpace(connectResult.Message)
            && !string.Equals(connectResult.Message, "Connection failed.", StringComparison.OrdinalIgnoreCase))
        {
            segments.Add(BuildBilingualMessage(
                $"连接回调：{connectResult.Message}",
                $"Connection callback: {connectResult.Message}"));
        }

        return string.Join(Environment.NewLine, segments);
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        CurrentSessionState = _runtime.SessionService.CurrentState;
        if (!CanStopExecution)
        {
            await ApplyResultAsync(
                UiOperationResult.Fail(UiErrorCode.SessionStateNotAllowed, BuildStopStateNotAllowedMessage(CurrentSessionState)),
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
        var result = await _runtime.ShellFeatureService.ImportLegacyConfigAsync(SelectedImportSource, manualImport: true, cancellationToken);
        var report = await ApplyResultAsync(result, "App.Shell.ImportLegacy", cancellationToken);
        if (report is null)
        {
            ImportStatus = result.Message;
            PushGrowl(result.Message);
            return;
        }

        ImportStatus = ImportReportTextFormatter.BuildStatusMessage(report, manualImport: true);
        GlobalStatus = ImportStatus;
        await TaskQueuePage.ReloadTasksAsync(cancellationToken);
        await SettingsPage.InitializeAsync(cancellationToken);
        TaskQueuePage.SetLanguage(SettingsPage.Language);
        SyncConnectionFromProfile();
        RefreshConfigValidationState(_runtime.ConfigurationService.CurrentValidationIssues);
        AppendImportReportToTaskQueue(report, manualImport: true);
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
                            _ => BuildLinkStartStateNotAllowedMessage(CurrentSessionState),
                        };
                        PushGrowl(blockedMessage);
                        if (CurrentSessionState != SessionState.Connected)
                        {
                            NavigateToSettingsSection("Connect");
                        }
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
                        var blockedStopMessage = BuildStopStateNotAllowedMessage(CurrentSessionState);
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

    private static string BuildLinkStartStateNotAllowedMessage(SessionState state)
    {
        var zh = $"会话状态 `{state}` 不允许 Start/LinkStart。请先前往“设置 > 连接设置”完成连接。";
        var en = $"Session state `{state}` does not allow Start/LinkStart. Go to Settings > Connection and connect first.";
        return BuildBilingualMessage(zh, en);
    }

    private static string BuildStopStateNotAllowedMessage(SessionState state)
    {
        var zh = $"会话状态 `{state}` 不允许 Stop。";
        var en = $"Session state `{state}` does not allow Stop.";
        return BuildBilingualMessage(zh, en);
    }

    private static string BuildBilingualMessage(string zh, string en)
    {
        return $"{zh}{Environment.NewLine}{en}";
    }

    private void ApplyDeveloperModeFromConfig()
    {
        var enabled = TryReadGlobalBool(_runtime.ConfigurationService.CurrentConfig.GlobalValues, DeveloperModeConfigKey, false);
        _runtime.LogService.SetVerboseEnabled(enabled);
    }

    private static bool TryReadGlobalBool(IReadOnlyDictionary<string, JsonNode?> globals, string key, bool fallback)
    {
        if (!globals.TryGetValue(key, out var node) || node is null)
        {
            return fallback;
        }

        if (node is JsonValue value)
        {
            if (value.TryGetValue<bool>(out var boolValue))
            {
                return boolValue;
            }

            if (value.TryGetValue<string>(out var textValue))
            {
                if (bool.TryParse(textValue, out var parsed))
                {
                    return parsed;
                }

                if (int.TryParse(textValue, NumberStyles.Integer, CultureInfo.InvariantCulture, out var intValue))
                {
                    return intValue != 0;
                }
            }

            if (value.TryGetValue<int>(out var intBool))
            {
                return intBool != 0;
            }
        }

        var raw = node.ToString();
        if (bool.TryParse(raw, out var fallbackParsed))
        {
            return fallbackParsed;
        }

        return fallback;
    }

    private void NavigateToSettingsSection(string sectionKey)
    {
        Dispatcher.UIThread.Post(() =>
        {
            var settingsTabIndex = -1;
            for (var i = 0; i < RootTabs.Count; i++)
            {
                if (string.Equals(RootTabs[i], "Settings", StringComparison.OrdinalIgnoreCase))
                {
                    settingsTabIndex = i;
                    break;
                }
            }

            if (settingsTabIndex >= 0)
            {
                SelectedRootTabIndex = settingsTabIndex;
            }

            SettingsPage.SelectSection(sectionKey);
        });
    }

    private void OnSettingsResourceVersionUpdated(object? sender, EventArgs e)
    {
        Dispatcher.UIThread.Post(() => TaskQueuePage.RefreshStagePresentation(forceReloadStageOptions: true));
    }

    private async void OnSettingsConfigurationContextChanged(object? sender, ConfigurationContextChangedEventArgs e)
    {
        try
        {
            await HandleSettingsConfigurationContextChangedAsync(e);
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                "Settings.ConfigurationContextChanged",
                ex,
                UiErrorCode.UiOperationFailed,
                $"配置上下文刷新失败: {ex.Message}");
        }
    }

    private void RefreshRootTextState()
    {
        WindowVersionUpdateInfo = RootTexts["Main.Update.VersionAvailable"];
        WindowResourceUpdateInfo = RootTexts["Main.Update.ResourceAvailable"];
        RefreshWindowTitle();
        OnPropertyChanged(nameof(BlockingConfigIssueSummary));

        var selected = SelectedImportSource;
        ImportSourceOptions.Clear();
        ImportSourceOptions.Add(new ImportSourceOptionItem(ImportSource.Auto, RootTexts["Main.ImportSource.Auto"]));
        ImportSourceOptions.Add(new ImportSourceOptionItem(ImportSource.GuiNewOnly, RootTexts["Main.ImportSource.GuiNewOnly"]));
        ImportSourceOptions.Add(new ImportSourceOptionItem(ImportSource.GuiOnly, RootTexts["Main.ImportSource.GuiOnly"]));
        SelectedImportSource = selected;
        SelectedImportSourceOption = ImportSourceOptions.FirstOrDefault(item => item.Source == SelectedImportSource)
            ?? ImportSourceOptions.FirstOrDefault();
    }

    private void RefreshWindowTitle()
    {
        var updateTags = new List<string>();
        if (HasWindowVersionUpdateInfo)
        {
            updateTags.Add(RootTexts.GetOrDefault("Main.Title.UpdateVersion", "Version Update"));
        }

        if (HasWindowResourceUpdateInfo)
        {
            updateTags.Add(RootTexts.GetOrDefault("Main.Title.UpdateResource", "Resource Update"));
        }

        WindowTitle = updateTags.Count == 0
            ? AppDisplayName
            : $"{AppDisplayName} [{string.Join(" / ", updateTags)}]";
    }

    private async Task HandleSettingsConfigurationContextChangedAsync(ConfigurationContextChangedEventArgs change)
    {
        SyncConnectionFromProfile();
        RefreshConfigValidationState(_runtime.ConfigurationService.CurrentValidationIssues);

        switch (change.Reason)
        {
            case ConfigurationContextChangeReason.ProfileSwitched:
                await TaskQueuePage.ReloadConfigurationContextAsync();
                break;

            case ConfigurationContextChangeReason.LegacyImport:
            case ConfigurationContextChangeReason.UnifiedImport:
                TaskQueuePage.SetLanguage(SettingsPage.Language);
                await ApplyGuiSettingsAsync(SettingsPage.CurrentGuiSnapshot);
                await TaskQueuePage.ReloadConfigurationContextAsync(forceReloadStageOptions: true);
                break;
        }

        if (change.Report is not null)
        {
            AppendImportReportToTaskQueue(change.Report, manualImport: true);
        }
        else if (!string.IsNullOrWhiteSpace(change.Message))
        {
            TaskQueuePage.AppendSystemLog(change.Message);
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
            ConnectionGameProfileSync.ReadFromProfile(profile, _connectionGameSharedState, tolerateMissing: false);
        }
        finally
        {
            _syncingConnectionState = false;
        }
    }

    private void AppendImportReportToTaskQueue(ImportReport? report, bool manualImport)
    {
        if (report is null)
        {
            return;
        }

        foreach (var line in ImportReportTextFormatter.BuildLogLines(report, manualImport))
        {
            TaskQueuePage.AppendSystemLog(line.Message, line.Level);
        }
    }

    private void ApplySessionCallback(CoreCallbackEvent callback)
    {
        AppendRootLogEntry($"[{callback.Timestamp:HH:mm:ss}] CORE {callback.MsgName}({callback.MsgId}) {callback.PayloadJson}");

        if (!string.Equals(callback.MsgName, "ConnectionInfo", StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        if (!TryParseScreencapCost(callback.PayloadJson, out var min, out var avg, out var max))
        {
            return;
        }

        _connectionGameSharedState.UpdateScreencapCost(min, avg, max, callback.Timestamp);
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
            RootTexts.Language = snapshot.Language;
            RefreshRootTextState();
            if (Avalonia.Application.Current is not null)
            {
                Avalonia.Application.Current.RequestedThemeVariant =
                    string.Equals(snapshot.Theme, "Dark", StringComparison.OrdinalIgnoreCase)
                        ? ThemeVariant.Dark
                        : string.Equals(snapshot.Theme, "SyncWithOs", StringComparison.OrdinalIgnoreCase)
                            ? ThemeVariant.Default
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
        RootTexts.Language = next;
        SettingsPage.Language = next;
        SettingsPage.AutostartStatus = PlatformCapabilityTextMap.FormatAutostartStatus(
            next,
            SettingsPage.StartSelf,
            ReportLocalizationFallback);
        TaskQueuePage.SetLanguage(next);
        RefreshRootTextState();
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

    private void AppendRootLogEntry(string message)
    {
        RootLogs.Add(message);
        const int maxCount = 400;
        while (RootLogs.Count > maxCount)
        {
            RootLogs.RemoveAt(0);
        }
    }

    private static bool TryParseScreencapCost(string? payloadJson, out long min, out long avg, out long max)
    {
        min = 0;
        avg = 0;
        max = 0;

        if (string.IsNullOrWhiteSpace(payloadJson))
        {
            return false;
        }

        JsonObject? root;
        try
        {
            root = JsonNode.Parse(payloadJson) as JsonObject;
        }
        catch
        {
            return false;
        }

        if (root is null)
        {
            return false;
        }

        if (root["what"] is not JsonValue whatValue
            || !whatValue.TryGetValue<string>(out var what)
            || !string.Equals(what, "ScreencapCost", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        if (root["details"] is not JsonObject details)
        {
            return false;
        }

        var parsedMin = TryReadInt64(details, "min");
        var parsedAvg = TryReadInt64(details, "avg");
        var parsedMax = TryReadInt64(details, "max");
        if (!parsedMin.HasValue || !parsedAvg.HasValue || !parsedMax.HasValue)
        {
            return false;
        }

        min = parsedMin.Value;
        avg = parsedAvg.Value;
        max = parsedMax.Value;
        return true;
    }

    private static long? TryReadInt64(JsonObject node, string propertyName)
    {
        if (node[propertyName] is not JsonNode valueNode)
        {
            return null;
        }

        if (valueNode is JsonValue value)
        {
            if (value.TryGetValue<long>(out var longValue))
            {
                return longValue;
            }

            if (value.TryGetValue<int>(out var intValue))
            {
                return intValue;
            }

            if (value.TryGetValue<string>(out var textValue)
                && long.TryParse(textValue, NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedText))
            {
                return parsedText;
            }
        }

        return long.TryParse(valueNode.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsed)
            ? parsed
            : null;
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
        await _runtime.DialogFeatureService.ReportErrorAsync(scope, result, cancellationToken);
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
        var failed = UiOperationResult.Fail(
            result.Error?.Code ?? UiErrorCode.UiOperationFailed,
            result.Message,
            result.Error?.Details);
        await RecordFailedResultAsync(scope, failed, cancellationToken);
        await _runtime.DialogFeatureService.ReportErrorAsync(scope, failed, cancellationToken);
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
        var failed = UiOperationResult.Fail(code, ex.Message, ex.ToString());
        await RecordFailedResultAsync(scope, failed, cancellationToken);
        await _runtime.DialogFeatureService.ReportErrorAsync(scope, failed, cancellationToken);
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

public sealed record ImportSourceOptionItem(ImportSource Source, string DisplayName);
