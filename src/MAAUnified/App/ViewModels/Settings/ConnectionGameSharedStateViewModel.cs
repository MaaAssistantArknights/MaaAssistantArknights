using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.ViewModels.Settings;

public sealed class ConnectionGameSharedStateViewModel : ObservableObject
{
    private string _connectAddress = "127.0.0.1:5555";
    private string _connectConfig = "General";
    private string _adbPath = string.Empty;
    private string _clientType = "Official";
    private bool _startGameEnabled = true;
    private string _touchMode = "minitouch";
    private bool _autoDetect = true;

    public IReadOnlyList<string> ConnectConfigOptions { get; } =
        ["General", "Mumu", "LDPlayer", "BlueStacks", "PC"];

    public IReadOnlyList<string> ClientTypeOptions { get; } =
        ["Official", "Bilibili", "Txwy", "YoStarEN", "YoStarJP", "YoStarKR"];

    public IReadOnlyList<string> TouchModeOptions { get; } =
        ["minitouch", "maatouch", "adb"];

    public string ConnectAddress
    {
        get => _connectAddress;
        set => SetProperty(ref _connectAddress, value);
    }

    public string ConnectConfig
    {
        get => _connectConfig;
        set
        {
            if (!SetProperty(ref _connectConfig, value))
            {
                return;
            }

            OnPropertyChanged(nameof(CanStartGameEnabled));
            if (string.Equals(_connectConfig, "PC", StringComparison.OrdinalIgnoreCase))
            {
                StartGameEnabled = false;
            }
        }
    }

    public string AdbPath
    {
        get => _adbPath;
        set => SetProperty(ref _adbPath, value);
    }

    public string ClientType
    {
        get => _clientType;
        set => SetProperty(ref _clientType, value);
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

    public string TouchMode
    {
        get => _touchMode;
        set => SetProperty(ref _touchMode, value);
    }

    public bool AutoDetect
    {
        get => _autoDetect;
        set => SetProperty(ref _autoDetect, value);
    }
}
