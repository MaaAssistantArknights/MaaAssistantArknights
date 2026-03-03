using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Text.Json.Nodes;
using Avalonia.Threading;
using MAAUnified.App.ViewModels.Copilot;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.App.ViewModels.TaskQueue;
using MAAUnified.App.ViewModels.Toolbox;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.Features;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels;

public sealed class MainShellViewModel : ObservableObject
{
    private readonly MAAUnifiedRuntime _runtime;
    private readonly ConnectionGameSharedStateViewModel _connectionGameSharedState;
    private bool _syncingConnectionState;
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

    public MainShellViewModel(MAAUnifiedRuntime runtime)
    {
        _runtime = runtime;
        _connectionGameSharedState = new ConnectionGameSharedStateViewModel();
        _connectionGameSharedState.PropertyChanged += OnSharedConnectionStateChanged;

        RootTabs = new[] { "任务队列", "Copilot", "Toolbox", "设置" };
        GrowlMessages = new ObservableCollection<string>();
        RootLogs = new ObservableCollection<string>();
        ConfigIssueDetails = new ObservableCollection<ConfigValidationIssue>();

        ImportSourceOptions = new ObservableCollection<string>
        {
            "自动(gui.new + gui)",
            "仅 gui.new.json",
            "仅 gui.json",
        };

        TaskQueuePage = new TaskQueuePageViewModel(runtime, _connectionGameSharedState);
        CopilotPage = new CopilotPageViewModel(runtime);
        ToolboxPage = new ToolboxPageViewModel(runtime);
        SettingsPage = new SettingsPageViewModel(runtime, _connectionGameSharedState);

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

    public ObservableCollection<ConfigValidationIssue> ConfigIssueDetails { get; }

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

    public bool CanStartExecution => !HasBlockingConfigIssues;

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
                await _runtime.DiagnosticsService.RecordErrorAsync(
                    "Config.LoadValidation",
                    $"{summary} | {LastError}",
                    cancellationToken: cancellationToken);
            }

            SyncConnectionFromProfile();
            RefreshConfigValidationState(loadResult.ValidationIssues);

            var initResult = await _runtime.ResourceWorkflowService.InitializeCoreAsync(_runtime.ConfigurationService.CurrentConfig, cancellationToken);
            if (!initResult.Success)
            {
                LastError = $"Core 初始化失败: {initResult.Error?.Code} {initResult.Error?.Message}";
                await _runtime.DiagnosticsService.RecordErrorAsync("App.Initialize", LastError, cancellationToken: cancellationToken);
            }

