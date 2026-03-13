using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services.Localization;

namespace MAAUnified.App.ViewModels.Settings;

public sealed class ConnectionGameSharedStateViewModel : ObservableObject
{
    private const int MaxConnectAddressHistory = 5;
    private const string DefaultAttachWindowScreencapMethod = "2";
    private const string DefaultAttachWindowInputMethod = "64";
    private const string DefaultScreencapCostText = "截图耗时 min/avg/max(ms): --- / --- / --- (--)";
    private static readonly IReadOnlyDictionary<string, IReadOnlyList<string>> _defaultAddressByConnectConfig =
        new Dictionary<string, IReadOnlyList<string>>(StringComparer.OrdinalIgnoreCase)
        {
            ["General"] = [string.Empty],
            ["BlueStacks"] = ["127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5575", "127.0.0.1:5585", "127.0.0.1:5595", "127.0.0.1:5554"],
            ["MuMuEmulator12"] = ["127.0.0.1:16384", "127.0.0.1:16416", "127.0.0.1:16448", "127.0.0.1:16480", "127.0.0.1:16512", "127.0.0.1:16544", "127.0.0.1:16576"],
            ["LDPlayer"] = ["emulator-5554", "emulator-5556", "emulator-5558", "emulator-5560", "127.0.0.1:5555", "127.0.0.1:5557", "127.0.0.1:5559", "127.0.0.1:5561"],
            ["Nox"] = ["127.0.0.1:62001", "127.0.0.1:59865"],
            ["XYAZ"] = ["127.0.0.1:21503"],
            ["WSA"] = ["127.0.0.1:58526"],
        };

    private string _language = UiLanguageCatalog.DefaultLanguage;
    private string _connectAddress = "127.0.0.1:5555";
    private string _connectConfig = "General";
    private string _adbPath = string.Empty;
    private string _clientType = "Official";
    private bool _startGameEnabled = true;
    private string _touchMode = "minitouch";
    private bool _autoDetect = true;
    private bool _alwaysAutoDetect;
    private bool _retryOnDisconnected;
    private bool _allowAdbRestart = true;
    private bool _allowAdbHardRestart = true;
    private bool _adbLiteEnabled;
    private bool _killAdbOnExit;
    private bool _adbReplaced;
    private bool _muMu12ExtrasEnabled;
    private string _muMu12EmulatorPath = string.Empty;
    private bool _muMuBridgeConnection;
    private string _muMu12Index = "0";
    private bool _ldPlayerExtrasEnabled;
    private string _ldPlayerEmulatorPath = string.Empty;
    private bool _ldPlayerManualSetIndex;
    private string _ldPlayerIndex = "0";
    private string _attachWindowScreencapMethod = DefaultAttachWindowScreencapMethod;
    private string _attachWindowMouseMethod = DefaultAttachWindowInputMethod;
    private string _attachWindowKeyboardMethod = DefaultAttachWindowInputMethod;
    private string _testLinkInfo = string.Empty;
    private string _screencapCost = DefaultScreencapCostText;
    private IReadOnlyList<ConnectionGameOptionItem> _connectConfigOptions = [];
    private IReadOnlyList<ConnectionGameOptionItem> _clientTypeOptions = [];
    private IReadOnlyList<ConnectionGameOptionItem> _touchModeOptions = [];
    private IReadOnlyList<ConnectionGameOptionItem> _attachWindowScreencapOptions = [];
    private IReadOnlyList<ConnectionGameOptionItem> _attachWindowInputOptions = [];
    private readonly ObservableCollection<string> _connectAddressHistory = [];

    public ConnectionGameSharedStateViewModel()
    {
        RebuildOptions();
        UpdateConnectAddressHistory(_connectAddress);
    }

    public IReadOnlyList<ConnectionGameOptionItem> ConnectConfigOptions
    {
        get => _connectConfigOptions;
        private set => SetProperty(ref _connectConfigOptions, value);
    }

    public IReadOnlyList<ConnectionGameOptionItem> ClientTypeOptions
    {
        get => _clientTypeOptions;
        private set => SetProperty(ref _clientTypeOptions, value);
    }

    public IReadOnlyList<ConnectionGameOptionItem> TouchModeOptions
    {
        get => _touchModeOptions;
        private set => SetProperty(ref _touchModeOptions, value);
    }

