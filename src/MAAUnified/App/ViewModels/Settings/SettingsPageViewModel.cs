using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.Json.Nodes;
using LegacyConfigurationKeys = MAAUnified.Compat.Constants.ConfigurationKeys;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Localization;
using MAAUnified.Compat.Constants;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels.Settings;

public sealed class SettingsPageViewModel : PageViewModelBase
{
    private const string ThemeModeKey = "Theme.Mode";
    private const string DefaultTheme = "Light";
    private const string DefaultLanguage = UiLanguageCatalog.DefaultLanguage;
    private const string DefaultBackgroundStretchMode = "Fill";
    private const string DefaultLogItemDateFormat = "HH:mm:ss";
    private const string DefaultOperNameLanguage = "OperNameLanguageMAA";
    private const string DefaultInverseClearMode = "Clear";
    private const string DeveloperModeConfigKey = "GUI.DeveloperMode";
    private const string ShowGuiHotkeyName = "ShowGui";
    private const string LinkStartHotkeyName = "LinkStart";
    private const string DefaultHotkeyShowGui = "Ctrl+Shift+Alt+M";
    private const string DefaultHotkeyLinkStart = "Ctrl+Shift+Alt+L";
    private const int EmulatorWaitSecondsMin = 0;
    private const int EmulatorWaitSecondsMax = 600;
    private const int DefaultEmulatorWaitSeconds = 60;
    private const int DefaultRemotePollIntervalMs = 1000;
    private const int DefaultTaskTimeoutMinutes = 60;
    private const int DefaultReminderIntervalMinutes = 30;
    private const int BackgroundOpacityMin = 0;
    private const int BackgroundOpacityMax = 100;
    private const int BackgroundBlurMin = 0;
    private const int BackgroundBlurMax = 80;
    private const int AutostartFeedbackDelayMs = 1000;
    private const int TimerSlotCount = 8;
    private const int TimerHourMin = 0;
    private const int TimerHourMax = 23;
    private const int TimerMinuteMin = 0;
    private const int TimerMinuteMax = 59;
    private const int DefaultTimerHour = 7;
    private const int DefaultTimerMinute = 0;
    private const string IssueReportHelpUrl = "https://maa.plus/docs/";
    private const string IssueReportIssueEntryUrl = "https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/new/choose";
    private const string AboutOfficialWebsiteUrl = "https://maa.plus/";
    private const string AboutCommunityUrl = "https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions";
    private const string AboutDownloadUrl = "https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases";
    private const string AchievementGuideUrl = "https://maa.plus/docs/manual/introduction/";
    private const string VersionUpdateChangelogUrl = "https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases";
    private const string VersionUpdateResourceRepositoryUrl = "https://github.com/MaaAssistantArknights/MaaResource";
    private const string VersionUpdateMirrorChyanUrl = "https://mirrorchyan.com/?source=maaunified-settings";
    private static readonly string[] SectionOrder =
    [
        "ConfigurationManager",
        "Timer",
        "Performance",
        "Game",
        "Connect",
        "Start",
        "RemoteControl",
        "GUI",
        "Background",
        "ExternalNotification",
        "HotKey",
        "Achievement",
        "VersionUpdate",
        "IssueReport",
        "About",
    ];
    private static readonly string[] DefaultNotificationProviders =
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
    private static readonly IReadOnlyDictionary<string, string> EmptySettingUpdates =
        new Dictionary<string, string>(StringComparer.Ordinal);
    private static readonly IReadOnlyDictionary<string, IReadOnlyDictionary<string, string>> ProviderConfigKeyMap =
        new Dictionary<string, IReadOnlyDictionary<string, string>>(StringComparer.OrdinalIgnoreCase)
        {
            ["Smtp"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["server"] = ConfigurationKeys.ExternalNotificationSmtpServer,
                ["port"] = ConfigurationKeys.ExternalNotificationSmtpPort,
                ["user"] = ConfigurationKeys.ExternalNotificationSmtpUser,
                ["password"] = ConfigurationKeys.ExternalNotificationSmtpPassword,
                ["useSsl"] = ConfigurationKeys.ExternalNotificationSmtpUseSsl,
                ["requiresAuthentication"] = ConfigurationKeys.ExternalNotificationSmtpRequiresAuthentication,
                ["from"] = ConfigurationKeys.ExternalNotificationSmtpFrom,
                ["to"] = ConfigurationKeys.ExternalNotificationSmtpTo,
            },
            ["ServerChan"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["sendKey"] = ConfigurationKeys.ExternalNotificationServerChanSendKey,
            },
            ["Bark"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["sendKey"] = ConfigurationKeys.ExternalNotificationBarkSendKey,
                ["server"] = ConfigurationKeys.ExternalNotificationBarkServer,
            },
            ["Discord"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["botToken"] = ConfigurationKeys.ExternalNotificationDiscordBotToken,
                ["userId"] = ConfigurationKeys.ExternalNotificationDiscordUserId,
                ["webhookUrl"] = ConfigurationKeys.ExternalNotificationDiscordWebhookUrl,
            },
            ["DingTalk"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["accessToken"] = ConfigurationKeys.ExternalNotificationDingTalkAccessToken,
                ["secret"] = ConfigurationKeys.ExternalNotificationDingTalkSecret,
            },
            ["Telegram"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["botToken"] = ConfigurationKeys.ExternalNotificationTelegramBotToken,
                ["chatId"] = ConfigurationKeys.ExternalNotificationTelegramChatId,
                ["topicId"] = ConfigurationKeys.ExternalNotificationTelegramTopicId,
            },
            ["Qmsg"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["server"] = ConfigurationKeys.ExternalNotificationQmsgServer,
                ["key"] = ConfigurationKeys.ExternalNotificationQmsgKey,
                ["user"] = ConfigurationKeys.ExternalNotificationQmsgUser,
                ["bot"] = ConfigurationKeys.ExternalNotificationQmsgBot,
            },
            ["Gotify"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["server"] = ConfigurationKeys.ExternalNotificationGotifyServer,
                ["token"] = ConfigurationKeys.ExternalNotificationGotifyToken,
            },
            ["CustomWebhook"] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["url"] = ConfigurationKeys.ExternalNotificationCustomWebhookUrl,
                ["body"] = ConfigurationKeys.ExternalNotificationCustomWebhookBody,
            },
        };
    private static readonly JsonSerializerOptions ConfigExportSerializerOptions = new()
    {
        WriteIndented = true,
    };

    private SettingsSectionViewModel? _selectedSection;
    private readonly SemaphoreSlim _guiSaveSemaphore = new(1, 1);
    private readonly SemaphoreSlim _configurationProfileSwitchSemaphore = new(1, 1);
    private readonly Action<LocalizationFallbackInfo>? _localizationFallbackReporter;
    private CancellationTokenSource? _guiAutoSaveCts;
    private CancellationTokenSource? _startPerformanceAutoSaveCts;
    private CancellationTokenSource? _timerAutoSaveCts;
    private CancellationTokenSource? _connectionGameAutoSaveCts;
    private CancellationTokenSource? _remoteControlAutoSaveCts;
    private CancellationTokenSource? _externalNotificationAutoSaveCts;
    private CancellationTokenSource? _versionUpdateAutoSaveCts;
    private CancellationTokenSource? _achievementAutoSaveCts;
    private CancellationTokenSource? _autostartAutoApplyCts;
    private CancellationTokenSource? _autostartFeedbackCts;
    private bool _suppressPageAutoSave;
    private bool _suppressGuiAutoSave;
    private bool _suppressStartPerformanceDirtyTracking;
    private bool _suppressConfigurationProfileSelectionHandling;
    private string _theme = DefaultTheme;
    private string _language = DefaultLanguage;
    private string _logItemDateFormatString = DefaultLogItemDateFormat;
    private string _operNameLanguage = DefaultOperNameLanguage;
    private string _inverseClearMode = DefaultInverseClearMode;
    private bool _useTray = true;
    private bool _minimizeToTray;
    private bool _windowTitleScrollable;
    private bool _developerModeEnabled;
    private bool _startSelf;
    private string _autostartStatus = string.Empty;
    private DateTimeOffset? _lastAutostartToggleAt;
    private string _autostartWarningMessage = string.Empty;
    private string _autostartErrorMessage = string.Empty;
    private string _hotkeyShowGui = DefaultHotkeyShowGui;
    private string _hotkeyLinkStart = DefaultHotkeyLinkStart;
    private string _persistedHotkeyShowGui = DefaultHotkeyShowGui;
    private string _persistedHotkeyLinkStart = DefaultHotkeyLinkStart;
    private string _hotkeyStatusMessage = string.Empty;
    private string _hotkeyWarningMessage = string.Empty;
    private string _hotkeyErrorMessage = string.Empty;
    private string _notificationTitle = "MAA 外部通知测试";
    private string _notificationMessage = "这是 MAA 外部通知测试信息。如果你看到了这段内容，就说明通知发送成功了！";
    private string _issueReportPath = string.Empty;
    private string _issueReportStatusMessage = string.Empty;
    private string _issueReportErrorMessage = string.Empty;
    private string _remoteGetTaskEndpoint = string.Empty;
    private string _remoteReportEndpoint = string.Empty;
    private string _remoteUserIdentity = string.Empty;
    private string _remoteDeviceIdentity = string.Empty;
    private int _remotePollInterval = DefaultRemotePollIntervalMs;
    private string _remoteControlStatusMessage = string.Empty;
    private string _remoteControlWarningMessage = string.Empty;
    private string _remoteControlErrorMessage = string.Empty;
    private string _backgroundImagePath = string.Empty;
    private int _backgroundOpacity = 45;
    private int _backgroundBlur = 12;
    private string _backgroundStretchMode = DefaultBackgroundStretchMode;
    private bool _hasPendingGuiChanges;
    private string _guiValidationMessage = string.Empty;
    private string _guiSectionValidationMessage = string.Empty;
    private string _backgroundValidationMessage = string.Empty;
    private DateTimeOffset? _lastSuccessfulGuiSaveAt;
    private bool _runDirectly;
    private bool _minimizeDirectly;
    private bool _openEmulatorAfterLaunch;
    private string _emulatorPath = string.Empty;
    private string _emulatorAddCommand = string.Empty;
    private int _emulatorWaitSeconds = DefaultEmulatorWaitSeconds;
    private bool _performanceUseGpu;
    private bool _performanceAllowDeprecatedGpu;
    private string _performancePreferredGpuDescription = string.Empty;
    private string _performancePreferredGpuInstancePath = string.Empty;
    private IReadOnlyList<GpuOptionDisplayItem> _availableGpuOptions = [];
    private GpuOptionDisplayItem? _selectedGpuOption;
    private string _gpuSupportMessage = string.Empty;
    private string _gpuWarningMessage = string.Empty;
    private string _gpuCustomDescription = string.Empty;
    private string _gpuCustomInstancePath = string.Empty;
    private bool _isGpuSelectionEnabled;
    private bool _isGpuDeprecatedToggleEnabled;
    private bool _isGpuCustomSelectionFieldsVisible;
    private bool _showGpuRestartRequiredHint;
    private bool _suppressGpuUiRefresh;
    private bool _suppressGpuSelectionChange;
    private bool _deploymentWithPause;
    private string _startsWithScript = string.Empty;
    private string _endsWithScript = string.Empty;
    private bool _copilotWithScript;
    private bool _manualStopWithScript;
    private bool _blockSleep;
    private bool _blockSleepWithScreenOn = true;
    private bool _enablePenguin = true;
    private bool _enableYituliu = true;
    private string _penguinId = string.Empty;
    private int _taskTimeoutMinutes = DefaultTaskTimeoutMinutes;
    private int _reminderIntervalMinutes = DefaultReminderIntervalMinutes;
    private bool _hasPendingStartPerformanceChanges;
    private string _startPerformanceValidationMessage = string.Empty;
    private DateTimeOffset? _lastSuccessfulStartPerformanceSaveAt;
    private bool _forceScheduledStart;
    private bool _showWindowBeforeForceScheduledStart;
    private bool _customTimerConfig;
    private bool _hasPendingTimerChanges;
    private string _timerValidationMessage = string.Empty;
    private DateTimeOffset? _lastSuccessfulTimerSaveAt;
    private bool _suppressTimerDirtyTracking;
    private bool _externalNotificationEnabled;
    private bool _externalNotificationSendWhenComplete = true;
    private bool _externalNotificationSendWhenError = true;
    private bool _externalNotificationSendWhenTimeout = true;
    private bool _externalNotificationEnableDetails;
    private string _externalNotificationStatusMessage = string.Empty;
    private string _externalNotificationWarningMessage = string.Empty;
    private string _externalNotificationErrorMessage = string.Empty;
    private string _selectedNotificationProvider = "Smtp";
    private string _notificationProviderParametersText = string.Empty;
    private string _versionUpdateProxy = string.Empty;
    private string _versionUpdateProxyType = "http";
    private string _versionUpdateVersionType = "Stable";
    private string _versionUpdateResourceSource = "Github";
    private bool _versionUpdateForceGithubSource;
    private string _versionUpdateMirrorChyanCdk = string.Empty;
    private string _versionUpdateMirrorChyanCdkExpired = string.Empty;
    private bool _versionUpdateStartupCheck = true;
    private bool _versionUpdateScheduledCheck;
    private string _versionUpdateResourceApi = string.Empty;
    private bool _versionUpdateAllowNightly;
    private bool _versionUpdateAcknowledgedNightlyWarning;
    private bool _versionUpdateUseAria2;
    private bool _versionUpdateAutoDownload = true;
    private bool _versionUpdateAutoInstall;
    private string _versionUpdateName = string.Empty;
    private string _versionUpdateBody = string.Empty;
    private bool _versionUpdateIsFirstBoot;
    private string _versionUpdatePackage = string.Empty;
    private bool _versionUpdateDoNotShow;
    private string _versionUpdateStatusMessage = string.Empty;
    private string _versionUpdateErrorMessage = string.Empty;
    private IReadOnlyList<DisplayValueOption> _themeOptions = [];
    private IReadOnlyList<DisplayValueOption> _supportedLanguages = [];
    private IReadOnlyList<DisplayValueOption> _backgroundStretchModes = [];
    private IReadOnlyList<DisplayValueOption> _operNameLanguageOptions = [];
    private IReadOnlyList<DisplayValueOption> _inverseClearModeOptions = [];
    private IReadOnlyList<DisplayValueOption> _versionUpdateVersionTypeOptions = [];
    private IReadOnlyList<DisplayValueOption> _versionUpdateProxyTypeOptions = [];
    private IReadOnlyList<DisplayValueOption> _versionUpdateResourceSourceOptions = [];
    private string _updatePanelUiVersion = "unknown";
    private string _updatePanelCoreVersion = "unknown";
    private string _updatePanelBuildTime = "unknown";
    private string _updatePanelResourceVersion = string.Empty;
    private string _updatePanelResourceTime = string.Empty;
    private bool _isVersionUpdateActionRunning;
    private string _configurationManagerSelectedProfile = string.Empty;
    private string _configurationManagerNewProfileName = string.Empty;
    private string _configurationManagerStatusMessage = string.Empty;
    private string _configurationManagerErrorMessage = string.Empty;
    private bool _achievementPopupDisabled;
    private bool _achievementPopupAutoClose;
    private string _achievementStatusMessage = string.Empty;
    private string _achievementErrorMessage = string.Empty;
    private string _achievementPolicySummary = string.Empty;
    private string _aboutVersionInfo = string.Empty;
    private string _aboutStatusMessage = string.Empty;
    private string _aboutErrorMessage = string.Empty;
    private readonly Dictionary<string, string> _notificationProviderParameters =
        new(StringComparer.OrdinalIgnoreCase);
    private readonly Func<string, CancellationToken, Task<UiOperationResult>> _openExternalTargetAsync;
    private readonly IAppDialogService _dialogService;

    public SettingsPageViewModel(
        MAAUnifiedRuntime runtime,
        ConnectionGameSharedStateViewModel connectionGameSharedState,
        Action<LocalizationFallbackInfo>? localizationFallbackReporter = null,
        Func<string, CancellationToken, Task<UiOperationResult>>? openExternalTargetAsync = null,
        IAppDialogService? dialogService = null)
        : base(runtime)
    {
        _localizationFallbackReporter = localizationFallbackReporter;
        _openExternalTargetAsync = openExternalTargetAsync ?? OpenExternalTargetAsync;
        _dialogService = dialogService ?? NoOpAppDialogService.Instance;
        RootTexts = new RootLocalizationTextMap("Root.Localization.Settings");
        RootTexts.FallbackReported += info => _localizationFallbackReporter?.Invoke(info);
        RootTexts.Language = _language;
        ConnectionGameSharedState = connectionGameSharedState;
        ConnectionGameSharedState.SetLanguage(_language);
        (_updatePanelUiVersion, _updatePanelBuildTime) = BuildVersionUpdateUiMetadata();
        RebuildGuiOptionLists();
        RebuildVersionUpdateOptionLists();
        _aboutVersionInfo = BuildAboutVersionInfo();
        UpdateAchievementPolicySummary(AchievementPolicy.Default);
        Sections = new ObservableCollection<SettingsSectionViewModel>();
        CurrentSectionActions = new ObservableCollection<SettingsSectionActionItem>();
        RebuildSections();

        Timers = new ObservableCollection<TimerSlotViewModel>(
            Enumerable.Range(1, TimerSlotCount).Select(i => new TimerSlotViewModel(i)));
        foreach (var slot in Timers)
        {
            slot.PropertyChanged += OnTimerSlotPropertyChanged;
        }

        RefreshGpuUiState();
        SelectedSection = Sections[0];
        PropertyChanged += OnSettingsPropertyChanged;
        ConnectionGameSharedState.PropertyChanged += OnConnectionGameSharedStateChanged;
    }

    public RootLocalizationTextMap RootTexts { get; }

    public event EventHandler<GuiSettingsAppliedEventArgs>? GuiSettingsApplied;
    public event EventHandler? ResourceVersionUpdated;
    public event EventHandler<ConfigurationContextChangedEventArgs>? ConfigurationContextChanged;

    public ObservableCollection<SettingsSectionViewModel> Sections { get; }

    public ObservableCollection<SettingsSectionActionItem> CurrentSectionActions { get; }

    public ObservableCollection<TimerSlotViewModel> Timers { get; }

    public ObservableCollection<string> ConfigurationProfiles { get; } = new();

    public ConnectionGameSharedStateViewModel ConnectionGameSharedState { get; }

    public IReadOnlyList<DisplayValueOption> ThemeOptions
    {
        get => _themeOptions;
        private set => SetProperty(ref _themeOptions, value);
    }

    public IReadOnlyList<DisplayValueOption> SupportedLanguages
    {
        get => _supportedLanguages;
        private set => SetProperty(ref _supportedLanguages, value);
    }

    public IReadOnlyList<DisplayValueOption> BackgroundStretchModes
    {
        get => _backgroundStretchModes;
        private set => SetProperty(ref _backgroundStretchModes, value);
    }

    public IReadOnlyList<DisplayValueOption> OperNameLanguageOptions
    {
        get => _operNameLanguageOptions;
        private set => SetProperty(ref _operNameLanguageOptions, value);
    }

    public IReadOnlyList<DisplayValueOption> InverseClearModeOptions
    {
        get => _inverseClearModeOptions;
        private set => SetProperty(ref _inverseClearModeOptions, value);
    }

    public IReadOnlyList<string> LogItemDateFormatOptions { get; } = SettingsOptionCatalog.GetLogItemDateFormatOptions();

    public DisplayValueOption? SelectedThemeOption
    {
        get => ThemeOptions.FirstOrDefault(
            option => string.Equals(option.Value, Theme, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            Theme = value.Value;
        }
    }

    public DisplayValueOption? SelectedLanguageOption
    {
        get => SupportedLanguages.FirstOrDefault(
            option => string.Equals(option.Value, Language, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            Language = value.Value;
        }
    }

    public DisplayValueOption? SelectedBackgroundStretchModeOption
    {
        get => BackgroundStretchModes.FirstOrDefault(
            option => string.Equals(option.Value, BackgroundStretchMode, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            BackgroundStretchMode = value.Value;
        }
    }

    public DisplayValueOption? SelectedOperNameLanguageOption
    {
        get => OperNameLanguageOptions.FirstOrDefault(
            option => string.Equals(option.Value, OperNameLanguage, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            OperNameLanguage = value.Value;
        }
    }

    public DisplayValueOption? SelectedInverseClearModeOption
    {
        get => InverseClearModeOptions.FirstOrDefault(
            option => string.Equals(option.Value, InverseClearMode, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            InverseClearMode = value.Value;
        }
    }

    public IReadOnlyList<DisplayValueOption> VersionUpdateVersionTypeOptions
    {
        get => _versionUpdateVersionTypeOptions;
        private set => SetProperty(ref _versionUpdateVersionTypeOptions, value);
    }

    public IReadOnlyList<DisplayValueOption> VersionUpdateProxyTypeOptions
    {
        get => _versionUpdateProxyTypeOptions;
        private set => SetProperty(ref _versionUpdateProxyTypeOptions, value);
    }

    public IReadOnlyList<DisplayValueOption> VersionUpdateResourceSourceOptions
    {
        get => _versionUpdateResourceSourceOptions;
        private set => SetProperty(ref _versionUpdateResourceSourceOptions, value);
    }

    public DisplayValueOption? SelectedVersionUpdateVersionTypeOption
    {
        get => VersionUpdateVersionTypeOptions.FirstOrDefault(
            option => string.Equals(option.Value, VersionUpdateVersionType, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            VersionUpdateVersionType = value.Value;
        }
    }

    public DisplayValueOption? SelectedVersionUpdateProxyTypeOption
    {
        get => VersionUpdateProxyTypeOptions.FirstOrDefault(
            option => string.Equals(option.Value, VersionUpdateProxyType, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            VersionUpdateProxyType = value.Value;
        }
    }

    public DisplayValueOption? SelectedVersionUpdateResourceSourceOption
    {
        get => VersionUpdateResourceSourceOptions.FirstOrDefault(
            option => string.Equals(option.Value, VersionUpdateResourceSource, StringComparison.OrdinalIgnoreCase));
        set
        {
            if (value is null)
            {
                return;
            }

            VersionUpdateResourceSource = value.Value;
        }
    }

    public ObservableCollection<string> AvailableNotificationProviders { get; } = new();

    public SettingsSectionViewModel? SelectedSection
    {
        get => _selectedSection;
        set
        {
            if (!SetProperty(ref _selectedSection, value))
            {
                return;
            }

            OnPropertyChanged(nameof(SelectedSectionTitle));
            OnPropertyChanged(nameof(IsConfigurationManagerSelected));
            OnPropertyChanged(nameof(IsTimerSelected));
            OnPropertyChanged(nameof(IsPerformanceSelected));
            OnPropertyChanged(nameof(IsGameSelected));
            OnPropertyChanged(nameof(IsConnectSelected));
            OnPropertyChanged(nameof(IsStartSelected));
            OnPropertyChanged(nameof(IsRemoteControlSelected));
            OnPropertyChanged(nameof(IsGuiSelected));
            OnPropertyChanged(nameof(IsBackgroundSelected));
            OnPropertyChanged(nameof(IsExternalNotificationSelected));
            OnPropertyChanged(nameof(IsHotkeySelected));
            OnPropertyChanged(nameof(IsAchievementSelected));
            OnPropertyChanged(nameof(IsVersionUpdateSelected));
            OnPropertyChanged(nameof(IsIssueReportSelected));
            OnPropertyChanged(nameof(IsAboutSelected));
            RefreshCurrentSectionActions();
        }
    }

    public string SelectedSectionTitle => SelectedSection?.DisplayName ?? string.Empty;

    public bool IsConfigurationManagerSelected => IsSelectedSection("ConfigurationManager");

    public bool IsTimerSelected => IsSelectedSection("Timer");

    public bool IsPerformanceSelected => IsSelectedSection("Performance");

    public bool IsGameSelected => IsSelectedSection("Game");

    public bool IsConnectSelected => IsSelectedSection("Connect");

    public bool IsStartSelected => IsSelectedSection("Start");

    public bool IsRemoteControlSelected => IsSelectedSection("RemoteControl");

    public bool IsGuiSelected => IsSelectedSection("GUI");

    public bool IsBackgroundSelected => IsSelectedSection("Background");

    public bool IsExternalNotificationSelected => IsSelectedSection("ExternalNotification");

    public bool IsHotkeySelected => IsSelectedSection("HotKey");

    public bool IsAchievementSelected => IsSelectedSection("Achievement");

    public bool IsVersionUpdateSelected => IsSelectedSection("VersionUpdate");

    public bool IsIssueReportSelected => IsSelectedSection("IssueReport");

    public bool IsAboutSelected => IsSelectedSection("About");

    public string Theme
    {
        get => _theme;
        set
        {
            var normalized = NormalizeTheme(value);
            if (SetProperty(ref _theme, normalized))
            {
                OnPropertyChanged(nameof(SelectedThemeOption));
                MarkGuiSettingsDirty();
            }
        }
    }

    public string Language
    {
        get => _language;
        set
        {
            var normalized = NormalizeLanguage(value);
            if (SetProperty(ref _language, normalized))
            {
                RootTexts.Language = normalized;
                ConnectionGameSharedState.SetLanguage(normalized);
                RebuildGuiOptionLists();
                RebuildSections(SelectedSection?.Key);
                RebuildVersionUpdateOptionLists();
                RefreshGpuUiState();
                MarkGuiSettingsDirty();
            }
        }
    }

    public string LogItemDateFormatString
    {
        get => _logItemDateFormatString;
        set
        {
            var normalized = NormalizeLogItemDateFormat(value);
            if (SetProperty(ref _logItemDateFormatString, normalized))
            {
                MarkGuiSettingsDirty();
            }
        }
    }

    public string OperNameLanguage
    {
        get => _operNameLanguage;
        set
        {
            var normalized = NormalizeOperNameLanguage(value);
            if (SetProperty(ref _operNameLanguage, normalized))
            {
                OnPropertyChanged(nameof(SelectedOperNameLanguageOption));
                MarkGuiSettingsDirty();
            }
        }
    }

    public string InverseClearMode
    {
        get => _inverseClearMode;
        set
        {
            var normalized = NormalizeInverseClearMode(value);
            if (SetProperty(ref _inverseClearMode, normalized))
            {
                OnPropertyChanged(nameof(SelectedInverseClearModeOption));
                MarkGuiSettingsDirty();
            }
        }
    }

    public bool UseTray
    {
        get => _useTray;
        set
        {
            if (SetProperty(ref _useTray, value))
            {
                if (!value && _minimizeToTray)
                {
                    _minimizeToTray = false;
                    OnPropertyChanged(nameof(MinimizeToTray));
                }

                OnPropertyChanged(nameof(CanMinimizeToTray));
                MarkGuiSettingsDirty();
            }
        }
    }

    public bool CanMinimizeToTray => UseTray;

    public bool MinimizeToTray
    {
        get => _minimizeToTray;
        set
        {
            var normalized = UseTray && value;
            if (SetProperty(ref _minimizeToTray, normalized))
            {
                MarkGuiSettingsDirty();
            }
        }
    }

    public bool WindowTitleScrollable
    {
        get => _windowTitleScrollable;
        set
        {
            if (SetProperty(ref _windowTitleScrollable, value))
            {
                MarkGuiSettingsDirty();
            }
        }
    }

    public bool DeveloperModeEnabled
    {
        get => _developerModeEnabled;
        set
        {
            if (SetProperty(ref _developerModeEnabled, value))
            {
                Runtime.LogService.SetVerboseEnabled(value);
                MarkGuiSettingsDirty();
            }
        }
    }

    public bool StartSelf
    {
        get => _startSelf;
        set => SetProperty(ref _startSelf, value);
    }

    public string AutostartStatus
    {
        get => _autostartStatus;
        set => SetProperty(ref _autostartStatus, value);
    }

    public string AutostartWarningMessage
    {
        get => _autostartWarningMessage;
        private set
        {
            if (SetProperty(ref _autostartWarningMessage, value))
            {
                OnPropertyChanged(nameof(HasAutostartWarningMessage));
            }
        }
    }

    public bool HasAutostartWarningMessage => !string.IsNullOrWhiteSpace(AutostartWarningMessage);

    public string AutostartErrorMessage
    {
        get => _autostartErrorMessage;
        private set
        {
            if (SetProperty(ref _autostartErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasAutostartErrorMessage));
            }
        }
    }

    public bool HasAutostartErrorMessage => !string.IsNullOrWhiteSpace(AutostartErrorMessage);

    public string HotkeyShowGui
    {
        get => _hotkeyShowGui;
        set => SetProperty(ref _hotkeyShowGui, value);
    }

    public string HotkeyLinkStart
    {
        get => _hotkeyLinkStart;
        set => SetProperty(ref _hotkeyLinkStart, value);
    }

    public string HotkeyStatusMessage
    {
        get => _hotkeyStatusMessage;
        private set => SetProperty(ref _hotkeyStatusMessage, value);
    }

    public string HotkeyWarningMessage
    {
        get => _hotkeyWarningMessage;
        private set
        {
            if (SetProperty(ref _hotkeyWarningMessage, value))
            {
                OnPropertyChanged(nameof(HasHotkeyWarningMessage));
            }
        }
    }

    public string HotkeyErrorMessage
    {
        get => _hotkeyErrorMessage;
        private set
        {
            if (SetProperty(ref _hotkeyErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasHotkeyErrorMessage));
            }
        }
    }

    public bool HasHotkeyWarningMessage => !string.IsNullOrWhiteSpace(HotkeyWarningMessage);

    public bool HasHotkeyErrorMessage => !string.IsNullOrWhiteSpace(HotkeyErrorMessage);

    public string NotificationTitle
    {
        get => _notificationTitle;
        set => SetProperty(ref _notificationTitle, value);
    }

    public string NotificationMessage
    {
        get => _notificationMessage;
        set => SetProperty(ref _notificationMessage, value);
    }

    public string IssueReportPath
    {
        get => _issueReportPath;
        set => SetProperty(ref _issueReportPath, value);
    }

    public string IssueReportStatusMessage
    {
        get => _issueReportStatusMessage;
        private set => SetProperty(ref _issueReportStatusMessage, value);
    }

    public string IssueReportErrorMessage
    {
        get => _issueReportErrorMessage;
        private set
        {
            if (SetProperty(ref _issueReportErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasIssueReportErrorMessage));
            }
        }
    }

    public bool HasIssueReportErrorMessage => !string.IsNullOrWhiteSpace(IssueReportErrorMessage);

    public string RemoteGetTaskEndpoint
    {
        get => _remoteGetTaskEndpoint;
        set => SetProperty(ref _remoteGetTaskEndpoint, value);
    }

    public string RemoteReportEndpoint
    {
        get => _remoteReportEndpoint;
        set => SetProperty(ref _remoteReportEndpoint, value);
    }

    public string RemoteUserIdentity
    {
        get => _remoteUserIdentity;
        set => SetProperty(ref _remoteUserIdentity, value);
    }

    public string RemoteDeviceIdentity
    {
        get => _remoteDeviceIdentity;
        set => SetProperty(ref _remoteDeviceIdentity, value);
    }

    public int RemotePollInterval
    {
        get => _remotePollInterval;
        set => SetProperty(ref _remotePollInterval, Math.Max(500, value));
    }

    public string RemoteControlStatusMessage
    {
        get => _remoteControlStatusMessage;
        private set => SetProperty(ref _remoteControlStatusMessage, value);
    }

    public string RemoteControlWarningMessage
    {
        get => _remoteControlWarningMessage;
        private set
        {
            if (SetProperty(ref _remoteControlWarningMessage, value))
            {
                OnPropertyChanged(nameof(HasRemoteControlWarningMessage));
            }
        }
    }

    public string RemoteControlErrorMessage
    {
        get => _remoteControlErrorMessage;
        private set
        {
            if (SetProperty(ref _remoteControlErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasRemoteControlErrorMessage));
            }
        }
    }

    public bool HasRemoteControlWarningMessage => !string.IsNullOrWhiteSpace(RemoteControlWarningMessage);

    public bool HasRemoteControlErrorMessage => !string.IsNullOrWhiteSpace(RemoteControlErrorMessage);

    public bool ExternalNotificationEnabled
    {
        get => _externalNotificationEnabled;
        set
        {
            if (SetProperty(ref _externalNotificationEnabled, value))
            {
                if (!value)
                {
                    ClearExternalNotificationStatus();
                }

                OnPropertyChanged(nameof(CanEditExternalNotification));
                OnPropertyChanged(nameof(CanEditExternalNotificationDetails));
                OnPropertyChanged(nameof(HasExternalNotificationStatusMessage));
                OnPropertyChanged(nameof(HasExternalNotificationWarningMessage));
                OnPropertyChanged(nameof(HasExternalNotificationErrorMessage));
            }
        }
    }

    public bool ExternalNotificationSendWhenComplete
    {
        get => _externalNotificationSendWhenComplete;
        set
        {
            if (SetProperty(ref _externalNotificationSendWhenComplete, value))
            {
                if (!value && _externalNotificationEnableDetails)
                {
                    _externalNotificationEnableDetails = false;
                    OnPropertyChanged(nameof(ExternalNotificationEnableDetails));
                }

                OnPropertyChanged(nameof(CanEditExternalNotificationDetails));
            }
        }
    }

    public bool ExternalNotificationSendWhenError
    {
        get => _externalNotificationSendWhenError;
        set => SetProperty(ref _externalNotificationSendWhenError, value);
    }

    public bool ExternalNotificationSendWhenTimeout
    {
        get => _externalNotificationSendWhenTimeout;
        set => SetProperty(ref _externalNotificationSendWhenTimeout, value);
    }

    public bool ExternalNotificationEnableDetails
    {
        get => _externalNotificationEnableDetails;
        set => SetProperty(ref _externalNotificationEnableDetails, CanEditExternalNotificationDetails && value);
    }

    public bool CanEditExternalNotification => ExternalNotificationEnabled;

    public bool CanEditExternalNotificationDetails =>
        ExternalNotificationEnabled && ExternalNotificationSendWhenComplete;

    public string SelectedNotificationProvider
    {
        get => _selectedNotificationProvider;
        set
        {
            var normalized = NormalizeNotificationProvider(value);
            if (string.IsNullOrWhiteSpace(normalized))
            {
                return;
            }

            if (string.Equals(_selectedNotificationProvider, normalized, StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            if (!string.IsNullOrWhiteSpace(_selectedNotificationProvider))
            {
                _notificationProviderParameters[_selectedNotificationProvider] = NotificationProviderParametersText;
            }

            if (SetProperty(ref _selectedNotificationProvider, normalized))
            {
                NotificationProviderParametersText = _notificationProviderParameters.TryGetValue(normalized, out var stored)
                    ? stored
                    : string.Empty;
            }
        }
    }

    public string NotificationProviderParametersText
    {
        get => _notificationProviderParametersText;
        set => SetProperty(ref _notificationProviderParametersText, value ?? string.Empty);
    }

    public string ExternalNotificationStatusMessage
    {
        get => _externalNotificationStatusMessage;
        private set
        {
            if (SetProperty(ref _externalNotificationStatusMessage, value))
            {
                OnPropertyChanged(nameof(HasExternalNotificationStatusMessage));
            }
        }
    }

    public string ExternalNotificationWarningMessage
    {
        get => _externalNotificationWarningMessage;
        private set
        {
            if (SetProperty(ref _externalNotificationWarningMessage, value))
            {
                OnPropertyChanged(nameof(HasExternalNotificationWarningMessage));
            }
        }
    }

    public string ExternalNotificationErrorMessage
    {
        get => _externalNotificationErrorMessage;
        private set
        {
            if (SetProperty(ref _externalNotificationErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasExternalNotificationErrorMessage));
            }
        }
    }

    public bool HasExternalNotificationStatusMessage =>
        ExternalNotificationEnabled && !string.IsNullOrWhiteSpace(ExternalNotificationStatusMessage);

    public bool HasExternalNotificationWarningMessage =>
        ExternalNotificationEnabled && !string.IsNullOrWhiteSpace(ExternalNotificationWarningMessage);

    public bool HasExternalNotificationErrorMessage =>
        ExternalNotificationEnabled && !string.IsNullOrWhiteSpace(ExternalNotificationErrorMessage);

    public string ConfigurationManagerSelectedProfile
    {
        get => _configurationManagerSelectedProfile;
        set => SetProperty(ref _configurationManagerSelectedProfile, value?.Trim() ?? string.Empty);
    }

    public string ConfigurationManagerNewProfileName
    {
        get => _configurationManagerNewProfileName;
        set => SetProperty(ref _configurationManagerNewProfileName, value ?? string.Empty);
    }

    public string ConfigurationManagerStatusMessage
    {
        get => _configurationManagerStatusMessage;
        private set => SetProperty(ref _configurationManagerStatusMessage, value);
    }

    public string ConfigurationManagerErrorMessage
    {
        get => _configurationManagerErrorMessage;
        private set
        {
            if (SetProperty(ref _configurationManagerErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasConfigurationManagerErrorMessage));
            }
        }
    }

    public bool HasConfigurationManagerErrorMessage => !string.IsNullOrWhiteSpace(ConfigurationManagerErrorMessage);

    public string VersionUpdateProxy
    {
        get => _versionUpdateProxy;
        set => SetProperty(ref _versionUpdateProxy, value?.Trim() ?? string.Empty);
    }

    public string VersionUpdateProxyType
    {
        get => _versionUpdateProxyType;
        set
        {
            if (SetProperty(ref _versionUpdateProxyType, NormalizeVersionUpdateProxyType(value)))
            {
                OnPropertyChanged(nameof(SelectedVersionUpdateProxyTypeOption));
            }
        }
    }

    public string VersionUpdateVersionType
    {
        get => _versionUpdateVersionType;
        set
        {
            if (SetProperty(ref _versionUpdateVersionType, value?.Trim() ?? "Stable"))
            {
                OnPropertyChanged(nameof(SelectedVersionUpdateVersionTypeOption));
            }
        }
    }

    public string VersionUpdateResourceSource
    {
        get => _versionUpdateResourceSource;
        set
        {
            var normalized = NormalizeVersionUpdateResourceSource(value);
            if (!SetProperty(ref _versionUpdateResourceSource, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(IsVersionUpdateMirrorChyanSource));
            OnPropertyChanged(nameof(IsVersionUpdateGithubSource));
            OnPropertyChanged(nameof(SelectedVersionUpdateResourceSourceOption));
        }
    }

    public bool VersionUpdateForceGithubSource
    {
        get => _versionUpdateForceGithubSource;
        set => SetProperty(ref _versionUpdateForceGithubSource, value);
    }

    public string VersionUpdateMirrorChyanCdk
    {
        get => _versionUpdateMirrorChyanCdk;
        set => SetProperty(ref _versionUpdateMirrorChyanCdk, value?.Trim() ?? string.Empty);
    }

    public string VersionUpdateMirrorChyanCdkExpired
    {
        get => _versionUpdateMirrorChyanCdkExpired;
        set
        {
            if (!SetProperty(ref _versionUpdateMirrorChyanCdkExpired, value?.Trim() ?? string.Empty))
            {
                return;
            }

            OnPropertyChanged(nameof(VersionUpdateMirrorChyanCdkExpiryText));
        }
    }

    public bool VersionUpdateStartupCheck
    {
        get => _versionUpdateStartupCheck;
        set => SetProperty(ref _versionUpdateStartupCheck, value);
    }

    public bool VersionUpdateScheduledCheck
    {
        get => _versionUpdateScheduledCheck;
        set => SetProperty(ref _versionUpdateScheduledCheck, value);
    }

    public string VersionUpdateResourceApi
    {
        get => _versionUpdateResourceApi;
        set => SetProperty(ref _versionUpdateResourceApi, value?.Trim() ?? string.Empty);
    }

    public bool VersionUpdateAllowNightly
    {
        get => _versionUpdateAllowNightly;
        set
        {
            if (!SetProperty(ref _versionUpdateAllowNightly, value))
            {
                return;
            }

            RebuildVersionUpdateOptionLists();
            if (!value && string.Equals(VersionUpdateVersionType, "Nightly", StringComparison.OrdinalIgnoreCase))
            {
                VersionUpdateVersionType = "Beta";
            }
        }
    }

    public bool VersionUpdateAcknowledgedNightlyWarning
    {
        get => _versionUpdateAcknowledgedNightlyWarning;
        set => SetProperty(ref _versionUpdateAcknowledgedNightlyWarning, value);
    }

    public bool VersionUpdateUseAria2
    {
        get => _versionUpdateUseAria2;
        set => SetProperty(ref _versionUpdateUseAria2, value);
    }

    public bool VersionUpdateAutoDownload
    {
        get => _versionUpdateAutoDownload;
        set => SetProperty(ref _versionUpdateAutoDownload, value);
    }

    public bool VersionUpdateAutoInstall
    {
        get => _versionUpdateAutoInstall;
        set => SetProperty(ref _versionUpdateAutoInstall, value);
    }

    public string VersionUpdateName
    {
        get => _versionUpdateName;
        set
        {
            if (!SetProperty(ref _versionUpdateName, value ?? string.Empty))
            {
                return;
            }

            if (!string.IsNullOrWhiteSpace(_versionUpdateName))
            {
                UpdatePanelCoreVersion = _versionUpdateName;
            }
        }
    }

    public string VersionUpdateBody
    {
        get => _versionUpdateBody;
        set => SetProperty(ref _versionUpdateBody, value ?? string.Empty);
    }

    public bool VersionUpdateIsFirstBoot
    {
        get => _versionUpdateIsFirstBoot;
        set => SetProperty(ref _versionUpdateIsFirstBoot, value);
    }

    public string VersionUpdatePackage
    {
        get => _versionUpdatePackage;
        set => SetProperty(ref _versionUpdatePackage, value ?? string.Empty);
    }

    public bool VersionUpdateDoNotShow
    {
        get => _versionUpdateDoNotShow;
        set => SetProperty(ref _versionUpdateDoNotShow, value);
    }

    public string VersionUpdateStatusMessage
    {
        get => _versionUpdateStatusMessage;
        private set => SetProperty(ref _versionUpdateStatusMessage, value);
    }

    public string VersionUpdateErrorMessage
    {
        get => _versionUpdateErrorMessage;
        private set
        {
            if (SetProperty(ref _versionUpdateErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasVersionUpdateErrorMessage));
            }
        }
    }

    public bool HasVersionUpdateErrorMessage => !string.IsNullOrWhiteSpace(VersionUpdateErrorMessage);

    public bool IsVersionUpdateMirrorChyanSource =>
        string.Equals(VersionUpdateResourceSource, "MirrorChyan", StringComparison.OrdinalIgnoreCase);

    public bool IsVersionUpdateGithubSource => !IsVersionUpdateMirrorChyanSource;

    public string VersionUpdateMirrorChyanCdkExpiryText =>
        BuildMirrorChyanExpiryText(VersionUpdateMirrorChyanCdkExpired);

    public string UpdatePanelUiVersion
    {
        get => _updatePanelUiVersion;
        private set => SetProperty(ref _updatePanelUiVersion, value);
    }

    public string UpdatePanelCoreVersion
    {
        get => _updatePanelCoreVersion;
        private set => SetProperty(ref _updatePanelCoreVersion, value);
    }

    public string UpdatePanelBuildTime
    {
        get => _updatePanelBuildTime;
        private set => SetProperty(ref _updatePanelBuildTime, value);
    }

    public string UpdatePanelResourceVersion
    {
        get => _updatePanelResourceVersion;
        private set => SetProperty(ref _updatePanelResourceVersion, value);
    }

    public string UpdatePanelResourceTime
    {
        get => _updatePanelResourceTime;
        private set => SetProperty(ref _updatePanelResourceTime, value);
    }

    public bool IsVersionUpdateActionRunning
    {
        get => _isVersionUpdateActionRunning;
        private set
        {
            if (SetProperty(ref _isVersionUpdateActionRunning, value))
            {
                OnPropertyChanged(nameof(CanRunVersionUpdateActions));
            }
        }
    }

    public bool CanRunVersionUpdateActions => !IsVersionUpdateActionRunning;

    public bool AchievementPopupDisabled
    {
        get => _achievementPopupDisabled;
        set
        {
            if (SetProperty(ref _achievementPopupDisabled, value))
            {
                OnPropertyChanged(nameof(CanEditAchievementPopupAutoClose));
            }
        }
    }

    public bool AchievementPopupAutoClose
    {
        get => _achievementPopupAutoClose;
        set => SetProperty(ref _achievementPopupAutoClose, value);
    }

    public bool CanEditAchievementPopupAutoClose => !AchievementPopupDisabled;

    public string AchievementStatusMessage
    {
        get => _achievementStatusMessage;
        private set => SetProperty(ref _achievementStatusMessage, value);
    }

    public string AchievementErrorMessage
    {
        get => _achievementErrorMessage;
        private set
        {
            if (SetProperty(ref _achievementErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasAchievementErrorMessage));
            }
        }
    }

    public bool HasAchievementErrorMessage => !string.IsNullOrWhiteSpace(AchievementErrorMessage);

    public string AchievementPolicySummary
    {
        get => _achievementPolicySummary;
        private set => SetProperty(ref _achievementPolicySummary, value);
    }

    public string AboutVersionInfo
    {
        get => _aboutVersionInfo;
        private set => SetProperty(ref _aboutVersionInfo, value);
    }

    public string AboutStatusMessage
    {
        get => _aboutStatusMessage;
        private set => SetProperty(ref _aboutStatusMessage, value);
    }

    public string AboutErrorMessage
    {
        get => _aboutErrorMessage;
        private set
        {
            if (SetProperty(ref _aboutErrorMessage, value))
            {
                OnPropertyChanged(nameof(HasAboutErrorMessage));
            }
        }
    }

    public bool HasAboutErrorMessage => !string.IsNullOrWhiteSpace(AboutErrorMessage);

    public async Task ExecuteSectionActionAsync(SettingsSectionActionItem? action, CancellationToken cancellationToken = default)
    {
        if (action is null)
        {
            return;
        }

        await ExecuteSectionActionAsync(action.ActionId, cancellationToken);
    }

    public async Task ExecuteSectionActionAsync(string? actionId, CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(actionId))
        {
            return;
        }

        switch (actionId)
        {
            case "settings.save-gui":
                await SaveGuiSettingsAsync(cancellationToken);
                break;
            case "settings.save-connection-game":
                await SaveConnectionGameSettingsAsync(cancellationToken);
                break;
            case "settings.save-start-performance":
                await SaveStartPerformanceSettingsAsync(cancellationToken);
                break;
            case "settings.save-timer":
                await SaveTimerSettingsAsync(cancellationToken);
                break;
            case "settings.save-remote":
                await SaveRemoteControlAsync(cancellationToken);
                break;
            case "settings.test-remote":
                await TestRemoteControlConnectivityAsync(cancellationToken);
                break;
            case "settings.register-hotkeys":
                await RegisterHotkeysAsync(cancellationToken: cancellationToken);
                break;
            case "settings.validate-notification":
                await ValidateExternalNotificationParametersAsync(cancellationToken);
                break;
            case "settings.test-notification":
                await TestExternalNotificationAsync(cancellationToken);
                break;
            case "settings.save-notification":
                await SaveExternalNotificationAsync(cancellationToken);
                break;
            case "settings.save-version-update":
                await SaveVersionUpdateSettingsAsync(cancellationToken);
                break;
            case "settings.check-version-update":
                await CheckVersionUpdateAsync(cancellationToken);
                break;
            case "settings.save-achievement":
                await SaveAchievementSettingsAsync(cancellationToken);
                break;
            case "settings.refresh-achievement":
                await RefreshAchievementPolicyAsync(cancellationToken);
                break;
            case "settings.show-achievement":
                await ShowAchievementListDialogAsync(cancellationToken);
                break;
            case "settings.open-achievement-guide":
                await OpenAchievementGuideAsync(cancellationToken);
                break;
            case "settings.build-issue-report":
                await BuildIssueReportAsync(cancellationToken);
                break;
            case "settings.open-debug-directory":
                await OpenIssueReportDebugDirectoryAsync(cancellationToken);
                break;
            case "settings.clear-image-cache":
                await ClearIssueReportImageCacheAsync(cancellationToken);
                break;
            case "settings.check-announcement":
                await CheckAboutAnnouncementWithDialogAsync(cancellationToken);
                break;
            case "settings.open-official":
                await OpenAboutOfficialWebsiteAsync(cancellationToken);
                break;
            case "settings.open-community":
                await OpenAboutCommunityAsync(cancellationToken);
                break;
            case "settings.open-download":
                await OpenAboutDownloadAsync(cancellationToken);
                break;
            case "settings.refresh-profiles":
                await RefreshConfigurationProfilesAsync(cancellationToken);
                break;
            default:
                await RecordEventAsync("Settings.SectionAction.Unknown", actionId, cancellationToken);
                break;
        }
    }

    public string BackgroundImagePath
    {
        get => _backgroundImagePath;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (SetProperty(ref _backgroundImagePath, normalized))
            {
                MarkGuiSettingsDirty(saveImmediately: false);
            }
        }
    }

    public int BackgroundOpacity
    {
        get => _backgroundOpacity;
        set
        {
            var clamped = Math.Clamp(value, BackgroundOpacityMin, BackgroundOpacityMax);
            if (SetProperty(ref _backgroundOpacity, clamped))
            {
                MarkGuiSettingsDirty();
            }
        }
    }

    public int BackgroundBlur
    {
        get => _backgroundBlur;
        set
        {
            var clamped = Math.Clamp(value, BackgroundBlurMin, BackgroundBlurMax);
            if (SetProperty(ref _backgroundBlur, clamped))
            {
                MarkGuiSettingsDirty();
            }
        }
    }

    public string BackgroundStretchMode
    {
        get => _backgroundStretchMode;
        set
        {
            var normalized = NormalizeBackgroundStretchMode(value);
            if (SetProperty(ref _backgroundStretchMode, normalized))
            {
                OnPropertyChanged(nameof(SelectedBackgroundStretchModeOption));
                MarkGuiSettingsDirty();
            }
        }
    }

    public bool HasPendingGuiChanges
    {
        get => _hasPendingGuiChanges;
        private set
        {
            if (SetProperty(ref _hasPendingGuiChanges, value))
            {
                OnPropertyChanged(nameof(IsGuiSaveInProgress));
                OnPropertyChanged(nameof(HasGuiSaveSucceeded));
            }
        }
    }

    public string GuiValidationMessage
    {
        get => _guiValidationMessage;
        private set
        {
            if (SetProperty(ref _guiValidationMessage, value))
            {
                OnPropertyChanged(nameof(HasGuiValidationMessage));
            }
        }
    }

    public bool HasGuiValidationMessage => !string.IsNullOrWhiteSpace(GuiValidationMessage);

    public string GuiSectionValidationMessage
    {
        get => _guiSectionValidationMessage;
        private set
        {
            if (SetProperty(ref _guiSectionValidationMessage, value))
            {
                OnPropertyChanged(nameof(HasGuiSectionValidationMessage));
                UpdateCombinedGuiValidationMessage();
            }
        }
    }

    public bool HasGuiSectionValidationMessage => !string.IsNullOrWhiteSpace(GuiSectionValidationMessage);

    public string BackgroundValidationMessage
    {
        get => _backgroundValidationMessage;
        private set
        {
            if (SetProperty(ref _backgroundValidationMessage, value))
            {
                OnPropertyChanged(nameof(HasBackgroundValidationMessage));
                UpdateCombinedGuiValidationMessage();
            }
        }
    }

    public bool HasBackgroundValidationMessage => !string.IsNullOrWhiteSpace(BackgroundValidationMessage);

    public DateTimeOffset? LastSuccessfulGuiSaveAt
    {
        get => _lastSuccessfulGuiSaveAt;
        private set
        {
            if (SetProperty(ref _lastSuccessfulGuiSaveAt, value))
            {
                OnPropertyChanged(nameof(HasGuiSaveSucceeded));
            }
        }
    }

    public bool IsGuiSaveInProgress => HasPendingGuiChanges;

    public bool HasGuiSaveSucceeded => !HasPendingGuiChanges && LastSuccessfulGuiSaveAt.HasValue;

    public bool RunDirectly
    {
        get => _runDirectly;
        set
        {
            if (SetProperty(ref _runDirectly, value))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool MinimizeDirectly
    {
        get => _minimizeDirectly;
        set
        {
            if (SetProperty(ref _minimizeDirectly, value))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool OpenEmulatorAfterLaunch
    {
        get => _openEmulatorAfterLaunch;
        set
        {
            if (SetProperty(ref _openEmulatorAfterLaunch, value))
            {
                if (!value)
                {
                    StartPerformanceValidationMessage = string.Empty;
                }

                OnPropertyChanged(nameof(CanEditEmulatorLaunchSettings));
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool CanEditEmulatorLaunchSettings => OpenEmulatorAfterLaunch;

    public string EmulatorPath
    {
        get => _emulatorPath;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (SetProperty(ref _emulatorPath, normalized))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public string EmulatorAddCommand
    {
        get => _emulatorAddCommand;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (SetProperty(ref _emulatorAddCommand, normalized))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public int EmulatorWaitSeconds
    {
        get => _emulatorWaitSeconds;
        set
        {
            if (SetProperty(ref _emulatorWaitSeconds, value))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool PerformanceUseGpu
    {
        get => _performanceUseGpu;
        set
        {
            if (SetProperty(ref _performanceUseGpu, value))
            {
                if (!_suppressGpuUiRefresh)
                {
                    RefreshGpuUiState();
                }

                MarkStartPerformanceDirty();
            }
        }
    }

    public bool PerformanceAllowDeprecatedGpu
    {
        get => _performanceAllowDeprecatedGpu;
        set
        {
            if (SetProperty(ref _performanceAllowDeprecatedGpu, value))
            {
                if (!_suppressGpuUiRefresh)
                {
                    RefreshGpuUiState();
                }

                MarkStartPerformanceDirty();
            }
        }
    }

    public string PerformancePreferredGpuDescription
    {
        get => _performancePreferredGpuDescription;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (SetProperty(ref _performancePreferredGpuDescription, normalized))
            {
                if (!_suppressGpuUiRefresh)
                {
                    RefreshGpuUiState();
                }

                MarkStartPerformanceDirty();
            }
        }
    }

    public string PerformancePreferredGpuInstancePath
    {
        get => _performancePreferredGpuInstancePath;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (SetProperty(ref _performancePreferredGpuInstancePath, normalized))
            {
                if (!_suppressGpuUiRefresh)
                {
                    RefreshGpuUiState();
                }

                MarkStartPerformanceDirty();
            }
        }
    }

    public IReadOnlyList<GpuOptionDisplayItem> AvailableGpuOptions
    {
        get => _availableGpuOptions;
        private set => SetProperty(ref _availableGpuOptions, value);
    }

    public GpuOptionDisplayItem? SelectedGpuOption
    {
        get => _selectedGpuOption;
        set
        {
            if (!SetProperty(ref _selectedGpuOption, value) || _suppressGpuSelectionChange || value is null)
            {
                return;
            }

            ApplyGpuSelection(value.Descriptor);
        }
    }

    public string GpuSupportMessage
    {
        get => _gpuSupportMessage;
        private set
        {
            if (SetProperty(ref _gpuSupportMessage, value))
            {
                OnPropertyChanged(nameof(HasGpuSupportMessage));
            }
        }
    }

    public bool HasGpuSupportMessage => !string.IsNullOrWhiteSpace(GpuSupportMessage);

    public string GpuWarningMessage
    {
        get => _gpuWarningMessage;
        private set
        {
            if (SetProperty(ref _gpuWarningMessage, value))
            {
                OnPropertyChanged(nameof(HasGpuWarningMessage));
            }
        }
    }

    public bool HasGpuWarningMessage => !string.IsNullOrWhiteSpace(GpuWarningMessage);

    public string GpuCustomDescription
    {
        get => _gpuCustomDescription;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetProperty(ref _gpuCustomDescription, normalized)
                || _suppressGpuUiRefresh
                || SelectedGpuOption?.Descriptor.IsCustomEntry != true)
            {
                return;
            }

            ApplyCustomGpuFields();
        }
    }

    public string GpuCustomInstancePath
    {
        get => _gpuCustomInstancePath;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetProperty(ref _gpuCustomInstancePath, normalized)
                || _suppressGpuUiRefresh
                || SelectedGpuOption?.Descriptor.IsCustomEntry != true)
            {
                return;
            }

            ApplyCustomGpuFields();
        }
    }

    public bool IsGpuSelectionEnabled
    {
        get => _isGpuSelectionEnabled;
        private set => SetProperty(ref _isGpuSelectionEnabled, value);
    }

    public bool IsGpuDeprecatedToggleEnabled
    {
        get => _isGpuDeprecatedToggleEnabled;
        private set => SetProperty(ref _isGpuDeprecatedToggleEnabled, value);
    }

    public bool IsGpuCustomSelectionFieldsVisible
    {
        get => _isGpuCustomSelectionFieldsVisible;
        private set => SetProperty(ref _isGpuCustomSelectionFieldsVisible, value);
    }

    public bool ShowGpuRestartRequiredHint
    {
        get => _showGpuRestartRequiredHint;
        private set => SetProperty(ref _showGpuRestartRequiredHint, value);
    }

    public bool DeploymentWithPause
    {
        get => _deploymentWithPause;
        set
        {
            if (SetProperty(ref _deploymentWithPause, value))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public string StartsWithScript
    {
        get => _startsWithScript;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (SetProperty(ref _startsWithScript, normalized))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public string EndsWithScript
    {
        get => _endsWithScript;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (SetProperty(ref _endsWithScript, normalized))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool CopilotWithScript
    {
        get => _copilotWithScript;
        set
        {
            if (SetProperty(ref _copilotWithScript, value))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool ManualStopWithScript
    {
        get => _manualStopWithScript;
        set
        {
            if (SetProperty(ref _manualStopWithScript, value))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool BlockSleep
    {
        get => _blockSleep;
        set
        {
            if (SetProperty(ref _blockSleep, value))
            {
                OnPropertyChanged(nameof(ShowBlockSleepWithScreenOnOption));
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool ShowBlockSleepWithScreenOnOption => BlockSleep;

    public bool BlockSleepWithScreenOn
    {
        get => _blockSleepWithScreenOn;
        set
        {
            if (SetProperty(ref _blockSleepWithScreenOn, value))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool EnablePenguin
    {
        get => _enablePenguin;
        set
        {
            if (SetProperty(ref _enablePenguin, value))
            {
                OnPropertyChanged(nameof(ShowPenguinIdField));
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool ShowPenguinIdField => EnablePenguin;

    public bool EnableYituliu
    {
        get => _enableYituliu;
        set
        {
            if (SetProperty(ref _enableYituliu, value))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public string PenguinId
    {
        get => _penguinId;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (SetProperty(ref _penguinId, normalized))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public int TaskTimeoutMinutes
    {
        get => _taskTimeoutMinutes;
        set
        {
            var normalized = Math.Max(0, value);
            if (SetProperty(ref _taskTimeoutMinutes, normalized))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public int ReminderIntervalMinutes
    {
        get => _reminderIntervalMinutes;
        set
        {
            var normalized = Math.Max(1, value);
            if (SetProperty(ref _reminderIntervalMinutes, normalized))
            {
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool HasPendingStartPerformanceChanges
    {
        get => _hasPendingStartPerformanceChanges;
        private set
        {
            if (SetProperty(ref _hasPendingStartPerformanceChanges, value))
            {
                OnPropertyChanged(nameof(IsStartPerformanceSaveInProgress));
                OnPropertyChanged(nameof(HasStartPerformanceSaveSucceeded));
            }
        }
    }

    public string StartPerformanceValidationMessage
    {
        get => _startPerformanceValidationMessage;
        private set
        {
            if (SetProperty(ref _startPerformanceValidationMessage, value))
            {
                OnPropertyChanged(nameof(HasStartPerformanceValidationMessage));
            }
        }
    }

    public bool HasStartPerformanceValidationMessage => !string.IsNullOrWhiteSpace(StartPerformanceValidationMessage);

    public DateTimeOffset? LastSuccessfulStartPerformanceSaveAt
    {
        get => _lastSuccessfulStartPerformanceSaveAt;
        private set
        {
            if (SetProperty(ref _lastSuccessfulStartPerformanceSaveAt, value))
            {
                OnPropertyChanged(nameof(HasStartPerformanceSaveSucceeded));
            }
        }
    }

    public bool IsStartPerformanceSaveInProgress => HasPendingStartPerformanceChanges;

    public bool HasStartPerformanceSaveSucceeded =>
        !HasPendingStartPerformanceChanges && LastSuccessfulStartPerformanceSaveAt.HasValue;

    public bool ForceScheduledStart
    {
        get => _forceScheduledStart;
        set
        {
            if (SetProperty(ref _forceScheduledStart, value))
            {
                MarkTimerDirty();
            }
        }
    }

    public bool ShowWindowBeforeForceScheduledStart
    {
        get => _showWindowBeforeForceScheduledStart;
        set
        {
            if (SetProperty(ref _showWindowBeforeForceScheduledStart, value))
            {
                MarkTimerDirty();
            }
        }
    }

    public bool CustomTimerConfig
    {
        get => _customTimerConfig;
        set
        {
            if (SetProperty(ref _customTimerConfig, value))
            {
                if (!value)
                {
                    TimerValidationMessage = string.Empty;
                }

                MarkTimerDirty();
            }
        }
    }

    public bool HasPendingTimerChanges
    {
        get => _hasPendingTimerChanges;
        private set
        {
            if (SetProperty(ref _hasPendingTimerChanges, value))
            {
                OnPropertyChanged(nameof(IsTimerSaveInProgress));
                OnPropertyChanged(nameof(HasTimerSaveSucceeded));
            }
        }
    }

    public string TimerValidationMessage
    {
        get => _timerValidationMessage;
        private set
        {
            if (SetProperty(ref _timerValidationMessage, value))
            {
                OnPropertyChanged(nameof(HasTimerValidationMessage));
            }
        }
    }

    public bool HasTimerValidationMessage => !string.IsNullOrWhiteSpace(TimerValidationMessage);

    public DateTimeOffset? LastSuccessfulTimerSaveAt
    {
        get => _lastSuccessfulTimerSaveAt;
        private set
        {
            if (SetProperty(ref _lastSuccessfulTimerSaveAt, value))
            {
                OnPropertyChanged(nameof(HasTimerSaveSucceeded));
            }
        }
    }

    public bool IsTimerSaveInProgress => HasPendingTimerChanges;

    public bool HasTimerSaveSucceeded => !HasPendingTimerChanges && LastSuccessfulTimerSaveAt.HasValue;

    public GuiSettingsSnapshot CurrentGuiSnapshot => BuildNormalizedGuiSnapshot();

    private bool IsSelectedSection(string key)
    {
        return string.Equals(SelectedSection?.Key, key, StringComparison.OrdinalIgnoreCase);
    }

    public bool SelectSection(string key)
    {
        if (string.IsNullOrWhiteSpace(key))
        {
            return false;
        }

        var target = Sections.FirstOrDefault(
            section => string.Equals(section.Key, key, StringComparison.OrdinalIgnoreCase));
        if (target is null)
        {
            return false;
        }

        SelectedSection = target;
        return true;
    }

    private void RebuildSections(string? preferredKey = null)
    {
        var target = preferredKey ?? SelectedSection?.Key ?? SectionOrder[0];
        Sections.Clear();
        foreach (var key in SectionOrder)
        {
            Sections.Add(new SettingsSectionViewModel(key, RootTexts[$"Settings.Section.{key}"]));
        }

        var selected = Sections.FirstOrDefault(section => string.Equals(section.Key, target, StringComparison.OrdinalIgnoreCase))
            ?? Sections.First();
        SelectedSection = selected;
    }

    private void RefreshCurrentSectionActions()
    {
        CurrentSectionActions.Clear();
        var key = SelectedSection?.Key;
        if (string.IsNullOrWhiteSpace(key))
        {
            return;
        }

        switch (key)
        {
            case "GUI":
            case "Background":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-gui", RootTexts["Settings.Action.SaveGui"], IsPrimary: true));
                break;
            case "Connect":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-connection-game", RootTexts["Settings.Action.SaveConnectionGame"], IsPrimary: true));
                break;
            case "Game":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-start-performance", RootTexts["Settings.Action.SaveStartPerformance"], IsPrimary: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-connection-game", RootTexts["Settings.Action.SaveConnectionGame"]));
                break;
            case "Start":
            case "Performance":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-start-performance", RootTexts["Settings.Action.SaveStartPerformance"], IsPrimary: true));
                break;
            case "Timer":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-timer", RootTexts["Settings.Action.SaveTimer"], IsPrimary: true));
                break;
            case "RemoteControl":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-remote", RootTexts["Settings.Action.SaveRemote"], IsPrimary: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.test-remote", RootTexts["Settings.Action.TestRemote"]));
                break;
            case "HotKey":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.register-hotkeys", RootTexts["Settings.Action.RegisterHotkeys"], IsPrimary: true));
                break;
            case "ExternalNotification":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-notification", RootTexts["Settings.Action.SaveNotification"], IsPrimary: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.validate-notification", RootTexts["Settings.Action.ValidateNotification"]));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.test-notification", RootTexts["Settings.Action.TestNotification"]));
                break;
            case "VersionUpdate":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-version-update", RootTexts["Settings.Action.SaveVersionUpdate"], IsPrimary: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.check-version-update", RootTexts["Settings.Action.CheckVersionUpdate"]));
                break;
            case "Achievement":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.save-achievement", RootTexts["Settings.Action.SaveAchievement"], IsPrimary: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.refresh-achievement", RootTexts["Settings.Action.RefreshAchievement"]));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.show-achievement", RootTexts["Settings.Action.ShowAchievement"]));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.open-achievement-guide", RootTexts["Settings.Action.OpenAchievementGuide"], IsSubtle: true));
                break;
            case "IssueReport":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.build-issue-report", RootTexts["Settings.Action.BuildIssueReport"], IsPrimary: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.open-debug-directory", RootTexts["Settings.Action.OpenDebugDirectory"]));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.clear-image-cache", RootTexts["Settings.Action.ClearImageCache"]));
                break;
            case "About":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.check-announcement", RootTexts["Settings.Action.CheckAnnouncement"], IsPrimary: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.open-official", RootTexts["Settings.Action.OpenOfficial"], IsSubtle: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.open-community", RootTexts["Settings.Action.OpenCommunity"], IsSubtle: true));
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.open-download", RootTexts["Settings.Action.OpenDownload"], IsSubtle: true));
                break;
            case "ConfigurationManager":
                CurrentSectionActions.Add(new SettingsSectionActionItem("settings.refresh-profiles", RootTexts["Settings.Action.RefreshProfiles"], IsPrimary: true));
                break;
            default:
                break;
        }
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        _suppressPageAutoSave = true;
        try
        {
            await LoadFromConfigAsync(Runtime.ConfigurationService.CurrentConfig, cancellationToken);
            await RefreshConfigurationProfilesAsync(cancellationToken);
            LoadConnectionSharedStateFromConfig();
            await RefreshAutostartStatusAsync(cancellationToken);
        }
        finally
        {
            _suppressPageAutoSave = false;
        }

        await RecordEventAsync("Settings", "Settings page initialized.", cancellationToken);
    }

    public async Task SaveGuiSettingsAsync(CancellationToken cancellationToken = default)
    {
        await SaveGuiSettingsCoreAsync(triggeredByAutoSave: false, cancellationToken);
    }

    public async Task SaveRemoteControlAsync(CancellationToken cancellationToken = default)
    {
        ClearRemoteControlStatus();
        var normalizedUserIdentity = (RemoteUserIdentity ?? string.Empty).Trim();
        var normalizedDeviceIdentity = (RemoteDeviceIdentity ?? string.Empty).Trim();
        if (ContainsInvalidRemoteIdentity(normalizedUserIdentity) || ContainsInvalidRemoteIdentity(normalizedDeviceIdentity))
        {
            var validation = UiOperationResult.Fail(
                UiErrorCode.RemoteControlInvalidParameters,
                "Remote user/device identity cannot contain control characters.");
            RemoteControlErrorMessage = FormatRemoteControlMessage(validation.Error?.Code, validation.Message);
            RemoteControlWarningMessage = string.Empty;
            RemoteControlStatusMessage = "远程控制配置保存失败。";
            LastErrorMessage = RemoteControlErrorMessage;
            StatusMessage = RemoteControlStatusMessage;
            await RecordFailedResultAsync(
                "Settings.RemoteControl.Save.Validation",
                validation,
                cancellationToken);
            return;
        }

        RemoteUserIdentity = normalizedUserIdentity;
        RemoteDeviceIdentity = normalizedDeviceIdentity;
        var updates = new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.RemoteControlGetTaskEndpointUri] = RemoteGetTaskEndpoint,
            [ConfigurationKeys.RemoteControlReportStatusUri] = RemoteReportEndpoint,
            [ConfigurationKeys.RemoteControlUserIdentity] = normalizedUserIdentity,
            [ConfigurationKeys.RemoteControlDeviceIdentity] = normalizedDeviceIdentity,
            [ConfigurationKeys.RemoteControlPollIntervalMs] = RemotePollInterval.ToString(),
        };

        var result = await SaveScopedSettingsAsync(
            profileUpdates: updates,
            successScope: "Settings.RemoteControl.Save",
            cancellationToken: cancellationToken);
        if (await ApplyResultAsync(result, "Settings.RemoteControl.Save", cancellationToken))
        {
            RemoteControlStatusMessage = "远程控制配置保存成功。";
            RemoteControlErrorMessage = string.Empty;
            RemoteControlWarningMessage = string.Empty;
            return;
        }

        RemoteControlErrorMessage = FormatRemoteControlMessage(result.Error?.Code, result.Message);
        RemoteControlWarningMessage = string.Empty;
        RemoteControlStatusMessage = "远程控制配置保存失败。";
    }

    public async Task TestRemoteControlConnectivityAsync(CancellationToken cancellationToken = default)
    {
        ClearRemoteControlStatus();
        var request = new RemoteControlConnectivityRequest(
            RemoteGetTaskEndpoint,
            RemoteReportEndpoint,
            RemotePollInterval);
        var result = await Runtime.RemoteControlFeatureService.TestConnectivityAsync(request, cancellationToken);
        if (result.Success)
        {
            var summary = BuildRemoteConnectivitySummary(result.Value);
            RemoteControlStatusMessage = $"连通测试成功。{summary}";
            StatusMessage = RemoteControlStatusMessage;
            LastErrorMessage = string.Empty;
            await RecordEventAsync(
                "Settings.RemoteControl.Test",
                RemoteControlStatusMessage,
                cancellationToken);
            return;
        }

        var message = FormatRemoteControlMessage(result.Error?.Code, result.Message);
        var detailsSummary = BuildRemoteConnectivitySummary(ParseRemoteConnectivityDetails(result.Error?.Details));
        if (!string.IsNullOrWhiteSpace(detailsSummary))
        {
            message = $"{message} {detailsSummary}";
        }

        if (string.Equals(result.Error?.Code, UiErrorCode.RemoteControlUnsupported, StringComparison.Ordinal))
        {
            RemoteControlWarningMessage = message;
            RemoteControlErrorMessage = string.Empty;
        }
        else
        {
            RemoteControlErrorMessage = message;
            RemoteControlWarningMessage = string.Empty;
        }

        RemoteControlStatusMessage = "连通测试失败。";
        LastErrorMessage = message;
        await RecordFailedResultAsync(
            "Settings.RemoteControl.Test",
            UiOperationResult.Fail(result.Error?.Code ?? UiErrorCode.RemoteControlConnectivityFailed, message, result.Error?.Details),
            cancellationToken);
    }

    public async Task ValidateExternalNotificationParametersAsync(CancellationToken cancellationToken = default)
    {
        ClearExternalNotificationStatus();
        PersistCurrentProviderParameters();
        if (!ExternalNotificationEnabled)
        {
            LastErrorMessage = string.Empty;
            return;
        }

        var provider = SelectedNotificationProvider;
        var result = await Runtime.NotificationProviderFeatureService.ValidateProviderParametersAsync(
            new NotificationProviderRequest(provider, NotificationProviderParametersText),
            cancellationToken);
        if (result.Success)
        {
            ExternalNotificationStatusMessage = $"Provider `{provider}` 参数校验通过。";
            StatusMessage = ExternalNotificationStatusMessage;
            LastErrorMessage = string.Empty;
            await RecordEventAsync(
                "Settings.ExternalNotification.Validate",
                ExternalNotificationStatusMessage,
                cancellationToken);
            return;
        }

        await ApplyExternalNotificationFailure(result, "Settings.ExternalNotification.Validate", cancellationToken);
    }

    public async Task TestExternalNotificationAsync(CancellationToken cancellationToken = default)
    {
        ClearExternalNotificationStatus();
        PersistCurrentProviderParameters();
        if (!ExternalNotificationEnabled)
        {
            LastErrorMessage = string.Empty;
            return;
        }

        var provider = SelectedNotificationProvider;
        var result = await Runtime.NotificationProviderFeatureService.SendTestAsync(
            new NotificationProviderTestRequest(
                provider,
                NotificationProviderParametersText,
                NotificationTitle,
                NotificationMessage),
            cancellationToken);
        if (result.Success)
        {
            ExternalNotificationStatusMessage = $"Provider `{provider}` 测试发送成功。";
            StatusMessage = ExternalNotificationStatusMessage;
            LastErrorMessage = string.Empty;
            await RecordEventAsync(
                "Settings.ExternalNotification.TestSend",
                ExternalNotificationStatusMessage,
                cancellationToken);
            return;
        }

        await ApplyExternalNotificationFailure(result, "Settings.ExternalNotification.TestSend", cancellationToken);
    }

    public async Task SaveExternalNotificationAsync(CancellationToken cancellationToken = default)
    {
        ClearExternalNotificationStatus();
        PersistCurrentProviderParameters();

        var updates = new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.ExternalNotificationEnabled] = ExternalNotificationEnabled.ToString(),
            [ConfigurationKeys.ExternalNotificationSendWhenComplete] = ExternalNotificationSendWhenComplete.ToString(),
            [ConfigurationKeys.ExternalNotificationSendWhenError] = ExternalNotificationSendWhenError.ToString(),
            [ConfigurationKeys.ExternalNotificationSendWhenTimeout] = ExternalNotificationSendWhenTimeout.ToString(),
            [ConfigurationKeys.ExternalNotificationEnableDetails] = ExternalNotificationEnableDetails.ToString(),
        };

        var applyProviderResult = await PopulateExternalNotificationProviderUpdatesAsync(
            updates,
            validateParameters: ExternalNotificationEnabled,
            cancellationToken);
        if (!applyProviderResult.Success)
        {
            await ApplyExternalNotificationFailure(
                applyProviderResult,
                ExternalNotificationEnabled
                    ? "Settings.ExternalNotification.Save.Validate"
                    : "Settings.ExternalNotification.Save.Disabled",
                cancellationToken);
            return;
        }

        var saveResult = await SaveScopedSettingsAsync(
            profileUpdates: updates,
            successScope: "Settings.ExternalNotification.Save",
            cancellationToken: cancellationToken);
        if (!saveResult.Success)
        {
            await ApplyExternalNotificationFailure(saveResult, "Settings.ExternalNotification.Save", cancellationToken);
            return;
        }

        ExternalNotificationStatusMessage = "外部通知配置保存成功。";
        ExternalNotificationErrorMessage = string.Empty;
        ExternalNotificationWarningMessage = string.Empty;
        StatusMessage = ExternalNotificationStatusMessage;
        LastErrorMessage = string.Empty;
        await RecordEventAsync(
            "Settings.ExternalNotification.Save",
            ExternalNotificationStatusMessage,
            cancellationToken);
    }

    public async Task RefreshConfigurationProfilesAsync(CancellationToken cancellationToken = default)
    {
        await LoadConfigurationProfilesAsync("Settings.ConfigurationManager.Load", cancellationToken);
    }

    public async Task AddConfigurationProfileAsync(CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        var profileName = (ConfigurationManagerNewProfileName ?? string.Empty).Trim();
        var copyFrom = string.IsNullOrWhiteSpace(ConfigurationManagerSelectedProfile)
            ? null
            : ConfigurationManagerSelectedProfile;
        var result = await Runtime.ConfigurationProfileFeatureService.AddProfileAsync(
            profileName,
            copyFrom,
            cancellationToken);
        await HandleConfigurationProfileResultAsync(
            result,
            "Settings.ConfigurationManager.Add",
            successMessage: $"配置 `{profileName}` 已新增。",
            failureMessage: "配置新增失败。",
            cancellationToken);
        if (result.Success)
        {
            ConfigurationManagerNewProfileName = string.Empty;
        }
    }

    public async Task DeleteConfigurationProfileAsync(CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        var target = ConfigurationManagerSelectedProfile;
        var previousCurrent = Runtime.ConfigurationService.CurrentConfig.CurrentProfile;
        var result = await Runtime.ConfigurationProfileFeatureService.DeleteProfileAsync(target, cancellationToken);
        var deleted = await HandleConfigurationProfileResultAsync(
            result,
            "Settings.ConfigurationManager.Delete",
            successMessage: $"配置 `{target}` 已删除。",
            failureMessage: "配置删除失败。",
            cancellationToken);
        if (!deleted)
        {
            return;
        }

        var current = Runtime.ConfigurationService.CurrentConfig.CurrentProfile;
        if (string.IsNullOrWhiteSpace(current))
        {
            return;
        }

        var message = string.Equals(previousCurrent, current, StringComparison.OrdinalIgnoreCase)
            ? $"配置 `{target}` 已删除，已重新加载配置 `{current}`。"
            : $"配置 `{target}` 已删除，已切换至配置 `{current}`。";
        await LoadFromConfigAsync(Runtime.ConfigurationService.CurrentConfig, cancellationToken);
        LoadConnectionSharedStateFromConfig();
        ConfigurationManagerStatusMessage = message;
        ConfigurationManagerErrorMessage = string.Empty;
        StatusMessage = message;
        LastErrorMessage = string.Empty;
        RaiseConfigurationContextChanged(
            ConfigurationContextChangeReason.ProfileSwitched,
            message);
    }

    public async Task MoveConfigurationProfileUpAsync(CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        var target = ConfigurationManagerSelectedProfile;
        var result = await Runtime.ConfigurationProfileFeatureService.MoveProfileAsync(target, -1, cancellationToken);
        await HandleConfigurationProfileResultAsync(
            result,
            "Settings.ConfigurationManager.MoveUp",
            successMessage: $"配置 `{target}` 已上移。",
            failureMessage: "配置上移失败。",
            cancellationToken);
    }

    public async Task MoveConfigurationProfileDownAsync(CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        var target = ConfigurationManagerSelectedProfile;
        var result = await Runtime.ConfigurationProfileFeatureService.MoveProfileAsync(target, 1, cancellationToken);
        await HandleConfigurationProfileResultAsync(
            result,
            "Settings.ConfigurationManager.MoveDown",
            successMessage: $"配置 `{target}` 已下移。",
            failureMessage: "配置下移失败。",
            cancellationToken);
    }

    public async Task SwitchConfigurationProfileAsync(CancellationToken cancellationToken = default)
    {
        if (_suppressConfigurationProfileSelectionHandling)
        {
            return;
        }

        await _configurationProfileSwitchSemaphore.WaitAsync(cancellationToken);
        try
        {
            ClearConfigurationManagerStatus();
            var target = ConfigurationManagerSelectedProfile;
            var current = Runtime.ConfigurationService.CurrentConfig.CurrentProfile;
            if (string.IsNullOrWhiteSpace(target)
                || string.Equals(target, current, StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            var result = await Runtime.ConfigurationProfileFeatureService.SwitchProfileAsync(target, cancellationToken);
            var payload = await ApplyResultAsync(result, "Settings.ConfigurationManager.Switch", cancellationToken);
            if (payload is null)
            {
                ConfigurationManagerErrorMessage = result.Message;
                ConfigurationManagerStatusMessage = "配置切换失败。";
                await LoadConfigurationProfilesAsync(
                    "Settings.ConfigurationManager.ReloadAfterFailure",
                    cancellationToken,
                    updateStatus: false);
                return;
            }

            var switchMessage = $"已切换至配置 `{target}`。";
            await LoadFromConfigAsync(Runtime.ConfigurationService.CurrentConfig, cancellationToken);
            ApplyConfigurationProfileState(payload);
            LoadConnectionSharedStateFromConfig();
            ConfigurationManagerStatusMessage = switchMessage;
            ConfigurationManagerErrorMessage = string.Empty;
            StatusMessage = switchMessage;
            LastErrorMessage = string.Empty;
            RaiseConfigurationContextChanged(
                ConfigurationContextChangeReason.ProfileSwitched,
                switchMessage);
        }
        finally
        {
            _configurationProfileSwitchSemaphore.Release();
        }
    }

    public async Task SaveCurrentConfigurationAsync(CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        try
        {
            await Runtime.ConfigurationService.SaveAsync(cancellationToken);
            await LoadConfigurationProfilesAsync(
                "Settings.ConfigurationManager.ReloadAfterSave",
                cancellationToken,
                updateStatus: false);
            ConfigurationManagerStatusMessage = "当前配置已保存。";
            ConfigurationManagerErrorMessage = string.Empty;
            StatusMessage = ConfigurationManagerStatusMessage;
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Settings.ConfigurationManager.SaveCurrent", ConfigurationManagerStatusMessage, cancellationToken);
        }
        catch (Exception ex)
        {
            ConfigurationManagerStatusMessage = "当前配置保存失败。";
            ConfigurationManagerErrorMessage = ex.Message;
            await RecordUnhandledExceptionAsync(
                "Settings.ConfigurationManager.SaveCurrent",
                ex,
                UiErrorCode.ConfigurationProfileSaveFailed,
                "Failed to save current configuration.",
                cancellationToken);
        }
    }

    public async Task ExportAllConfigurationsAsync(string filePath, CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        var normalizedPath = NormalizeConfigPath(filePath);
        if (string.IsNullOrWhiteSpace(normalizedPath))
        {
            ConfigurationManagerStatusMessage = "导出所有配置失败。";
            ConfigurationManagerErrorMessage = "导出路径为空。";
            return;
        }

        try
        {
            var exportConfig = CloneConfig(Runtime.ConfigurationService.CurrentConfig);
            await WriteConfigFileAsync(exportConfig, normalizedPath, cancellationToken);
            ConfigurationManagerStatusMessage = $"全部配置已导出到 `{normalizedPath}`。";
            ConfigurationManagerErrorMessage = string.Empty;
            StatusMessage = ConfigurationManagerStatusMessage;
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Settings.ConfigurationManager.ExportAll", ConfigurationManagerStatusMessage, cancellationToken);
        }
        catch (Exception ex)
        {
            ConfigurationManagerStatusMessage = "导出所有配置失败。";
            ConfigurationManagerErrorMessage = ex.Message;
            await RecordUnhandledExceptionAsync(
                "Settings.ConfigurationManager.ExportAll",
                ex,
                UiErrorCode.ConfigurationProfileSaveFailed,
                "Failed to export all configuration profiles.",
                cancellationToken);
        }
    }

    public async Task ExportCurrentConfigurationAsync(string filePath, CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        var normalizedPath = NormalizeConfigPath(filePath);
        if (string.IsNullOrWhiteSpace(normalizedPath))
        {
            ConfigurationManagerStatusMessage = "导出当前配置失败。";
            ConfigurationManagerErrorMessage = "导出路径为空。";
            return;
        }

        try
        {
            var exportConfig = BuildCurrentProfileOnlyConfig(Runtime.ConfigurationService.CurrentConfig);
            await WriteConfigFileAsync(exportConfig, normalizedPath, cancellationToken);
            ConfigurationManagerStatusMessage = $"当前配置已导出到 `{normalizedPath}`。";
            ConfigurationManagerErrorMessage = string.Empty;
            StatusMessage = ConfigurationManagerStatusMessage;
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Settings.ConfigurationManager.ExportCurrent", ConfigurationManagerStatusMessage, cancellationToken);
        }
        catch (Exception ex)
        {
            ConfigurationManagerStatusMessage = "导出当前配置失败。";
            ConfigurationManagerErrorMessage = ex.Message;
            await RecordUnhandledExceptionAsync(
                "Settings.ConfigurationManager.ExportCurrent",
                ex,
                UiErrorCode.ConfigurationProfileSaveFailed,
                "Failed to export current configuration profile.",
                cancellationToken);
        }
    }

    public async Task ImportConfigurationsAsync(string filePath, CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        var normalizedPath = NormalizeConfigPath(filePath);
        if (string.IsNullOrWhiteSpace(normalizedPath))
        {
            ConfigurationManagerStatusMessage = "导入配置失败。";
            ConfigurationManagerErrorMessage = "导入路径为空。";
            return;
        }

        if (!File.Exists(normalizedPath))
        {
            ConfigurationManagerStatusMessage = "导入配置失败。";
            ConfigurationManagerErrorMessage = "导入文件不存在。";
            return;
        }

        try
        {
            await using var stream = File.OpenRead(normalizedPath);
            var imported = await JsonSerializer.DeserializeAsync<UnifiedConfig>(stream, cancellationToken: cancellationToken);
            if (imported is null || imported.Profiles.Count == 0)
            {
                ConfigurationManagerStatusMessage = "导入配置失败。";
                ConfigurationManagerErrorMessage = "导入文件中未找到有效配置。";
                return;
            }

            var currentConfig = Runtime.ConfigurationService.CurrentConfig;
            var existingNames = new HashSet<string>(currentConfig.Profiles.Keys, StringComparer.OrdinalIgnoreCase);
            var importedCount = 0;
            var renamedCount = 0;

            foreach (var (name, profile) in imported.Profiles)
            {
                if (string.IsNullOrWhiteSpace(name) || profile is null)
                {
                    continue;
                }

                var normalizedName = name.Trim();
                var targetName = AllocateUniqueProfileName(existingNames, normalizedName);
                if (!string.Equals(targetName, normalizedName, StringComparison.OrdinalIgnoreCase))
                {
                    renamedCount++;
                }

                currentConfig.Profiles[targetName] = CloneProfile(profile);
                existingNames.Add(targetName);
                importedCount++;
            }

            if (importedCount == 0)
            {
                ConfigurationManagerStatusMessage = "导入配置失败。";
                ConfigurationManagerErrorMessage = "导入文件中未找到可用配置项。";
                return;
            }

            if (string.IsNullOrWhiteSpace(currentConfig.CurrentProfile)
                || !currentConfig.Profiles.ContainsKey(currentConfig.CurrentProfile))
            {
                currentConfig.CurrentProfile = currentConfig.Profiles.Keys.First();
            }

            await Runtime.ConfigurationService.SaveAsync(cancellationToken);
            ConfigurationManagerStatusMessage = renamedCount > 0
                ? $"已导入 {importedCount} 个配置（{renamedCount} 个重命名以避免冲突）。"
                : $"已导入 {importedCount} 个配置。";
            ConfigurationManagerErrorMessage = string.Empty;
            StatusMessage = ConfigurationManagerStatusMessage;
            LastErrorMessage = string.Empty;
            await RefreshAfterConfigurationImportAsync(
                ConfigurationContextChangeReason.UnifiedImport,
                "Settings.ConfigurationManager.Import.ContextRefresh",
                ConfigurationManagerStatusMessage,
                report: null,
                cancellationToken);
            ConfigurationManagerStatusMessage = renamedCount > 0
                ? $"已导入 {importedCount} 个配置（{renamedCount} 个重命名以避免冲突）。"
                : $"已导入 {importedCount} 个配置。";
            ConfigurationManagerErrorMessage = string.Empty;
            StatusMessage = ConfigurationManagerStatusMessage;
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Settings.ConfigurationManager.Import", ConfigurationManagerStatusMessage, cancellationToken);
        }
        catch (Exception ex)
        {
            ConfigurationManagerStatusMessage = "导入配置失败。";
            ConfigurationManagerErrorMessage = ex.Message;
            await RecordUnhandledExceptionAsync(
                "Settings.ConfigurationManager.Import",
                ex,
                UiErrorCode.ImportFailed,
                "Failed to import configuration profiles.",
                cancellationToken);
        }
    }

    public async Task<ImportReport> ImportLegacyConfigurationsAsync(
        LegacyImportRequest request,
        CancellationToken cancellationToken = default)
    {
        ClearConfigurationManagerStatus();
        var report = await Runtime.ConfigurationService.ImportLegacyAsync(request, cancellationToken);
        var statusMessage = ImportReportTextFormatter.BuildStatusMessage(report, manualImport: request.ManualImport);
        var errorMessage = report.Errors.Count == 0
            ? string.Empty
            : string.Join("; ", report.Errors);

        ConfigurationManagerStatusMessage = statusMessage;
        ConfigurationManagerErrorMessage = errorMessage;
        StatusMessage = statusMessage;
        LastErrorMessage = report.AppliedConfig ? errorMessage : (errorMessage.Length == 0 ? statusMessage : errorMessage);

        if (report.AppliedConfig)
        {
            await RefreshAfterConfigurationImportAsync(
                ConfigurationContextChangeReason.LegacyImport,
                "Settings.ConfigurationManager.ImportLegacy.ContextRefresh",
                statusMessage,
                report,
                cancellationToken);
            ConfigurationManagerStatusMessage = statusMessage;
            ConfigurationManagerErrorMessage = errorMessage;
            StatusMessage = statusMessage;
            LastErrorMessage = errorMessage;
            await RecordEventAsync(
                "Settings.ConfigurationManager.ImportLegacy",
                $"{statusMessage} {report.Summary}",
                cancellationToken);
            return report;
        }

        if (report.DamagedFiles.Count > 0 && report.ImportedFiles.Count > 0 && !request.AllowPartialImport)
        {
            await RecordEventAsync(
                "Settings.ConfigurationManager.ImportLegacy.PendingConfirmation",
                $"{statusMessage} {report.Summary}",
                cancellationToken);
            return report;
        }

        var failureMessage = errorMessage.Length == 0 ? statusMessage : errorMessage;
        await RecordFailedResultAsync(
            "Settings.ConfigurationManager.ImportLegacy",
            UiOperationResult.Fail(UiErrorCode.ImportFailed, failureMessage),
            cancellationToken);
        return report;
    }

    public async Task SaveVersionUpdateSettingsAsync(CancellationToken cancellationToken = default)
    {
        if (IsVersionUpdateActionRunning)
        {
            return;
        }

        IsVersionUpdateActionRunning = true;
        try
        {
            await SaveVersionUpdateChannelAsync(cancellationToken);
            if (HasVersionUpdateErrorMessage)
            {
                return;
            }

            await SaveVersionUpdateProxyAsync(cancellationToken);
        }
        finally
        {
            IsVersionUpdateActionRunning = false;
        }
    }

    public async Task SaveVersionUpdateChannelAsync(CancellationToken cancellationToken = default)
    {
        VersionUpdateStatusMessage = string.Empty;
        VersionUpdateErrorMessage = string.Empty;

        var policy = BuildVersionUpdatePolicy();
        var saveResult = await Runtime.VersionUpdateFeatureService.SaveChannelAsync(policy, cancellationToken);
        if (!await ApplyResultAsync(saveResult, "Settings.VersionUpdate.Channel.Save", cancellationToken))
        {
            VersionUpdateErrorMessage = saveResult.Message;
            VersionUpdateStatusMessage = "版本更新通道配置保存失败。";
            return;
        }

        VersionUpdateStatusMessage = "版本更新通道配置保存成功。";
        VersionUpdateErrorMessage = string.Empty;
    }

    public async Task SaveVersionUpdateProxyAsync(CancellationToken cancellationToken = default)
    {
        VersionUpdateStatusMessage = string.Empty;
        VersionUpdateErrorMessage = string.Empty;

        var policy = BuildVersionUpdatePolicy();
        var saveResult = await Runtime.VersionUpdateFeatureService.SaveProxyAsync(policy, cancellationToken);
        if (!await ApplyResultAsync(saveResult, "Settings.VersionUpdate.Proxy.Save", cancellationToken))
        {
            VersionUpdateErrorMessage = saveResult.Message;
            VersionUpdateStatusMessage = "版本更新代理配置保存失败。";
            return;
        }

        VersionUpdateStatusMessage = "版本更新代理配置保存成功。";
        VersionUpdateErrorMessage = string.Empty;
    }

    public async Task CheckVersionUpdateAsync(CancellationToken cancellationToken = default)
    {
        if (IsVersionUpdateActionRunning)
        {
            return;
        }

        IsVersionUpdateActionRunning = true;
        VersionUpdateStatusMessage = string.Empty;
        VersionUpdateErrorMessage = string.Empty;

        try
        {
            var policy = BuildVersionUpdatePolicy();
            var checkResult = await Runtime.VersionUpdateFeatureService.CheckForUpdatesAsync(policy, cancellationToken);
            var payload = await ApplyResultNoDialogAsync(checkResult, "Settings.VersionUpdate.Check", cancellationToken);
            if (payload is null)
            {
                VersionUpdateErrorMessage = checkResult.Message;
                VersionUpdateStatusMessage = "检查更新失败。";
                return;
            }

            VersionUpdateStatusMessage = payload;
            VersionUpdateErrorMessage = string.Empty;
        }
        finally
        {
            IsVersionUpdateActionRunning = false;
        }
    }

    public async Task CheckVersionUpdateWithDialogAsync(CancellationToken cancellationToken = default)
    {
        if (IsVersionUpdateActionRunning)
        {
            return;
        }

        IsVersionUpdateActionRunning = true;
        VersionUpdateStatusMessage = string.Empty;
        VersionUpdateErrorMessage = string.Empty;

        try
        {
            var policy = BuildVersionUpdatePolicy();
            var checkResult = await Runtime.VersionUpdateFeatureService.CheckForUpdatesAsync(policy, cancellationToken);
            var payload = await ApplyResultNoDialogAsync(checkResult, "Settings.VersionUpdate.Check", cancellationToken);
            if (payload is null)
            {
                VersionUpdateErrorMessage = checkResult.Message;
                VersionUpdateStatusMessage = "检查更新失败。";
                return;
            }

            var request = new VersionUpdateDialogRequest(
                Title: "Version Update",
                CurrentVersion: string.IsNullOrWhiteSpace(VersionUpdateName) ? "unknown" : VersionUpdateName,
                TargetVersion: string.IsNullOrWhiteSpace(VersionUpdatePackage) ? "no-package" : VersionUpdatePackage,
                Summary: payload,
                Body: VersionUpdateBody ?? string.Empty,
                ConfirmText: "Confirm",
                CancelText: "Later");
            var dialogResult = await _dialogService.ShowVersionUpdateAsync(request, "Settings.VersionUpdate.Dialog", cancellationToken);
            VersionUpdateStatusMessage = dialogResult.Return switch
            {
                DialogReturnSemantic.Confirm => "版本更新弹窗确认完成。",
                DialogReturnSemantic.Cancel => "版本更新弹窗已取消。",
                _ => "版本更新弹窗已关闭。",
            };
            VersionUpdateErrorMessage = string.Empty;
        }
        finally
        {
            IsVersionUpdateActionRunning = false;
        }
    }

    public async Task ManualUpdateResourceAsync(CancellationToken cancellationToken = default)
    {
        if (IsVersionUpdateActionRunning)
        {
            return;
        }

        IsVersionUpdateActionRunning = true;
        VersionUpdateStatusMessage = string.Empty;
        VersionUpdateErrorMessage = string.Empty;

        try
        {
            var policy = BuildVersionUpdatePolicy();
            var updateResult = await Runtime.VersionUpdateFeatureService.UpdateResourceAsync(
                policy,
                ConnectionGameSharedState.ClientType,
                cancellationToken);
            var payload = await ApplyResultNoDialogAsync(updateResult, "Settings.VersionUpdate.Resource.Update", cancellationToken);
            if (payload is null)
            {
                VersionUpdateErrorMessage = updateResult.Message;
                VersionUpdateStatusMessage = "更新资源失败。";
                return;
            }

            VersionUpdateStatusMessage = payload;
            VersionUpdateErrorMessage = string.Empty;
            await RefreshVersionUpdateResourceInfoAsync(cancellationToken);
            ResourceVersionUpdated?.Invoke(this, EventArgs.Empty);
        }
        finally
        {
            IsVersionUpdateActionRunning = false;
        }
    }

    public async Task RefreshVersionUpdateResourceInfoAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var result = await Runtime.VersionUpdateFeatureService.LoadResourceVersionInfoAsync(
                ConnectionGameSharedState.ClientType,
                cancellationToken);
            var info = await ApplyResultNoDialogAsync(result, "Settings.VersionUpdate.ResourceInfo.Load", cancellationToken);
            if (info is null)
            {
                UpdatePanelResourceVersion = string.Empty;
                UpdatePanelResourceTime = string.Empty;
                return;
            }

            UpdatePanelResourceVersion = info.VersionName;
            UpdatePanelResourceTime = info.LastUpdatedAt == DateTime.MinValue
                ? string.Empty
                : info.LastUpdatedAt.ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture);
        }
        catch
        {
            UpdatePanelResourceVersion = string.Empty;
            UpdatePanelResourceTime = string.Empty;
        }
    }

    public async Task OpenVersionUpdateChangelogAsync(CancellationToken cancellationToken = default)
    {
        var result = await _openExternalTargetAsync(VersionUpdateChangelogUrl, cancellationToken);
        if (!await ApplyResultAsync(result, "Settings.VersionUpdate.OpenChangelog", cancellationToken))
        {
            VersionUpdateErrorMessage = result.Message;
            VersionUpdateStatusMessage = "打开更新日志失败。";
            return;
        }

        VersionUpdateStatusMessage = "已打开更新日志。";
        VersionUpdateErrorMessage = string.Empty;
    }

    public async Task OpenVersionUpdateResourceRepositoryAsync(CancellationToken cancellationToken = default)
    {
        var result = await _openExternalTargetAsync(VersionUpdateResourceRepositoryUrl, cancellationToken);
        if (!await ApplyResultAsync(result, "Settings.VersionUpdate.OpenResourceRepository", cancellationToken))
        {
            VersionUpdateErrorMessage = result.Message;
            VersionUpdateStatusMessage = "打开资源仓库失败。";
            return;
        }

        VersionUpdateStatusMessage = "已打开资源仓库。";
        VersionUpdateErrorMessage = string.Empty;
    }

    public async Task OpenVersionUpdateMirrorChyanAsync(CancellationToken cancellationToken = default)
    {
        var result = await _openExternalTargetAsync(VersionUpdateMirrorChyanUrl, cancellationToken);
        if (!await ApplyResultAsync(result, "Settings.VersionUpdate.OpenMirrorChyan", cancellationToken))
        {
            VersionUpdateErrorMessage = result.Message;
            VersionUpdateStatusMessage = "打开 MirrorChyan 失败。";
            return;
        }

        VersionUpdateStatusMessage = "已打开 MirrorChyan。";
        VersionUpdateErrorMessage = string.Empty;
    }

    public async Task SaveAchievementSettingsAsync(CancellationToken cancellationToken = default)
    {
        AchievementStatusMessage = string.Empty;
        AchievementErrorMessage = string.Empty;

        var policy = new AchievementPolicy(
            PopupDisabled: AchievementPopupDisabled,
            PopupAutoClose: AchievementPopupAutoClose);
        var saveResult = await Runtime.AchievementFeatureService.SavePolicyAsync(policy, cancellationToken);
        if (!await ApplyResultAsync(saveResult, "Settings.Achievement.Save", cancellationToken))
        {
            AchievementErrorMessage = saveResult.Message;
            AchievementStatusMessage = "成就配置保存失败。";
            return;
        }

        UpdateAchievementPolicySummary(policy);
        AchievementStatusMessage = "成就配置保存成功。";
        AchievementErrorMessage = string.Empty;
    }

    public async Task RefreshAchievementPolicyAsync(CancellationToken cancellationToken = default)
    {
        AchievementStatusMessage = string.Empty;
        AchievementErrorMessage = string.Empty;
        var result = await Runtime.AchievementFeatureService.LoadPolicyAsync(cancellationToken);
        var policy = await ApplyResultAsync(result, "Settings.Achievement.Refresh", cancellationToken);
        if (policy is null)
        {
            AchievementErrorMessage = result.Message;
            AchievementStatusMessage = "成就配置刷新失败。";
            return;
        }

        ApplyAchievementPolicy(policy);
        UpdateAchievementPolicySummary(policy);
        AchievementStatusMessage = "成就配置已刷新。";
        AchievementErrorMessage = string.Empty;
    }

    public async Task OpenAchievementGuideAsync(CancellationToken cancellationToken = default)
    {
        var result = await _openExternalTargetAsync(AchievementGuideUrl, cancellationToken);
        if (!await ApplyResultAsync(result, "Settings.Achievement.OpenGuide", cancellationToken))
        {
            AchievementStatusMessage = "打开成就说明失败。";
            AchievementErrorMessage = result.Message;
            return;
        }

        AchievementStatusMessage = "已打开成就说明。";
        AchievementErrorMessage = string.Empty;
    }

    public async Task ShowAchievementListDialogAsync(CancellationToken cancellationToken = default)
    {
        var items = new[]
        {
            new AchievementListItem(
                Id: "achievement-popup-disabled",
                Title: "Popup Disabled",
                Description: "Whether achievement popup is disabled.",
                Status: AchievementPopupDisabled ? "enabled" : "disabled"),
            new AchievementListItem(
                Id: "achievement-popup-auto-close",
                Title: "Popup Auto Close",
                Description: "Whether achievement popup auto closes.",
                Status: AchievementPopupAutoClose ? "enabled" : "disabled"),
            new AchievementListItem(
                Id: "achievement-policy-summary",
                Title: "Policy Summary",
                Description: AchievementPolicySummary ?? string.Empty,
                Status: "snapshot"),
        };
        var request = new AchievementListDialogRequest(
            Title: "Achievement List",
            Items: items,
            InitialFilter: string.Empty,
            ConfirmText: "Confirm",
            CancelText: "Cancel");
        var dialogResult = await _dialogService.ShowAchievementListAsync(request, "Settings.Achievement.Dialog", cancellationToken);
        AchievementStatusMessage = dialogResult.Return switch
        {
            DialogReturnSemantic.Confirm => "成就列表弹窗确认完成。",
            DialogReturnSemantic.Cancel => "成就列表弹窗已取消。",
            _ => "成就列表弹窗已关闭。",
        };
        AchievementErrorMessage = string.Empty;
    }

    public async Task SaveConnectionGameSettingsAsync(CancellationToken cancellationToken = default)
    {
        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            LastErrorMessage = "Current profile is missing.";
            await RecordFailedResultAsync(
                "Settings.ConnectionGame.Save",
                UiOperationResult.Fail(UiErrorCode.ProfileMissing, LastErrorMessage),
                cancellationToken: cancellationToken);
            return;
        }

        ConnectionGameProfileSync.WriteToProfile(profile, ConnectionGameSharedState);

        await ApplyResultAsync(
            await Runtime.TaskQueueFeatureService.SaveAsync(cancellationToken),
            "Settings.ConnectionGame.Save",
            cancellationToken);
    }

    public async Task SaveTimerSettingsAsync(CancellationToken cancellationToken = default)
    {
        var snapshot = BuildTimerSnapshot();
        var validation = ValidateTimerSnapshot(snapshot);
        if (!validation.Success)
        {
            HasPendingTimerChanges = true;
            TimerValidationMessage = validation.Message;
            LastErrorMessage = validation.Message;
            await RecordFailedResultAsync(
                "Settings.Save.Timer.Validation",
                validation,
                cancellationToken);
            return;
        }

        var saveResult = await Runtime.SettingsFeatureService.SaveGlobalSettingsAsync(
            snapshot.ToGlobalSettingUpdates(),
            cancellationToken);
        if (!await ApplyResultAsync(saveResult, "Settings.Save.Timer", cancellationToken))
        {
            HasPendingTimerChanges = true;
            TimerValidationMessage = saveResult.Message;
            return;
        }

        var readBackWarnings = new List<string>();
        var readBackSnapshot = ReadTimerSnapshot(Runtime.ConfigurationService.CurrentConfig, readBackWarnings);
        ApplyTimerSnapshot(readBackSnapshot);

        HasPendingTimerChanges = false;
        TimerValidationMessage = readBackWarnings.Count > 0
            ? string.Join(" ", readBackWarnings)
            : string.Empty;
        LastSuccessfulTimerSaveAt = DateTimeOffset.Now;

        if (readBackWarnings.Count > 0)
        {
            await RecordEventAsync(
                "Settings.Timer.Normalize",
                string.Join(" | ", readBackWarnings),
                cancellationToken);
        }
    }

    private async Task<UiOperationResult> SaveScopedSettingsAsync(
        IReadOnlyDictionary<string, string>? globalUpdates = null,
        IReadOnlyDictionary<string, string>? profileUpdates = null,
        string successScope = "Settings.SaveScoped",
        CancellationToken cancellationToken = default)
    {
        globalUpdates ??= EmptySettingUpdates;
        profileUpdates ??= EmptySettingUpdates;
        if (globalUpdates.Count == 0 && profileUpdates.Count == 0)
        {
            return UiOperationResult.Fail(UiErrorCode.SettingBatchEmpty, "No settings were provided.");
        }

        UnifiedProfile? profile = null;
        if (profileUpdates.Count > 0 && !Runtime.ConfigurationService.TryGetCurrentProfile(out profile))
        {
            return UiOperationResult.Fail(
                UiErrorCode.ProfileMissing,
                $"Current profile `{Runtime.ConfigurationService.CurrentConfig.CurrentProfile}` not found.");
        }

        var config = Runtime.ConfigurationService.CurrentConfig;
        var globalSnapshot = CloneJsonNodeMap(config.GlobalValues);
        Dictionary<string, JsonNode?>? profileSnapshot = profile is null
            ? null
            : CloneJsonNodeMap(profile.Values);

        try
        {
            foreach (var (key, value) in globalUpdates)
            {
                if (string.IsNullOrWhiteSpace(key))
                {
                    return UiOperationResult.Fail(UiErrorCode.SettingKeyMissing, "Setting key cannot be empty.");
                }

                config.GlobalValues[key] = JsonValue.Create(value);
            }

            if (profile is not null)
            {
                foreach (var (key, value) in profileUpdates)
                {
                    if (string.IsNullOrWhiteSpace(key))
                    {
                        return UiOperationResult.Fail(UiErrorCode.SettingKeyMissing, "Setting key cannot be empty.");
                    }

                    profile.Values[key] = JsonValue.Create(value);
                }
            }

            await Runtime.ConfigurationService.SaveAsync(cancellationToken);
            await RecordEventAsync(
                successScope,
                $"Saved settings batch: global={globalUpdates.Count}, profile={profileUpdates.Count}",
                cancellationToken);
            return UiOperationResult.Ok($"Saved {globalUpdates.Count + profileUpdates.Count} settings.");
        }
        catch (Exception ex)
        {
            config.GlobalValues = globalSnapshot;
            if (profile is not null && profileSnapshot is not null)
            {
                profile.Values = profileSnapshot;
            }

            Runtime.ConfigurationService.RevalidateCurrentConfig(logIssues: false);
            await RecordUnhandledExceptionAsync(
                $"{successScope}.Persist",
                ex,
                UiErrorCode.SettingsSaveFailed,
                $"Failed to save settings: {ex.Message}");
            return UiOperationResult.Fail(UiErrorCode.SettingsSaveFailed, $"Failed to save settings: {ex.Message}");
        }
    }

    public async Task SaveStartPerformanceSettingsAsync(CancellationToken cancellationToken = default)
    {
        var snapshot = BuildNormalizedStartPerformanceSnapshot();
        ApplyStartPerformanceSnapshotWithoutDirtyTracking(snapshot);

        var validation = ValidateStartPerformanceSnapshot(snapshot);
        if (!validation.Success)
        {
            HasPendingStartPerformanceChanges = true;
            StartPerformanceValidationMessage = validation.Message;
            LastErrorMessage = validation.Message;
            await RecordFailedResultAsync(
                "Settings.Save.StartPerformance.Validation",
                validation,
                cancellationToken);
            return;
        }

        var saveResult = await SaveScopedSettingsAsync(
            globalUpdates: snapshot.ToGlobalSettingUpdates(),
            profileUpdates: snapshot.ToProfileSettingUpdates(),
            successScope: "Settings.Save.StartPerformance",
            cancellationToken: cancellationToken);
        if (!await ApplyResultAsync(saveResult, "Settings.Save.StartPerformance", cancellationToken))
        {
            HasPendingStartPerformanceChanges = true;
            StartPerformanceValidationMessage = saveResult.Message;
            return;
        }

        var readBackWarnings = new List<string>();
        var readBackSnapshot = ReadStartPerformanceSnapshot(Runtime.ConfigurationService.CurrentConfig, readBackWarnings);
        ApplyStartPerformanceSnapshotWithoutDirtyTracking(readBackSnapshot);

        HasPendingStartPerformanceChanges = false;
        StartPerformanceValidationMessage = readBackWarnings.Count > 0
            ? string.Join(" ", readBackWarnings)
            : string.Empty;
        LastSuccessfulStartPerformanceSaveAt = DateTimeOffset.Now;
    }

    public async Task SelectEmulatorPathWithDialogAsync(CancellationToken cancellationToken = default)
    {
        var candidates = BuildEmulatorPathDialogCandidates();
        var request = new EmulatorPathDialogRequest(
            Title: "Emulator Path Selection",
            CandidatePaths: candidates,
            SelectedPath: EmulatorPath,
            ConfirmText: "Confirm",
            CancelText: "Cancel");
        var dialogResult = await _dialogService.ShowEmulatorPathAsync(request, "Settings.Start.SelectEmulatorPath.Dialog", cancellationToken);
        if (dialogResult.Return == DialogReturnSemantic.Confirm && dialogResult.Payload is not null)
        {
            EmulatorPath = dialogResult.Payload.SelectedPath;
            StatusMessage = $"模拟器路径已更新：{EmulatorPath}";
            return;
        }

        StatusMessage = dialogResult.Return == DialogReturnSemantic.Cancel
            ? "模拟器路径选择已取消。"
            : "模拟器路径选择弹窗已关闭。";
    }

    public async Task RegisterHotkeysAsync(
        HotkeyRegistrationSource source = HotkeyRegistrationSource.Manual,
        CancellationToken cancellationToken = default)
    {
        ClearHotkeyStatus();

        var showGesture = NormalizeHotkeyGesture(HotkeyShowGui, DefaultHotkeyShowGui);
        var showResult = await Runtime.SettingsFeatureService.RegisterHotkeyAsync(
            ShowGuiHotkeyName,
            showGesture,
            cancellationToken);
        await RecordHotkeyRegistrationResultAsync(
            ShowGuiHotkeyName,
            showGesture,
            showResult,
            cancellationToken);

        if (!showResult.Success)
        {
            HotkeyErrorMessage = BuildHotkeyErrorMessage(ShowGuiHotkeyName, showResult);
            HotkeyStatusMessage = $"{GetHotkeySourceText(source)}: {ShowGuiHotkeyName} 注册失败，未继续注册 {LinkStartHotkeyName}。";
            LastErrorMessage = HotkeyErrorMessage;
            StatusMessage = HotkeyStatusMessage;
            await RecordFailedResultAsync(
                "Settings.Hotkey.Batch",
                UiOperationResult.Fail(showResult.Error?.Code ?? UiErrorCode.HotkeyRegistrationFailed, HotkeyStatusMessage),
                cancellationToken);
            await RefreshHotkeyFallbackWarningAsync(source, cancellationToken);
            return;
        }

        HotkeyShowGui = showGesture;

        var linkGesture = NormalizeHotkeyGesture(HotkeyLinkStart, DefaultHotkeyLinkStart);
        var linkResult = await Runtime.SettingsFeatureService.RegisterHotkeyAsync(
            LinkStartHotkeyName,
            linkGesture,
            cancellationToken);
        await RecordHotkeyRegistrationResultAsync(
            LinkStartHotkeyName,
            linkGesture,
            linkResult,
            cancellationToken);

        HotkeyLinkStart = linkGesture;

        var serializedHotkeys = SerializeHotkeys(
            persistedShowGui: showGesture,
            persistedLinkStart: linkResult.Success ? linkGesture : _persistedHotkeyLinkStart);
        var saveResult = await Runtime.SettingsFeatureService.SaveGlobalSettingsAsync(
            new Dictionary<string, string>(StringComparer.Ordinal)
            {
                [ConfigurationKeys.HotKeys] = serializedHotkeys,
            },
            cancellationToken);

        if (!saveResult.Success)
        {
            HotkeyErrorMessage = $"Hotkeys were registered but configuration persistence failed: {saveResult.Message}";
            HotkeyStatusMessage = $"{GetHotkeySourceText(source)}: 热键注册成功，但持久化失败。";
            LastErrorMessage = HotkeyErrorMessage;
            StatusMessage = HotkeyStatusMessage;
            await RecordErrorAsync(
                "Settings.Hotkey.Save",
                HotkeyErrorMessage,
                cancellationToken: cancellationToken);
            await RecordFailedResultAsync(
                "Settings.Hotkey.Batch",
                UiOperationResult.Fail(saveResult.Error?.Code ?? UiErrorCode.HotkeyPersistenceFailed, HotkeyStatusMessage),
                cancellationToken);
            await RefreshHotkeyFallbackWarningAsync(source, cancellationToken);
            return;
        }

        _persistedHotkeyShowGui = showGesture;
        if (linkResult.Success)
        {
            _persistedHotkeyLinkStart = linkGesture;
        }

        if (linkResult.Success)
        {
            HotkeyStatusMessage = $"{GetHotkeySourceText(source)}: ShowGui / LinkStart 热键注册成功。";
            HotkeyErrorMessage = string.Empty;
            LastErrorMessage = string.Empty;
            await RecordEventAsync("Settings.Hotkey.Batch", HotkeyStatusMessage, cancellationToken);
        }
        else
        {
            HotkeyErrorMessage = BuildHotkeyErrorMessage(LinkStartHotkeyName, linkResult);
            HotkeyStatusMessage = $"{GetHotkeySourceText(source)}: ShowGui 注册成功，LinkStart 注册失败（已保留历史配置值）。";
            LastErrorMessage = HotkeyErrorMessage;
            await RecordFailedResultAsync(
                "Settings.Hotkey.Batch",
                UiOperationResult.Fail(linkResult.Error?.Code ?? UiErrorCode.HotkeyRegistrationFailed, HotkeyStatusMessage),
                cancellationToken);
        }

        StatusMessage = HotkeyStatusMessage;
        await RefreshHotkeyFallbackWarningAsync(source, cancellationToken);
    }

    public async Task TestNotificationAsync(CancellationToken cancellationToken = default)
    {
        await ApplyResultAsync(
            await Runtime.SettingsFeatureService.TestNotificationAsync(NotificationTitle, NotificationMessage, cancellationToken),
            "Settings.Notification.Test",
            cancellationToken);
    }

    public async Task BuildIssueReportAsync(CancellationToken cancellationToken = default)
    {
        ClearIssueReportStatus();
        var outputPath = await ApplyResultAsync(
            await Runtime.SettingsFeatureService.BuildIssueReportAsync(cancellationToken),
            "Settings.IssueReport.Build",
            cancellationToken);

        if (!string.IsNullOrWhiteSpace(outputPath))
        {
            IssueReportPath = outputPath;
            IssueReportStatusMessage = $"支持包已生成：{outputPath}";
            IssueReportErrorMessage = string.Empty;
            return;
        }

        IssueReportStatusMessage = "生成支持包失败。";
        IssueReportErrorMessage = LastErrorMessage;
    }

    public async Task OpenIssueReportHelpAsync(CancellationToken cancellationToken = default)
    {
        ClearIssueReportStatus();
        var result = await _openExternalTargetAsync(IssueReportHelpUrl, cancellationToken);
        if (!await ApplyResultAsync(result, "Settings.IssueReport.OpenHelp", cancellationToken))
        {
            IssueReportStatusMessage = "打开帮助文档失败。";
            IssueReportErrorMessage = result.Message;
            return;
        }

        IssueReportStatusMessage = "已打开帮助文档。";
        IssueReportErrorMessage = string.Empty;
    }

    public async Task OpenIssueReportEntryAsync(CancellationToken cancellationToken = default)
    {
        ClearIssueReportStatus();
        var result = await OpenIssueReportEntryForDialogAsync(cancellationToken);
        if (!await ApplyResultAsync(result, "Settings.IssueReport.OpenEntry", cancellationToken))
        {
            IssueReportStatusMessage = "打开 Issue 入口失败。";
            IssueReportErrorMessage = result.Message;
            return;
        }

        IssueReportStatusMessage = "已打开 Issue 入口。";
        IssueReportErrorMessage = string.Empty;
    }

    public Task<UiOperationResult> OpenIssueReportEntryForDialogAsync(CancellationToken cancellationToken = default)
    {
        return _openExternalTargetAsync(IssueReportIssueEntryUrl, cancellationToken);
    }

    public async Task OpenIssueReportDebugDirectoryAsync(CancellationToken cancellationToken = default)
    {
        ClearIssueReportStatus();
        var debugDirectory = ResolveDebugDirectoryPath();
        Directory.CreateDirectory(debugDirectory);
        var result = await _openExternalTargetAsync(debugDirectory, cancellationToken);
        if (!await ApplyResultAsync(result, "Settings.IssueReport.OpenDebugDirectory", cancellationToken))
        {
            IssueReportStatusMessage = "打开 debug 目录失败。";
            IssueReportErrorMessage = result.Message;
            return;
        }

        IssueReportStatusMessage = $"已打开目录：{debugDirectory}";
        IssueReportErrorMessage = string.Empty;
    }

    public async Task ClearIssueReportImageCacheAsync(CancellationToken cancellationToken = default)
    {
        ClearIssueReportStatus();
        var imageCacheDirectory = ResolveImageCacheDirectoryPath();
        try
        {
            var removedFiles = 0;
            var removedDirectories = 0;
            if (Directory.Exists(imageCacheDirectory))
            {
                var files = Directory.EnumerateFiles(imageCacheDirectory, "*", SearchOption.AllDirectories).ToList();
                foreach (var file in files)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    File.Delete(file);
                    removedFiles++;
                }

                var directories = Directory.EnumerateDirectories(imageCacheDirectory, "*", SearchOption.AllDirectories)
                    .OrderByDescending(static path => path.Length)
                    .ToList();
                foreach (var directory in directories)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    Directory.Delete(directory, recursive: false);
                    removedDirectories++;
                }
            }

            Directory.CreateDirectory(imageCacheDirectory);
            var result = UiOperationResult.Ok(
                removedFiles == 0 && removedDirectories == 0
                    ? $"图像缓存目录为空：{imageCacheDirectory}"
                    : $"图像缓存已清理：文件 {removedFiles} 个，目录 {removedDirectories} 个。");
            if (!await ApplyResultAsync(result, "Settings.IssueReport.ClearImageCache", cancellationToken))
            {
                IssueReportStatusMessage = "清理图像缓存失败。";
                IssueReportErrorMessage = result.Message;
                return;
            }

            IssueReportStatusMessage = result.Message;
            IssueReportErrorMessage = string.Empty;
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception ex)
        {
            var failure = UiOperationResult.Fail(
                UiErrorCode.IssueReportImageCacheClearFailed,
                $"清理图像缓存失败：{ex.Message}",
                ex.Message);
            _ = await ApplyResultAsync(failure, "Settings.IssueReport.ClearImageCache", cancellationToken);
            IssueReportStatusMessage = "清理图像缓存失败。";
            IssueReportErrorMessage = failure.Message;
        }
    }

    public async Task OpenAboutOfficialWebsiteAsync(CancellationToken cancellationToken = default)
    {
        await OpenAboutExternalTargetAsync(AboutOfficialWebsiteUrl, "Settings.About.OpenOfficialWebsite", "已打开官网。", cancellationToken);
    }

    public async Task OpenAboutCommunityAsync(CancellationToken cancellationToken = default)
    {
        await OpenAboutExternalTargetAsync(AboutCommunityUrl, "Settings.About.OpenCommunity", "已打开社区入口。", cancellationToken);
    }

    public async Task OpenAboutDownloadAsync(CancellationToken cancellationToken = default)
    {
        await OpenAboutExternalTargetAsync(AboutDownloadUrl, "Settings.About.OpenDownload", "已打开下载页。", cancellationToken);
    }

    public async Task CheckAboutAnnouncementAsync(CancellationToken cancellationToken = default)
    {
        ClearAboutStatus();
        var result = await Runtime.AnnouncementFeatureService.LoadStateAsync(cancellationToken);
        var state = await ApplyResultAsync(result, "Settings.About.CheckAnnouncement", cancellationToken);
        if (state is null)
        {
            AboutStatusMessage = "公告读取失败。";
            AboutErrorMessage = result.Message;
            return;
        }

        var info = string.IsNullOrWhiteSpace(state.AnnouncementInfo)
            ? "当前没有公告内容。"
            : state.AnnouncementInfo;
        var flag = $"不再提醒={state.DoNotRemindThisAnnouncementAgain}; 不显示={state.DoNotShowAnnouncement}";
        AboutStatusMessage = $"公告状态：{flag}。{info}";
        AboutErrorMessage = string.Empty;
    }

    public async Task CheckAboutAnnouncementWithDialogAsync(CancellationToken cancellationToken = default)
    {
        ClearAboutStatus();
        var result = await Runtime.AnnouncementFeatureService.LoadStateAsync(cancellationToken);
        var state = await ApplyResultAsync(result, "Settings.About.CheckAnnouncement", cancellationToken);
        if (state is null)
        {
            AboutStatusMessage = "公告读取失败。";
            AboutErrorMessage = result.Message;
            return;
        }

        var request = new AnnouncementDialogRequest(
            Title: "Announcement",
            AnnouncementInfo: state.AnnouncementInfo,
            DoNotRemindThisAnnouncementAgain: state.DoNotRemindThisAnnouncementAgain,
            DoNotShowAnnouncement: state.DoNotShowAnnouncement,
            ConfirmText: "Confirm",
            CancelText: "Cancel");
        var dialogResult = await _dialogService.ShowAnnouncementAsync(request, "Settings.About.Announcement.Dialog", cancellationToken);
        if (dialogResult.Return == DialogReturnSemantic.Confirm && dialogResult.Payload is not null)
        {
            var nextState = new AnnouncementState(
                AnnouncementInfo: dialogResult.Payload.AnnouncementInfo,
                DoNotRemindThisAnnouncementAgain: dialogResult.Payload.DoNotRemindThisAnnouncementAgain,
                DoNotShowAnnouncement: dialogResult.Payload.DoNotShowAnnouncement);
            var saveResult = await Runtime.AnnouncementFeatureService.SaveStateAsync(nextState, cancellationToken);
            if (!await ApplyResultAsync(saveResult, "Settings.About.Announcement.Save", cancellationToken))
            {
                AboutStatusMessage = "公告状态保存失败。";
                AboutErrorMessage = saveResult.Message;
                return;
            }

            AboutStatusMessage = "公告状态已保存。";
            AboutErrorMessage = string.Empty;
            return;
        }

        AboutStatusMessage = dialogResult.Return == DialogReturnSemantic.Cancel
            ? "公告弹窗已取消。"
            : "公告弹窗已关闭。";
        AboutErrorMessage = string.Empty;
    }

    public async Task ApplyAutostartAsync(CancellationToken cancellationToken = default)
    {
        var desired = StartSelf;
        var setResult = await Runtime.SettingsFeatureService.SetAutostartAsync(desired, cancellationToken);
        if (!setResult.Success)
        {
            LastErrorMessage = setResult.Message;
            await RecordFailedResultAsync("Settings.Autostart.Set", setResult, cancellationToken);
            await ShowAutostartErrorWithDelayAsync(
                BuildAutostartSetErrorMessage(setResult.Error?.Code, setResult.Message),
                cancellationToken);
            return;
        }

        StatusMessage = setResult.Message;
        LastErrorMessage = string.Empty;
        await RecordEventAsync("Settings.Autostart.Set", setResult.Message, cancellationToken);
        await RefreshAutostartStatusAsync(
            cancellationToken,
            syncDesiredState: false,
            delayMismatchHint: true,
            desiredState: desired);
    }

    public async Task RefreshAutostartStatusAsync(
        CancellationToken cancellationToken = default,
        bool syncDesiredState = true,
        bool delayMismatchHint = false,
        bool? desiredState = null)
    {
        var result = await Runtime.SettingsFeatureService.GetAutostartStatusAsync(cancellationToken);
        if (!result.Success)
        {
            AutostartStatus = result.Message;
            LastErrorMessage = result.Message;
            await RecordFailedResultAsync(
                "Settings.Autostart.Query",
                UiOperationResult.Fail(result.Error?.Code ?? UiErrorCode.AutostartQueryFailed, result.Message, result.Error?.Details),
                cancellationToken);

            if (delayMismatchHint)
            {
                await ShowAutostartErrorWithDelayAsync(
                    BuildAutostartSetErrorMessage(result.Error?.Code, result.Message),
                    cancellationToken);
            }

            return;
        }

        var enabled = result.Value;
        if (syncDesiredState)
        {
            _suppressPageAutoSave = true;
            try
            {
                StartSelf = enabled;
            }
            finally
            {
                _suppressPageAutoSave = false;
            }
        }

        AutostartStatus = PlatformCapabilityTextMap.FormatAutostartStatus(
            Language,
            enabled,
            _localizationFallbackReporter);

        if (syncDesiredState)
        {
            ClearAutostartFeedback();
            return;
        }

        var expected = desiredState ?? StartSelf;
        if (enabled == expected)
        {
            ClearAutostartFeedback();
            LastErrorMessage = string.Empty;
            return;
        }

        var warningMessage = BuildAutostartMismatchMessage(enabled);
        LastErrorMessage = warningMessage;
        await RecordFailedResultAsync(
            "Settings.Autostart.Verify",
            UiOperationResult.Fail(PlatformErrorCodes.AutostartVerificationFailed, warningMessage),
            cancellationToken);

        if (delayMismatchHint)
        {
            await ShowAutostartWarningWithDelayAsync(warningMessage, cancellationToken);
        }
    }

    private async Task SaveGuiSettingsCoreAsync(bool triggeredByAutoSave, CancellationToken cancellationToken = default)
    {
        var lockAcquired = false;
        try
        {
            await _guiSaveSemaphore.WaitAsync(cancellationToken);
            lockAcquired = true;

            var snapshot = BuildNormalizedGuiSnapshot();
            ApplyGuiSnapshotWithoutAutoSave(snapshot);

            var validation = ValidateGuiSnapshot(snapshot);
            if (!validation.Success)
            {
                HasPendingGuiChanges = true;
                SetGuiValidationMessageForResult(validation);
                LastErrorMessage = validation.Message;
                await RecordFailedResultAsync("Settings.Save.GuiBatch.Validation", validation, cancellationToken);
                return;
            }

            var saveResult = await SaveScopedSettingsAsync(
                globalUpdates: snapshot.ToGlobalSettingUpdates(),
                profileUpdates: snapshot.ToProfileSettingUpdates(),
                successScope: "Settings.Save.GuiBatch",
                cancellationToken: cancellationToken);
            if (!await ApplyResultAsync(saveResult, "Settings.Save.GuiBatch", cancellationToken))
            {
                HasPendingGuiChanges = true;
                SetGuiValidationMessageForCurrentSection(saveResult.Message);
                return;
            }

            HasPendingGuiChanges = false;
            ClearGuiValidationMessages();
            LastSuccessfulGuiSaveAt = DateTimeOffset.Now;
            RaiseGuiSettingsApplied(snapshot);

            if (triggeredByAutoSave)
            {
                await RecordEventAsync(
                    "Settings.Save.GuiBatch.Auto",
                    "GUI settings auto-save succeeded.",
                    cancellationToken);
            }
        }
        catch (OperationCanceledException)
        {
            // No-op for canceled save requests.
        }
        catch (Exception ex)
        {
            HasPendingGuiChanges = true;
            SetGuiValidationMessageForCurrentSection($"GUI settings save failed: {ex.Message}");
            await RecordUnhandledExceptionAsync(
                "Settings.Save.GuiBatch",
                ex,
                UiErrorCode.SettingsSaveFailed,
                GuiValidationMessage,
                cancellationToken);
        }
        finally
        {
            if (lockAcquired)
            {
                _guiSaveSemaphore.Release();
            }
        }
    }

    private void MarkGuiSettingsDirty(bool saveImmediately = true)
    {
        HasPendingGuiChanges = true;
        if (_suppressGuiAutoSave || _suppressPageAutoSave || !saveImmediately)
        {
            return;
        }

        ScheduleGuiAutoSave();
    }

    private void ScheduleGuiAutoSave()
    {
        ScheduleAutoSave(
            ref _guiAutoSaveCts,
            "Settings.AutoSave.Gui",
            500,
            ct => SaveGuiSettingsCoreAsync(triggeredByAutoSave: true, cancellationToken: ct));
    }

    private void MarkStartPerformanceDirty()
    {
        if (_suppressStartPerformanceDirtyTracking || _suppressPageAutoSave)
        {
            return;
        }

        HasPendingStartPerformanceChanges = true;
        StartPerformanceValidationMessage = string.Empty;
        ScheduleAutoSave(
            ref _startPerformanceAutoSaveCts,
            "Settings.AutoSave.StartPerformance",
            550,
            SaveStartPerformanceSettingsAsync);
    }

    private void MarkTimerDirty()
    {
        if (_suppressTimerDirtyTracking || _suppressPageAutoSave)
        {
            return;
        }

        HasPendingTimerChanges = true;
        TimerValidationMessage = string.Empty;
        ScheduleAutoSave(
            ref _timerAutoSaveCts,
            "Settings.AutoSave.Timer",
            550,
            SaveTimerSettingsAsync);
    }

    private void MarkConnectionGameDirty()
    {
        if (_suppressPageAutoSave)
        {
            return;
        }

        ScheduleAutoSave(
            ref _connectionGameAutoSaveCts,
            "Settings.AutoSave.ConnectionGame",
            500,
            SaveConnectionGameSettingsAsync);
    }

    private void MarkRemoteControlDirty()
    {
        if (_suppressPageAutoSave)
        {
            return;
        }

        ScheduleAutoSave(
            ref _remoteControlAutoSaveCts,
            "Settings.AutoSave.Remote",
            650,
            SaveRemoteControlAsync);
    }

    private void MarkExternalNotificationDirty()
    {
        if (_suppressPageAutoSave)
        {
            return;
        }

        ScheduleAutoSave(
            ref _externalNotificationAutoSaveCts,
            "Settings.AutoSave.Notification",
            700,
            SaveExternalNotificationAsync);
    }

    private void MarkVersionUpdateDirty()
    {
        if (_suppressPageAutoSave)
        {
            return;
        }

        ScheduleAutoSave(
            ref _versionUpdateAutoSaveCts,
            "Settings.AutoSave.VersionUpdate",
            700,
            SaveVersionUpdateSettingsAsync);
    }

    private void MarkAchievementDirty()
    {
        if (_suppressPageAutoSave)
        {
            return;
        }

        ScheduleAutoSave(
            ref _achievementAutoSaveCts,
            "Settings.AutoSave.Achievement",
            500,
            SaveAchievementSettingsAsync);
    }

    private void MarkAutostartDirty()
    {
        if (_suppressPageAutoSave)
        {
            return;
        }

        BeginAutostartInteraction();
        ScheduleAutoSave(
            ref _autostartAutoApplyCts,
            "Settings.AutoSave.Autostart",
            300,
            ApplyAutostartAsync);
    }

    private void ScheduleAutoSave(
        ref CancellationTokenSource? debounceCts,
        string scope,
        int delayMs,
        Func<CancellationToken, Task> saveAsync)
    {
        debounceCts?.Cancel();
        debounceCts?.Dispose();
        debounceCts = new CancellationTokenSource();
        var token = debounceCts.Token;

        _ = RunDebouncedSaveAsync(scope, delayMs, saveAsync, token);
    }

    private async Task RunDebouncedSaveAsync(
        string scope,
        int delayMs,
        Func<CancellationToken, Task> saveAsync,
        CancellationToken cancellationToken)
    {
        try
        {
            await Task.Delay(delayMs, cancellationToken);
            await saveAsync(cancellationToken);
        }
        catch (OperationCanceledException)
        {
            // Newer input superseded previous autosave request.
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                scope,
                ex,
                UiErrorCode.SettingsSaveFailed,
                $"{scope} failed.");
        }
    }

    private void OnSettingsPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_suppressPageAutoSave || string.IsNullOrWhiteSpace(e.PropertyName))
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(StartSelf):
                MarkAutostartDirty();
                break;
            case nameof(RemoteGetTaskEndpoint):
            case nameof(RemoteReportEndpoint):
            case nameof(RemoteUserIdentity):
            case nameof(RemoteDeviceIdentity):
            case nameof(RemotePollInterval):
                MarkRemoteControlDirty();
                break;
            case nameof(ExternalNotificationEnabled):
            case nameof(ExternalNotificationSendWhenComplete):
            case nameof(ExternalNotificationSendWhenError):
            case nameof(ExternalNotificationSendWhenTimeout):
            case nameof(ExternalNotificationEnableDetails):
            case nameof(SelectedNotificationProvider):
            case nameof(NotificationProviderParametersText):
                MarkExternalNotificationDirty();
                break;
            case nameof(AchievementPopupDisabled):
            case nameof(AchievementPopupAutoClose):
                MarkAchievementDirty();
                break;
            default:
                if (e.PropertyName.StartsWith("VersionUpdate", StringComparison.Ordinal)
                    && e.PropertyName != nameof(VersionUpdateStatusMessage)
                    && e.PropertyName != nameof(VersionUpdateErrorMessage)
                    && e.PropertyName != nameof(HasVersionUpdateErrorMessage)
                    && e.PropertyName != nameof(VersionUpdateMirrorChyanCdkExpiryText))
                {
                    MarkVersionUpdateDirty();
                }

                break;
        }
    }

    private void OnConnectionGameSharedStateChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (string.IsNullOrWhiteSpace(e.PropertyName))
        {
            return;
        }

        if (ConnectionGameProfileSync.ShouldSyncProperty(e.PropertyName))
        {
            MarkConnectionGameDirty();
        }

        if (string.Equals(
                e.PropertyName,
                nameof(ConnectionGameSharedStateViewModel.ClientType),
                StringComparison.Ordinal))
        {
            _ = RefreshVersionUpdateResourceInfoAsync();
        }
    }

    private void OnTimerSlotPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_suppressTimerDirtyTracking || string.IsNullOrEmpty(e.PropertyName))
        {
            return;
        }

        if (e.PropertyName == nameof(TimerSlotViewModel.Enabled)
            || e.PropertyName == nameof(TimerSlotViewModel.Time)
            || e.PropertyName == nameof(TimerSlotViewModel.Hour)
            || e.PropertyName == nameof(TimerSlotViewModel.Minute)
            || e.PropertyName == nameof(TimerSlotViewModel.Profile))
        {
            MarkTimerDirty();
        }
    }

    private void ClearHotkeyStatus()
    {
        HotkeyStatusMessage = string.Empty;
        HotkeyWarningMessage = string.Empty;
        HotkeyErrorMessage = string.Empty;
    }

    private void ClearRemoteControlStatus()
    {
        RemoteControlStatusMessage = string.Empty;
        RemoteControlWarningMessage = string.Empty;
        RemoteControlErrorMessage = string.Empty;
    }

    private void ClearExternalNotificationStatus()
    {
        ExternalNotificationStatusMessage = string.Empty;
        ExternalNotificationWarningMessage = string.Empty;
        ExternalNotificationErrorMessage = string.Empty;
    }

    private void ClearConfigurationManagerStatus()
    {
        ConfigurationManagerStatusMessage = string.Empty;
        ConfigurationManagerErrorMessage = string.Empty;
    }

    private void ClearIssueReportStatus()
    {
        IssueReportStatusMessage = string.Empty;
        IssueReportErrorMessage = string.Empty;
    }

    private void ClearAboutStatus()
    {
        AboutStatusMessage = string.Empty;
        AboutErrorMessage = string.Empty;
    }

    private async Task LoadConfigurationProfilesAsync(string scope, CancellationToken cancellationToken, bool updateStatus = true)
    {
        var stateResult = await Runtime.ConfigurationProfileFeatureService.LoadStateAsync(cancellationToken);
        if (!stateResult.Success || stateResult.Value is null)
        {
            if (updateStatus)
            {
                await ApplyResultAsync(stateResult, scope, cancellationToken);
            }
            else
            {
                    await RecordFailedResultAsync(
                        scope,
                        UiOperationResult.Fail(
                            stateResult.Error?.Code ?? UiErrorCode.ConfigurationProfileLoadFailed,
                            stateResult.Message,
                            stateResult.Error?.Details),
                        cancellationToken);
            }

            if (updateStatus)
            {
                ConfigurationManagerErrorMessage = stateResult.Message;
                ConfigurationManagerStatusMessage = "配置列表加载失败。";
            }

            return;
        }

        if (updateStatus)
        {
            await ApplyResultAsync(stateResult, scope, cancellationToken);
        }
        else
        {
            await RecordEventAsync(scope, stateResult.Message, cancellationToken);
        }

        ApplyConfigurationProfileState(stateResult.Value);
        if (updateStatus)
        {
            ConfigurationManagerErrorMessage = string.Empty;
            ConfigurationManagerStatusMessage = "配置列表已同步。";
        }
    }

    private async Task<bool> HandleConfigurationProfileResultAsync(
        UiOperationResult<ConfigurationProfileState> result,
        string scope,
        string successMessage,
        string failureMessage,
        CancellationToken cancellationToken)
    {
        var payload = await ApplyResultAsync(result, scope, cancellationToken);
        if (payload is null)
        {
            ConfigurationManagerErrorMessage = result.Message;
            ConfigurationManagerStatusMessage = failureMessage;
            await LoadConfigurationProfilesAsync(
                "Settings.ConfigurationManager.ReloadAfterFailure",
                cancellationToken,
                updateStatus: false);
            return false;
        }

        ApplyConfigurationProfileState(payload);
        LoadConnectionSharedStateFromConfig();
        ConfigurationManagerStatusMessage = successMessage;
        ConfigurationManagerErrorMessage = string.Empty;
        return true;
    }

    private void ApplyConfigurationProfileState(ConfigurationProfileState state)
    {
        var normalizedProfiles = state.OrderedProfiles
            .Where(static profile => !string.IsNullOrWhiteSpace(profile))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToArray();

        var previousSuppressSelectionHandling = _suppressConfigurationProfileSelectionHandling;
        _suppressConfigurationProfileSelectionHandling = true;
        try
        {
            SynchronizeConfigurationProfiles(normalizedProfiles);

            if (ConfigurationProfiles.Count == 0)
            {
                ConfigurationManagerSelectedProfile = string.Empty;
                return;
            }

            var selected = state.CurrentProfile;
            if (string.IsNullOrWhiteSpace(selected)
                || !ConfigurationProfiles.Contains(selected, StringComparer.OrdinalIgnoreCase))
            {
                selected = ConfigurationProfiles[0];
            }

            ConfigurationManagerSelectedProfile = selected;
        }
        finally
        {
            _suppressConfigurationProfileSelectionHandling = previousSuppressSelectionHandling;
        }
    }

    private void SynchronizeConfigurationProfiles(IReadOnlyList<string> orderedProfiles)
    {
        for (var index = 0; index < orderedProfiles.Count; index++)
        {
            var desired = orderedProfiles[index];

            if (index < ConfigurationProfiles.Count
                && string.Equals(ConfigurationProfiles[index], desired, StringComparison.OrdinalIgnoreCase))
            {
                if (!string.Equals(ConfigurationProfiles[index], desired, StringComparison.Ordinal))
                {
                    ConfigurationProfiles[index] = desired;
                }

                continue;
            }

            var existingIndex = FindConfigurationProfileIndex(desired, index);
            if (existingIndex >= 0)
            {
                ConfigurationProfiles.Move(existingIndex, index);
                if (!string.Equals(ConfigurationProfiles[index], desired, StringComparison.Ordinal))
                {
                    ConfigurationProfiles[index] = desired;
                }
            }
            else
            {
                ConfigurationProfiles.Insert(index, desired);
            }
        }

        while (ConfigurationProfiles.Count > orderedProfiles.Count)
        {
            ConfigurationProfiles.RemoveAt(ConfigurationProfiles.Count - 1);
        }
    }

    private int FindConfigurationProfileIndex(string profile, int startIndex)
    {
        for (var index = startIndex; index < ConfigurationProfiles.Count; index++)
        {
            if (string.Equals(ConfigurationProfiles[index], profile, StringComparison.OrdinalIgnoreCase))
            {
                return index;
            }
        }

        return -1;
    }

    private static string NormalizeConfigPath(string? filePath)
    {
        if (string.IsNullOrWhiteSpace(filePath))
        {
            return string.Empty;
        }

        return Path.GetFullPath(filePath.Trim());
    }

    private async Task RefreshAfterConfigurationImportAsync(
        ConfigurationContextChangeReason reason,
        string scope,
        string message,
        ImportReport? report,
        CancellationToken cancellationToken)
    {
        await LoadFromConfigAsync(Runtime.ConfigurationService.CurrentConfig, cancellationToken);
        await LoadConfigurationProfilesAsync(scope, cancellationToken, updateStatus: false);
        LoadConnectionSharedStateFromConfig();
        RaiseConfigurationContextChanged(reason, message, report);
    }

    private void RaiseConfigurationContextChanged(
        ConfigurationContextChangeReason reason,
        string message,
        ImportReport? report = null)
    {
        ConfigurationContextChanged?.Invoke(this, new ConfigurationContextChangedEventArgs(reason, message, report));
    }

    private static async Task WriteConfigFileAsync(UnifiedConfig config, string filePath, CancellationToken cancellationToken)
    {
        var directory = Path.GetDirectoryName(filePath);
        if (!string.IsNullOrWhiteSpace(directory))
        {
            Directory.CreateDirectory(directory);
        }

        await using var stream = File.Create(filePath);
        await JsonSerializer.SerializeAsync(stream, config, ConfigExportSerializerOptions, cancellationToken);
    }

    private static UnifiedConfig BuildCurrentProfileOnlyConfig(UnifiedConfig source)
    {
        var normalizedCurrent = source.CurrentProfile;
        if (string.IsNullOrWhiteSpace(normalizedCurrent) || !source.Profiles.ContainsKey(normalizedCurrent))
        {
            normalizedCurrent = source.Profiles.Keys.FirstOrDefault() ?? string.Empty;
        }

        if (string.IsNullOrWhiteSpace(normalizedCurrent) || !source.Profiles.TryGetValue(normalizedCurrent, out var currentProfile))
        {
            throw new InvalidOperationException("No available profile to export.");
        }

        var config = new UnifiedConfig
        {
            SchemaVersion = source.SchemaVersion,
            CurrentProfile = normalizedCurrent,
            Profiles = new Dictionary<string, UnifiedProfile>(StringComparer.OrdinalIgnoreCase)
            {
                [normalizedCurrent] = CloneProfile(currentProfile),
            },
            GlobalValues = CloneJsonNodeMap(source.GlobalValues),
            Migration = CloneMigration(source.Migration),
        };
        return config;
    }

    private static UnifiedConfig CloneConfig(UnifiedConfig source)
    {
        var profiles = new Dictionary<string, UnifiedProfile>(StringComparer.OrdinalIgnoreCase);
        foreach (var (name, profile) in source.Profiles)
        {
            profiles[name] = CloneProfile(profile);
        }

        return new UnifiedConfig
        {
            SchemaVersion = source.SchemaVersion,
            CurrentProfile = source.CurrentProfile,
            Profiles = profiles,
            GlobalValues = CloneJsonNodeMap(source.GlobalValues),
            Migration = CloneMigration(source.Migration),
        };
    }

    private static UnifiedProfile CloneProfile(UnifiedProfile source)
    {
        var values = CloneJsonNodeMap(source.Values);
        var tasks = source.TaskQueue
            .Select(task => new UnifiedTaskItem
            {
                Type = task.Type,
                Name = task.Name,
                IsEnabled = task.IsEnabled,
                Params = task.Params?.DeepClone() as JsonObject ?? [],
                LegacyRawTask = task.LegacyRawTask?.DeepClone() as JsonObject,
            })
            .ToList();
        return new UnifiedProfile
        {
            Values = values,
            TaskQueue = tasks,
        };
    }

    private static Dictionary<string, JsonNode?> CloneJsonNodeMap(Dictionary<string, JsonNode?> source)
    {
        var result = new Dictionary<string, JsonNode?>(StringComparer.OrdinalIgnoreCase);
        foreach (var (key, value) in source)
        {
            result[key] = value?.DeepClone();
        }

        return result;
    }

    private static UnifiedMigrationMetadata CloneMigration(UnifiedMigrationMetadata source)
    {
        return new UnifiedMigrationMetadata
        {
            ImportedAt = source.ImportedAt,
            ImportedBy = source.ImportedBy,
            ImportedFromGui = source.ImportedFromGui,
            ImportedFromGuiNew = source.ImportedFromGuiNew,
            Warnings = [.. source.Warnings],
        };
    }

    private static string AllocateUniqueProfileName(HashSet<string> existingNames, string preferredName)
    {
        var baseName = string.IsNullOrWhiteSpace(preferredName)
            ? "Imported"
            : preferredName.Trim();
        if (!existingNames.Contains(baseName))
        {
            return baseName;
        }

        var suffix = 1;
        string candidate;
        do
        {
            candidate = $"{baseName}_{suffix}";
            suffix++;
        }
        while (existingNames.Contains(candidate));

        return candidate;
    }

    private string NormalizeNotificationProvider(string? provider)
    {
        if (string.IsNullOrWhiteSpace(provider))
        {
            return AvailableNotificationProviders.Count > 0
                ? AvailableNotificationProviders[0]
                : DefaultNotificationProviders[0];
        }

        var normalized = provider.Trim();
        foreach (var candidate in AvailableNotificationProviders)
        {
            if (string.Equals(candidate, normalized, StringComparison.OrdinalIgnoreCase))
            {
                return candidate;
            }
        }

        return AvailableNotificationProviders.Count > 0
            ? AvailableNotificationProviders[0]
            : DefaultNotificationProviders[0];
    }

    private async Task EnsureNotificationProvidersLoadedAsync(CancellationToken cancellationToken)
    {
        string[] providers;
        try
        {
            providers = await Runtime.NotificationProviderFeatureService.GetAvailableProvidersAsync(cancellationToken);
        }
        catch (Exception ex)
        {
            providers = DefaultNotificationProviders;
            await RecordUnhandledExceptionAsync(
                "Settings.ExternalNotification.Providers",
                ex,
                UiErrorCode.NotificationProviderFailed,
                $"Failed to load provider list. Falling back to defaults: {ex.Message}",
                cancellationToken);
        }

        var normalizedProviders = providers
            .Where(static p => !string.IsNullOrWhiteSpace(p))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToList();
        if (normalizedProviders.Count == 0)
        {
            normalizedProviders = [.. DefaultNotificationProviders];
        }

        AvailableNotificationProviders.Clear();
        foreach (var provider in normalizedProviders)
        {
            AvailableNotificationProviders.Add(provider);
            if (!_notificationProviderParameters.ContainsKey(provider))
            {
                _notificationProviderParameters[provider] = string.Empty;
            }
        }

        _selectedNotificationProvider = NormalizeNotificationProvider(_selectedNotificationProvider);
        NotificationProviderParametersText = _notificationProviderParameters.TryGetValue(_selectedNotificationProvider, out var current)
            ? current
            : string.Empty;
        OnPropertyChanged(nameof(SelectedNotificationProvider));
    }

    private void PersistCurrentProviderParameters()
    {
        if (!string.IsNullOrWhiteSpace(SelectedNotificationProvider))
        {
            _notificationProviderParameters[SelectedNotificationProvider] = NotificationProviderParametersText;
        }
    }

    private void LoadExternalNotificationProviderParametersFromConfig(UnifiedConfig config)
    {
        _notificationProviderParameters.Clear();
        foreach (var provider in AvailableNotificationProviders)
        {
            _notificationProviderParameters[provider] = BuildProviderParameterTextFromConfig(provider, config);
        }

        var selected = NormalizeNotificationProvider(_selectedNotificationProvider);
        _selectedNotificationProvider = selected;
        NotificationProviderParametersText = _notificationProviderParameters.TryGetValue(selected, out var text)
            ? text
            : string.Empty;
        OnPropertyChanged(nameof(SelectedNotificationProvider));
    }

    private static string BuildProviderParameterTextFromConfig(string provider, UnifiedConfig config)
    {
        if (!ProviderConfigKeyMap.TryGetValue(provider, out var keyMap))
        {
            return string.Empty;
        }

        var lines = new List<string>();
        foreach (var (parameterKey, configKey) in keyMap)
        {
            if (!TryGetConfigNode(config, configKey, ConfigValuePreference.ProfileFirst, out var node) || node is null)
            {
                continue;
            }

            var value = node is JsonValue jsonValue
                ? jsonValue.ToString()
                : node.ToString();
            if (string.IsNullOrWhiteSpace(value))
            {
                continue;
            }

            lines.Add($"{parameterKey}={value.Trim()}");
        }

        return string.Join(Environment.NewLine, lines);
    }

    private static bool TryParseProviderParameterText(
        string? text,
        out Dictionary<string, string> parameters,
        out string? error)
    {
        parameters = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        error = null;
        if (string.IsNullOrWhiteSpace(text))
        {
            return true;
        }

        var lines = text.Replace("\r\n", "\n", StringComparison.Ordinal).Split('\n', StringSplitOptions.TrimEntries);
        for (var i = 0; i < lines.Length; i++)
        {
            var line = lines[i].Trim();
            if (line.Length == 0)
            {
                continue;
            }

            var separator = line.IndexOf('=');
            if (separator <= 0)
            {
                error = $"参数格式错误（第 {i + 1} 行），应为 key=value：`{line}`";
                return false;
            }

            var key = line[..separator].Trim();
            var value = line[(separator + 1)..].Trim();
            if (key.Length == 0)
            {
                error = $"参数格式错误（第 {i + 1} 行），key 不能为空。";
                return false;
            }

            parameters[key] = value;
        }

        return true;
    }

    private async Task<UiOperationResult> PopulateExternalNotificationProviderUpdatesAsync(
        Dictionary<string, string> updates,
        bool validateParameters,
        CancellationToken cancellationToken)
    {
        foreach (var provider in AvailableNotificationProviders)
        {
            if (!ProviderConfigKeyMap.TryGetValue(provider, out var keyMap))
            {
                continue;
            }

            var parameterText = _notificationProviderParameters.TryGetValue(provider, out var stored)
                ? stored
                : string.Empty;

            if (string.IsNullOrWhiteSpace(parameterText))
            {
                foreach (var (_, configKey) in keyMap)
                {
                    updates[configKey] = string.Empty;
                }

                continue;
            }

            if (validateParameters)
            {
                var validate = await Runtime.NotificationProviderFeatureService.ValidateProviderParametersAsync(
                    new NotificationProviderRequest(provider, parameterText),
                    cancellationToken);
                if (!validate.Success)
                {
                    return validate;
                }
            }

            if (!TryParseProviderParameterText(parameterText, out var parsed, out var parseError))
            {
                if (!validateParameters)
                {
                    continue;
                }

                return UiOperationResult.Fail(
                    UiErrorCode.NotificationProviderInvalidParameters,
                    parseError ?? "Provider parameter parsing failed.");
            }

            foreach (var (parameterKey, configKey) in keyMap)
            {
                updates[configKey] = parsed.TryGetValue(parameterKey, out var value)
                    ? value
                    : string.Empty;
            }
        }

        return UiOperationResult.Ok("External notification provider updates prepared.");
    }

    private static string FormatRemoteControlMessage(string? code, string fallbackMessage)
    {
        return code switch
        {
            UiErrorCode.RemoteControlInvalidParameters => $"远程控制参数错误：{fallbackMessage} ({UiErrorCode.RemoteControlInvalidParameters})",
            UiErrorCode.RemoteControlNetworkFailure => $"远程控制网络连通失败：{fallbackMessage} ({UiErrorCode.RemoteControlNetworkFailure})",
            UiErrorCode.RemoteControlUnsupported => $"当前环境不支持远程控制连通测试：{fallbackMessage} ({UiErrorCode.RemoteControlUnsupported})",
            _ => fallbackMessage,
        };
    }

    private static bool ContainsInvalidRemoteIdentity(string value)
    {
        if (string.IsNullOrEmpty(value))
        {
            return false;
        }

        foreach (var ch in value)
        {
            if (char.IsControl(ch))
            {
                return true;
            }
        }

        return false;
    }

    private static RemoteControlConnectivityResult? ParseRemoteConnectivityDetails(string? details)
    {
        if (string.IsNullOrWhiteSpace(details))
        {
            return null;
        }

        try
        {
            return JsonSerializer.Deserialize<RemoteControlConnectivityResult>(details);
        }
        catch
        {
            return null;
        }
    }

    private static string BuildRemoteConnectivitySummary(RemoteControlConnectivityResult? result)
    {
        if (result is null)
        {
            return string.Empty;
        }

        return $"GetTask={result.GetTaskProbe.Message}; Report={result.ReportProbe.Message}; Poll={result.PollIntervalMs}ms";
    }

    private static string FormatExternalNotificationMessage(string? code, string fallbackMessage)
    {
        return code switch
        {
            UiErrorCode.NotificationProviderInvalidParameters
                => $"外部通知参数错误：{fallbackMessage} ({UiErrorCode.NotificationProviderInvalidParameters})",
            UiErrorCode.NotificationProviderNetworkFailure
                => $"外部通知网络失败：{fallbackMessage} ({UiErrorCode.NotificationProviderNetworkFailure})",
            UiErrorCode.NotificationProviderUnsupported
                => $"当前环境不支持该外部通知能力：{fallbackMessage} ({UiErrorCode.NotificationProviderUnsupported})",
            _ => fallbackMessage,
        };
    }

    private async Task ApplyExternalNotificationFailure(
        UiOperationResult result,
        string scope,
        CancellationToken cancellationToken)
    {
        var message = FormatExternalNotificationMessage(result.Error?.Code, result.Message);
        if (string.Equals(result.Error?.Code, UiErrorCode.NotificationProviderUnsupported, StringComparison.Ordinal))
        {
            ExternalNotificationWarningMessage = message;
            ExternalNotificationErrorMessage = string.Empty;
        }
        else
        {
            ExternalNotificationErrorMessage = message;
            ExternalNotificationWarningMessage = string.Empty;
        }

        ExternalNotificationStatusMessage = "外部通知操作失败。";
        LastErrorMessage = message;
        await RecordFailedResultAsync(
            scope,
            UiOperationResult.Fail(result.Error?.Code ?? UiErrorCode.NotificationProviderFailed, message, result.Error?.Details),
            cancellationToken);
    }

    private static string NormalizeHotkeyGesture(string? value, string fallback)
    {
        var normalized = value?.Trim();
        return string.IsNullOrWhiteSpace(normalized)
            ? fallback
            : normalized;
    }

    private async Task RecordHotkeyRegistrationResultAsync(
        string hotkeyName,
        string gesture,
        UiOperationResult result,
        CancellationToken cancellationToken)
    {
        var scope = $"Settings.Hotkey.Register.{hotkeyName}";
        if (result.Success)
        {
            await RecordEventAsync(
                scope,
                $"source={hotkeyName} gesture={gesture} message={result.Message}",
                cancellationToken);
            return;
        }

        await RecordFailedResultAsync(
            scope,
            UiOperationResult.Fail(result.Error?.Code ?? UiErrorCode.HotkeyRegistrationFailed, result.Message, result.Error?.Details),
            cancellationToken);
    }

    private string BuildHotkeyErrorMessage(string hotkeyName, UiOperationResult result)
    {
        var localized = PlatformCapabilityTextMap.FormatErrorCode(
            Language,
            result.Error?.Code,
            result.Message,
            _localizationFallbackReporter);
        return string.IsNullOrWhiteSpace(result.Error?.Code)
            ? $"{hotkeyName}: {localized}"
            : $"{hotkeyName}: {localized} ({result.Error.Code})";
    }

    private static string GetHotkeySourceText(HotkeyRegistrationSource source)
    {
        return source == HotkeyRegistrationSource.Startup
            ? "启动自动注册"
            : "手动注册";
    }

    private async Task RefreshHotkeyFallbackWarningAsync(
        HotkeyRegistrationSource source,
        CancellationToken cancellationToken)
    {
        var snapshotResult = await Runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
        if (!snapshotResult.Success || snapshotResult.Value is null)
        {
            HotkeyWarningMessage = string.Empty;
            return;
        }

        var hotkeyCapability = snapshotResult.Value.Hotkey;
        if (hotkeyCapability.Supported || !hotkeyCapability.HasFallback)
        {
            HotkeyWarningMessage = string.Empty;
            return;
        }

        var localizedFallback = PlatformCapabilityTextMap.FormatErrorCode(
            Language,
            PlatformErrorCodes.HotkeyFallback,
            hotkeyCapability.Message,
            _localizationFallbackReporter);
        HotkeyWarningMessage =
            $"{localizedFallback} provider={hotkeyCapability.Provider}, mode={hotkeyCapability.FallbackMode ?? "unknown"}";
        await RecordEventAsync(
            "Settings.Hotkey.Fallback",
            $"source={source} provider={hotkeyCapability.Provider} mode={hotkeyCapability.FallbackMode ?? "unknown"} message={hotkeyCapability.Message}",
            cancellationToken);
    }

    private static IReadOnlyDictionary<string, string> ParseHotkeys(string raw, ICollection<string> warnings)
    {
        var parsed = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        if (string.IsNullOrWhiteSpace(raw))
        {
            return parsed;
        }

        var segments = raw.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        foreach (var segment in segments)
        {
            var index = segment.IndexOf('=');
            if (index <= 0 || index >= segment.Length - 1)
            {
                warnings.Add($"Ignored malformed hotkey segment: `{segment}`.");
                continue;
            }

            var key = segment[..index].Trim();
            var value = segment[(index + 1)..].Trim();
            if (string.IsNullOrWhiteSpace(value))
            {
                warnings.Add($"Ignored empty hotkey gesture for `{key}`.");
                continue;
            }

            var canonicalKey = string.Equals(key, ShowGuiHotkeyName, StringComparison.OrdinalIgnoreCase)
                ? ShowGuiHotkeyName
                : string.Equals(key, LinkStartHotkeyName, StringComparison.OrdinalIgnoreCase)
                    ? LinkStartHotkeyName
                    : null;
            if (canonicalKey is null)
            {
                warnings.Add($"Ignored unknown hotkey key: `{key}`.");
                continue;
            }

            parsed[canonicalKey] = value;
        }

        return parsed;
    }

    private static string SerializeHotkeys(string persistedShowGui, string persistedLinkStart)
    {
        return $"{ShowGuiHotkeyName}={persistedShowGui};{LinkStartHotkeyName}={persistedLinkStart}";
    }

    private UiOperationResult ValidateGuiSnapshot(GuiSettingsSnapshot snapshot)
    {
        if (!string.IsNullOrWhiteSpace(snapshot.BackgroundImagePath) && !File.Exists(snapshot.BackgroundImagePath))
        {
            return UiOperationResult.Fail(
                UiErrorCode.BackgroundImagePathNotFound,
                $"Background image path does not exist: {snapshot.BackgroundImagePath}");
        }

        return UiOperationResult.Ok("GUI settings validation passed.");
    }

    private GuiSettingsSnapshot BuildNormalizedGuiSnapshot()
    {
        return new GuiSettingsSnapshot(
            Theme: NormalizeTheme(Theme),
            Language: NormalizeLanguage(Language),
            UseTray: UseTray,
            MinimizeToTray: UseTray && MinimizeToTray,
            WindowTitleScrollable: WindowTitleScrollable,
            DeveloperModeEnabled: DeveloperModeEnabled,
            LogItemDateFormatString: NormalizeLogItemDateFormat(LogItemDateFormatString),
            OperNameLanguage: NormalizeOperNameLanguage(OperNameLanguage),
            InverseClearMode: NormalizeInverseClearMode(InverseClearMode),
            BackgroundImagePath: NormalizeBackgroundPath(BackgroundImagePath),
            BackgroundOpacity: Math.Clamp(BackgroundOpacity, BackgroundOpacityMin, BackgroundOpacityMax),
            BackgroundBlur: Math.Clamp(BackgroundBlur, BackgroundBlurMin, BackgroundBlurMax),
            BackgroundStretchMode: NormalizeBackgroundStretchMode(BackgroundStretchMode));
    }

    private void ApplyGuiSnapshotWithoutAutoSave(GuiSettingsSnapshot snapshot)
    {
        _suppressGuiAutoSave = true;
        try
        {
            Theme = snapshot.Theme;
            Language = snapshot.Language;
            UseTray = snapshot.UseTray;
            MinimizeToTray = snapshot.MinimizeToTray;
            WindowTitleScrollable = snapshot.WindowTitleScrollable;
            DeveloperModeEnabled = snapshot.DeveloperModeEnabled;
            LogItemDateFormatString = snapshot.LogItemDateFormatString;
            OperNameLanguage = snapshot.OperNameLanguage;
            InverseClearMode = snapshot.InverseClearMode;
            BackgroundImagePath = snapshot.BackgroundImagePath;
            BackgroundOpacity = snapshot.BackgroundOpacity;
            BackgroundBlur = snapshot.BackgroundBlur;
            BackgroundStretchMode = snapshot.BackgroundStretchMode;
        }
        finally
        {
            _suppressGuiAutoSave = false;
        }
    }

    private void RaiseGuiSettingsApplied(GuiSettingsSnapshot snapshot)
    {
        GuiSettingsApplied?.Invoke(this, new GuiSettingsAppliedEventArgs(snapshot));
    }

    private VersionUpdatePolicy BuildVersionUpdatePolicy()
    {
        return new VersionUpdatePolicy(
            Proxy: VersionUpdateProxy,
            ProxyType: VersionUpdateProxyType,
            VersionType: VersionUpdateVersionType,
            ResourceUpdateSource: VersionUpdateResourceSource,
            ForceGithubGlobalSource: VersionUpdateForceGithubSource,
            MirrorChyanCdk: VersionUpdateMirrorChyanCdk,
            MirrorChyanCdkExpired: VersionUpdateMirrorChyanCdkExpired,
            StartupUpdateCheck: VersionUpdateStartupCheck,
            ScheduledUpdateCheck: VersionUpdateScheduledCheck,
            ResourceApi: VersionUpdateResourceApi,
            AllowNightlyUpdates: VersionUpdateAllowNightly,
            HasAcknowledgedNightlyWarning: VersionUpdateAcknowledgedNightlyWarning,
            UseAria2: VersionUpdateUseAria2,
            AutoDownloadUpdatePackage: VersionUpdateAutoDownload,
            AutoInstallUpdatePackage: VersionUpdateAutoInstall,
            VersionName: VersionUpdateName,
            VersionBody: VersionUpdateBody,
            IsFirstBoot: VersionUpdateIsFirstBoot,
            VersionPackage: VersionUpdatePackage,
            DoNotShowUpdate: VersionUpdateDoNotShow);
    }

    private void ApplyVersionUpdatePolicy(VersionUpdatePolicy policy)
    {
        VersionUpdateProxy = policy.Proxy;
        VersionUpdateProxyType = policy.ProxyType;
        VersionUpdateVersionType = policy.VersionType;
        VersionUpdateResourceSource = policy.ResourceUpdateSource;
        VersionUpdateForceGithubSource = policy.ForceGithubGlobalSource;
        VersionUpdateMirrorChyanCdk = policy.MirrorChyanCdk;
        VersionUpdateMirrorChyanCdkExpired = policy.MirrorChyanCdkExpired;
        VersionUpdateStartupCheck = policy.StartupUpdateCheck;
        VersionUpdateScheduledCheck = policy.ScheduledUpdateCheck;
        VersionUpdateResourceApi = policy.ResourceApi;
        VersionUpdateAllowNightly = policy.AllowNightlyUpdates;
        VersionUpdateAcknowledgedNightlyWarning = policy.HasAcknowledgedNightlyWarning;
        VersionUpdateUseAria2 = policy.UseAria2;
        VersionUpdateAutoDownload = policy.AutoDownloadUpdatePackage;
        VersionUpdateAutoInstall = policy.AutoInstallUpdatePackage;
        VersionUpdateName = policy.VersionName;
        VersionUpdateBody = policy.VersionBody;
        VersionUpdateIsFirstBoot = policy.IsFirstBoot;
        VersionUpdatePackage = policy.VersionPackage;
        VersionUpdateDoNotShow = policy.DoNotShowUpdate;
    }

    private void ApplyAchievementPolicy(AchievementPolicy policy)
    {
        AchievementPopupDisabled = policy.PopupDisabled;
        AchievementPopupAutoClose = policy.PopupAutoClose;
    }

    private void UpdateAchievementPolicySummary(AchievementPolicy policy)
    {
        AchievementPolicySummary =
            $"当前策略：禁用弹窗={policy.PopupDisabled}；自动关闭={policy.PopupAutoClose}";
    }

    private static string NormalizeVersionUpdateProxyType(string? value)
    {
        var normalized = (value ?? string.Empty).Trim();
        if (string.Equals(normalized, "socks5", StringComparison.OrdinalIgnoreCase))
        {
            return "socks5";
        }

        return "http";
    }

    private static string NormalizeVersionUpdateResourceSource(string? value)
    {
        var normalized = (value ?? string.Empty).Trim();
        if (string.Equals(normalized, "Mirror", StringComparison.OrdinalIgnoreCase)
            || string.Equals(normalized, "MirrorChyan", StringComparison.OrdinalIgnoreCase))
        {
            return "MirrorChyan";
        }

        return "Github";
    }

    private void RebuildGuiOptionLists()
    {
        ThemeOptions = SettingsOptionCatalog.BuildThemeOptions(Language);
        SupportedLanguages = SettingsOptionCatalog.BuildLanguageOptions();
        BackgroundStretchModes = SettingsOptionCatalog.BuildBackgroundStretchOptions(Language);
        OperNameLanguageOptions = SettingsOptionCatalog.BuildOperNameLanguageOptions(Language);
        InverseClearModeOptions = SettingsOptionCatalog.BuildInverseClearModeOptions(Language);

        OnPropertyChanged(nameof(SelectedThemeOption));
        OnPropertyChanged(nameof(SelectedLanguageOption));
        OnPropertyChanged(nameof(SelectedBackgroundStretchModeOption));
        OnPropertyChanged(nameof(SelectedOperNameLanguageOption));
        OnPropertyChanged(nameof(SelectedInverseClearModeOption));
    }

    private void RebuildVersionUpdateOptionLists()
    {
        VersionUpdateVersionTypeOptions = SettingsOptionCatalog.BuildVersionTypeOptions(
            Language,
            VersionUpdateAllowNightly);

        VersionUpdateProxyTypeOptions =
        [
            new DisplayValueOption("HTTP Proxy", "http"),
            new DisplayValueOption("SOCKS5 Proxy", "socks5"),
        ];

        VersionUpdateResourceSourceOptions = SettingsOptionCatalog.BuildVersionResourceSourceOptions(Language);

        OnPropertyChanged(nameof(SelectedVersionUpdateVersionTypeOption));
        OnPropertyChanged(nameof(SelectedVersionUpdateProxyTypeOption));
        OnPropertyChanged(nameof(SelectedVersionUpdateResourceSourceOption));
    }

    private static (string UiVersion, string BuildTime) BuildVersionUpdateUiMetadata()
    {
        var assembly = typeof(SettingsPageViewModel).Assembly;
        var informationalVersion = assembly
            .GetCustomAttributes(typeof(System.Reflection.AssemblyInformationalVersionAttribute), inherit: false)
            .OfType<System.Reflection.AssemblyInformationalVersionAttribute>()
            .FirstOrDefault()
            ?.InformationalVersion;
        var uiVersion = string.IsNullOrWhiteSpace(informationalVersion)
            ? assembly.GetName().Version?.ToString() ?? "unknown"
            : informationalVersion.Split('+')[0];

        var buildTime = "unknown";
        try
        {
            if (!string.IsNullOrWhiteSpace(assembly.Location) && File.Exists(assembly.Location))
            {
                buildTime = File.GetLastWriteTime(assembly.Location)
                    .ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture);
            }
        }
        catch
        {
            buildTime = "unknown";
        }

        return (uiVersion, buildTime);
    }

    private async Task<T?> ApplyResultNoDialogAsync<T>(
        UiOperationResult<T> result,
        string scope,
        CancellationToken cancellationToken = default)
    {
        if (result.Success)
        {
            await RecordEventAsync(scope, result.Message, cancellationToken);
            LastErrorMessage = string.Empty;
            return result.Value;
        }

        LastErrorMessage = result.Message;
        var failed = UiOperationResult.Fail(
            result.Error?.Code ?? UiErrorCode.UiOperationFailed,
            result.Message,
            result.Error?.Details);
        await RecordFailedResultAsync(scope, failed, cancellationToken);
        return default;
    }

    private static string BuildMirrorChyanExpiryText(string? rawValue)
    {
        if (!long.TryParse(rawValue, NumberStyles.Integer, CultureInfo.InvariantCulture, out var unixSeconds)
            || unixSeconds <= 0)
        {
            return string.Empty;
        }

        if (unixSeconds == 1)
        {
            return "MirrorChyan CDK 已过期。";
        }

        var expiry = DateTimeOffset.FromUnixTimeSeconds(unixSeconds).LocalDateTime;
        var remaining = expiry - DateTime.Now;
        if (remaining.TotalSeconds <= 0)
        {
            return $"MirrorChyan CDK 已过期（{expiry:yyyy-MM-dd HH:mm}）。";
        }

        return $"MirrorChyan CDK 剩余 {remaining.TotalDays:F1} 天（至 {expiry:yyyy-MM-dd HH:mm}）。";
    }

    private static string BuildAboutVersionInfo()
    {
        var assembly = typeof(SettingsPageViewModel).Assembly.GetName();
        var version = assembly.Version?.ToString() ?? "unknown";
        return
            $"{assembly.Name} {version} | .NET {Environment.Version} | {RuntimeInformation.OSDescription}";
    }

    private string ResolveDebugDirectoryPath()
    {
        var debugDirectory = Path.GetDirectoryName(Runtime.DiagnosticsService.EventLogPath);
        if (!string.IsNullOrWhiteSpace(debugDirectory))
        {
            return debugDirectory;
        }

        return Path.Combine(ResolveRuntimeBaseDirectory(), "debug");
    }

    private string ResolveImageCacheDirectoryPath()
    {
        return Path.Combine(ResolveRuntimeBaseDirectory(), "cache", "images");
    }

    private IReadOnlyList<string> BuildEmulatorPathDialogCandidates()
    {
        var candidates = new List<string>();
        void AddCandidate(string? value)
        {
            var normalized = (value ?? string.Empty).Trim();
            if (normalized.Length == 0)
            {
                return;
            }

            if (candidates.Any(existing => string.Equals(existing, normalized, StringComparison.OrdinalIgnoreCase)))
            {
                return;
            }

            candidates.Add(normalized);
        }

        AddCandidate(EmulatorPath);
        var config = Runtime.ConfigurationService.CurrentConfig;
        AddCandidate(ReadConfigValue(config, LegacyConfigurationKeys.EmulatorPath));
        AddCandidate(ReadConfigValue(config, LegacyConfigurationKeys.MuMu12EmulatorPath));
        AddCandidate(ReadConfigValue(config, LegacyConfigurationKeys.LdPlayerEmulatorPath));
        return candidates;
    }

    private static string ReadConfigValue(UnifiedConfig config, string key)
    {
        if (!TryGetConfigNode(config, key, ConfigValuePreference.ProfileFirst, out var node) || node is null)
        {
            return string.Empty;
        }

        return node.ToString().Trim();
    }

    private string ResolveRuntimeBaseDirectory()
    {
        var debugDirectory = Path.GetDirectoryName(Runtime.DiagnosticsService.EventLogPath);
        if (!string.IsNullOrWhiteSpace(debugDirectory))
        {
            var parent = Directory.GetParent(debugDirectory);
            if (parent is not null)
            {
                return parent.FullName;
            }
        }

        return Environment.CurrentDirectory;
    }

    private async Task OpenAboutExternalTargetAsync(
        string target,
        string scope,
        string successMessage,
        CancellationToken cancellationToken)
    {
        ClearAboutStatus();
        var result = await _openExternalTargetAsync(target, cancellationToken);
        if (!await ApplyResultAsync(result, scope, cancellationToken))
        {
            AboutStatusMessage = "外链打开失败。";
            AboutErrorMessage = result.Message;
            return;
        }

        AboutStatusMessage = successMessage;
        AboutErrorMessage = string.Empty;
    }

    private static Task<UiOperationResult> OpenExternalTargetAsync(
        string target,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (string.IsNullOrWhiteSpace(target))
        {
            return Task.FromResult(
                UiOperationResult.Fail(
                    UiErrorCode.ExternalTargetMissing,
                    "External target cannot be empty."));
        }

        try
        {
            _ = Process.Start(new ProcessStartInfo
            {
                FileName = target,
                UseShellExecute = true,
            });
            return Task.FromResult(UiOperationResult.Ok($"Opened target: {target}"));
        }
        catch (Exception ex)
        {
            return Task.FromResult(UiOperationResult.Fail(
                UiErrorCode.ExternalTargetOpenFailed,
                $"Failed to open target `{target}`: {ex.Message}",
                ex.Message));
        }
    }

    private async Task LoadFromConfigAsync(UnifiedConfig config, CancellationToken cancellationToken)
    {
        ClearHotkeyStatus();
        ClearRemoteControlStatus();
        ClearExternalNotificationStatus();
        ClearAutostartFeedback();
        await EnsureNotificationProvidersLoadedAsync(cancellationToken);
        var guiWarnings = new List<string>();
        var backgroundWarnings = new List<string>();
        var hotkeyWarnings = new List<string>();

        var rawTheme = ReadGlobalString(config, ThemeModeKey, DefaultTheme);
        var rawLanguage = ReadGlobalString(config, ConfigurationKeys.Localization, DefaultLanguage);
        var rawBackgroundPath = ReadGlobalString(config, ConfigurationKeys.BackgroundImagePath, string.Empty);
        var rawOpacity = ReadGlobalInt(config, ConfigurationKeys.BackgroundOpacity, _backgroundOpacity);
        var rawBlur = ReadGlobalInt(config, ConfigurationKeys.BackgroundBlurEffectRadius, _backgroundBlur);
        var rawStretchMode = ReadGlobalString(config, ConfigurationKeys.BackgroundImageStretchMode, DefaultBackgroundStretchMode);
        var rawLogItemDateFormat = ReadGlobalString(config, ConfigurationKeys.LogItemDateFormat, DefaultLogItemDateFormat);
        var rawOperNameLanguage = ReadGlobalString(config, ConfigurationKeys.OperNameLanguage, DefaultOperNameLanguage);
        var rawInverseClearMode = ReadProfileString(config, ConfigurationKeys.InverseClearMode, DefaultInverseClearMode);
        var rawHotkeys = ReadGlobalString(config, ConfigurationKeys.HotKeys, string.Empty);
        var parsedHotkeys = ParseHotkeys(rawHotkeys, hotkeyWarnings);
        var loadedShowGui = NormalizeHotkeyGesture(
            parsedHotkeys.TryGetValue(ShowGuiHotkeyName, out var configuredShowGui)
                ? configuredShowGui
                : DefaultHotkeyShowGui,
            DefaultHotkeyShowGui);
        var loadedLinkStart = NormalizeHotkeyGesture(
            parsedHotkeys.TryGetValue(LinkStartHotkeyName, out var configuredLinkStart)
                ? configuredLinkStart
                : DefaultHotkeyLinkStart,
            DefaultHotkeyLinkStart);

        var theme = NormalizeTheme(rawTheme);
        if (!string.Equals(rawTheme, theme, StringComparison.Ordinal))
        {
            guiWarnings.Add($"Theme normalized to `{theme}` from `{rawTheme}`.");
        }

        var language = NormalizeLanguage(rawLanguage);
        if (!string.Equals(rawLanguage, language, StringComparison.OrdinalIgnoreCase))
        {
            guiWarnings.Add($"Language normalized to `{language}` from `{rawLanguage}`.");
        }

        var backgroundPath = NormalizeBackgroundPath(rawBackgroundPath);
        if (!string.IsNullOrWhiteSpace(backgroundPath) && !File.Exists(backgroundPath))
        {
            backgroundWarnings.Add($"Background path not found and reset: {backgroundPath}");
            backgroundPath = string.Empty;
        }

        var opacity = Math.Clamp(rawOpacity, BackgroundOpacityMin, BackgroundOpacityMax);
        if (opacity != rawOpacity)
        {
            backgroundWarnings.Add($"Background opacity clamped to {opacity} from {rawOpacity}.");
        }

        var blur = Math.Clamp(rawBlur, BackgroundBlurMin, BackgroundBlurMax);
        if (blur != rawBlur)
        {
            backgroundWarnings.Add($"Background blur clamped to {blur} from {rawBlur}.");
        }

        var stretch = NormalizeBackgroundStretchMode(rawStretchMode);
        if (!string.Equals(rawStretchMode, stretch, StringComparison.OrdinalIgnoreCase))
        {
            backgroundWarnings.Add($"Background stretch mode normalized to `{stretch}` from `{rawStretchMode}`.");
        }

        var logItemDateFormat = NormalizeLogItemDateFormat(rawLogItemDateFormat);
        if (!string.Equals(rawLogItemDateFormat, logItemDateFormat, StringComparison.Ordinal))
        {
            guiWarnings.Add(
                $"Log item date format normalized to `{logItemDateFormat}` from `{rawLogItemDateFormat}`.");
        }

        var operNameLanguage = NormalizeOperNameLanguage(rawOperNameLanguage);
        if (!string.Equals(rawOperNameLanguage, operNameLanguage, StringComparison.OrdinalIgnoreCase))
        {
            guiWarnings.Add(
                $"Oper name language normalized to `{operNameLanguage}` from `{rawOperNameLanguage}`.");
        }

        var inverseClearMode = NormalizeInverseClearMode(rawInverseClearMode);
        if (!string.Equals(rawInverseClearMode, inverseClearMode, StringComparison.OrdinalIgnoreCase))
        {
            guiWarnings.Add(
                $"Inverse clear mode normalized to `{inverseClearMode}` from `{rawInverseClearMode}`.");
        }

        _suppressGuiAutoSave = true;
        try
        {
            Theme = theme;
            Language = language;
            UseTray = ReadGlobalBool(config, ConfigurationKeys.UseTray, true);
            MinimizeToTray = ReadGlobalBool(config, ConfigurationKeys.MinimizeToTray, false);
            WindowTitleScrollable = ReadGlobalBool(config, ConfigurationKeys.WindowTitleScrollable, false);
            DeveloperModeEnabled = ReadGlobalBool(config, DeveloperModeConfigKey, false);
            LogItemDateFormatString = logItemDateFormat;
            OperNameLanguage = operNameLanguage;
            InverseClearMode = inverseClearMode;
            BackgroundImagePath = backgroundPath;
            BackgroundOpacity = opacity;
            BackgroundBlur = blur;
            BackgroundStretchMode = stretch;
            RemoteGetTaskEndpoint = ReadProfileString(config, ConfigurationKeys.RemoteControlGetTaskEndpointUri, string.Empty);
            RemoteReportEndpoint = ReadProfileString(config, ConfigurationKeys.RemoteControlReportStatusUri, string.Empty);
            RemoteUserIdentity = ReadProfileString(config, ConfigurationKeys.RemoteControlUserIdentity, string.Empty).Trim();
            RemoteDeviceIdentity = ReadProfileString(config, ConfigurationKeys.RemoteControlDeviceIdentity, string.Empty).Trim();
            RemotePollInterval = ReadProfileInt(config, ConfigurationKeys.RemoteControlPollIntervalMs, DefaultRemotePollIntervalMs);
            ExternalNotificationEnabled = ReadProfileBool(config, ConfigurationKeys.ExternalNotificationEnabled, false);
            ExternalNotificationSendWhenComplete = ReadProfileBool(config, ConfigurationKeys.ExternalNotificationSendWhenComplete, true);
            ExternalNotificationSendWhenError = ReadProfileBool(config, ConfigurationKeys.ExternalNotificationSendWhenError, true);
            ExternalNotificationSendWhenTimeout = ReadProfileBool(config, ConfigurationKeys.ExternalNotificationSendWhenTimeout, true);
            ExternalNotificationEnableDetails = ReadProfileBool(config, ConfigurationKeys.ExternalNotificationEnableDetails, false);
            HotkeyShowGui = loadedShowGui;
            HotkeyLinkStart = loadedLinkStart;
            _persistedHotkeyShowGui = loadedShowGui;
            _persistedHotkeyLinkStart = loadedLinkStart;
            LoadExternalNotificationProviderParametersFromConfig(config);
            HasPendingGuiChanges = false;
        }
        finally
        {
            _suppressGuiAutoSave = false;
        }

        var startPerformanceWarnings = new List<string>();
        NormalizeUnsupportedGpuSettingsInConfig(config, startPerformanceWarnings);
        var startPerformanceSnapshot = ReadStartPerformanceSnapshot(config, startPerformanceWarnings);
        ApplyStartPerformanceSnapshotWithoutDirtyTracking(startPerformanceSnapshot);
        HasPendingStartPerformanceChanges = false;

        var timerWarnings = new List<string>();
        var timerSnapshot = ReadTimerSnapshot(config, timerWarnings);
        ApplyTimerSnapshot(timerSnapshot);
        HasPendingTimerChanges = false;

        var versionPolicyResult = await Runtime.VersionUpdateFeatureService.LoadPolicyAsync(cancellationToken);
        if (versionPolicyResult.Success && versionPolicyResult.Value is not null)
        {
            ApplyVersionUpdatePolicy(versionPolicyResult.Value);
            UpdatePanelCoreVersion = string.IsNullOrWhiteSpace(versionPolicyResult.Value.VersionName)
                ? "unknown"
                : versionPolicyResult.Value.VersionName;
            VersionUpdateErrorMessage = string.Empty;
        }
        else
        {
            UpdatePanelCoreVersion = "unknown";
            VersionUpdateErrorMessage = versionPolicyResult.Message;
        }

        await RefreshVersionUpdateResourceInfoAsync(cancellationToken);

        var achievementPolicyResult = await Runtime.AchievementFeatureService.LoadPolicyAsync(cancellationToken);
        if (achievementPolicyResult.Success && achievementPolicyResult.Value is not null)
        {
            ApplyAchievementPolicy(achievementPolicyResult.Value);
            UpdateAchievementPolicySummary(achievementPolicyResult.Value);
            AchievementErrorMessage = string.Empty;
        }
        else
        {
            UpdateAchievementPolicySummary(AchievementPolicy.Default);
            AchievementErrorMessage = achievementPolicyResult.Message;
        }

        var warnings = guiWarnings.Concat(backgroundWarnings).ToArray();
        if (warnings.Length > 0)
        {
            GuiSectionValidationMessage = string.Join(" ", guiWarnings);
            BackgroundValidationMessage = string.Join(" ", backgroundWarnings);
            StatusMessage = GuiValidationMessage;
            await RecordEventAsync(
                "Settings.Gui.Normalize",
                string.Join(" | ", warnings),
                cancellationToken);
        }
        else
        {
            ClearGuiValidationMessages();
        }

        if (hotkeyWarnings.Count > 0)
        {
            HotkeyWarningMessage = string.Join(" ", hotkeyWarnings);
            await RecordEventAsync(
                "Settings.Hotkey.Normalize",
                string.Join(" | ", hotkeyWarnings),
                cancellationToken);
        }
        else
        {
            HotkeyWarningMessage = string.Empty;
        }

        if (startPerformanceWarnings.Count > 0)
        {
            StartPerformanceValidationMessage = string.Join(" ", startPerformanceWarnings);
            await RecordEventAsync(
                "Settings.StartPerformance.Normalize",
                string.Join(" | ", startPerformanceWarnings),
                cancellationToken);
        }
        else
        {
            StartPerformanceValidationMessage = string.Empty;
        }

        if (timerWarnings.Count > 0)
        {
            TimerValidationMessage = string.Join(" ", timerWarnings);
            await RecordEventAsync(
                "Settings.Timer.Normalize",
                string.Join(" | ", timerWarnings),
                cancellationToken);
        }
        else
        {
            TimerValidationMessage = string.Empty;
        }

    }

    private void BeginAutostartInteraction()
    {
        _lastAutostartToggleAt = DateTimeOffset.UtcNow;
        ClearAutostartFeedback();
    }

    private void ClearAutostartFeedback()
    {
        var pendingFeedback = _autostartFeedbackCts;
        _autostartFeedbackCts = null;
        pendingFeedback?.Cancel();
        AutostartWarningMessage = string.Empty;
        AutostartErrorMessage = string.Empty;
    }

    private async Task ShowAutostartWarningWithDelayAsync(string message, CancellationToken cancellationToken)
    {
        await ShowAutostartFeedbackWithDelayAsync(
            warningMessage: message,
            errorMessage: string.Empty,
            cancellationToken);
    }

    private async Task ShowAutostartErrorWithDelayAsync(string message, CancellationToken cancellationToken)
    {
        await ShowAutostartFeedbackWithDelayAsync(
            warningMessage: string.Empty,
            errorMessage: message,
            cancellationToken);
    }

    private async Task ShowAutostartFeedbackWithDelayAsync(
        string warningMessage,
        string errorMessage,
        CancellationToken cancellationToken)
    {
        var pendingFeedback = _autostartFeedbackCts;
        pendingFeedback?.Cancel();
        _autostartFeedbackCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        var feedbackCts = _autostartFeedbackCts;

        try
        {
            var remainingDelay = GetRemainingAutostartFeedbackDelay();
            if (remainingDelay > TimeSpan.Zero)
            {
                await Task.Delay(remainingDelay, feedbackCts.Token);
            }

            if (feedbackCts.Token.IsCancellationRequested)
            {
                return;
            }

            AutostartWarningMessage = warningMessage;
            AutostartErrorMessage = errorMessage;
        }
        catch (OperationCanceledException) when (feedbackCts.IsCancellationRequested)
        {
            // Newer toggle state superseded the pending feedback.
        }
        finally
        {
            if (ReferenceEquals(_autostartFeedbackCts, feedbackCts))
            {
                feedbackCts.Dispose();
                _autostartFeedbackCts = null;
            }
            else
            {
                feedbackCts.Dispose();
            }
        }
    }

    private TimeSpan GetRemainingAutostartFeedbackDelay()
    {
        if (!_lastAutostartToggleAt.HasValue)
        {
            return TimeSpan.Zero;
        }

        var remaining = TimeSpan.FromMilliseconds(AutostartFeedbackDelayMs)
            - (DateTimeOffset.UtcNow - _lastAutostartToggleAt.Value);
        return remaining > TimeSpan.Zero ? remaining : TimeSpan.Zero;
    }

    private string BuildAutostartSetErrorMessage(string? errorCode, string fallbackMessage)
    {
        var localized = PlatformCapabilityTextMap.FormatErrorCode(
            Language,
            errorCode,
            fallbackMessage,
            _localizationFallbackReporter);

        return string.Equals(localized, fallbackMessage, StringComparison.Ordinal)
            ? fallbackMessage
            : $"{localized}：{fallbackMessage}";
    }

    private string BuildAutostartMismatchMessage(bool actualEnabled)
    {
        var verificationFailed = PlatformCapabilityTextMap.FormatErrorCode(
            Language,
            PlatformErrorCodes.AutostartVerificationFailed,
            "Autostart verification failed",
            _localizationFallbackReporter);
        var actualStatus = PlatformCapabilityTextMap.FormatAutostartStatus(
            Language,
            actualEnabled,
            _localizationFallbackReporter);
        return $"{verificationFailed}，{actualStatus}";
    }

    private void UpdateCombinedGuiValidationMessage()
    {
        GuiValidationMessage = CombineValidationMessages(GuiSectionValidationMessage, BackgroundValidationMessage);
    }

    private void ClearGuiValidationMessages()
    {
        GuiSectionValidationMessage = string.Empty;
        BackgroundValidationMessage = string.Empty;
    }

    private void SetGuiValidationMessageForCurrentSection(string message)
    {
        if (IsBackgroundSelected)
        {
            GuiSectionValidationMessage = string.Empty;
            BackgroundValidationMessage = message;
            return;
        }

        GuiSectionValidationMessage = message;
        BackgroundValidationMessage = string.Empty;
    }

    private void SetGuiValidationMessageForResult(UiOperationResult result)
    {
        if (string.Equals(result.Error?.Code, UiErrorCode.BackgroundImagePathNotFound, StringComparison.Ordinal))
        {
            GuiSectionValidationMessage = string.Empty;
            BackgroundValidationMessage = result.Message;
            return;
        }

        SetGuiValidationMessageForCurrentSection(result.Message);
    }

    private static string CombineValidationMessages(params string[] messages)
    {
        return string.Join(
            " ",
            messages.Where(static message => !string.IsNullOrWhiteSpace(message)));
    }

    private void LoadConnectionSharedStateFromConfig()
    {
        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return;
        }

        ConnectionGameProfileSync.ReadFromProfile(profile, ConnectionGameSharedState, tolerateMissing: false);
    }

    private static string ReadGlobalString(UnifiedConfig config, string key, string fallback)
        => ReadString(config, key, fallback, ConfigValuePreference.GlobalFirst);

    private static string ReadProfileString(UnifiedConfig config, string key, string fallback)
        => ReadString(config, key, fallback, ConfigValuePreference.ProfileFirst);

    private static bool ReadGlobalBool(UnifiedConfig config, string key, bool fallback)
        => ReadBool(config, key, fallback, ConfigValuePreference.GlobalFirst);

    private static bool ReadProfileBool(UnifiedConfig config, string key, bool fallback)
        => ReadBool(config, key, fallback, ConfigValuePreference.ProfileFirst);

    private static int ReadGlobalInt(UnifiedConfig config, string key, int fallback)
        => ReadInt(config, key, fallback, ConfigValuePreference.GlobalFirst);

    private static int ReadProfileInt(UnifiedConfig config, string key, int fallback)
        => ReadInt(config, key, fallback, ConfigValuePreference.ProfileFirst);

    private static string ReadString(
        UnifiedConfig config,
        string key,
        string fallback,
        ConfigValuePreference preference)
    {
        if (TryGetConfigNode(config, key, preference, out var node) && node is not null)
        {
            if (node is JsonValue value && value.TryGetValue(out string? text) && !string.IsNullOrWhiteSpace(text))
            {
                return text;
            }

            var raw = node.ToString();
            if (!string.IsNullOrWhiteSpace(raw))
            {
                return raw;
            }
        }

        return fallback;
    }

    private static bool ReadBool(
        UnifiedConfig config,
        string key,
        bool fallback,
        ConfigValuePreference preference)
    {
        if (TryGetConfigNode(config, key, preference, out var node) && node is not null)
        {
            return bool.TryParse(node.ToString(), out var parsed) ? parsed : fallback;
        }

        return fallback;
    }

    private static int ReadInt(
        UnifiedConfig config,
        string key,
        int fallback,
        ConfigValuePreference preference)
    {
        if (TryGetConfigNode(config, key, preference, out var node) && node is not null)
        {
            return int.TryParse(node.ToString(), out var parsed) ? parsed : fallback;
        }

        return fallback;
    }

    private string NormalizeTheme(string? value)
    {
        var normalized = value?.Trim();
        if (string.Equals(normalized, "System", StringComparison.OrdinalIgnoreCase)
            || string.Equals(normalized, "Default", StringComparison.OrdinalIgnoreCase))
        {
            return "SyncWithOs";
        }

        return ThemeOptions.FirstOrDefault(
                option => string.Equals(option.Value, normalized, StringComparison.OrdinalIgnoreCase))
            ?.Value
            ?? DefaultTheme;
    }

    private string NormalizeLanguage(string? value)
    {
        return UiLanguageCatalog.Normalize(value);
    }

    private string NormalizeBackgroundStretchMode(string? value)
    {
        var normalized = value?.Trim();
        return BackgroundStretchModes.FirstOrDefault(
                option => string.Equals(option.Value, normalized, StringComparison.OrdinalIgnoreCase))
            ?.Value
            ?? DefaultBackgroundStretchMode;
    }

    private string NormalizeLogItemDateFormat(string? value)
    {
        var normalized = (value ?? string.Empty).Trim();
        return LogItemDateFormatOptions.Contains(normalized, StringComparer.Ordinal)
            ? normalized
            : DefaultLogItemDateFormat;
    }

    private string NormalizeOperNameLanguage(string? value)
    {
        var normalized = (value ?? string.Empty).Trim();
        return OperNameLanguageOptions.FirstOrDefault(
                option => string.Equals(option.Value, normalized, StringComparison.OrdinalIgnoreCase))
            ?.Value
            ?? DefaultOperNameLanguage;
    }

    private string NormalizeInverseClearMode(string? value)
    {
        var normalized = (value ?? string.Empty).Trim();
        return InverseClearModeOptions.FirstOrDefault(
                option => string.Equals(option.Value, normalized, StringComparison.OrdinalIgnoreCase))
            ?.Value
            ?? DefaultInverseClearMode;
    }

    private static string NormalizeBackgroundPath(string? value)
    {
        return value?.Trim() ?? string.Empty;
    }

    private GpuPreference BuildCurrentGpuPreference()
    {
        return new GpuPreference(
            UseGpu: PerformanceUseGpu,
            AllowDeprecatedGpu: PerformanceAllowDeprecatedGpu,
            PreferredGpuDescription: PerformancePreferredGpuDescription,
            PreferredGpuInstancePath: PerformancePreferredGpuInstancePath);
    }

    private void RefreshGpuUiState()
    {
        if (_suppressGpuUiRefresh)
        {
            return;
        }

        var resolution = Runtime.Platform.GpuCapabilityService.Resolve(BuildCurrentGpuPreference());
        var selectedOption = resolution.SelectedOption;

        var previousSuppressUi = _suppressGpuUiRefresh;
        var previousSuppressSelection = _suppressGpuSelectionChange;
        var previousSuppressDirty = _suppressStartPerformanceDirtyTracking;
        _suppressGpuUiRefresh = true;
        _suppressGpuSelectionChange = true;
        _suppressStartPerformanceDirtyTracking = true;

        try
        {
            SetGpuLegacyPropertiesSilently(
                selectedOption,
                selectedOption.IsCustomEntry ? PerformancePreferredGpuDescription : null,
                selectedOption.IsCustomEntry ? PerformancePreferredGpuInstancePath : null);

            AvailableGpuOptions = resolution.Snapshot.Options
                .Select(BuildGpuOptionDisplayItem)
                .ToArray();

            SelectedGpuOption = AvailableGpuOptions.FirstOrDefault(
                option => string.Equals(option.Descriptor.Id, selectedOption.Id, StringComparison.Ordinal))
                ?? AvailableGpuOptions.FirstOrDefault();

            GpuCustomDescription = PerformancePreferredGpuDescription;
            GpuCustomInstancePath = PerformancePreferredGpuInstancePath;
            GpuSupportMessage = LocalizeRootText(resolution.Snapshot.StatusTextKey);
            GpuWarningMessage = BuildGpuWarningMessage(resolution);
            IsGpuSelectionEnabled = resolution.Snapshot.IsEditable;
            IsGpuDeprecatedToggleEnabled = resolution.Snapshot.IsEditable && resolution.Snapshot.SupportsDeprecatedToggle;
            IsGpuCustomSelectionFieldsVisible = resolution.Snapshot.IsEditable && selectedOption.IsCustomEntry;
            ShowGpuRestartRequiredHint = resolution.Snapshot.AppliesToCore;
        }
        finally
        {
            _suppressStartPerformanceDirtyTracking = previousSuppressDirty;
            _suppressGpuSelectionChange = previousSuppressSelection;
            _suppressGpuUiRefresh = previousSuppressUi;
        }
    }

    private void ApplyGpuSelection(GpuOptionDescriptor descriptor)
    {
        SetGpuLegacyPropertiesSilently(
            descriptor,
            descriptor.IsCustomEntry ? GpuCustomDescription : null,
            descriptor.IsCustomEntry ? GpuCustomInstancePath : null);

        if (descriptor.IsCustomEntry)
        {
            IsGpuCustomSelectionFieldsVisible = true;
            GpuWarningMessage = BuildGpuWarningMessage(Runtime.Platform.GpuCapabilityService.Resolve(BuildCurrentGpuPreference()));
            MarkStartPerformanceDirty();
            return;
        }

        RefreshGpuUiState();
        MarkStartPerformanceDirty();
    }

    private void ApplyCustomGpuFields()
    {
        if (SelectedGpuOption?.Descriptor.IsCustomEntry != true)
        {
            return;
        }

        SetGpuLegacyPropertiesSilently(
            SelectedGpuOption.Descriptor,
            GpuCustomDescription,
            GpuCustomInstancePath);
        RefreshGpuUiState();
        MarkStartPerformanceDirty();
    }

    private void SetGpuLegacyPropertiesSilently(
        GpuOptionDescriptor descriptor,
        string? descriptionOverride = null,
        string? instancePathOverride = null)
    {
        var previousSuppressUi = _suppressGpuUiRefresh;
        var previousSuppressDirty = _suppressStartPerformanceDirtyTracking;
        _suppressGpuUiRefresh = true;
        _suppressStartPerformanceDirtyTracking = true;

        try
        {
            switch (descriptor.Kind)
            {
                case GpuOptionKind.Disabled:
                    PerformanceUseGpu = false;
                    PerformancePreferredGpuDescription = string.Empty;
                    PerformancePreferredGpuInstancePath = string.Empty;
                    break;

                case GpuOptionKind.SystemDefault:
                    PerformanceUseGpu = true;
                    PerformancePreferredGpuDescription = string.Empty;
                    PerformancePreferredGpuInstancePath = string.Empty;
                    break;

                case GpuOptionKind.SpecificGpu:
                    PerformanceUseGpu = true;
                    PerformancePreferredGpuDescription = (descriptionOverride ?? descriptor.Description ?? string.Empty).Trim();
                    PerformancePreferredGpuInstancePath = (instancePathOverride ?? descriptor.InstancePath ?? string.Empty).Trim();
                    break;
            }
        }
        finally
        {
            _suppressStartPerformanceDirtyTracking = previousSuppressDirty;
            _suppressGpuUiRefresh = previousSuppressUi;
        }
    }

    private GpuOptionDisplayItem BuildGpuOptionDisplayItem(GpuOptionDescriptor descriptor)
    {
        return new GpuOptionDisplayItem(descriptor, FormatGpuOptionDisplay(descriptor));
    }

    private string FormatGpuOptionDisplay(GpuOptionDescriptor descriptor)
    {
        return descriptor.Kind switch
        {
            GpuOptionKind.Disabled => RootTexts["Settings.Performance.Gpu.Option.Disabled"],
            GpuOptionKind.SystemDefault => string.IsNullOrWhiteSpace(descriptor.DisplayName)
                ? RootTexts["Settings.Performance.Gpu.Option.SystemDefault"]
                : $"{RootTexts["Settings.Performance.Gpu.Option.SystemDefault"]} ({descriptor.DisplayName})",
            GpuOptionKind.SpecificGpu when descriptor.IsCustomEntry
                => !string.IsNullOrWhiteSpace(descriptor.DisplayName)
                    ? descriptor.DisplayName
                    : !string.IsNullOrWhiteSpace(descriptor.InstancePath)
                        ? descriptor.InstancePath
                        : RootTexts["Settings.Performance.Gpu.Option.Custom"],
            _ => !string.IsNullOrWhiteSpace(descriptor.DisplayName)
                ? descriptor.DisplayName
                : descriptor.Description,
        };
    }

    private string BuildGpuWarningMessage(GpuSelectionResolution resolution)
    {
        var warnings = new List<string>();

        if (!string.IsNullOrWhiteSpace(resolution.Snapshot.WarningTextKey))
        {
            warnings.Add(LocalizeRootText(resolution.Snapshot.WarningTextKey));
        }

        if (!string.IsNullOrWhiteSpace(resolution.SelectionWarningTextKey))
        {
            warnings.Add(LocalizeRootText(resolution.SelectionWarningTextKey));
        }

        if (resolution.Snapshot.SupportMode == GpuPlatformSupportMode.WindowsSupported)
        {
            if (resolution.SelectedOption.IsDeprecated)
            {
                warnings.Add(LocalizeRootText("Settings.Performance.Gpu.Warning.Deprecated"));
            }

            if (resolution.SelectedOption.DriverDate.HasValue
                && resolution.SelectedOption.DriverDate.Value < GpuCapabilityConstants.DirectMlDriverMinimumDate)
            {
                warnings.Add(LocalizeRootText("Settings.Performance.Gpu.Warning.OutdatedDriver"));
            }
        }

        return string.Join(
            " ",
            warnings
                .Where(static warning => !string.IsNullOrWhiteSpace(warning))
                .Distinct(StringComparer.Ordinal));
    }

    private string LocalizeRootText(string? key)
    {
        return string.IsNullOrWhiteSpace(key) ? string.Empty : RootTexts[key];
    }

    private StartPerformanceSettingsSnapshot BuildNormalizedStartPerformanceSnapshot()
    {
        return new StartPerformanceSettingsSnapshot(
            RunDirectly: RunDirectly,
            MinimizeDirectly: MinimizeDirectly,
            OpenEmulatorAfterLaunch: OpenEmulatorAfterLaunch,
            EmulatorPath: (EmulatorPath ?? string.Empty).Trim(),
            EmulatorAddCommand: (EmulatorAddCommand ?? string.Empty).Trim(),
            EmulatorWaitSeconds: EmulatorWaitSeconds,
            PerformanceUseGpu: PerformanceUseGpu,
            PerformanceAllowDeprecatedGpu: PerformanceAllowDeprecatedGpu,
            PerformancePreferredGpuDescription: (PerformancePreferredGpuDescription ?? string.Empty).Trim(),
            PerformancePreferredGpuInstancePath: (PerformancePreferredGpuInstancePath ?? string.Empty).Trim(),
            DeploymentWithPause: DeploymentWithPause,
            StartsWithScript: (StartsWithScript ?? string.Empty).Trim(),
            EndsWithScript: (EndsWithScript ?? string.Empty).Trim(),
            CopilotWithScript: CopilotWithScript,
            ManualStopWithScript: ManualStopWithScript,
            BlockSleep: BlockSleep,
            BlockSleepWithScreenOn: BlockSleepWithScreenOn,
            EnablePenguin: EnablePenguin,
            EnableYituliu: EnableYituliu,
            PenguinId: (PenguinId ?? string.Empty).Trim(),
            TaskTimeoutMinutes: Math.Max(0, TaskTimeoutMinutes),
            ReminderIntervalMinutes: Math.Max(1, ReminderIntervalMinutes));
    }

    private static UiOperationResult ValidateStartPerformanceSnapshot(StartPerformanceSettingsSnapshot snapshot)
    {
        if (snapshot.EmulatorWaitSeconds < EmulatorWaitSecondsMin || snapshot.EmulatorWaitSeconds > EmulatorWaitSecondsMax)
        {
            return UiOperationResult.Fail(
                UiErrorCode.EmulatorWaitSecondsOutOfRange,
                $"Emulator wait seconds must be within {EmulatorWaitSecondsMin}-{EmulatorWaitSecondsMax}.");
        }

        if (!snapshot.OpenEmulatorAfterLaunch)
        {
            return UiOperationResult.Ok("Start/Performance settings validation passed.");
        }

        if (string.IsNullOrWhiteSpace(snapshot.EmulatorPath))
        {
            return UiOperationResult.Fail(
                UiErrorCode.EmulatorPathMissing,
                "Emulator path is required when OpenEmulatorAfterLaunch is enabled.");
        }

        if (!File.Exists(snapshot.EmulatorPath))
        {
            return UiOperationResult.Fail(
                UiErrorCode.EmulatorPathNotFound,
                $"Emulator path does not exist: {snapshot.EmulatorPath}");
        }

        return UiOperationResult.Ok("Start/Performance settings validation passed.");
    }

    private StartPerformanceSettingsSnapshot ReadStartPerformanceSnapshot(
        UnifiedConfig config,
        ICollection<string> warnings)
    {
        var emulatorPath = ReadProfileString(config, ConfigurationKeys.EmulatorPath, string.Empty).Trim();
        var emulatorAddCommand = ReadProfileString(config, ConfigurationKeys.EmulatorAddCommand, string.Empty).Trim();
        var preferredGpuDescription = ReadProfileString(config, ConfigurationKeys.PerformancePreferredGpuDescription, string.Empty).Trim();
        var preferredGpuInstancePath = ReadProfileString(config, ConfigurationKeys.PerformancePreferredGpuInstancePath, string.Empty).Trim();

        var rawWaitSeconds = ReadProfileInt(config, ConfigurationKeys.EmulatorWaitSeconds, DefaultEmulatorWaitSeconds);
        var emulatorWaitSeconds = Math.Clamp(rawWaitSeconds, EmulatorWaitSecondsMin, EmulatorWaitSecondsMax);
        if (emulatorWaitSeconds != rawWaitSeconds)
        {
            warnings.Add(
                $"Start.EmulatorWaitSeconds clamped to {emulatorWaitSeconds} from {rawWaitSeconds}.");
        }

        return new StartPerformanceSettingsSnapshot(
            RunDirectly: ReadProfileBoolFlexible(config, ConfigurationKeys.RunDirectly, false),
            MinimizeDirectly: ReadGlobalBoolFlexible(config, ConfigurationKeys.MinimizeDirectly, false),
            OpenEmulatorAfterLaunch: ReadProfileBoolFlexible(config, ConfigurationKeys.StartEmulator, false),
            EmulatorPath: emulatorPath,
            EmulatorAddCommand: emulatorAddCommand,
            EmulatorWaitSeconds: emulatorWaitSeconds,
            PerformanceUseGpu: ReadProfileBoolFlexible(config, ConfigurationKeys.PerformanceUseGpu, false),
            PerformanceAllowDeprecatedGpu: ReadProfileBoolFlexible(config, ConfigurationKeys.PerformanceAllowDeprecatedGpu, false),
            PerformancePreferredGpuDescription: preferredGpuDescription,
            PerformancePreferredGpuInstancePath: preferredGpuInstancePath,
            DeploymentWithPause: ReadProfileBoolFlexible(config, ConfigurationKeys.RoguelikeDeploymentWithPause, false),
            StartsWithScript: ReadProfileString(config, ConfigurationKeys.StartsWithScript, string.Empty).Trim(),
            EndsWithScript: ReadProfileString(config, ConfigurationKeys.EndsWithScript, string.Empty).Trim(),
            CopilotWithScript: ReadProfileBoolFlexible(config, ConfigurationKeys.CopilotWithScript, false),
            ManualStopWithScript: ReadProfileBoolFlexible(config, ConfigurationKeys.ManualStopWithScript, false),
            BlockSleep: ReadProfileBoolFlexible(config, ConfigurationKeys.BlockSleep, false),
            BlockSleepWithScreenOn: ReadProfileBoolFlexible(config, ConfigurationKeys.BlockSleepWithScreenOn, true),
            EnablePenguin: ReadProfileBoolFlexible(config, ConfigurationKeys.EnablePenguin, true),
            EnableYituliu: ReadProfileBoolFlexible(config, ConfigurationKeys.EnableYituliu, true),
            PenguinId: ReadProfileString(config, ConfigurationKeys.PenguinId, string.Empty).Trim(),
            TaskTimeoutMinutes: Math.Max(0, ReadProfileInt(config, ConfigurationKeys.TaskTimeoutMinutes, DefaultTaskTimeoutMinutes)),
            ReminderIntervalMinutes: Math.Max(1, ReadProfileInt(config, ConfigurationKeys.ReminderIntervalMinutes, DefaultReminderIntervalMinutes)));
    }

    private void NormalizeUnsupportedGpuSettingsInConfig(UnifiedConfig config, ICollection<string> warnings)
    {
        var supportMode = Runtime.Platform.GpuCapabilityService.Resolve(
            new GpuPreference(false, false, string.Empty, string.Empty)).Snapshot.SupportMode;
        if (supportMode == GpuPlatformSupportMode.WindowsSupported)
        {
            return;
        }

        var normalized = NormalizeUnsupportedGpuSettings(config.GlobalValues);
        foreach (var profile in config.Profiles.Values)
        {
            normalized |= NormalizeUnsupportedGpuSettings(profile.Values);
        }

        if (normalized)
        {
            warnings.Add("Unsupported GPU settings were removed for this platform. CPU OCR fallback will be used.");
        }
    }

    private static bool NormalizeUnsupportedGpuSettings(IDictionary<string, JsonNode?> values)
    {
        if (!ContainsUnsafeGpuSettings(values))
        {
            return false;
        }

        values.Remove(ConfigurationKeys.PerformanceUseGpu);
        values.Remove(ConfigurationKeys.PerformanceAllowDeprecatedGpu);
        values.Remove(ConfigurationKeys.PerformancePreferredGpuDescription);
        values.Remove(ConfigurationKeys.PerformancePreferredGpuInstancePath);
        return true;
    }

    private static bool ContainsUnsafeGpuSettings(IDictionary<string, JsonNode?> values)
    {
        return ReadGpuBool(values, ConfigurationKeys.PerformanceUseGpu)
               || ReadGpuBool(values, ConfigurationKeys.PerformanceAllowDeprecatedGpu)
               || !string.IsNullOrWhiteSpace(ReadGpuString(values, ConfigurationKeys.PerformancePreferredGpuDescription))
               || !string.IsNullOrWhiteSpace(ReadGpuString(values, ConfigurationKeys.PerformancePreferredGpuInstancePath));
    }

    private static bool ReadGpuBool(IDictionary<string, JsonNode?> values, string key)
    {
        if (!values.TryGetValue(key, out var node) || node is null)
        {
            return false;
        }

        if (node is JsonValue value)
        {
            if (value.TryGetValue(out bool parsedBool))
            {
                return parsedBool;
            }

            if (value.TryGetValue(out int parsedInt))
            {
                return parsedInt != 0;
            }

            if (value.TryGetValue(out string? parsedText))
            {
                if (bool.TryParse(parsedText, out var parsed))
                {
                    return parsed;
                }

                if (int.TryParse(parsedText, out parsedInt))
                {
                    return parsedInt != 0;
                }
            }
        }

        var raw = node.ToString();
        if (bool.TryParse(raw, out var fallbackParsed))
        {
            return fallbackParsed;
        }

        return int.TryParse(raw, out var fallbackInt) && fallbackInt != 0;
    }

    private static string ReadGpuString(IDictionary<string, JsonNode?> values, string key)
    {
        if (!values.TryGetValue(key, out var node) || node is null)
        {
            return string.Empty;
        }

        if (node is JsonValue value && value.TryGetValue(out string? parsedText) && parsedText is not null)
        {
            return parsedText.Trim();
        }

        return node.ToString().Trim();
    }

    private void ApplyStartPerformanceSnapshotWithoutDirtyTracking(StartPerformanceSettingsSnapshot snapshot)
    {
        _suppressStartPerformanceDirtyTracking = true;
        _suppressGpuUiRefresh = true;
        try
        {
            RunDirectly = snapshot.RunDirectly;
            MinimizeDirectly = snapshot.MinimizeDirectly;
            OpenEmulatorAfterLaunch = snapshot.OpenEmulatorAfterLaunch;
            EmulatorPath = snapshot.EmulatorPath;
            EmulatorAddCommand = snapshot.EmulatorAddCommand;
            EmulatorWaitSeconds = snapshot.EmulatorWaitSeconds;
            PerformanceUseGpu = snapshot.PerformanceUseGpu;
            PerformanceAllowDeprecatedGpu = snapshot.PerformanceAllowDeprecatedGpu;
            PerformancePreferredGpuDescription = snapshot.PerformancePreferredGpuDescription;
            PerformancePreferredGpuInstancePath = snapshot.PerformancePreferredGpuInstancePath;
            DeploymentWithPause = snapshot.DeploymentWithPause;
            StartsWithScript = snapshot.StartsWithScript;
            EndsWithScript = snapshot.EndsWithScript;
            CopilotWithScript = snapshot.CopilotWithScript;
            ManualStopWithScript = snapshot.ManualStopWithScript;
            BlockSleep = snapshot.BlockSleep;
            BlockSleepWithScreenOn = snapshot.BlockSleepWithScreenOn;
            EnablePenguin = snapshot.EnablePenguin;
            EnableYituliu = snapshot.EnableYituliu;
            PenguinId = snapshot.PenguinId;
            TaskTimeoutMinutes = snapshot.TaskTimeoutMinutes;
            ReminderIntervalMinutes = snapshot.ReminderIntervalMinutes;
        }
        finally
        {
            _suppressGpuUiRefresh = false;
            _suppressStartPerformanceDirtyTracking = false;
        }

        RefreshGpuUiState();
    }

    private TimerSettingsSnapshot BuildTimerSnapshot()
    {
        var slots = Timers
            .OrderBy(static s => s.Index)
            .Select(slot => new TimerSlotSettingsSnapshot(
                Index: slot.Index,
                Enabled: slot.Enabled,
                Time: NormalizeTimerTime(slot.Time),
                Profile: (slot.Profile ?? string.Empty).Trim()))
            .ToArray();

        return new TimerSettingsSnapshot(
            ForceScheduledStart: ForceScheduledStart,
            ShowWindowBeforeForceScheduledStart: ShowWindowBeforeForceScheduledStart,
            CustomTimerConfig: CustomTimerConfig,
            Slots: slots);
    }

    private UiOperationResult ValidateTimerSnapshot(TimerSettingsSnapshot snapshot)
    {
        if (snapshot.Slots.Count != TimerSlotCount)
        {
            return UiOperationResult.Fail(
                UiErrorCode.TimerSlotCountMismatch,
                $"Expected {TimerSlotCount} timer slots but got {snapshot.Slots.Count}.");
        }

        foreach (var slot in snapshot.Slots.OrderBy(static s => s.Index))
        {
            if (slot.Index < 1 || slot.Index > TimerSlotCount)
            {
                return UiOperationResult.Fail(
                    UiErrorCode.TimerSlotIndexOutOfRange,
                    $"Timer slot index must be within 1-{TimerSlotCount}, got {slot.Index}.");
            }

            if (slot.Enabled && !TryParseTimerTime(slot.Time, out _, out _))
            {
                return UiOperationResult.Fail(
                    UiErrorCode.TimerTimeInvalid,
                    $"Timer {slot.Index} time must be HH:mm (00:00-23:59).");
            }

            if (!snapshot.CustomTimerConfig)
            {
                continue;
            }

            if (string.IsNullOrWhiteSpace(slot.Profile))
            {
                return UiOperationResult.Fail(
                    UiErrorCode.TimerProfileMissing,
                    $"Timer {slot.Index} profile cannot be empty when CustomConfig is enabled.");
            }

            if (!Runtime.ConfigurationService.CurrentConfig.Profiles.ContainsKey(slot.Profile))
            {
                return UiOperationResult.Fail(
                    UiErrorCode.TimerProfileNotFound,
                    $"Timer {slot.Index} profile `{slot.Profile}` does not exist.");
            }
        }

        return UiOperationResult.Ok("Timer settings validation passed.");
    }

    private TimerSettingsSnapshot ReadTimerSnapshot(UnifiedConfig config, ICollection<string> warnings)
    {
        var currentProfile = config.CurrentProfile;
        if (string.IsNullOrWhiteSpace(currentProfile) || !config.Profiles.ContainsKey(currentProfile))
        {
            currentProfile = config.Profiles.Keys.FirstOrDefault() ?? "Default";
            warnings.Add($"Timer profile fallback applied to `{currentProfile}`.");
        }

        var slots = new List<TimerSlotSettingsSnapshot>(TimerSlotCount);
        for (var index = 1; index <= TimerSlotCount; index++)
        {
            var enabledKey = BuildTimerEnabledKey(index);
            var hourKey = BuildTimerHourKey(index);
            var minuteKey = BuildTimerMinuteKey(index);
            var profileKey = BuildTimerProfileKey(index);

            var enabled = ReadGlobalBoolFlexible(config, enabledKey, false);

            var rawHour = ReadGlobalIntFlexible(config, hourKey, DefaultTimerHour, out var parsedHour);
            if (!parsedHour && HasConfigKey(config, hourKey, ConfigValuePreference.GlobalFirst))
            {
                warnings.Add($"Timer {index} hour parse failed and fell back to {DefaultTimerHour}.");
            }

            var hour = Math.Clamp(rawHour, TimerHourMin, TimerHourMax);
            if (hour != rawHour)
            {
                warnings.Add($"Timer {index} hour clamped to {hour} from {rawHour}.");
            }

            var rawMinute = ReadGlobalIntFlexible(config, minuteKey, DefaultTimerMinute, out var parsedMinute);
            if (!parsedMinute && HasConfigKey(config, minuteKey, ConfigValuePreference.GlobalFirst))
            {
                warnings.Add($"Timer {index} minute parse failed and fell back to {DefaultTimerMinute}.");
            }

            var minute = Math.Clamp(rawMinute, TimerMinuteMin, TimerMinuteMax);
            if (minute != rawMinute)
            {
                warnings.Add($"Timer {index} minute clamped to {minute} from {rawMinute}.");
            }

            var profile = NormalizeTimerProfile(ReadGlobalString(config, profileKey, currentProfile), currentProfile);
            if (!config.Profiles.ContainsKey(profile))
            {
                warnings.Add($"Timer {index} profile `{profile}` not found and fell back to `{currentProfile}`.");
                profile = currentProfile;
            }

            slots.Add(new TimerSlotSettingsSnapshot(
                Index: index,
                Enabled: enabled,
                Time: FormatTimerTime(hour, minute),
                Profile: profile));
        }

        return new TimerSettingsSnapshot(
            ForceScheduledStart: ReadGlobalBoolFlexible(config, LegacyConfigurationKeys.ForceScheduledStart, false),
            ShowWindowBeforeForceScheduledStart: ReadGlobalBoolFlexible(config, LegacyConfigurationKeys.ShowWindowBeforeForceScheduledStart, false),
            CustomTimerConfig: ReadGlobalBoolFlexible(config, LegacyConfigurationKeys.CustomConfig, false),
            Slots: slots);
    }

    private void ApplyTimerSnapshot(TimerSettingsSnapshot snapshot)
    {
        _suppressTimerDirtyTracking = true;
        try
        {
            ForceScheduledStart = snapshot.ForceScheduledStart;
            ShowWindowBeforeForceScheduledStart = snapshot.ShowWindowBeforeForceScheduledStart;
            CustomTimerConfig = snapshot.CustomTimerConfig;

            var byIndex = snapshot.Slots.ToDictionary(static s => s.Index);
            foreach (var slot in Timers)
            {
                if (!byIndex.TryGetValue(slot.Index, out var source))
                {
                    continue;
                }

                slot.Enabled = source.Enabled;
                slot.Time = source.Time;
                slot.Profile = source.Profile;
            }
        }
        finally
        {
            _suppressTimerDirtyTracking = false;
        }
    }

    private static string BuildTimerEnabledKey(int index) => $"Timer.Timer{index}";

    private static string BuildTimerHourKey(int index) => $"Timer.Timer{index}Hour";

    private static string BuildTimerMinuteKey(int index) => $"Timer.Timer{index}Min";

    private static string BuildTimerProfileKey(int index) => $"Timer.Timer{index}.Config";

    private static string NormalizeTimerTime(string? value)
    {
        var trimmed = value?.Trim() ?? string.Empty;
        return TryParseTimerTime(trimmed, out var hour, out var minute)
            ? FormatTimerTime(hour, minute)
            : trimmed;
    }

    private static string NormalizeTimerProfile(string? value, string fallback)
    {
        var normalized = value?.Trim() ?? string.Empty;
        return string.IsNullOrWhiteSpace(normalized)
            ? fallback
            : normalized;
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

        if (hour < TimerHourMin || hour > TimerHourMax)
        {
            return false;
        }

        if (minute < TimerMinuteMin || minute > TimerMinuteMax)
        {
            return false;
        }

        return true;
    }

    private static string FormatTimerTime(int hour, int minute)
    {
        return FormattableString.Invariant($"{hour:00}:{minute:00}");
    }

    private static bool ReadProfileBoolFlexible(UnifiedConfig config, string key, bool fallback)
        => ReadBoolFlexible(config, key, fallback, ConfigValuePreference.ProfileFirst);

    private static bool ReadGlobalBoolFlexible(UnifiedConfig config, string key, bool fallback)
        => ReadBoolFlexible(config, key, fallback, ConfigValuePreference.GlobalFirst);

    private static bool ReadBoolFlexible(
        UnifiedConfig config,
        string key,
        bool fallback,
        ConfigValuePreference preference)
    {
        if (!TryGetConfigNode(config, key, preference, out var node) || node is null)
        {
            return fallback;
        }

        if (node is JsonValue value)
        {
            if (value.TryGetValue(out bool parsedBool))
            {
                return parsedBool;
            }

            if (value.TryGetValue(out int parsedInt))
            {
                return parsedInt != 0;
            }

            if (value.TryGetValue(out string? text))
            {
                if (bool.TryParse(text, out var parsedText))
                {
                    return parsedText;
                }

                if (int.TryParse(text, out var parsedIntText))
                {
                    return parsedIntText != 0;
                }
            }
        }

        if (bool.TryParse(node.ToString(), out var parsed))
        {
            return parsed;
        }

        return fallback;
    }

    private static int ReadProfileIntFlexible(UnifiedConfig config, string key, int fallback, out bool parsed)
        => ReadIntFlexible(config, key, fallback, ConfigValuePreference.ProfileFirst, out parsed);

    private static int ReadGlobalIntFlexible(UnifiedConfig config, string key, int fallback, out bool parsed)
        => ReadIntFlexible(config, key, fallback, ConfigValuePreference.GlobalFirst, out parsed);

    private static int ReadIntFlexible(
        UnifiedConfig config,
        string key,
        int fallback,
        ConfigValuePreference preference,
        out bool parsed)
    {
        parsed = false;
        if (!TryGetConfigNode(config, key, preference, out var node) || node is null)
        {
            return fallback;
        }

        if (node is JsonValue value)
        {
            if (value.TryGetValue(out int parsedInt))
            {
                parsed = true;
                return parsedInt;
            }

            if (value.TryGetValue(out bool parsedBool))
            {
                parsed = true;
                return parsedBool ? 1 : 0;
            }

            if (value.TryGetValue(out string? text)
                && int.TryParse(text, NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedTextInt))
            {
                parsed = true;
                return parsedTextInt;
            }
        }

        if (int.TryParse(node.ToString(), NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedNode))
        {
            parsed = true;
            return parsedNode;
        }

        return fallback;
    }

    private static bool HasConfigKey(UnifiedConfig config, string key, ConfigValuePreference preference)
    {
        if (string.IsNullOrWhiteSpace(key))
        {
            return false;
        }

        return preference == ConfigValuePreference.ProfileFirst
            ? HasProfileValue(config, key) || config.GlobalValues.ContainsKey(key)
            : config.GlobalValues.ContainsKey(key) || HasProfileValue(config, key);
    }

    private static bool TryGetConfigNode(
        UnifiedConfig config,
        string key,
        ConfigValuePreference preference,
        out JsonNode? node)
    {
        if (preference == ConfigValuePreference.ProfileFirst)
        {
            if (TryGetProfileValue(config, key, out node))
            {
                return true;
            }

            if (config.GlobalValues.TryGetValue(key, out node) && node is not null)
            {
                return true;
            }
        }
        else
        {
            if (config.GlobalValues.TryGetValue(key, out node) && node is not null)
            {
                return true;
            }

            if (TryGetProfileValue(config, key, out node))
            {
                return true;
            }
        }

        node = null;
        return false;
    }

    private static bool HasProfileValue(UnifiedConfig config, string key)
    {
        return !string.IsNullOrWhiteSpace(config.CurrentProfile)
               && config.Profiles.TryGetValue(config.CurrentProfile, out var profile)
               && profile.Values.ContainsKey(key);
    }

    private static bool TryGetProfileValue(UnifiedConfig config, string key, out JsonNode? node)
    {
        if (!string.IsNullOrWhiteSpace(config.CurrentProfile)
            && config.Profiles.TryGetValue(config.CurrentProfile, out var profile)
            && profile.Values.TryGetValue(key, out node)
            && node is not null)
        {
            return true;
        }

        node = null;
        return false;
    }
}

