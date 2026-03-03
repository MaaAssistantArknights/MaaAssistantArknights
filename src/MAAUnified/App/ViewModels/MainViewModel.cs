using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using Avalonia.Threading;
using MAAUnified.Application.Configuration;
using MAAUnified.Application.Services;
using MAAUnified.Platform;

namespace MAAUnified.App.ViewModels;

public sealed class MainViewModel : INotifyPropertyChanged
{
    private readonly MAAUnifiedRuntime _runtime;

    private string _connectionAddress = "127.0.0.1:5555";
    private string _connectConfig = "General";
    private string _adbPath = string.Empty;
    private string _startupStatus = "Initializing...";
    private string _importStatus = string.Empty;
    private string _capabilityStatus = string.Empty;
    private string _selectedImportSource = "自动(gui.new + gui)";
    private FeatureModule? _selectedModule;
    private string _moduleFilter = string.Empty;
    private string _moduleActionStatus = string.Empty;

    public MainViewModel(MAAUnifiedRuntime runtime)
    {
        _runtime = runtime;

        ImportSourceOptions = new ObservableCollection<string>
        {
            "自动(gui.new + gui)",
            "仅 gui.new.json",
            "仅 gui.json",
        };

        Logs = new ObservableCollection<string>();
        Modules = new ObservableCollection<FeatureModule>(FeatureManifest.All);
        SelectedModule = Modules.FirstOrDefault();

        _runtime.LogService.LogReceived += log => Dispatcher.UIThread.Post(() => Logs.Add($"[{log.Timestamp:HH:mm:ss}] {log.Level} {log.Message}"));
        _runtime.ConfigurationService.ConfigChanged += _ =>
        {
            OnPropertyChanged(nameof(CurrentProfile));
            OnPropertyChanged(nameof(ProfileCount));
            OnPropertyChanged(nameof(CurrentProfileText));
            OnPropertyChanged(nameof(ProfileCountText));
        };

        CapabilityStatus = BuildCapabilityStatus();
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public ObservableCollection<string> ImportSourceOptions { get; }

    public ObservableCollection<string> Logs { get; }

    public ObservableCollection<FeatureModule> Modules { get; }

    public string ConnectionAddress
    {
        get => _connectionAddress;
        set => SetProperty(ref _connectionAddress, value);
    }

    public string ConnectConfig
    {
        get => _connectConfig;
        set => SetProperty(ref _connectConfig, value);
    }

    public string AdbPath
    {
        get => _adbPath;
        set => SetProperty(ref _adbPath, value);
    }

    public string StartupStatus
    {
        get => _startupStatus;
        set => SetProperty(ref _startupStatus, value);
    }

    public string ImportStatus
    {
        get => _importStatus;
        set => SetProperty(ref _importStatus, value);
    }

    public string CapabilityStatus
    {
        get => _capabilityStatus;
        set => SetProperty(ref _capabilityStatus, value);
    }

    public string SelectedImportSource
    {
        get => _selectedImportSource;
        set => SetProperty(ref _selectedImportSource, value);
    }

    public FeatureModule? SelectedModule
    {
        get => _selectedModule;
        set
        {
            if (SetProperty(ref _selectedModule, value))
            {
                OnPropertyChanged(nameof(SelectedModuleTitle));
                OnPropertyChanged(nameof(SelectedModuleDescription));
                OnPropertyChanged(nameof(SelectedModuleScope));
                ModuleChanged?.Invoke(value);
            }
        }
    }

    public string ModuleFilter
    {
        get => _moduleFilter;
        set
        {
            if (SetProperty(ref _moduleFilter, value))
            {
                ApplyModuleFilter();
            }
        }
    }

    public string CurrentProfile => _runtime.ConfigurationService.CurrentConfig.CurrentProfile;

    public int ProfileCount => _runtime.ConfigurationService.CurrentConfig.Profiles.Count;

    public string CurrentProfileText => $"当前配置: {CurrentProfile}";

    public string ProfileCountText => $"配置数量: {ProfileCount}";

    public string SelectedModuleTitle => SelectedModule?.Title ?? "未选择模块";

    public string SelectedModuleDescription => SelectedModule?.Description ?? "";

    public string SelectedModuleScope => SelectedModule?.ParityScope ?? "";

    public string ModuleActionStatus
    {
        get => _moduleActionStatus;
        set => SetProperty(ref _moduleActionStatus, value);
    }

    public event Action<FeatureModule?>? ModuleChanged;

    public async Task InitializeAsync()
    {
        var loadResult = await _runtime.ConfigurationService.LoadOrBootstrapAsync();
        if (loadResult.LoadedFromExistingConfig)
        {
            StartupStatus = "Loaded existing config/avalonia.json";
        }
        else if (loadResult.ImportReport is not null)
        {
            StartupStatus = $"Bootstrapped from legacy config: {loadResult.ImportReport.Summary}";
        }

        var initResult = await _runtime.ResourceWorkflowService.InitializeCoreAsync(_runtime.ConfigurationService.CurrentConfig);
        if (!initResult.Success)
        {
            StartupStatus = $"Core init failed: {initResult.Error?.Code} {initResult.Error?.Message}";
        }

        _ = Task.Run(() => _runtime.SessionService.StartCallbackPumpAsync(callback =>
        {
            Dispatcher.UIThread.Post(() => Logs.Add($"[{callback.Timestamp:HH:mm:ss}] CORE {callback.MsgName}({callback.MsgId}) {callback.PayloadJson}"));
            return Task.CompletedTask;
        }));

        OnPropertyChanged(nameof(CurrentProfile));
        OnPropertyChanged(nameof(ProfileCount));
        OnPropertyChanged(nameof(CurrentProfileText));
        OnPropertyChanged(nameof(ProfileCountText));
        OnPropertyChanged(nameof(SelectedModuleTitle));
        OnPropertyChanged(nameof(SelectedModuleDescription));
        OnPropertyChanged(nameof(SelectedModuleScope));
    }

    public async Task ConnectAsync()
    {
        var result = await _runtime.ConnectFeatureService.ValidateAndConnectAsync(
            ConnectionAddress,
            ConnectConfig,
            string.IsNullOrWhiteSpace(AdbPath) ? null : AdbPath);
        if (!result.Success)
        {
            StartupStatus = $"Connect failed: {result.Error?.Code} {result.Error?.Message}";
        }
    }

    public async Task AppendTasksAsync()
    {
        var result = await _runtime.TaskQueueFeatureService.QueueEnabledTasksAsync();
        if (!result.Success)
        {
            ModuleActionStatus = $"Append failed: {result.Error?.Code} {result.Error?.Message}";
            return;
        }

        ModuleActionStatus = $"Appended {result.Value} task(s).";
    }

    public async Task StartAsync()
    {
        var result = await _runtime.SessionService.StartAsync();
        if (!result.Success)
        {
            ModuleActionStatus = $"Start failed: {result.Error?.Code} {result.Error?.Message}";
        }
    }

    public async Task StopAsync()
    {
        var result = await _runtime.SessionService.StopAsync();
        if (!result.Success)
        {
            ModuleActionStatus = $"Stop failed: {result.Error?.Code} {result.Error?.Message}";
        }
    }

    public async Task ManualImportAsync()
    {
        var source = SelectedImportSource switch
        {
            "仅 gui.new.json" => ImportSource.GuiNewOnly,
            "仅 gui.json" => ImportSource.GuiOnly,
            _ => ImportSource.Auto,
        };

        var report = await _runtime.ConfigurationService.ImportLegacyAsync(source, manualImport: true);
        ImportStatus = report.Success
            ? $"导入完成：{report.Summary}，报告：{report.ReportPath}"
            : $"导入失败：{string.Join("; ", report.Errors)}";

        OnPropertyChanged(nameof(CurrentProfile));
        OnPropertyChanged(nameof(ProfileCount));
        OnPropertyChanged(nameof(CurrentProfileText));
        OnPropertyChanged(nameof(ProfileCountText));
    }

    public void SelectModule(FeatureModule module)
    {
        SelectedModule = module;
    }

    public async Task RunSelectedModuleActionAsync()
    {
        if (SelectedModule is null)
        {
            ModuleActionStatus = "未选择模块";
            return;
        }

        ModuleActionStatus = SelectedModule.Key switch
        {
            "Advanced.Copilot" => await _runtime.CopilotFeatureService.ImportCopilotAsync("manual"),
            "Advanced.Toolbox" => await _runtime.ToolboxFeatureService.RunToolAsync("default-tool"),
            "Advanced.RemoteControlCenter" => (await _runtime.RemoteControlFeatureService.StartRemotePollingAsync()).Success ? "RemoteControl polling started" : "RemoteControl polling failed",
            "Advanced.Overlay" => await _runtime.OverlayFeatureService.GetOverlayModeAsync(),
            "Advanced.ExternalNotificationProviders" => string.Join(", ", await _runtime.NotificationProviderFeatureService.GetAvailableProvidersAsync()),
            var key when key.StartsWith("Dialog.", StringComparison.OrdinalIgnoreCase) => await _runtime.DialogFeatureService.PrepareDialogPayloadAsync(key),
            _ => $"模块 {SelectedModule.Title} 当前使用统一框架处理，无额外动作。",
        };
    }

    private void ApplyModuleFilter()
    {
        var selectedKey = SelectedModule?.Key;
        Modules.Clear();

        IEnumerable<FeatureModule> filtered = FeatureManifest.All;
        if (!string.IsNullOrWhiteSpace(ModuleFilter))
        {
            filtered = filtered.Where(m =>
                m.Title.Contains(ModuleFilter, StringComparison.OrdinalIgnoreCase)
                || m.Description.Contains(ModuleFilter, StringComparison.OrdinalIgnoreCase)
                || m.Group.Contains(ModuleFilter, StringComparison.OrdinalIgnoreCase)
                || m.Key.Contains(ModuleFilter, StringComparison.OrdinalIgnoreCase));
        }

        foreach (var module in filtered)
        {
            Modules.Add(module);
        }

        SelectedModule = Modules.FirstOrDefault(m => string.Equals(m.Key, selectedKey, StringComparison.OrdinalIgnoreCase))
            ?? Modules.FirstOrDefault();
    }

    private string BuildCapabilityStatus()
    {
        var p = _runtime.Platform;
        return string.Join(
            Environment.NewLine,
            BuildCapabilityLine("Tray", p.TrayService.Capability),
            BuildCapabilityLine("Notification", p.NotificationService.Capability),
            BuildCapabilityLine("Hotkey", p.HotkeyService.Capability),
            BuildCapabilityLine("Autostart", p.AutostartService.Capability),
            BuildCapabilityLine("Overlay", p.OverlayService.Capability));
    }

    private static string BuildCapabilityLine(string name, PlatformCapabilityStatus status)
    {
        var mode = status.Supported ? "Supported" : "Fallback";
        var fallback = status.HasFallback && !string.IsNullOrWhiteSpace(status.FallbackMode)
            ? $", fallback={status.FallbackMode}"
            : string.Empty;
        return $"{name}: {mode} (provider={status.Provider}{fallback}) - {status.Message}";
    }

    private bool SetProperty<T>(ref T backingField, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(backingField, value))
        {
            return false;
        }

        backingField = value;
        OnPropertyChanged(propertyName);
        return true;
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
