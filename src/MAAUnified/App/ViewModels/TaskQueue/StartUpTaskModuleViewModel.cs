using System.ComponentModel;
using MAAUnified.App.ViewModels.Settings;
using MAAUnified.Application.Models;
using MAAUnified.Application.Models.TaskParams;
using MAAUnified.Application.Services;
using MAAUnified.Application.Services.TaskParams;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class StartUpTaskModuleViewModel : TypedTaskModuleViewModelBase<StartUpTaskParamsDto>
{
    private readonly ConnectionGameSharedStateViewModel _sharedState;
    private string _accountName = string.Empty;
    private string _attachWindowScreencapMethod = "2";
    private string _attachWindowMouseMethod = "64";
    private string _attachWindowKeyboardMethod = "64";

    public StartUpTaskModuleViewModel(
        MAAUnifiedRuntime runtime,
        LocalizedTextMap texts,
        ConnectionGameSharedStateViewModel sharedState)
        : base(runtime, texts, "TaskQueue.StartUp")
    {
        _sharedState = sharedState;
        _sharedState.PropertyChanged += OnSharedStateChanged;
    }

    public IReadOnlyList<string> ClientTypeOptions => _sharedState.ClientTypeOptions;

    public IReadOnlyList<string> ConnectConfigOptions => _sharedState.ConnectConfigOptions;

    public IReadOnlyList<string> TouchModeOptions => _sharedState.TouchModeOptions;

    public string AccountName
    {
        get => _accountName;
        set => SetTrackedProperty(ref _accountName, value);
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
            if (!string.Equals(normalized, "Official", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(normalized, "Bilibili", StringComparison.OrdinalIgnoreCase))
            {
                SetTrackedProperty(ref _accountName, string.Empty, nameof(AccountName));
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

    public string AttachWindowScreencapMethod
    {
        get => _attachWindowScreencapMethod;
        set => SetTrackedProperty(ref _attachWindowScreencapMethod, value);
    }

    public string AttachWindowMouseMethod
    {
        get => _attachWindowMouseMethod;
        set => SetTrackedProperty(ref _attachWindowMouseMethod, value);
    }

    public string AttachWindowKeyboardMethod
    {
        get => _attachWindowKeyboardMethod;
        set => SetTrackedProperty(ref _attachWindowKeyboardMethod, value);
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
        ClientType = dto.ClientType;
        StartGameEnabled = dto.StartGameEnabled;
        ConnectConfig = dto.ConnectConfig;
        ConnectAddress = dto.ConnectAddress;
        AdbPath = dto.AdbPath;
        TouchMode = dto.TouchMode;
        AutoDetectConnection = dto.AutoDetectConnection;
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
                break;
            case nameof(ConnectionGameSharedStateViewModel.StartGameEnabled):
                OnPropertyChanged(nameof(StartGameEnabled));
                break;
            case nameof(ConnectionGameSharedStateViewModel.TouchMode):
                OnPropertyChanged(nameof(TouchMode));
                break;
            case nameof(ConnectionGameSharedStateViewModel.AutoDetect):
                OnPropertyChanged(nameof(AutoDetectConnection));
                break;
            case nameof(ConnectionGameSharedStateViewModel.CanStartGameEnabled):
                OnPropertyChanged(nameof(CanEditStartGameEnabled));
                break;
        }

        if (!IsApplyingDto && IsTaskBound)
        {
            IsDirty = true;
        }
    }
}