internal enum ConfigValuePreference
{
    GlobalFirst = 0,
    ProfileFirst = 1,
}

public sealed record GuiSettingsSnapshot(
    string Theme,
    string Language,
    bool UseTray,
    bool MinimizeToTray,
    bool WindowTitleScrollable,
    string LogItemDateFormatString,
    string OperNameLanguage,
    string InverseClearMode,
    string BackgroundImagePath,
    int BackgroundOpacity,
    int BackgroundBlur,
    string BackgroundStretchMode,
    bool DeveloperModeEnabled = false)
{
    public IReadOnlyDictionary<string, string> ToGlobalSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            ["Theme.Mode"] = Theme,
            [ConfigurationKeys.Localization] = Language,
            [ConfigurationKeys.UseTray] = UseTray.ToString(),
            [ConfigurationKeys.MinimizeToTray] = MinimizeToTray.ToString(),
            [ConfigurationKeys.WindowTitleScrollable] = WindowTitleScrollable.ToString(),
            ["GUI.DeveloperMode"] = DeveloperModeEnabled.ToString(),
            [ConfigurationKeys.LogItemDateFormat] = LogItemDateFormatString,
            [ConfigurationKeys.OperNameLanguage] = OperNameLanguage,
            [ConfigurationKeys.BackgroundImagePath] = BackgroundImagePath,
            [ConfigurationKeys.BackgroundOpacity] = BackgroundOpacity.ToString(),
            [ConfigurationKeys.BackgroundBlurEffectRadius] = BackgroundBlur.ToString(),
            [ConfigurationKeys.BackgroundImageStretchMode] = BackgroundStretchMode,
        };
    }

    public IReadOnlyDictionary<string, string> ToProfileSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.InverseClearMode] = InverseClearMode,
        };
    }
}