            await TaskQueuePage.InitializeAsync(cancellationToken);
            await CopilotPage.InitializeAsync(cancellationToken);
            await ToolboxPage.InitializeAsync(cancellationToken);
            await SettingsPage.InitializeAsync(cancellationToken);
            TaskQueuePage.SetLanguage(SettingsPage.Language);

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
        }
        catch (Exception ex)
        {
            LastError = $"初始化异常: {ex.Message}";
            await _runtime.DiagnosticsService.RecordErrorAsync("App.Initialize", LastError, ex, cancellationToken);
        }
    }

    public async Task ConnectAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            var result = await _runtime.ConnectFeatureService.ConnectAsync(
                _connectionGameSharedState.ConnectAddress,
                _connectionGameSharedState.ConnectConfig,
                string.IsNullOrWhiteSpace(_connectionGameSharedState.AdbPath) ? null : _connectionGameSharedState.AdbPath,
                cancellationToken);
            if (!result.Success)
            {
                LastError = result.Message;
                await _runtime.DiagnosticsService.RecordFailedResultAsync("App.Connect", result, cancellationToken);
                return;
            }

            GlobalStatus = result.Message;
            await _runtime.DiagnosticsService.RecordEventAsync("App.Connect", result.Message, cancellationToken);
        }
        catch (Exception ex)
        {
            LastError = ex.Message;
            await _runtime.DiagnosticsService.RecordErrorAsync("App.Connect", "Connection failed unexpectedly.", ex, cancellationToken);
        }
    }

    public async Task StartAsync(CancellationToken cancellationToken = default)
    {
        var latestIssues = _runtime.ConfigurationService.RevalidateCurrentConfig();
        RefreshConfigValidationState(latestIssues);

        if (!CanStartExecution)
        {
            var first = ConfigIssueDetails.FirstOrDefault(i => i.Blocking);
            var message = first is null
                ? "Config validation has blocking issues."
                : $"{first.Scope}:{first.Code}:{first.Field}:{first.Message}";
            LastError = message;
            await _runtime.DiagnosticsService.RecordConfigValidationFailureAsync(first, cancellationToken);
            return;
        }

        await TaskQueuePage.StartAsync(cancellationToken);
        await SyncTrayMenuStateAsync(cancellationToken);
    }

    public async Task StopAsync(CancellationToken cancellationToken = default)
    {
        await TaskQueuePage.StopAsync(cancellationToken);
        await SyncTrayMenuStateAsync(cancellationToken);
    }

    public async Task ManualImportAsync(CancellationToken cancellationToken = default)
    {
        var source = SelectedImportSource switch
        {
            "仅 gui.new.json" => ImportSource.GuiNewOnly,
            "仅 gui.json" => ImportSource.GuiOnly,
            _ => ImportSource.Auto,
        };

        var result = await _runtime.ConnectFeatureService.ImportLegacyConfigAsync(source, manualImport: true, cancellationToken);
        if (!result.Success || result.Value is null)
        {
            ImportStatus = result.Message;
            LastError = result.Message;
            await _runtime.DiagnosticsService.RecordFailedResultAsync(
                "App.ImportLegacy",
                UiOperationResult.Fail(result.Error?.Code ?? "ImportFailed", result.Message, result.Error?.Details),
                cancellationToken);
            return;
        }

        ImportStatus = $"导入完成: {result.Value.Summary}";
        GlobalStatus = ImportStatus;
        await TaskQueuePage.ReloadTasksAsync(cancellationToken);
        await SettingsPage.InitializeAsync(cancellationToken);
        TaskQueuePage.SetLanguage(SettingsPage.Language);
        SyncConnectionFromProfile();
        RefreshConfigValidationState(_runtime.ConfigurationService.CurrentValidationIssues);
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

    private void SyncConnectionToProfile()
    {
        if (_syncingConnectionState)
        {
            return;
        }

        if (!_runtime.ConfigurationService.TryGetCurrentProfile(out var profile))
        {
            return;
        }

        profile.Values["ConnectAddress"] = JsonValue.Create(_connectionGameSharedState.ConnectAddress.Trim());
        profile.Values["ConnectConfig"] = JsonValue.Create(_connectionGameSharedState.ConnectConfig.Trim());
        profile.Values["AdbPath"] = JsonValue.Create(_connectionGameSharedState.AdbPath.Trim());
        profile.Values["ClientType"] = JsonValue.Create(_connectionGameSharedState.ClientType.Trim());
        profile.Values["StartGame"] = JsonValue.Create(_connectionGameSharedState.StartGameEnabled);
        profile.Values["TouchMode"] = JsonValue.Create(_connectionGameSharedState.TouchMode.Trim());
        profile.Values["AutoDetect"] = JsonValue.Create(_connectionGameSharedState.AutoDetect);
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
            _connectionGameSharedState.ConnectAddress = ReadString(profile, "ConnectAddress", _connectionGameSharedState.ConnectAddress);
            _connectionGameSharedState.ConnectConfig = ReadString(profile, "ConnectConfig", _connectionGameSharedState.ConnectConfig);
            _connectionGameSharedState.AdbPath = ReadString(profile, "AdbPath", _connectionGameSharedState.AdbPath);
            _connectionGameSharedState.ClientType = ReadString(profile, "ClientType", _connectionGameSharedState.ClientType);
            _connectionGameSharedState.StartGameEnabled = ReadBool(profile, "StartGame", _connectionGameSharedState.StartGameEnabled);
            _connectionGameSharedState.TouchMode = ReadString(profile, "TouchMode", _connectionGameSharedState.TouchMode);
            _connectionGameSharedState.AutoDetect = ReadBool(profile, "AutoDetect", _connectionGameSharedState.AutoDetect);
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

        if (string.IsNullOrEmpty(e.PropertyName)
            || e.PropertyName is nameof(ConnectionGameSharedStateViewModel.ConnectAddress)
                or nameof(ConnectionGameSharedStateViewModel.ConnectConfig)
                or nameof(ConnectionGameSharedStateViewModel.AdbPath)
                or nameof(ConnectionGameSharedStateViewModel.ClientType)
                or nameof(ConnectionGameSharedStateViewModel.StartGameEnabled)
                or nameof(ConnectionGameSharedStateViewModel.TouchMode)
                or nameof(ConnectionGameSharedStateViewModel.AutoDetect))
        {
            SyncConnectionToProfile();
        }
    }

    private void RefreshConfigValidationState(IReadOnlyList<ConfigValidationIssue> issues)
    {
        ConfigIssueDetails.Clear();
        foreach (var issue in issues)
        {
            ConfigIssueDetails.Add(issue);
        }

        BlockingConfigIssueCount = issues.Count(i => i.Blocking);
        HasBlockingConfigIssues = BlockingConfigIssueCount > 0;
        _ = SyncTrayMenuStateAsync();
    }

    private static string ReadString(UnifiedProfile profile, string key, string fallback)
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

    private static bool ReadBool(UnifiedProfile profile, string key, bool fallback)
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

    private async Task RefreshCapabilitySummaryAsync(CancellationToken cancellationToken = default)
    {
        var snapshotResult = await _runtime.PlatformCapabilityService.GetSnapshotAsync(cancellationToken);
        if (!snapshotResult.Success || snapshotResult.Value is null)
        {
            CapabilitySummary = PlatformCapabilityTextMap.FormatSnapshotUnavailable(SettingsPage.Language, snapshotResult.Message);
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
        if (!result.Success)
        {
            LastError = result.Message;
            await _runtime.DiagnosticsService.RecordFailedResultAsync("Tray.SetVisible", result, cancellationToken);
            return;
        }

        await _runtime.DiagnosticsService.RecordEventAsync("Tray.SetVisible", result.Message, cancellationToken);
        PushGrowl(result.Message);
    }

    public async Task ToggleOverlayFromTrayAsync(CancellationToken cancellationToken = default)
    {
        await TaskQueuePage.ToggleOverlayAsync(cancellationToken);
        await SyncTrayMenuStateAsync(cancellationToken);
    }

    public string SwitchLanguageCycle()
    {
        var ordered = new[] { "zh-cn", "en-us", "ja-jp", "ko-kr", "zh-tw" };
        var current = SettingsPage.Language;
        var index = Array.FindIndex(ordered, lang => string.Equals(lang, current, StringComparison.OrdinalIgnoreCase));
        var next = index < 0 ? ordered[0] : ordered[(index + 1) % ordered.Length];
        SettingsPage.Language = next;
        SettingsPage.AutostartStatus = PlatformCapabilityTextMap.FormatAutostartStatus(next, SettingsPage.StartSelf);
        TaskQueuePage.SetLanguage(next);
        _ = RefreshCapabilitySummaryAsync();
        return next;
    }

    private async Task SyncTrayMenuStateAsync(CancellationToken cancellationToken = default)
    {
        var state = new TrayMenuState(
            StartEnabled: !TaskQueuePage.IsRunning && CanStartExecution,
            StopEnabled: TaskQueuePage.IsRunning,
            OverlayEnabled: true,
            ForceShowEnabled: true,
            HideTrayEnabled: true);
        await _runtime.PlatformCapabilityService.SetTrayMenuStateAsync(state, cancellationToken);
    }

    private static string BuildCapabilityLine(string language, PlatformCapabilityId capability, PlatformCapabilityStatus status)
    {
        return PlatformCapabilityTextMap.FormatCapabilityLine(language, capability, status);
    }
}
