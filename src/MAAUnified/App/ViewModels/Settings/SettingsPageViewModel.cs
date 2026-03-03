using System.Collections.ObjectModel;
using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Compat.Constants;

namespace MAAUnified.App.ViewModels.Settings;

public sealed class SettingsPageViewModel : PageViewModelBase
{
    private SettingsSectionViewModel? _selectedSection;
    private string _theme = "Light";
    private string _language = "zh-cn";
    private bool _useTray = true;
    private bool _minimizeToTray;
    private bool _windowTitleScrollable = true;
    private bool _startSelf;
    private string _autostartStatus = string.Empty;
    private string _hotkeyShowGui = "Ctrl+Shift+Alt+M";
    private string _hotkeyLinkStart = "Ctrl+Shift+Alt+L";
    private string _notificationTitle = "MAA Test";
    private string _notificationMessage = "Cross-platform notification test";
    private string _issueReportPath = string.Empty;
    private string _remoteGetTaskEndpoint = string.Empty;
    private string _remoteReportEndpoint = string.Empty;
    private int _remotePollInterval = 5000;
    private string _backgroundImagePath = string.Empty;
    private int _backgroundOpacity = 45;
    private int _backgroundBlur = 12;

    public SettingsPageViewModel(MAAUnifiedRuntime runtime, ConnectionGameSharedStateViewModel connectionGameSharedState)
        : base(runtime)
    {
        ConnectionGameSharedState = connectionGameSharedState;
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
            Enumerable.Range(1, 8).Select(i => new TimerSlotViewModel(i)));