public sealed record StartPerformanceSettingsSnapshot(
    bool RunDirectly,
    bool MinimizeDirectly,
    bool OpenEmulatorAfterLaunch,
    string EmulatorPath,
    string EmulatorAddCommand,
    int EmulatorWaitSeconds,
    bool PerformanceUseGpu,
    bool PerformanceAllowDeprecatedGpu,
    string PerformancePreferredGpuDescription,
    string PerformancePreferredGpuInstancePath,
    bool DeploymentWithPause,
    string StartsWithScript,
    string EndsWithScript,
    bool CopilotWithScript,
    bool ManualStopWithScript,
    bool BlockSleep,
    bool BlockSleepWithScreenOn,
    bool EnablePenguin,
    bool EnableYituliu,
    string PenguinId,
    int TaskTimeoutMinutes,
    int ReminderIntervalMinutes)
{
    public IReadOnlyDictionary<string, string> ToGlobalSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.MinimizeDirectly] = MinimizeDirectly.ToString(),
        };
    }

    public IReadOnlyDictionary<string, string> ToProfileSettingUpdates(bool includeGpuSettings = true)
    {
        var updates = new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.RunDirectly] = RunDirectly.ToString(),
            [ConfigurationKeys.StartEmulator] = OpenEmulatorAfterLaunch.ToString(),
            [ConfigurationKeys.EmulatorPath] = EmulatorPath,
            [ConfigurationKeys.EmulatorAddCommand] = EmulatorAddCommand,
            [ConfigurationKeys.EmulatorWaitSeconds] = EmulatorWaitSeconds.ToString(),
            [ConfigurationKeys.RoguelikeDeploymentWithPause] = DeploymentWithPause.ToString(),
            [ConfigurationKeys.StartsWithScript] = StartsWithScript,
            [ConfigurationKeys.EndsWithScript] = EndsWithScript,
            [ConfigurationKeys.CopilotWithScript] = CopilotWithScript.ToString(),
            [ConfigurationKeys.ManualStopWithScript] = ManualStopWithScript.ToString(),
            [ConfigurationKeys.BlockSleep] = BlockSleep.ToString(),
            [ConfigurationKeys.BlockSleepWithScreenOn] = BlockSleepWithScreenOn.ToString(),
            [ConfigurationKeys.EnablePenguin] = EnablePenguin.ToString(),
            [ConfigurationKeys.EnableYituliu] = EnableYituliu.ToString(),
            [ConfigurationKeys.PenguinId] = PenguinId,
            [ConfigurationKeys.TaskTimeoutMinutes] = TaskTimeoutMinutes.ToString(),
            [ConfigurationKeys.ReminderIntervalMinutes] = ReminderIntervalMinutes.ToString(),
        };

        if (includeGpuSettings)
        {
            updates[ConfigurationKeys.PerformanceUseGpu] = PerformanceUseGpu.ToString();
            updates[ConfigurationKeys.PerformanceAllowDeprecatedGpu] = PerformanceAllowDeprecatedGpu.ToString();
            updates[ConfigurationKeys.PerformancePreferredGpuDescription] = PerformancePreferredGpuDescription;
            updates[ConfigurationKeys.PerformancePreferredGpuInstancePath] = PerformancePreferredGpuInstancePath;
        }

        return updates;
    }
}