    public IReadOnlyList<ConnectionGameOptionItem> AttachWindowScreencapOptions
    {
        get => _attachWindowScreencapOptions;
        private set => SetProperty(ref _attachWindowScreencapOptions, value);
    }

    public IReadOnlyList<ConnectionGameOptionItem> AttachWindowInputOptions
    {
        get => _attachWindowInputOptions;
        private set => SetProperty(ref _attachWindowInputOptions, value);
    }

    public ObservableCollection<string> ConnectAddressHistory => _connectAddressHistory;

    public ConnectionGameOptionItem? SelectedConnectConfigOption
    {
        get => ResolveSelectedOption(ConnectConfigOptions, ConnectConfig, NormalizeConnectConfigAlias);
        set
        {
            if (value is null)
            {
                return;
            }

            ConnectConfig = value.Value;
        }
    }

    public ConnectionGameOptionItem? SelectedClientTypeOption
    {
        get => ResolveSelectedOption(ClientTypeOptions, ClientType, NormalizeClientTypeAlias);
        set
        {
            if (value is null)
            {
                return;
            }

            ClientType = value.Value;
        }
    }

    public ConnectionGameOptionItem? SelectedTouchModeOption
    {
        get => ResolveSelectedOption(TouchModeOptions, TouchMode, NormalizeTouchModeAlias);
        set
        {
            if (value is null)
            {
                return;
            }

            TouchMode = value.Value;
        }
    }

    public ConnectionGameOptionItem? SelectedAttachWindowScreencapOption
    {
        get => ResolveSelectedOption(AttachWindowScreencapOptions, AttachWindowScreencapMethod, NormalizeTouchModeAlias);
        set
        {
            if (value is null)
            {
                return;
            }

            AttachWindowScreencapMethod = value.Value;
        }
    }

    public ConnectionGameOptionItem? SelectedAttachWindowMouseOption
    {
        get => ResolveSelectedOption(AttachWindowInputOptions, AttachWindowMouseMethod, NormalizeTouchModeAlias);
        set
        {
            if (value is null)
            {
                return;
            }

            AttachWindowMouseMethod = value.Value;
        }
    }

    public ConnectionGameOptionItem? SelectedAttachWindowKeyboardOption
    {
        get => ResolveSelectedOption(AttachWindowInputOptions, AttachWindowKeyboardMethod, NormalizeTouchModeAlias);
        set
        {
            if (value is null)
            {
                return;
            }

            AttachWindowKeyboardMethod = value.Value;
        }
    }