        SelectedSection = Sections[0];
    }

    public ObservableCollection<SettingsSectionViewModel> Sections { get; }

    public ObservableCollection<TimerSlotViewModel> Timers { get; }

    public ConnectionGameSharedStateViewModel ConnectionGameSharedState { get; }

    public SettingsSectionViewModel? SelectedSection
    {
        get => _selectedSection;
        set => SetProperty(ref _selectedSection, value);
    }

    public string Theme
    {
        get => _theme;
        set => SetProperty(ref _theme, value);
    }

    public string Language
    {
        get => _language;
        set => SetProperty(ref _language, value);
    }

    public bool UseTray
    {
        get => _useTray;
        set => SetProperty(ref _useTray, value);
    }

    public bool MinimizeToTray
    {
        get => _minimizeToTray;
        set => SetProperty(ref _minimizeToTray, value);
    }

    public bool WindowTitleScrollable
    {
        get => _windowTitleScrollable;
        set => SetProperty(ref _windowTitleScrollable, value);
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

    public int RemotePollInterval
    {
        get => _remotePollInterval;
        set => SetProperty(ref _remotePollInterval, Math.Max(500, value));
    }

    public string BackgroundImagePath
    {
        get => _backgroundImagePath;
        set => SetProperty(ref _backgroundImagePath, value);
    }

    public int BackgroundOpacity
    {
        get => _backgroundOpacity;
        set => SetProperty(ref _backgroundOpacity, Math.Clamp(value, 0, 100));
    }

    public int BackgroundBlur
    {
        get => _backgroundBlur;
        set => SetProperty(ref _backgroundBlur, Math.Clamp(value, 0, 80));
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        LoadFromConfig(Runtime.ConfigurationService.CurrentConfig);
        LoadConnectionSharedStateFromConfig();
        await RefreshAutostartStatusAsync(cancellationToken);
        await Runtime.DiagnosticsService.RecordEventAsync("Settings", "Settings page initialized.", cancellationToken);
    }

    public async Task SaveGuiSettingsAsync(CancellationToken cancellationToken = default)
    {
        var updates = new Dictionary<string, string>
        {
            [ConfigurationKeys.Localization] = Language,
            [ConfigurationKeys.UseTray] = UseTray.ToString(),
            [ConfigurationKeys.MinimizeToTray] = MinimizeToTray.ToString(),
            [ConfigurationKeys.WindowTitleScrollable] = WindowTitleScrollable.ToString(),
            [ConfigurationKeys.BackgroundImagePath] = BackgroundImagePath,
            [ConfigurationKeys.BackgroundOpacity] = BackgroundOpacity.ToString(),
            [ConfigurationKeys.BackgroundBlurEffectRadius] = BackgroundBlur.ToString(),
        };

        foreach (var (key, value) in updates)
        {
            var result = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(key, value, cancellationToken);
            if (!await ApplyResultAsync(result, $"Settings.Save.{key}", cancellationToken))
            {
                return;
            }
        }
    }

    public async Task SaveRemoteControlAsync(CancellationToken cancellationToken = default)
    {
        var updates = new Dictionary<string, string>
        {
            [ConfigurationKeys.RemoteControlGetTaskEndpointUri] = RemoteGetTaskEndpoint,
            [ConfigurationKeys.RemoteControlReportStatusUri] = RemoteReportEndpoint,
            [ConfigurationKeys.RemoteControlPollIntervalMs] = RemotePollInterval.ToString(),
        };

        foreach (var (key, value) in updates)
        {
            var result = await Runtime.SettingsFeatureService.SaveGlobalSettingAsync(key, value, cancellationToken);
            if (!await ApplyResultAsync(result, $"Settings.Remote.{key}", cancellationToken))
            {
                return;
            }
        }
    }

    public async Task SaveConnectionGameSettingsAsync(CancellationToken cancellationToken = default)
    {
        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            LastErrorMessage = "Current profile is missing.";
            await Runtime.DiagnosticsService.RecordErrorAsync(
                "Settings.ConnectionGame.Save",
                LastErrorMessage,
                cancellationToken: cancellationToken);
            return;
        }

        profile.Values["ConnectAddress"] = JsonValue.Create(ConnectionGameSharedState.ConnectAddress.Trim());
        profile.Values["ConnectConfig"] = JsonValue.Create(ConnectionGameSharedState.ConnectConfig.Trim());
        profile.Values["AdbPath"] = JsonValue.Create(ConnectionGameSharedState.AdbPath.Trim());
        profile.Values["ClientType"] = JsonValue.Create(ConnectionGameSharedState.ClientType.Trim());
        profile.Values["StartGame"] = JsonValue.Create(ConnectionGameSharedState.StartGameEnabled);
        profile.Values["TouchMode"] = JsonValue.Create(ConnectionGameSharedState.TouchMode.Trim());
        profile.Values["AutoDetect"] = JsonValue.Create(ConnectionGameSharedState.AutoDetect);

        await ApplyResultAsync(
            await Runtime.TaskQueueFeatureService.SaveAsync(cancellationToken),
            "Settings.ConnectionGame.Save",
            cancellationToken);
    }

    public async Task RegisterHotkeysAsync(CancellationToken cancellationToken = default)
    {
        if (!await ApplyResultAsync(
                await Runtime.SettingsFeatureService.RegisterHotkeyAsync("ShowGui", HotkeyShowGui, cancellationToken),
                "Settings.Hotkey.ShowGui",
                cancellationToken))
        {
            return;
        }

        await ApplyResultAsync(
            await Runtime.SettingsFeatureService.RegisterHotkeyAsync("LinkStart", HotkeyLinkStart, cancellationToken),
            "Settings.Hotkey.LinkStart",
            cancellationToken);

        Runtime.ConfigurationService.CurrentConfig.GlobalValues[ConfigurationKeys.HotKeys] = JsonValue.Create(
            $"ShowGui={HotkeyShowGui};LinkStart={HotkeyLinkStart}");
        await Runtime.ConfigurationService.SaveAsync(cancellationToken);
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
        var outputPath = await ApplyResultAsync(
            await Runtime.SettingsFeatureService.BuildIssueReportAsync(cancellationToken),
            "Settings.IssueReport.Build",
            cancellationToken);

        if (!string.IsNullOrWhiteSpace(outputPath))
        {
            IssueReportPath = outputPath;
        }
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
            await Runtime.DiagnosticsService.RecordFailedResultAsync(
                "Settings.Autostart.Query",
                UiOperationResult.Fail(result.Error?.Code ?? "AutostartQueryFailed", result.Message, result.Error?.Details),
                cancellationToken);
            return;
        }

        var enabled = result.Value;
        StartSelf = enabled;
        AutostartStatus = PlatformCapabilityTextMap.FormatAutostartStatus(Language, enabled);
    }

    private void LoadFromConfig(UnifiedConfig config)
    {
        Theme = ReadString(config, "Theme.Mode", Theme);
        Language = ReadString(config, ConfigurationKeys.Localization, Language);
        UseTray = ReadBool(config, ConfigurationKeys.UseTray, UseTray);
        MinimizeToTray = ReadBool(config, ConfigurationKeys.MinimizeToTray, MinimizeToTray);
        WindowTitleScrollable = ReadBool(config, ConfigurationKeys.WindowTitleScrollable, WindowTitleScrollable);
        BackgroundImagePath = ReadString(config, ConfigurationKeys.BackgroundImagePath, string.Empty);
        BackgroundOpacity = ReadInt(config, ConfigurationKeys.BackgroundOpacity, BackgroundOpacity);
        BackgroundBlur = ReadInt(config, ConfigurationKeys.BackgroundBlurEffectRadius, BackgroundBlur);
        RemoteGetTaskEndpoint = ReadString(config, ConfigurationKeys.RemoteControlGetTaskEndpointUri, string.Empty);
        RemoteReportEndpoint = ReadString(config, ConfigurationKeys.RemoteControlReportStatusUri, string.Empty);
        RemotePollInterval = ReadInt(config, ConfigurationKeys.RemoteControlPollIntervalMs, RemotePollInterval);
    }

    private void LoadConnectionSharedStateFromConfig()
    {
        if (!Runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return;
        }

        ConnectionGameSharedState.ConnectAddress = ReadProfileString(profile, "ConnectAddress", ConnectionGameSharedState.ConnectAddress);
        ConnectionGameSharedState.ConnectConfig = ReadProfileString(profile, "ConnectConfig", ConnectionGameSharedState.ConnectConfig);
        ConnectionGameSharedState.AdbPath = ReadProfileString(profile, "AdbPath", ConnectionGameSharedState.AdbPath);
        ConnectionGameSharedState.ClientType = ReadProfileString(profile, "ClientType", ConnectionGameSharedState.ClientType);
        ConnectionGameSharedState.StartGameEnabled = ReadProfileBool(profile, "StartGame", ConnectionGameSharedState.StartGameEnabled);
        ConnectionGameSharedState.TouchMode = ReadProfileString(profile, "TouchMode", ConnectionGameSharedState.TouchMode);
        ConnectionGameSharedState.AutoDetect = ReadProfileBool(profile, "AutoDetect", ConnectionGameSharedState.AutoDetect);
    }

    private static string ReadString(UnifiedConfig config, string key, string fallback)
    {
        if (config.GlobalValues.TryGetValue(key, out var node) && node is not null)
        {
            return node.GetValue<string>();
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

    private static string ReadProfileString(UnifiedProfile profile, string key, string fallback)
    {
        if (profile.Values.TryGetValue(key, out var node)
            && node is JsonValue value
            && value.TryGetValue(out string? text)
            && !string.IsNullOrWhiteSpace(text))
        {
            return text;
        }

        return fallback;
    }

    private static bool ReadProfileBool(UnifiedProfile profile, string key, bool fallback)
    {
        if (!profile.Values.TryGetValue(key, out var node) || node is not JsonValue value)
        {
            return fallback;
        }

        if (value.TryGetValue(out bool parsed))
        {
            return parsed;
        }

        if (value.TryGetValue(out int parsedInt))
        {
            return parsedInt != 0;
        }

        if (value.TryGetValue(out string? text) && bool.TryParse(text, out var parsedText))
        {
            return parsedText;
        }

        return fallback;
    }
}

public sealed class TimerSlotViewModel : ObservableObject
{
    private bool _enabled;
    private string _time = "07:00";
    private bool _forceStart;
    private string _profile = "Default";
    private bool _showWindowBeforeStart = true;

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
        set => SetProperty(ref _time, value);
    }

    public bool ForceStart
    {
        get => _forceStart;
        set => SetProperty(ref _forceStart, value);
    }

    public string Profile
    {
        get => _profile;
        set => SetProperty(ref _profile, value);
    }

    public bool ShowWindowBeforeStart
    {
        get => _showWindowBeforeStart;
        set => SetProperty(ref _showWindowBeforeStart, value);
    }
}