public sealed record GpuOptionDisplayItem(
    GpuOptionDescriptor Descriptor,
    string Display);

public sealed record DisplayValueOption(string Display, string Value);

public sealed record TimerSlotSettingsSnapshot(
    int Index,
    bool Enabled,
    string Time,
    string Profile);

public sealed record TimerSettingsSnapshot(
    bool ForceScheduledStart,
    bool ShowWindowBeforeForceScheduledStart,
    bool CustomTimerConfig,
    IReadOnlyList<TimerSlotSettingsSnapshot> Slots)
{
    public IReadOnlyDictionary<string, string> ToGlobalSettingUpdates()
    {
        var updates = new Dictionary<string, string>(StringComparer.Ordinal);
        updates[LegacyConfigurationKeys.ForceScheduledStart] = ForceScheduledStart.ToString();
        updates[LegacyConfigurationKeys.ShowWindowBeforeForceScheduledStart] = ShowWindowBeforeForceScheduledStart.ToString();
        updates[LegacyConfigurationKeys.CustomConfig] = CustomTimerConfig.ToString();

        foreach (var slot in Slots.OrderBy(static s => s.Index))
        {
            var index = Math.Clamp(slot.Index, 1, 8);
            updates[$"Timer.Timer{index}"] = slot.Enabled.ToString();

            var hour = 7;
            var minute = 0;
            if (!string.IsNullOrWhiteSpace(slot.Time))
            {
                var split = slot.Time.Split(':');
                if (split.Length == 2)
                {
                    if (int.TryParse(split[0], NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedHour))
                    {
                        hour = parsedHour;
                    }

                    if (int.TryParse(split[1], NumberStyles.Integer, CultureInfo.InvariantCulture, out var parsedMinute))
                    {
                        minute = parsedMinute;
                    }
                }
            }

            hour = Math.Clamp(hour, 0, 23);
            minute = Math.Clamp(minute, 0, 59);

            updates[$"Timer.Timer{index}Hour"] = hour.ToString(CultureInfo.InvariantCulture);
            updates[$"Timer.Timer{index}Min"] = minute.ToString(CultureInfo.InvariantCulture);
            updates[$"Timer.Timer{index}.Config"] = slot.Profile ?? string.Empty;
        }

        return updates;
    }
}

