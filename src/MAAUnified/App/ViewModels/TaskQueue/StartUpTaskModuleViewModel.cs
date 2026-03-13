using System.Collections.ObjectModel;
using System.ComponentModel;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class StartUpTaskModuleViewModel : TypedTaskModuleViewModelBase<StartUpTaskParamsDto>
{
    private static readonly (string Value, string TextKey, string Fallback)[] ClientTypeOptionSpecs =
    [
        ("Official", "StartUp.Option.ClientType.Official", "Official"),
        ("Bilibili", "StartUp.Option.ClientType.Bilibili", "Bilibili"),
        ("YoStarEN", "StartUp.Option.ClientType.YoStarEN", "YoStarEN"),
        ("YoStarJP", "StartUp.Option.ClientType.YoStarJP", "YoStarJP"),
        ("YoStarKR", "StartUp.Option.ClientType.YoStarKR", "YoStarKR"),
        ("txwy", "StartUp.Option.ClientType.Txwy", "txwy"),
    ];

    private static readonly (string Value, string TextKey, string Fallback)[] ConnectConfigOptionSpecs =
    [
        ("General", "StartUp.Option.ConnectConfig.General", "General Mode"),
        ("BlueStacks", "StartUp.Option.ConnectConfig.BlueStacks", "BlueStacks"),
        ("MuMuEmulator12", "StartUp.Option.ConnectConfig.MuMuEmulator12", "MuMu Emulator 12"),
        ("LDPlayer", "StartUp.Option.ConnectConfig.LDPlayer", "LD Player"),
        ("Nox", "StartUp.Option.ConnectConfig.Nox", "Nox"),
        ("XYAZ", "StartUp.Option.ConnectConfig.XYAZ", "MEmu"),
        ("PC", "StartUp.Option.ConnectConfig.PC", "PC Client"),
        ("WSA", "StartUp.Option.ConnectConfig.WSA", "Old version of WSA"),
        ("Compatible", "StartUp.Option.ConnectConfig.Compatible", "Compatible Mode"),
        ("SecondResolution", "StartUp.Option.ConnectConfig.SecondResolution", "2nd Resolution"),
        ("GeneralWithoutScreencapErr", "StartUp.Option.ConnectConfig.GeneralWithoutScreencapErr", "General Mode (Blocked exception output)"),
    ];

    private static readonly (string Value, string TextKey, string Fallback)[] TouchModeOptionSpecs =
    [
        ("minitouch", "StartUp.Option.TouchMode.MiniTouch", "Minitouch (Default)"),
        ("maatouch", "StartUp.Option.TouchMode.MaaTouch", "MaaTouch (Experimental)"),
        ("adb", "StartUp.Option.TouchMode.AdbTouch", "ADB Input (Deprecated)"),
    ];

    private static readonly (string Value, string TextKey, string Fallback)[] AttachWindowScreencapOptionSpecs =
    [
        ("2", "StartUp.Option.AttachScreencap.FramePool", "FramePool (Default, Background)"),
        ("16", "StartUp.Option.AttachScreencap.PrintWindow", "PrintWindow (Background, Backup 1)"),
        ("32", "StartUp.Option.AttachScreencap.ScreenDC", "ScreenDC (Background, Backup 2)"),
        ("8", "StartUp.Option.AttachScreencap.DesktopWindow", "DesktopWindow (Foreground, More Stable)"),
    ];

    private static readonly (string Value, string TextKey, string Fallback)[] AttachWindowInputOptionSpecs =
    [
        ("1", "StartUp.Option.AttachInput.Seize", "Seize (Foreground, More Stable)"),
        ("64", "StartUp.Option.AttachInput.PostWithCursor", "PostMessageWithCursor (Semi-background)"),
        ("32", "StartUp.Option.AttachInput.SendWithCursor", "SendMessageWithCursor (Semi-background, Backup)"),
        ("256", "StartUp.Option.AttachInput.PostWithWindowPos", "PostMessageWithWindowPos (Background Window)"),
        ("128", "StartUp.Option.AttachInput.SendWithWindowPos", "SendMessageWithWindowPos (Background Window, Backup)"),
    ];

    private readonly ConnectionGameSharedStateViewModel _sharedState;
    private readonly Func<CancellationToken, Task>? _accountSwitchManualRunAction;
    private string _accountName = string.Empty;
    private IReadOnlyList<TaskModuleOption> _clientTypeOptions = [];
    private IReadOnlyList<TaskModuleOption> _connectConfigOptions = [];
    private IReadOnlyList<TaskModuleOption> _touchModeOptions = [];
    private IReadOnlyList<TaskModuleOption> _attachWindowScreencapOptions = [];
    private IReadOnlyList<TaskModuleOption> _attachWindowInputOptions = [];
    private bool _isAccountSwitchRunning;

    public StartUpTaskModuleViewModel(
        MAAUnifiedRuntime runtime,
        LocalizedTextMap texts,
        ConnectionGameSharedStateViewModel sharedState,
        Func<CancellationToken, Task>? accountSwitchManualRunAction = null)
        : base(runtime, texts, "TaskQueue.StartUp")
    {
        _sharedState = sharedState;
        _accountSwitchManualRunAction = accountSwitchManualRunAction;
        _sharedState.PropertyChanged += OnSharedStateChanged;
        Texts.PropertyChanged += OnTextsPropertyChanged;
        RebuildOptionLists();
    }

    public IReadOnlyList<TaskModuleOption> ClientTypeOptions => _clientTypeOptions;

    public IReadOnlyList<TaskModuleOption> ConnectConfigOptions => _connectConfigOptions;

    public IReadOnlyList<TaskModuleOption> TouchModeOptions => _touchModeOptions;

    public IReadOnlyList<TaskModuleOption> AttachWindowScreencapOptions => _attachWindowScreencapOptions;

    public IReadOnlyList<TaskModuleOption> AttachWindowInputOptions => _attachWindowInputOptions;

    public ObservableCollection<string> ConnectAddressHistory => _sharedState.ConnectAddressHistory;

    public TaskModuleOption? SelectedClientTypeOption
    {
        get
        {
            if (string.Equals(ClientType, "Txwy", StringComparison.OrdinalIgnoreCase))
            {
                return new TaskModuleOption(
                    ClientType,
                    Texts.GetOrDefault("StartUp.Option.ClientType.Txwy", "txwy"));
            }

            return ResolveSelectedOption(ClientTypeOptions, ClientType);
        }
        set => ClientType = value?.Type ?? string.Empty;
    }

    public TaskModuleOption? SelectedConnectConfigOption
    {
        get
        {
            if (string.Equals(ConnectConfig, "Mumu", StringComparison.OrdinalIgnoreCase))
            {
                return new TaskModuleOption(
                    ConnectConfig,
                    Texts.GetOrDefault("StartUp.Option.ConnectConfig.MuMuEmulator12", "MuMu Emulator 12"));
            }

            return ResolveSelectedOption(ConnectConfigOptions, ConnectConfig);
        }
        set => ConnectConfig = value?.Type ?? string.Empty;
    }

    public TaskModuleOption? SelectedTouchModeOption
    {
        get => ResolveSelectedOption(TouchModeOptions, TouchMode);
        set => TouchMode = value?.Type ?? string.Empty;
    }

    public TaskModuleOption? SelectedAttachWindowScreencapOption
    {
        get => ResolveSelectedOption(AttachWindowScreencapOptions, AttachWindowScreencapMethod);
        set => AttachWindowScreencapMethod = value?.Type ?? string.Empty;
    }

    public TaskModuleOption? SelectedAttachWindowMouseOption
    {
        get => ResolveSelectedOption(AttachWindowInputOptions, AttachWindowMouseMethod);
        set => AttachWindowMouseMethod = value?.Type ?? string.Empty;
    }

    public TaskModuleOption? SelectedAttachWindowKeyboardOption
    {
        get => ResolveSelectedOption(AttachWindowInputOptions, AttachWindowKeyboardMethod);
        set => AttachWindowKeyboardMethod = value?.Type ?? string.Empty;
    }

    public string AccountName
    {
        get => _accountName;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (!SetTrackedProperty(ref _accountName, normalized))
            {
                return;
            }

            OnPropertyChanged(nameof(CanRunAccountSwitchNow));
        }
    }

    public string ClientType
    {
        get => _sharedState.ClientType;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (string.Equals(_sharedState.ClientType, normalized, StringComparison.Ordinal))
            {
                return;
            }

            _sharedState.ClientType = normalized;
            MarkDirty();
            OnPropertyChanged();
            OnPropertyChanged(nameof(SelectedClientTypeOption));
            OnPropertyChanged(nameof(ShowAccountSwitch));
            OnPropertyChanged(nameof(CanRunAccountSwitchNow));
            if (!IsAccountSwitchSupportedClient(normalized))
            {
                SetTrackedProperty(ref _accountName, string.Empty, nameof(AccountName));
                OnPropertyChanged(nameof(CanRunAccountSwitchNow));
            }
        }
    }

    public bool StartGameEnabled
    {
        get => _sharedState.StartGameEnabled;
        set
        {
            if (_sharedState.StartGameEnabled == value)
            {
                return;
            }

            _sharedState.StartGameEnabled = value;
            MarkDirty();
            OnPropertyChanged();
        }
    }

    public string ConnectConfig
    {
        get => _sharedState.ConnectConfig;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (string.Equals(_sharedState.ConnectConfig, normalized, StringComparison.Ordinal))
            {
                return;
            }

            _sharedState.ConnectConfig = normalized;
            MarkDirty();
            OnPropertyChanged();
            OnPropertyChanged(nameof(SelectedConnectConfigOption));
        }
    }

    public string ConnectAddress
    {
        get => _sharedState.ConnectAddress;
        set
        {
            if (string.Equals(_sharedState.ConnectAddress, value, StringComparison.Ordinal))
            {
                return;
            }

            _sharedState.ConnectAddress = value;
            MarkDirty();
            OnPropertyChanged();
        }
    }

    public string AdbPath
    {
        get => _sharedState.AdbPath;
        set
        {
            if (string.Equals(_sharedState.AdbPath, value, StringComparison.Ordinal))
            {
                return;
            }

            _sharedState.AdbPath = value;
            MarkDirty();
            OnPropertyChanged();
        }
    }

    public string TouchMode
    {
        get => _sharedState.TouchMode;
        set
        {
            if (string.Equals(_sharedState.TouchMode, value, StringComparison.Ordinal))
            {
                return;
            }

            _sharedState.TouchMode = value;
            MarkDirty();
            OnPropertyChanged();
            OnPropertyChanged(nameof(SelectedTouchModeOption));
        }
    }

    public bool AutoDetectConnection
    {
        get => _sharedState.AutoDetect;
        set
        {
            if (_sharedState.AutoDetect == value)
            {
                return;
            }

            _sharedState.AutoDetect = value;
            MarkDirty();
            OnPropertyChanged();
        }
    }

    public bool CanEditStartGameEnabled => _sharedState.CanStartGameEnabled;

    public bool ShowAccountSwitch => IsAccountSwitchSupportedClient(ClientType);

    public bool CanRunAccountSwitchNow =>
        ShowAccountSwitch
        && !string.IsNullOrWhiteSpace(AccountName)
        && !_isAccountSwitchRunning;

    public string AttachWindowScreencapMethod
    {
        get => _sharedState.AttachWindowScreencapMethod;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (string.Equals(_sharedState.AttachWindowScreencapMethod, normalized, StringComparison.Ordinal))
            {
                return;
            }

            _sharedState.AttachWindowScreencapMethod = normalized;
            MarkDirty();
            OnPropertyChanged();
            OnPropertyChanged(nameof(SelectedAttachWindowScreencapOption));
        }
    }

    public string AttachWindowMouseMethod
    {
        get => _sharedState.AttachWindowMouseMethod;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (string.Equals(_sharedState.AttachWindowMouseMethod, normalized, StringComparison.Ordinal))
            {
                return;
            }

            _sharedState.AttachWindowMouseMethod = normalized;
            MarkDirty();
            OnPropertyChanged();
            OnPropertyChanged(nameof(SelectedAttachWindowMouseOption));
        }
    }

    public string AttachWindowKeyboardMethod
    {
        get => _sharedState.AttachWindowKeyboardMethod;
        set
        {
            var normalized = value?.Trim() ?? string.Empty;
            if (string.Equals(_sharedState.AttachWindowKeyboardMethod, normalized, StringComparison.Ordinal))
            {
                return;
            }

            _sharedState.AttachWindowKeyboardMethod = normalized;
            MarkDirty();
            OnPropertyChanged();
            OnPropertyChanged(nameof(SelectedAttachWindowKeyboardOption));
        }
    }

    public async Task RunAccountSwitchManualAsync(CancellationToken cancellationToken = default)
    {
        if (!CanRunAccountSwitchNow || _accountSwitchManualRunAction is null)
        {
            return;
        }

        _isAccountSwitchRunning = true;
        OnPropertyChanged(nameof(CanRunAccountSwitchNow));
        try
        {
            await _accountSwitchManualRunAction(cancellationToken);
        }
        finally
        {
            _isAccountSwitchRunning = false;
            OnPropertyChanged(nameof(CanRunAccountSwitchNow));
        }
    }

    protected override Task<UiOperationResult<StartUpTaskParamsDto>> LoadDtoAsync(int index, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.GetStartUpParamsAsync(index, cancellationToken);
    }

    protected override Task<UiOperationResult> SaveDtoAsync(int index, StartUpTaskParamsDto dto, CancellationToken cancellationToken)
    {
        return Runtime.TaskQueueFeatureService.SaveStartUpParamsAsync(index, dto, cancellationToken);
    }

    protected override TaskCompileOutput CompileDto(StartUpTaskParamsDto dto, UnifiedProfile profile, UnifiedConfig config)
    {
        return TaskParamCompiler.CompileStartUp(dto, profile, config);
    }

    protected override void ApplyDto(StartUpTaskParamsDto dto)
    {
        AccountName = dto.AccountName;
        AttachWindowScreencapMethod = dto.AttachWindowScreencapMethod;
        AttachWindowMouseMethod = dto.AttachWindowMouseMethod;
        AttachWindowKeyboardMethod = dto.AttachWindowKeyboardMethod;
    }

    protected override StartUpTaskParamsDto BuildDto()
    {
        return new StartUpTaskParamsDto
        {
            AccountName = AccountName.Trim(),
            ClientType = ClientType.Trim(),
            StartGameEnabled = StartGameEnabled,
            ConnectConfig = ConnectConfig.Trim(),
            ConnectAddress = ConnectAddress.Trim(),
            AdbPath = AdbPath.Trim(),
            TouchMode = TouchMode.Trim(),
            AutoDetectConnection = AutoDetectConnection,
            AttachWindowScreencapMethod = AttachWindowScreencapMethod.Trim(),
            AttachWindowMouseMethod = AttachWindowMouseMethod.Trim(),
            AttachWindowKeyboardMethod = AttachWindowKeyboardMethod.Trim(),
        };
    }

    private void OnSharedStateChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (string.IsNullOrEmpty(e.PropertyName))
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(ConnectionGameSharedStateViewModel.ConnectConfig):
                OnPropertyChanged(nameof(ConnectConfig));
                OnPropertyChanged(nameof(SelectedConnectConfigOption));
                OnPropertyChanged(nameof(CanEditStartGameEnabled));
                break;
            case nameof(ConnectionGameSharedStateViewModel.ConnectAddress):
                OnPropertyChanged(nameof(ConnectAddress));
                break;
            case nameof(ConnectionGameSharedStateViewModel.AdbPath):
                OnPropertyChanged(nameof(AdbPath));
                break;
            case nameof(ConnectionGameSharedStateViewModel.ClientType):
                OnPropertyChanged(nameof(ClientType));
                OnPropertyChanged(nameof(SelectedClientTypeOption));
                OnPropertyChanged(nameof(ShowAccountSwitch));
                OnPropertyChanged(nameof(CanRunAccountSwitchNow));
                break;
            case nameof(ConnectionGameSharedStateViewModel.StartGameEnabled):
                OnPropertyChanged(nameof(StartGameEnabled));
                break;
            case nameof(ConnectionGameSharedStateViewModel.TouchMode):
                OnPropertyChanged(nameof(TouchMode));
                OnPropertyChanged(nameof(SelectedTouchModeOption));
                break;
            case nameof(ConnectionGameSharedStateViewModel.AutoDetect):
                OnPropertyChanged(nameof(AutoDetectConnection));
                break;
            case nameof(ConnectionGameSharedStateViewModel.CanStartGameEnabled):
                OnPropertyChanged(nameof(CanEditStartGameEnabled));
                break;
            case nameof(ConnectionGameSharedStateViewModel.AttachWindowScreencapMethod):
                OnPropertyChanged(nameof(AttachWindowScreencapMethod));
                OnPropertyChanged(nameof(SelectedAttachWindowScreencapOption));
                break;
            case nameof(ConnectionGameSharedStateViewModel.AttachWindowMouseMethod):
                OnPropertyChanged(nameof(AttachWindowMouseMethod));
                OnPropertyChanged(nameof(SelectedAttachWindowMouseOption));
                break;
            case nameof(ConnectionGameSharedStateViewModel.AttachWindowKeyboardMethod):
                OnPropertyChanged(nameof(AttachWindowKeyboardMethod));
                OnPropertyChanged(nameof(SelectedAttachWindowKeyboardOption));
                break;
        }

        if (!IsApplyingDto && IsTaskBound)
        {
            IsDirty = true;
        }
    }

    private void OnTextsPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!string.IsNullOrEmpty(e.PropertyName)
            && !string.Equals(e.PropertyName, nameof(LocalizedTextMap.Language), StringComparison.Ordinal)
            && !string.Equals(e.PropertyName, "Item[]", StringComparison.Ordinal))
        {
            return;
        }

        RebuildOptionLists();
    }

    private void RebuildOptionLists()
    {
        _clientTypeOptions = BuildOptions(ClientTypeOptionSpecs);
        _connectConfigOptions = BuildOptions(ConnectConfigOptionSpecs);
        _touchModeOptions = BuildOptions(TouchModeOptionSpecs);
        _attachWindowScreencapOptions = BuildOptions(AttachWindowScreencapOptionSpecs);
        _attachWindowInputOptions = BuildOptions(AttachWindowInputOptionSpecs);

        OnPropertyChanged(nameof(ClientTypeOptions));
        OnPropertyChanged(nameof(ConnectConfigOptions));
        OnPropertyChanged(nameof(TouchModeOptions));
        OnPropertyChanged(nameof(AttachWindowScreencapOptions));
        OnPropertyChanged(nameof(AttachWindowInputOptions));
        OnPropertyChanged(nameof(SelectedClientTypeOption));
        OnPropertyChanged(nameof(SelectedConnectConfigOption));
        OnPropertyChanged(nameof(SelectedTouchModeOption));
        OnPropertyChanged(nameof(SelectedAttachWindowScreencapOption));
        OnPropertyChanged(nameof(SelectedAttachWindowMouseOption));
        OnPropertyChanged(nameof(SelectedAttachWindowKeyboardOption));
    }

    private IReadOnlyList<TaskModuleOption> BuildOptions(
        IEnumerable<(string Value, string TextKey, string Fallback)> specs)
    {
        return specs
            .Select(spec => new TaskModuleOption(spec.Value, Texts.GetOrDefault(spec.TextKey, spec.Fallback)))
            .ToArray();
    }

    private static TaskModuleOption? ResolveSelectedOption(
        IReadOnlyList<TaskModuleOption> options,
        string? value)
    {
        var normalized = value?.Trim() ?? string.Empty;
        if (string.IsNullOrEmpty(normalized))
        {
            return options.FirstOrDefault();
        }

        foreach (var option in options)
        {
            if (string.Equals(option.Type, normalized, StringComparison.OrdinalIgnoreCase))
            {
                return option;
            }
        }

        return new TaskModuleOption(normalized, normalized);
    }

    private static bool IsAccountSwitchSupportedClient(string? clientType)
    {
        return string.Equals(clientType, "Official", StringComparison.OrdinalIgnoreCase)
               || string.Equals(clientType, "Bilibili", StringComparison.OrdinalIgnoreCase);
    }
}
