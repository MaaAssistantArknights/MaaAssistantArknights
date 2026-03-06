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
    private const string DefaultBackgroundStretchMode = "UniformToFill";
    private const string ShowGuiHotkeyName = "ShowGui";
    private const string LinkStartHotkeyName = "LinkStart";
    private const string DefaultHotkeyShowGui = "Ctrl+Shift+Alt+M";
    private const string DefaultHotkeyLinkStart = "Ctrl+Shift+Alt+L";
    private const int EmulatorWaitSecondsMin = 0;
    private const int EmulatorWaitSecondsMax = 600;
    private const int DefaultEmulatorWaitSeconds = 60;
    private const int BackgroundOpacityMin = 0;
    private const int BackgroundOpacityMax = 100;
    private const int BackgroundBlurMin = 0;
    private const int BackgroundBlurMax = 80;
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

    private SettingsSectionViewModel? _selectedSection;
    private readonly SemaphoreSlim _guiSaveSemaphore = new(1, 1);
    private readonly Action<LocalizationFallbackInfo>? _localizationFallbackReporter;
    private bool _suppressGuiAutoSave;
    private bool _suppressStartPerformanceDirtyTracking;
    private string _theme = DefaultTheme;
    private string _language = DefaultLanguage;
    private bool _useTray = true;
    private bool _minimizeToTray;
    private bool _windowTitleScrollable = true;
    private bool _startSelf;
    private string _autostartStatus = string.Empty;
    private string _hotkeyShowGui = DefaultHotkeyShowGui;
    private string _hotkeyLinkStart = DefaultHotkeyLinkStart;
    private string _persistedHotkeyShowGui = DefaultHotkeyShowGui;
    private string _persistedHotkeyLinkStart = DefaultHotkeyLinkStart;
    private string _hotkeyStatusMessage = string.Empty;
    private string _hotkeyWarningMessage = string.Empty;
    private string _hotkeyErrorMessage = string.Empty;
    private string _notificationTitle = "MAA Test";
    private string _notificationMessage = "Cross-platform notification test";
    private string _issueReportPath = string.Empty;
    private string _issueReportStatusMessage = string.Empty;
    private string _issueReportErrorMessage = string.Empty;
    private string _remoteGetTaskEndpoint = string.Empty;
    private string _remoteReportEndpoint = string.Empty;
    private string _remoteUserIdentity = string.Empty;
    private string _remoteDeviceIdentity = string.Empty;
    private int _remotePollInterval = 5000;
    private string _remoteControlStatusMessage = string.Empty;
    private string _remoteControlWarningMessage = string.Empty;
    private string _remoteControlErrorMessage = string.Empty;
    private string _backgroundImagePath = string.Empty;
    private int _backgroundOpacity = 45;
    private int _backgroundBlur = 12;
    private string _backgroundStretchMode = DefaultBackgroundStretchMode;
    private bool _hasPendingGuiChanges;
    private string _guiValidationMessage = string.Empty;
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
    private bool _hasPendingStartPerformanceChanges;
    private string _startPerformanceValidationMessage = string.Empty;
    private DateTimeOffset? _lastSuccessfulStartPerformanceSaveAt;
    private bool _forceScheduledStart;
    private bool _showWindowBeforeForceScheduledStart = true;
    private bool _customTimerConfig;
    private bool _hasPendingTimerChanges;
    private string _timerValidationMessage = string.Empty;
    private bool _suppressTimerDirtyTracking;
    private bool _externalNotificationEnabled;
    private bool _externalNotificationSendWhenComplete;
    private bool _externalNotificationSendWhenError;
    private bool _externalNotificationSendWhenTimeout;
    private bool _externalNotificationEnableDetails;
    private string _externalNotificationStatusMessage = string.Empty;
    private string _externalNotificationWarningMessage = string.Empty;
    private string _externalNotificationErrorMessage = string.Empty;
    private string _selectedNotificationProvider = "Smtp";
    private string _notificationProviderParametersText = string.Empty;
    private string _versionUpdateProxy = string.Empty;
    private string _versionUpdateProxyType = "system";
    private string _versionUpdateVersionType = "Stable";
    private string _versionUpdateResourceSource = "Official";
    private bool _versionUpdateForceGithubSource;
    private string _versionUpdateMirrorChyanCdk = string.Empty;
    private string _versionUpdateMirrorChyanCdkExpired = string.Empty;
    private bool _versionUpdateStartupCheck = true;
    private bool _versionUpdateScheduledCheck;
    private string _versionUpdateResourceApi = string.Empty;
    private bool _versionUpdateAllowNightly;
    private bool _versionUpdateAcknowledgedNightlyWarning;
    private bool _versionUpdateUseAria2;
    private bool _versionUpdateAutoDownload;
    private bool _versionUpdateAutoInstall;
    private string _versionUpdateName = string.Empty;
    private string _versionUpdateBody = string.Empty;
    private bool _versionUpdateIsFirstBoot;
    private string _versionUpdatePackage = string.Empty;
    private bool _versionUpdateDoNotShow;
    private string _versionUpdateStatusMessage = string.Empty;
    private string _versionUpdateErrorMessage = string.Empty;
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
        ConnectionGameSharedState = connectionGameSharedState;
        _aboutVersionInfo = BuildAboutVersionInfo();
        UpdateAchievementPolicySummary(AchievementPolicy.Default);
        Sections = new ObservableCollection<SettingsSectionViewModel>
        {
            new("ConfigurationManager", "Configuration Manager"),
            new("Timer", "Timer"),
            new("Performance", "Performance"),
            new("Game", "Game"),
            new("Connect", "Connect"),
            new("Start", "Start"),
            new("RemoteControl", "Remote Control"),
            new("GUI", "GUI"),
            new("Background", "Background"),
            new("ExternalNotification", "External Notification"),
            new("HotKey", "HotKey"),
            new("Achievement", "Achievement"),
            new("VersionUpdate", "Version Update"),
            new("IssueReport", "Issue Report"),
            new("About", "About"),
        };

        Timers = new ObservableCollection<TimerSlotViewModel>(
            Enumerable.Range(1, TimerSlotCount).Select(i => new TimerSlotViewModel(i)));
        foreach (var slot in Timers)
        {
            slot.PropertyChanged += OnTimerSlotPropertyChanged;
        }

        SelectedSection = Sections[0];
    }

    public event EventHandler<GuiSettingsAppliedEventArgs>? GuiSettingsApplied;

    public ObservableCollection<SettingsSectionViewModel> Sections { get; }

    public ObservableCollection<TimerSlotViewModel> Timers { get; }

    public ObservableCollection<string> ConfigurationProfiles { get; } = new();

    public ConnectionGameSharedStateViewModel ConnectionGameSharedState { get; }

    public IReadOnlyList<string> ThemeOptions { get; } = [DefaultTheme, "Dark"];

    public IReadOnlyList<string> SupportedLanguages { get; } = UiLanguageCatalog.Ordered;

    public IReadOnlyList<string> BackgroundStretchModes { get; } = [DefaultBackgroundStretchMode, "Uniform", "Fill", "None"];

    public IReadOnlyList<string> VersionUpdateVersionTypeOptions { get; } = ["Stable", "Beta", "Nightly"];

    public IReadOnlyList<string> VersionUpdateProxyTypeOptions { get; } = ["system", "http", "https"];

    public ObservableCollection<string> AvailableNotificationProviders { get; } = new();

    public SettingsSectionViewModel? SelectedSection
    {
        get => _selectedSection;
        set => SetProperty(ref _selectedSection, value);
    }

    public string Theme
    {
        get => _theme;
        set
        {
            var normalized = NormalizeTheme(value);
            if (SetProperty(ref _theme, normalized))
            {
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
                MarkGuiSettingsDirty();
            }
        }
    }

    public bool MinimizeToTray
    {
        get => _minimizeToTray;
        set
        {
            if (SetProperty(ref _minimizeToTray, value))
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
        set => SetProperty(ref _externalNotificationEnabled, value);
    }

    public bool ExternalNotificationSendWhenComplete
    {
        get => _externalNotificationSendWhenComplete;
        set => SetProperty(ref _externalNotificationSendWhenComplete, value);
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
        set => SetProperty(ref _externalNotificationEnableDetails, value);
    }

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
        private set => SetProperty(ref _externalNotificationStatusMessage, value);
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

    public bool HasExternalNotificationWarningMessage => !string.IsNullOrWhiteSpace(ExternalNotificationWarningMessage);

    public bool HasExternalNotificationErrorMessage => !string.IsNullOrWhiteSpace(ExternalNotificationErrorMessage);

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
        set => SetProperty(ref _versionUpdateProxyType, value?.Trim() ?? "system");
    }

    public string VersionUpdateVersionType
    {
        get => _versionUpdateVersionType;
        set => SetProperty(ref _versionUpdateVersionType, value?.Trim() ?? "Stable");
    }

    public string VersionUpdateResourceSource
    {
        get => _versionUpdateResourceSource;
        set => SetProperty(ref _versionUpdateResourceSource, value?.Trim() ?? "Official");
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
        set => SetProperty(ref _versionUpdateMirrorChyanCdkExpired, value?.Trim() ?? string.Empty);
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
        set => SetProperty(ref _versionUpdateAllowNightly, value);
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
        set => SetProperty(ref _versionUpdateName, value ?? string.Empty);
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

    public bool AchievementPopupDisabled
    {
        get => _achievementPopupDisabled;
        set => SetProperty(ref _achievementPopupDisabled, value);
    }

    public bool AchievementPopupAutoClose
    {
        get => _achievementPopupAutoClose;
        set => SetProperty(ref _achievementPopupAutoClose, value);
    }

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
                MarkGuiSettingsDirty();
            }
        }
    }

    public bool HasPendingGuiChanges
    {
        get => _hasPendingGuiChanges;
        private set => SetProperty(ref _hasPendingGuiChanges, value);
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

    public DateTimeOffset? LastSuccessfulGuiSaveAt
    {
        get => _lastSuccessfulGuiSaveAt;
        private set => SetProperty(ref _lastSuccessfulGuiSaveAt, value);
    }

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
                MarkStartPerformanceDirty();
            }
        }
    }

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
                MarkStartPerformanceDirty();
            }
        }
    }

    public bool HasPendingStartPerformanceChanges
    {
        get => _hasPendingStartPerformanceChanges;
        private set => SetProperty(ref _hasPendingStartPerformanceChanges, value);
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
        private set => SetProperty(ref _lastSuccessfulStartPerformanceSaveAt, value);
    }

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
                MarkTimerDirty();
            }
        }
    }

    public bool HasPendingTimerChanges
    {
        get => _hasPendingTimerChanges;
        private set => SetProperty(ref _hasPendingTimerChanges, value);
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

    public GuiSettingsSnapshot CurrentGuiSnapshot => BuildNormalizedGuiSnapshot();

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        await LoadFromConfigAsync(Runtime.ConfigurationService.CurrentConfig, cancellationToken);
        await RefreshConfigurationProfilesAsync(cancellationToken);
        LoadConnectionSharedStateFromConfig();
        await RefreshAutostartStatusAsync(cancellationToken);
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

        var result = await Runtime.SettingsFeatureService.SaveGlobalSettingsAsync(updates, cancellationToken);
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

        foreach (var provider in AvailableNotificationProviders)
        {
            var parameterText = _notificationProviderParameters.TryGetValue(provider, out var stored)
                ? stored
                : string.Empty;
            var shouldValidate = !string.IsNullOrWhiteSpace(parameterText);
            if (shouldValidate)
            {
                var validate = await Runtime.NotificationProviderFeatureService.ValidateProviderParametersAsync(
                    new NotificationProviderRequest(provider, parameterText),
                    cancellationToken);
                if (!validate.Success)
                {
                    await ApplyExternalNotificationFailure(validate, "Settings.ExternalNotification.Save.Validate", cancellationToken);
                    return;
                }
            }

            if (!TryParseProviderParameterText(parameterText, out var parsed, out var parseError))
            {
                var fail = UiOperationResult.Fail(
                    UiErrorCode.NotificationProviderInvalidParameters,
                    parseError ?? "Provider parameter parsing failed.");
                await ApplyExternalNotificationFailure(fail, "Settings.ExternalNotification.Save.Parse", cancellationToken);
                return;
            }

            if (!ProviderConfigKeyMap.TryGetValue(provider, out var keyMap))
            {
                continue;
            }

            foreach (var (parameterKey, configKey) in keyMap)
            {
                updates[configKey] = parsed.TryGetValue(parameterKey, out var value)
                    ? value
                    : string.Empty;
            }
        }

        var saveResult = await Runtime.SettingsFeatureService.SaveGlobalSettingsAsync(updates, cancellationToken);
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
        var result = await Runtime.ConfigurationProfileFeatureService.DeleteProfileAsync(target, cancellationToken);
        await HandleConfigurationProfileResultAsync(
            result,
            "Settings.ConfigurationManager.Delete",
            successMessage: $"配置 `{target}` 已删除。",
            failureMessage: "配置删除失败。",
            cancellationToken);
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
        ClearConfigurationManagerStatus();
        var target = ConfigurationManagerSelectedProfile;
        var result = await Runtime.ConfigurationProfileFeatureService.SwitchProfileAsync(target, cancellationToken);
        await HandleConfigurationProfileResultAsync(
            result,
            "Settings.ConfigurationManager.Switch",
            successMessage: $"已切换至配置 `{target}`。",
            failureMessage: "配置切换失败。",
            cancellationToken);
    }

    public async Task SaveVersionUpdateSettingsAsync(CancellationToken cancellationToken = default)
    {
        await SaveVersionUpdateChannelAsync(cancellationToken);
        if (HasVersionUpdateErrorMessage)
        {
            return;
        }

        await SaveVersionUpdateProxyAsync(cancellationToken);
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
        VersionUpdateStatusMessage = string.Empty;
        VersionUpdateErrorMessage = string.Empty;

        var policy = BuildVersionUpdatePolicy();
        var checkResult = await Runtime.VersionUpdateFeatureService.CheckForUpdatesAsync(policy, cancellationToken);
        var payload = await ApplyResultAsync(checkResult, "Settings.VersionUpdate.Check", cancellationToken);
        if (payload is null)
        {
            VersionUpdateErrorMessage = checkResult.Message;
            VersionUpdateStatusMessage = "检查更新失败。";
            return;
        }

        VersionUpdateStatusMessage = payload;
        VersionUpdateErrorMessage = string.Empty;
    }

    public async Task CheckVersionUpdateWithDialogAsync(CancellationToken cancellationToken = default)
    {
        VersionUpdateStatusMessage = string.Empty;
        VersionUpdateErrorMessage = string.Empty;

        var policy = BuildVersionUpdatePolicy();
        var checkResult = await Runtime.VersionUpdateFeatureService.CheckForUpdatesAsync(policy, cancellationToken);
        var payload = await ApplyResultAsync(checkResult, "Settings.VersionUpdate.Check", cancellationToken);
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

        if (readBackWarnings.Count > 0)
        {
            await RecordEventAsync(
                "Settings.Timer.Normalize",
                string.Join(" | ", readBackWarnings),
                cancellationToken);
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

        var saveResult = await Runtime.SettingsFeatureService.SaveGlobalSettingsAsync(
            snapshot.ToGlobalSettingUpdates(),
            cancellationToken);
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
        var setResult = await ApplyResultAsync(
            await Runtime.SettingsFeatureService.SetAutostartAsync(StartSelf, cancellationToken),
            "Settings.Autostart.Set",
            cancellationToken);

        if (!setResult)
        {
            return;
        }

        await RefreshAutostartStatusAsync(cancellationToken);
    }

    public async Task RefreshAutostartStatusAsync(CancellationToken cancellationToken = default)
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
            return;
        }

        var enabled = result.Value;
        StartSelf = enabled;
        AutostartStatus = PlatformCapabilityTextMap.FormatAutostartStatus(
            Language,
            enabled,
            _localizationFallbackReporter);
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
                GuiValidationMessage = validation.Message;
                LastErrorMessage = validation.Message;
                await RecordFailedResultAsync("Settings.Save.GuiBatch.Validation", validation, cancellationToken);
                return;
            }

            var saveResult = await Runtime.SettingsFeatureService.SaveGlobalSettingsAsync(snapshot.ToGlobalSettingUpdates(), cancellationToken);
            if (!await ApplyResultAsync(saveResult, "Settings.Save.GuiBatch", cancellationToken))
            {
                HasPendingGuiChanges = true;
                GuiValidationMessage = saveResult.Message;
                return;
            }

            HasPendingGuiChanges = false;
            GuiValidationMessage = string.Empty;
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
            GuiValidationMessage = $"GUI settings save failed: {ex.Message}";
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
        if (_suppressGuiAutoSave || !saveImmediately)
        {
            return;
        }

        ScheduleGuiAutoSave();
    }

    private void ScheduleGuiAutoSave()
    {
        _ = SaveGuiSettingsCoreAsync(triggeredByAutoSave: true);
    }

    private void MarkStartPerformanceDirty()
    {
        if (_suppressStartPerformanceDirtyTracking)
        {
            return;
        }

        HasPendingStartPerformanceChanges = true;
        StartPerformanceValidationMessage = string.Empty;
    }

    private void MarkTimerDirty()
    {
        if (_suppressTimerDirtyTracking)
        {
            return;
        }

        HasPendingTimerChanges = true;
        TimerValidationMessage = string.Empty;
    }

    private void OnTimerSlotPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (_suppressTimerDirtyTracking || string.IsNullOrEmpty(e.PropertyName))
        {
            return;
        }

        if (e.PropertyName == nameof(TimerSlotViewModel.Enabled)
            || e.PropertyName == nameof(TimerSlotViewModel.Time)
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

    private async Task HandleConfigurationProfileResultAsync(
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
            return;
        }

        ApplyConfigurationProfileState(payload);
        LoadConnectionSharedStateFromConfig();
        ConfigurationManagerStatusMessage = successMessage;
        ConfigurationManagerErrorMessage = string.Empty;
    }

    private void ApplyConfigurationProfileState(ConfigurationProfileState state)
    {
        ConfigurationProfiles.Clear();
        foreach (var profile in state.OrderedProfiles)
        {
            ConfigurationProfiles.Add(profile);
        }

        if (ConfigurationProfiles.Count == 0)
        {
            ConfigurationManagerSelectedProfile = string.Empty;
            return;
        }

        var selected = state.CurrentProfile;
        if (string.IsNullOrWhiteSpace(selected) || !ConfigurationProfiles.Contains(selected, StringComparer.OrdinalIgnoreCase))
        {
            selected = ConfigurationProfiles[0];
        }

        ConfigurationManagerSelectedProfile = selected;
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
            if (!config.GlobalValues.TryGetValue(configKey, out var node) || node is null)
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
            MinimizeToTray: MinimizeToTray,
            WindowTitleScrollable: WindowTitleScrollable,
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
        var globalValues = Runtime.ConfigurationService.CurrentConfig.GlobalValues;
        AddCandidate(ReadConfigValue(globalValues, LegacyConfigurationKeys.EmulatorPath));
        AddCandidate(ReadConfigValue(globalValues, LegacyConfigurationKeys.MuMu12EmulatorPath));
        AddCandidate(ReadConfigValue(globalValues, LegacyConfigurationKeys.LdPlayerEmulatorPath));
        return candidates;
    }

    private static string ReadConfigValue(IReadOnlyDictionary<string, JsonNode?> values, string key)
    {
        if (!values.TryGetValue(key, out var node) || node is null)
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
        await EnsureNotificationProvidersLoadedAsync(cancellationToken);
        var warnings = new List<string>();
        var hotkeyWarnings = new List<string>();

        var rawTheme = ReadString(config, ThemeModeKey, DefaultTheme);
        var rawLanguage = ReadString(config, ConfigurationKeys.Localization, DefaultLanguage);
        var rawBackgroundPath = ReadString(config, ConfigurationKeys.BackgroundImagePath, string.Empty);
        var rawOpacity = ReadInt(config, ConfigurationKeys.BackgroundOpacity, _backgroundOpacity);
        var rawBlur = ReadInt(config, ConfigurationKeys.BackgroundBlurEffectRadius, _backgroundBlur);
        var rawStretchMode = ReadString(config, ConfigurationKeys.BackgroundImageStretchMode, DefaultBackgroundStretchMode);
        var rawHotkeys = ReadString(config, ConfigurationKeys.HotKeys, string.Empty);
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
            warnings.Add($"Theme normalized to `{theme}` from `{rawTheme}`.");
        }

        var language = NormalizeLanguage(rawLanguage);
        if (!string.Equals(rawLanguage, language, StringComparison.OrdinalIgnoreCase))
        {
            warnings.Add($"Language normalized to `{language}` from `{rawLanguage}`.");
        }

        var backgroundPath = NormalizeBackgroundPath(rawBackgroundPath);
        if (!string.IsNullOrWhiteSpace(backgroundPath) && !File.Exists(backgroundPath))
        {
            warnings.Add($"Background path not found and reset: {backgroundPath}");
            backgroundPath = string.Empty;
        }

        var opacity = Math.Clamp(rawOpacity, BackgroundOpacityMin, BackgroundOpacityMax);
        if (opacity != rawOpacity)
        {
            warnings.Add($"Background opacity clamped to {opacity} from {rawOpacity}.");
        }

        var blur = Math.Clamp(rawBlur, BackgroundBlurMin, BackgroundBlurMax);
        if (blur != rawBlur)
        {
            warnings.Add($"Background blur clamped to {blur} from {rawBlur}.");
        }

        var stretch = NormalizeBackgroundStretchMode(rawStretchMode);
        if (!string.Equals(rawStretchMode, stretch, StringComparison.OrdinalIgnoreCase))
        {
            warnings.Add($"Background stretch mode normalized to `{stretch}` from `{rawStretchMode}`.");
        }

        _suppressGuiAutoSave = true;
        try
        {
            Theme = theme;
            Language = language;
            UseTray = ReadBool(config, ConfigurationKeys.UseTray, UseTray);
            MinimizeToTray = ReadBool(config, ConfigurationKeys.MinimizeToTray, MinimizeToTray);
            WindowTitleScrollable = ReadBool(config, ConfigurationKeys.WindowTitleScrollable, WindowTitleScrollable);
            BackgroundImagePath = backgroundPath;
            BackgroundOpacity = opacity;
            BackgroundBlur = blur;
            BackgroundStretchMode = stretch;
            RemoteGetTaskEndpoint = ReadString(config, ConfigurationKeys.RemoteControlGetTaskEndpointUri, string.Empty);
            RemoteReportEndpoint = ReadString(config, ConfigurationKeys.RemoteControlReportStatusUri, string.Empty);
            RemoteUserIdentity = ReadString(config, ConfigurationKeys.RemoteControlUserIdentity, string.Empty).Trim();
            RemoteDeviceIdentity = ReadString(config, ConfigurationKeys.RemoteControlDeviceIdentity, string.Empty).Trim();
            RemotePollInterval = ReadInt(config, ConfigurationKeys.RemoteControlPollIntervalMs, RemotePollInterval);
            ExternalNotificationEnabled = ReadBool(config, ConfigurationKeys.ExternalNotificationEnabled, false);
            ExternalNotificationSendWhenComplete = ReadBool(config, ConfigurationKeys.ExternalNotificationSendWhenComplete, false);
            ExternalNotificationSendWhenError = ReadBool(config, ConfigurationKeys.ExternalNotificationSendWhenError, false);
            ExternalNotificationSendWhenTimeout = ReadBool(config, ConfigurationKeys.ExternalNotificationSendWhenTimeout, false);
            ExternalNotificationEnableDetails = ReadBool(config, ConfigurationKeys.ExternalNotificationEnableDetails, false);
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

        var versionPolicyResult = await Runtime.VersionUpdateFeatureService.LoadPolicyAsync(cancellationToken);
        if (versionPolicyResult.Success && versionPolicyResult.Value is not null)
        {
            ApplyVersionUpdatePolicy(versionPolicyResult.Value);
            VersionUpdateErrorMessage = string.Empty;
        }
        else
        {
            VersionUpdateErrorMessage = versionPolicyResult.Message;
        }

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

        var startPerformanceWarnings = new List<string>();
        var startPerformanceSnapshot = ReadStartPerformanceSnapshot(config, startPerformanceWarnings);
        ApplyStartPerformanceSnapshotWithoutDirtyTracking(startPerformanceSnapshot);
        HasPendingStartPerformanceChanges = false;

        var timerWarnings = new List<string>();
        var timerSnapshot = ReadTimerSnapshot(config, timerWarnings);
        ApplyTimerSnapshot(timerSnapshot);
        HasPendingTimerChanges = false;

        if (warnings.Count > 0)
        {
            GuiValidationMessage = string.Join(" ", warnings);
            StatusMessage = GuiValidationMessage;
            await RecordEventAsync(
                "Settings.Gui.Normalize",
                string.Join(" | ", warnings),
                cancellationToken);
        }
        else
        {
            GuiValidationMessage = string.Empty;
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

    private void LoadConnectionSharedStateFromConfig()
    {
        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return;
        }

        ConnectionGameProfileSync.ReadFromProfile(profile, ConnectionGameSharedState);
    }

    private static string ReadString(UnifiedConfig config, string key, string fallback)
    {
        if (config.GlobalValues.TryGetValue(key, out var node) && node is not null)
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

    private static bool ReadBool(UnifiedConfig config, string key, bool fallback)
    {
        if (config.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            return bool.TryParse(node.ToString(), out var parsed) ? parsed : fallback;
        }

        return fallback;
    }

    private static int ReadInt(UnifiedConfig config, string key, int fallback)
    {
        if (config.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            return int.TryParse(node.ToString(), out var parsed) ? parsed : fallback;
        }

        return fallback;
    }

    private string NormalizeTheme(string? value)
    {
        var normalized = value?.Trim();
        return ThemeOptions.Any(option => string.Equals(option, normalized, StringComparison.OrdinalIgnoreCase))
            ? ThemeOptions.First(option => string.Equals(option, normalized, StringComparison.OrdinalIgnoreCase))
            : DefaultTheme;
    }

    private string NormalizeLanguage(string? value)
    {
        return UiLanguageCatalog.Normalize(value);
    }

    private string NormalizeBackgroundStretchMode(string? value)
    {
        var normalized = value?.Trim();
        return BackgroundStretchModes.Any(option => string.Equals(option, normalized, StringComparison.OrdinalIgnoreCase))
            ? BackgroundStretchModes.First(option => string.Equals(option, normalized, StringComparison.OrdinalIgnoreCase))
            : DefaultBackgroundStretchMode;
    }

    private static string NormalizeBackgroundPath(string? value)
    {
        return value?.Trim() ?? string.Empty;
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
            PerformancePreferredGpuInstancePath: (PerformancePreferredGpuInstancePath ?? string.Empty).Trim());
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
        var emulatorPath = ReadString(config, ConfigurationKeys.EmulatorPath, string.Empty).Trim();
        var emulatorAddCommand = ReadString(config, ConfigurationKeys.EmulatorAddCommand, string.Empty).Trim();
        var preferredGpuDescription = ReadString(config, ConfigurationKeys.PerformancePreferredGpuDescription, string.Empty).Trim();
        var preferredGpuInstancePath = ReadString(config, ConfigurationKeys.PerformancePreferredGpuInstancePath, string.Empty).Trim();

        var rawWaitSeconds = ReadInt(config, ConfigurationKeys.EmulatorWaitSeconds, DefaultEmulatorWaitSeconds);
        var emulatorWaitSeconds = Math.Clamp(rawWaitSeconds, EmulatorWaitSecondsMin, EmulatorWaitSecondsMax);
        if (emulatorWaitSeconds != rawWaitSeconds)
        {
            warnings.Add(
                $"Start.EmulatorWaitSeconds clamped to {emulatorWaitSeconds} from {rawWaitSeconds}.");
        }

        return new StartPerformanceSettingsSnapshot(
            RunDirectly: ReadBoolFlexible(config, ConfigurationKeys.RunDirectly, false),
            MinimizeDirectly: ReadBoolFlexible(config, ConfigurationKeys.MinimizeDirectly, false),
            OpenEmulatorAfterLaunch: ReadBoolFlexible(config, ConfigurationKeys.StartEmulator, false),
            EmulatorPath: emulatorPath,
            EmulatorAddCommand: emulatorAddCommand,
            EmulatorWaitSeconds: emulatorWaitSeconds,
            PerformanceUseGpu: ReadBoolFlexible(config, ConfigurationKeys.PerformanceUseGpu, false),
            PerformanceAllowDeprecatedGpu: ReadBoolFlexible(config, ConfigurationKeys.PerformanceAllowDeprecatedGpu, false),
            PerformancePreferredGpuDescription: preferredGpuDescription,
            PerformancePreferredGpuInstancePath: preferredGpuInstancePath);
    }

    private void ApplyStartPerformanceSnapshotWithoutDirtyTracking(StartPerformanceSettingsSnapshot snapshot)
    {
        _suppressStartPerformanceDirtyTracking = true;
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
        }
        finally
        {
            _suppressStartPerformanceDirtyTracking = false;
        }
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

            var enabled = ReadBoolFlexible(config, enabledKey, false);

            var rawHour = ReadIntFlexible(config, hourKey, DefaultTimerHour, out var parsedHour);
            if (!parsedHour && config.GlobalValues.ContainsKey(hourKey))
            {
                warnings.Add($"Timer {index} hour parse failed and fell back to {DefaultTimerHour}.");
            }

            var hour = Math.Clamp(rawHour, TimerHourMin, TimerHourMax);
            if (hour != rawHour)
            {
                warnings.Add($"Timer {index} hour clamped to {hour} from {rawHour}.");
            }

            var rawMinute = ReadIntFlexible(config, minuteKey, DefaultTimerMinute, out var parsedMinute);
            if (!parsedMinute && config.GlobalValues.ContainsKey(minuteKey))
            {
                warnings.Add($"Timer {index} minute parse failed and fell back to {DefaultTimerMinute}.");
            }

            var minute = Math.Clamp(rawMinute, TimerMinuteMin, TimerMinuteMax);
            if (minute != rawMinute)
            {
                warnings.Add($"Timer {index} minute clamped to {minute} from {rawMinute}.");
            }

            var profile = NormalizeTimerProfile(ReadString(config, profileKey, currentProfile), currentProfile);
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
            ForceScheduledStart: ReadBoolFlexible(config, LegacyConfigurationKeys.ForceScheduledStart, false),
            ShowWindowBeforeForceScheduledStart: ReadBoolFlexible(config, LegacyConfigurationKeys.ShowWindowBeforeForceScheduledStart, true),
            CustomTimerConfig: ReadBoolFlexible(config, LegacyConfigurationKeys.CustomConfig, false),
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

    private static bool ReadBoolFlexible(UnifiedConfig config, string key, bool fallback)
    {
        if (!config.GlobalValues.TryGetValue(key, out var node) || node is null)
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

    private static int ReadIntFlexible(UnifiedConfig config, string key, int fallback, out bool parsed)
    {
        parsed = false;
        if (!config.GlobalValues.TryGetValue(key, out var node) || node is null)
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
}

public sealed record GuiSettingsSnapshot(
    string Theme,
    string Language,
    bool UseTray,
    bool MinimizeToTray,
    bool WindowTitleScrollable,
    string BackgroundImagePath,
    int BackgroundOpacity,
    int BackgroundBlur,
    string BackgroundStretchMode)
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
            [ConfigurationKeys.BackgroundImagePath] = BackgroundImagePath,
            [ConfigurationKeys.BackgroundOpacity] = BackgroundOpacity.ToString(),
            [ConfigurationKeys.BackgroundBlurEffectRadius] = BackgroundBlur.ToString(),
            [ConfigurationKeys.BackgroundImageStretchMode] = BackgroundStretchMode,
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
    string PerformancePreferredGpuInstancePath)
{
    public IReadOnlyDictionary<string, string> ToGlobalSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.RunDirectly] = RunDirectly.ToString(),
            [ConfigurationKeys.MinimizeDirectly] = MinimizeDirectly.ToString(),
            [ConfigurationKeys.StartEmulator] = OpenEmulatorAfterLaunch.ToString(),
            [ConfigurationKeys.EmulatorPath] = EmulatorPath,
            [ConfigurationKeys.EmulatorAddCommand] = EmulatorAddCommand,
            [ConfigurationKeys.EmulatorWaitSeconds] = EmulatorWaitSeconds.ToString(),
            [ConfigurationKeys.PerformanceUseGpu] = PerformanceUseGpu.ToString(),
            [ConfigurationKeys.PerformanceAllowDeprecatedGpu] = PerformanceAllowDeprecatedGpu.ToString(),
            [ConfigurationKeys.PerformancePreferredGpuDescription] = PerformancePreferredGpuDescription,
            [ConfigurationKeys.PerformancePreferredGpuInstancePath] = PerformancePreferredGpuInstancePath,
        };
    }
}

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

public enum HotkeyRegistrationSource
{
    Manual = 0,
    Startup = 1,
}

public sealed class TimerSlotViewModel : ObservableObject
{
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
        set => SetProperty(ref _time, value?.Trim() ?? string.Empty);
    }

    public string Profile
    {
        get => _profile;
        set => SetProperty(ref _profile, value?.Trim() ?? string.Empty);
    }
}