public sealed class GuiSettingsAppliedEventArgs : EventArgs
{
    public GuiSettingsAppliedEventArgs(GuiSettingsSnapshot snapshot)
    {
        Snapshot = snapshot;
    }

    public GuiSettingsSnapshot Snapshot { get; }
}

public sealed class ConfigurationContextChangedEventArgs : EventArgs
{
    public ConfigurationContextChangedEventArgs(
        ConfigurationContextChangeReason reason,
        string message,
        ImportReport? report)
    {
        Reason = reason;
        Message = message;
        Report = report;
    }

    public ConfigurationContextChangeReason Reason { get; }

    public string Message { get; }

    public ImportReport? Report { get; }
}

public enum ConfigurationContextChangeReason
{
    ProfileSwitched = 0,
    LegacyImport = 1,
    UnifiedImport = 2,
}

public enum HotkeyRegistrationSource
{
    Manual = 0,
    Startup = 1,
}

public sealed class TimerSlotViewModel : ObservableObject
{
    private const int DefaultHour = 7;
    private const int DefaultMinute = 0;
    private const int HourMin = 0;
    private const int HourMax = 23;
    private const int MinuteMin = 0;
    private const int MinuteMax = 59;

    private bool _enabled;
    private string _time = "07:00";
    private string _profile = "Default";