    public void SetLanguage(string? language)
    {
        var normalized = UiLanguageCatalog.Normalize(language);
        if (string.Equals(_language, normalized, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        _language = normalized;
        RebuildOptions();
    }

    public string ConnectAddress
    {
        get => _connectAddress;
        set
        {
            var normalized = (value ?? string.Empty)
                .Replace("：", ":", StringComparison.Ordinal)
                .Replace("；", ":", StringComparison.Ordinal)
                .Replace(";", ":", StringComparison.Ordinal)
                .Trim();
            if (!SetProperty(ref _connectAddress, normalized))
            {
                return;
            }

            UpdateConnectAddressHistory(normalized);
        }
    }

    public string ConnectConfig
    {
        get => _connectConfig;
        set
        {
            var normalized = (value ?? string.Empty).Trim();
            if (!SetProperty(ref _connectConfig, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(CanStartGameEnabled));
            OnPropertyChanged(nameof(SelectedConnectConfigOption));
            OnPropertyChanged(nameof(IsAttachWindowMode));
            OnPropertyChanged(nameof(IsAdbConnectionMode));
            OnPropertyChanged(nameof(IsMuMuEmulator12Mode));
            OnPropertyChanged(nameof(IsLdPlayerMode));
            OnPropertyChanged(nameof(ShowMuMuExtrasSection));
            OnPropertyChanged(nameof(ShowLdPlayerExtrasSection));
            OnPropertyChanged(nameof(CanEditAdbConnectionFields));
            if (string.Equals(_connectConfig, "PC", StringComparison.OrdinalIgnoreCase))
            {
                StartGameEnabled = false;
            }
        }
    }

    public string AdbPath
    {
        get => _adbPath;
        set
        {
            var normalized = (value ?? string.Empty).Trim();
            SetProperty(ref _adbPath, normalized);
        }
    }

    public string ClientType
    {
        get => _clientType;
        set
        {
            var normalized = (value ?? string.Empty).Trim();
            if (SetProperty(ref _clientType, normalized))
            {
                OnPropertyChanged(nameof(SelectedClientTypeOption));
            }
        }
    }

    public bool StartGameEnabled
    {
        get => _startGameEnabled;
        set
        {
            if (string.Equals(ConnectConfig, "PC", StringComparison.OrdinalIgnoreCase) && value)
            {
                value = false;
            }

            SetProperty(ref _startGameEnabled, value);
        }
    }

    public bool CanStartGameEnabled => !string.Equals(ConnectConfig, "PC", StringComparison.OrdinalIgnoreCase);

    public bool CanEditConnectionFields => !AutoDetect;

    public bool CanEditAdbConnectionFields => !AutoDetect && !IsAttachWindowMode;

    public bool IsAttachWindowMode => string.Equals(ConnectConfig, "PC", StringComparison.OrdinalIgnoreCase);

    public bool IsAdbConnectionMode => !IsAttachWindowMode;

    public bool IsMuMuEmulator12Mode => string.Equals(ConnectConfig, "MuMuEmulator12", StringComparison.OrdinalIgnoreCase);

    public bool IsLdPlayerMode => string.Equals(ConnectConfig, "LDPlayer", StringComparison.OrdinalIgnoreCase);

    public bool ShowMuMuExtrasSection => IsAdbConnectionMode && IsMuMuEmulator12Mode;

    public bool ShowLdPlayerExtrasSection => IsAdbConnectionMode && IsLdPlayerMode;

    public string TouchMode
    {
        get => _touchMode;
        set
        {
            var normalized = (value ?? string.Empty).Trim();
            if (SetProperty(ref _touchMode, normalized))
            {
                OnPropertyChanged(nameof(SelectedTouchModeOption));
            }
        }
    }

    public bool AutoDetect
    {
        get => _autoDetect;
        set
        {
            if (SetProperty(ref _autoDetect, value))
            {
                OnPropertyChanged(nameof(CanEditConnectionFields));
                OnPropertyChanged(nameof(CanEditAdbConnectionFields));
            }
        }
    }

    public bool AlwaysAutoDetect
    {
        get => _alwaysAutoDetect;
        set => SetProperty(ref _alwaysAutoDetect, value);
    }

    public bool RetryOnDisconnected
    {
        get => _retryOnDisconnected;
        set => SetProperty(ref _retryOnDisconnected, value);
    }

    public bool AllowAdbRestart
    {
        get => _allowAdbRestart;
        set => SetProperty(ref _allowAdbRestart, value);
    }

    public bool AllowAdbHardRestart
    {
        get => _allowAdbHardRestart;
        set => SetProperty(ref _allowAdbHardRestart, value);
    }

    public bool AdbLiteEnabled
    {
        get => _adbLiteEnabled;
        set => SetProperty(ref _adbLiteEnabled, value);
    }

    public bool KillAdbOnExit
    {
        get => _killAdbOnExit;
        set => SetProperty(ref _killAdbOnExit, value);
    }

    public bool AdbReplaced
    {
        get => _adbReplaced;
        set => SetProperty(ref _adbReplaced, value);
    }

    public bool MuMu12ExtrasEnabled
    {
        get => _muMu12ExtrasEnabled;
        set => SetProperty(ref _muMu12ExtrasEnabled, value);
    }

    public string MuMu12EmulatorPath
    {
        get => _muMu12EmulatorPath;
        set => SetProperty(ref _muMu12EmulatorPath, (value ?? string.Empty).Trim());
    }

    public bool MuMuBridgeConnection
    {
        get => _muMuBridgeConnection;
        set => SetProperty(ref _muMuBridgeConnection, value);
    }

    public string MuMu12Index
    {
        get => _muMu12Index;
        set => SetProperty(ref _muMu12Index, (value ?? string.Empty).Trim());
    }

    public bool LdPlayerExtrasEnabled
    {
        get => _ldPlayerExtrasEnabled;
        set => SetProperty(ref _ldPlayerExtrasEnabled, value);
    }

    public string LdPlayerEmulatorPath
    {
        get => _ldPlayerEmulatorPath;
        set => SetProperty(ref _ldPlayerEmulatorPath, (value ?? string.Empty).Trim());
    }

    public bool LdPlayerManualSetIndex
    {
        get => _ldPlayerManualSetIndex;
        set => SetProperty(ref _ldPlayerManualSetIndex, value);
    }

    public string LdPlayerIndex
    {
        get => _ldPlayerIndex;
        set => SetProperty(ref _ldPlayerIndex, (value ?? string.Empty).Trim());
    }

    public string AttachWindowScreencapMethod
    {
        get => _attachWindowScreencapMethod;
        set
        {
            var normalized = (value ?? string.Empty).Trim();
            if (SetProperty(ref _attachWindowScreencapMethod, normalized))
            {
                OnPropertyChanged(nameof(SelectedAttachWindowScreencapOption));
            }
        }
    }

    public string AttachWindowMouseMethod
    {
        get => _attachWindowMouseMethod;
        set
        {
            var normalized = (value ?? string.Empty).Trim();
            if (SetProperty(ref _attachWindowMouseMethod, normalized))
            {
                OnPropertyChanged(nameof(SelectedAttachWindowMouseOption));
            }
        }
    }

    public string AttachWindowKeyboardMethod
    {
        get => _attachWindowKeyboardMethod;
        set
        {
            var normalized = (value ?? string.Empty).Trim();
            if (SetProperty(ref _attachWindowKeyboardMethod, normalized))
            {
                OnPropertyChanged(nameof(SelectedAttachWindowKeyboardOption));
            }
        }
    }

    public string TestLinkInfo
    {
        get => _testLinkInfo;
        set
        {
            if (SetProperty(ref _testLinkInfo, value ?? string.Empty))
            {
                OnPropertyChanged(nameof(HasTestLinkInfo));
            }
        }
    }

    public bool HasTestLinkInfo => !string.IsNullOrWhiteSpace(TestLinkInfo);

    public string ScreencapCost
    {
        get => _screencapCost;
        set => SetProperty(ref _screencapCost, string.IsNullOrWhiteSpace(value) ? DefaultScreencapCostText : value);
    }

    public void UpdateScreencapCost(long min, long avg, long max, DateTimeOffset timestamp)
    {
        ScreencapCost = FormatScreencapCost(min, avg, max, timestamp);
    }

    public static string FormatScreencapCost(long min, long avg, long max, DateTimeOffset timestamp)
    {
        return $"截图耗时 min/avg/max(ms): {min} / {avg} / {max} ({timestamp.ToLocalTime():HH:mm:ss})";
    }

    public IReadOnlyList<string> BuildConnectAddressCandidates(bool includeConfiguredAddress = true)
    {
        var candidates = new List<string>();
        if (includeConfiguredAddress)
        {
            AddAddressCandidate(candidates, ConnectAddress);
        }

        if (AutoDetect || AlwaysAutoDetect)
        {
            foreach (var fallbackAddress in GetDefaultAddressesForCurrentConfig())
            {
                AddAddressCandidate(candidates, fallbackAddress);
            }
        }

        if (candidates.Count == 0)
        {
            foreach (var fallbackAddress in GetDefaultAddressesForCurrentConfig())
            {
                AddAddressCandidate(candidates, fallbackAddress);
            }
        }

        return candidates;
    }

    public string? BuildConnectionSettingsHintMessage()
    {
        if (string.IsNullOrWhiteSpace(ConnectAddress) && !AutoDetect && !AlwaysAutoDetect)
        {
            return BuildBilingualMessage(
                "连接地址为空，请填写“IP:端口”后再连接。",
                "Connection address is empty. Enter \"IP:port\" and try again.");
        }

        return BuildAdbPathHintMessage();
    }

    public string? BuildAdbPathHintMessage()
    {
        var normalized = (AdbPath ?? string.Empty).Trim();
        if (string.IsNullOrEmpty(normalized))
        {
            return null;
        }

        if (File.Exists(normalized))
        {
            return null;
        }

        if (Directory.Exists(normalized))
        {
            if (string.IsNullOrEmpty(TryFindAdbUnderDirectory(normalized)))
            {
                return BuildBilingualMessage(
                    $"ADB 路径是目录，但目录内未找到 adb 可执行文件：{normalized}",
                    $"ADB path is a directory, but adb executable was not found in it: {normalized}");
            }

            return null;
        }

        if (IsWindowsDrivePathMissingSlash(normalized))
        {
            return BuildBilingualMessage(
                $"ADB 路径格式可能不正确（盘符后缺少斜杠）：{normalized}",
                $"ADB path format looks invalid (missing slash after drive letter): {normalized}");
        }

        if (!OperatingSystem.IsWindows() && LooksLikeWindowsPath(normalized))
        {
            return BuildBilingualMessage(
                $"当前系统是 {GetPlatformDisplayName()}，但 ADB 路径看起来是 Windows 路径：{normalized}",
                $"Current system is {GetPlatformDisplayName()}, but ADB path looks like a Windows path: {normalized}");
        }

        return BuildBilingualMessage(
            $"ADB 路径不存在：{normalized}",
            $"ADB path does not exist: {normalized}");
    }

    public string? ResolveEffectiveAdbPath(bool updateStateWhenResolved = false)
    {
        var normalized = (AdbPath ?? string.Empty).Trim();
        if (string.IsNullOrEmpty(normalized))
        {
            return null;
        }

        if (ShouldIgnoreCustomAdbPathForCurrentPlatform(normalized))
        {
            return null;
        }

        var resolved = ResolveAdbPathCore(normalized);
        if (string.IsNullOrEmpty(resolved))
        {
            return normalized;
        }

        if (updateStateWhenResolved
            && !string.Equals(normalized, resolved, StringComparison.Ordinal))
        {
            AdbPath = resolved;
        }

        return resolved;
    }

    public void RemoveAddressFromHistory(string? address)
    {
        if (string.IsNullOrWhiteSpace(address))
        {
            return;
        }

        for (var index = _connectAddressHistory.Count - 1; index >= 0; index--)
        {
            if (string.Equals(_connectAddressHistory[index], address, StringComparison.OrdinalIgnoreCase))
            {
                _connectAddressHistory.RemoveAt(index);
            }
        }
    }

    private void UpdateConnectAddressHistory(string address)
    {
        if (string.IsNullOrWhiteSpace(address))
        {
            return;
        }

        for (var index = _connectAddressHistory.Count - 1; index >= 0; index--)
        {
            if (string.Equals(_connectAddressHistory[index], address, StringComparison.OrdinalIgnoreCase))
            {
                _connectAddressHistory.RemoveAt(index);
            }
        }

        _connectAddressHistory.Insert(0, address);
        while (_connectAddressHistory.Count > MaxConnectAddressHistory)
        {
            _connectAddressHistory.RemoveAt(_connectAddressHistory.Count - 1);
        }
    }

    private static string NormalizeConnectConfigAlias(string normalized)
    {
        if (string.Equals(normalized, "Mumu", StringComparison.OrdinalIgnoreCase))
        {
            return "MuMuEmulator12";
        }

        return normalized;
    }

    private static string NormalizeClientTypeAlias(string normalized)
    {
        if (string.Equals(normalized, "Txwy", StringComparison.OrdinalIgnoreCase))
        {
            return "txwy";
        }

        return normalized;
    }

    private static string NormalizeTouchModeAlias(string normalized)
    {
        return normalized;
    }

    private void RebuildOptions()
    {
        ConnectConfigOptions = SettingsOptionCatalog.BuildConnectConfigOptions(_language);
        ClientTypeOptions = SettingsOptionCatalog.BuildClientTypeOptions(_language);
        TouchModeOptions = SettingsOptionCatalog.BuildTouchModeOptions(_language);
        AttachWindowScreencapOptions = SettingsOptionCatalog.BuildAttachWindowScreencapOptions(_language);
        AttachWindowInputOptions = SettingsOptionCatalog.BuildAttachWindowInputOptions(_language);

        OnPropertyChanged(nameof(SelectedConnectConfigOption));
        OnPropertyChanged(nameof(SelectedClientTypeOption));
        OnPropertyChanged(nameof(SelectedTouchModeOption));
        OnPropertyChanged(nameof(SelectedAttachWindowScreencapOption));
        OnPropertyChanged(nameof(SelectedAttachWindowMouseOption));
        OnPropertyChanged(nameof(SelectedAttachWindowKeyboardOption));
    }

    private static ConnectionGameOptionItem? ResolveSelectedOption(
        IReadOnlyList<ConnectionGameOptionItem> options,
        string? value,
        Func<string, string> normalizeAlias)
    {
        var normalized = normalizeAlias((value ?? string.Empty).Trim());
        if (string.IsNullOrEmpty(normalized))
        {
            return options.FirstOrDefault();
        }

        foreach (var option in options)
        {
            if (string.Equals(option.Value, normalized, StringComparison.OrdinalIgnoreCase))
            {
                return option;
            }
        }

        return new ConnectionGameOptionItem(normalized, normalized);
    }

    private static string? ResolveAdbPathCore(string input)
    {
        if (File.Exists(input))
        {
            return input;
        }

        if (Directory.Exists(input))
        {
            var fromDirectory = TryFindAdbUnderDirectory(input);
            if (!string.IsNullOrEmpty(fromDirectory))
            {
                return fromDirectory;
            }
        }

        var directory = Path.GetDirectoryName(input);
        if (!string.IsNullOrEmpty(directory) && Directory.Exists(directory))
        {
            var fileName = Path.GetFileName(input);
            if (string.Equals(fileName, "adb", StringComparison.OrdinalIgnoreCase)
                || string.Equals(fileName, "adb.exe", StringComparison.OrdinalIgnoreCase))
            {
                return input;
            }
        }

        return string.Empty;
    }

    private static string? TryFindAdbUnderDirectory(string directory)
    {
        var candidateDirectories = new[]
        {
            directory,
            Path.Combine(directory, "platform-tools"),
            Path.Combine(directory, "shell"),
        };
        var candidates = candidateDirectories
            .Where(Directory.Exists)
            .SelectMany(dir => GetAdbFileNameCandidates().Select(file => Path.Combine(dir, file)))
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToArray();

        foreach (var candidate in candidates)
        {
            if (File.Exists(candidate))
            {
                return candidate;
            }
        }

        return null;
    }

    private IReadOnlyList<string> GetDefaultAddressesForCurrentConfig()
    {
        return _defaultAddressByConnectConfig.TryGetValue(ConnectConfig, out var addresses)
            ? addresses
            : [string.Empty];
    }

    private static void AddAddressCandidate(ICollection<string> candidates, string? rawAddress)
    {
        var normalized = (rawAddress ?? string.Empty)
            .Replace("：", ":", StringComparison.Ordinal)
            .Replace("；", ":", StringComparison.Ordinal)
            .Replace(";", ":", StringComparison.Ordinal)
            .Trim();
        if (string.IsNullOrWhiteSpace(normalized))
        {
            return;
        }

        if (candidates.Contains(normalized, StringComparer.OrdinalIgnoreCase))
        {
            return;
        }

        candidates.Add(normalized);
    }

    private static bool LooksLikeWindowsPath(string path)
    {
        return path.Length >= 2 && char.IsLetter(path[0]) && path[1] == ':';
    }

    private static bool ShouldIgnoreCustomAdbPathForCurrentPlatform(string path)
    {
        return !OperatingSystem.IsWindows() && LooksLikeWindowsPath(path);
    }

    private static bool IsWindowsDrivePathMissingSlash(string path)
    {
        return path.Length >= 3
               && char.IsLetter(path[0])
               && path[1] == ':'
               && path[2] != '\\'
               && path[2] != '/';
    }

    private static string GetPlatformDisplayName()
    {
        if (OperatingSystem.IsWindows())
        {
            return "Windows";
        }

        if (OperatingSystem.IsMacOS())
        {
            return "macOS";
        }

        if (OperatingSystem.IsLinux())
        {
            return "Linux";
        }

        return "Unknown";
    }

    private static string BuildBilingualMessage(string zh, string en)
    {
        return $"{zh}{Environment.NewLine}{en}";
    }

    private static IReadOnlyList<string> GetAdbFileNameCandidates()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return ["adb.exe", "adb"];
        }

        return ["adb", "adb.exe"];
    }
}

public sealed record ConnectionGameOptionItem(string Value, string DisplayName);