    public TimerSlotViewModel(int index)
    {
        Index = index;
    }

    public int Index { get; }

    public bool Enabled
    {
        get => _enabled;
        set => SetProperty(ref _enabled, value);
    }

    public string Time
    {
        get => _time;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetProperty(ref _time, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(Hour));
            OnPropertyChanged(nameof(Minute));
        }
    }

    public int Hour
    {
        get => TryParseTime(Time, out var hour, out _) ? hour : DefaultHour;
        set => UpdateTime(value, Minute);
    }

    public int Minute
    {
        get => TryParseTime(Time, out _, out var minute) ? minute : DefaultMinute;
        set => UpdateTime(Hour, value);
    }

    public string Profile
    {
        get => _profile;
        set => SetProperty(ref _profile, value?.Trim() ?? string.Empty);
    }

    private void UpdateTime(int hour, int minute)
    {
        var normalizedHour = Math.Clamp(hour, HourMin, HourMax);
        var normalizedMinute = Math.Clamp(minute, MinuteMin, MinuteMax);
        Time = FormattableString.Invariant($"{normalizedHour:00}:{normalizedMinute:00}");
    }

    private static bool TryParseTime(string? value, out int hour, out int minute)
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

        if (hour < HourMin || hour > HourMax)
        {
            return false;
        }

        if (minute < MinuteMin || minute > MinuteMax)
        {
            return false;
        }

        return true;
    }
}
