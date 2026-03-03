using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.TaskQueue;

public sealed class PostActionModuleViewModel : ObservableObject
{
    private readonly MAAUnifiedRuntime _runtime;
    private readonly LocalizedTextMap _texts;
    private CancellationTokenSource? _persistDebounceCts;
    private bool _once;
    private bool _exitArknights;
    private bool _backToAndroidHome;
    private bool _exitEmulator;
    private bool _exitSelf;
    private bool _ifNoOtherMaa;
    private bool _hibernate;
    private bool _shutdown;
    private bool _sleep;
    private string _exitArknightsCommand = string.Empty;
    private string _backToAndroidHomeCommand = string.Empty;
    private string _exitEmulatorCommand = string.Empty;
    private string _exitSelfCommand = string.Empty;
    private string _statusMessage = string.Empty;
    private string _lastErrorMessage = string.Empty;
    private bool _suppressPersist = true;
    private bool _hasPendingCommandPersist;
    private PostActionConfig _persistentConfig = PostActionConfig.Default;

    public PostActionModuleViewModel(MAAUnifiedRuntime runtime, LocalizedTextMap texts)
    {
        _runtime = runtime;
        _texts = texts;
    }

    public LocalizedTextMap Texts => _texts;

    public bool Once
    {
        get => _once;
        set
        {
            if (!SetProperty(ref _once, value))
            {
                return;
            }

            if (!value)
            {
                QueuePersist();
            }
            else
            {
                _ = ValidateAndUpdateWarningAsync(CancellationToken.None);
            }
        }
    }

    public bool ExitArknights
    {
        get => _exitArknights;
        set
        {
            if (!SetProperty(ref _exitArknights, value))
            {
                return;
            }

            if (value)
            {
                BackToAndroidHome = false;
            }

            QueuePersist();
        }
    }

    public bool BackToAndroidHome
    {
        get => _backToAndroidHome;
        set
        {
            if (!SetProperty(ref _backToAndroidHome, value))
            {
                return;
            }

            if (value)
            {
                ExitArknights = false;
            }

            QueuePersist();
        }
    }

    public bool ExitEmulator
    {
        get => _exitEmulator;
        set
        {
            if (!SetProperty(ref _exitEmulator, value))
            {
                return;
            }

            if (value)
            {
                ExitArknights = false;
                BackToAndroidHome = false;
            }
            else
            {
                IfNoOtherMaa = false;
            }

            QueuePersist();
        }
    }

    public bool ExitSelf
    {
        get => _exitSelf;
        set
        {
            if (!SetProperty(ref _exitSelf, value))
            {
                return;
            }

            if (!value)
            {
                IfNoOtherMaa = false;
            }

            QueuePersist();
        }
    }

    public bool IfNoOtherMaa
    {
        get => _ifNoOtherMaa;
        set
        {
            if (!SetProperty(ref _ifNoOtherMaa, value))
            {
                return;
            }

            if (value)
            {
                ExitEmulator = true;
                ExitSelf = true;
            }

            QueuePersist();
        }
    }

    public bool Hibernate
    {
        get => _hibernate;
        set
        {
            if (!SetProperty(ref _hibernate, value))
            {
                return;
            }

            if (value)
            {
                Shutdown = false;
                Sleep = false;
            }
            else if (!Shutdown && !Sleep)
            {
                IfNoOtherMaa = false;
            }

            QueuePersist();
        }
    }

    public bool Shutdown
    {
        get => _shutdown;
        set
        {
            if (!SetProperty(ref _shutdown, value))
            {
                return;
            }

            if (value)
            {
                Hibernate = false;
                Sleep = false;
                ExitArknights = false;
                BackToAndroidHome = false;
            }
            else if (!Hibernate && !Sleep)
            {
                IfNoOtherMaa = false;
            }

            QueuePersist();
        }
    }

    public bool Sleep
    {
        get => _sleep;
        set
        {
            if (!SetProperty(ref _sleep, value))
            {
                return;
            }

            if (value)
            {
                Hibernate = false;
                Shutdown = false;
            }
            else if (!Hibernate && !Shutdown)
            {
                IfNoOtherMaa = false;
            }

            QueuePersist();
        }
    }

    public string ExitArknightsCommand
    {
        get => _exitArknightsCommand;
        set
        {
            if (!SetProperty(ref _exitArknightsCommand, value))
            {
                return;
            }

            QueuePersist(forcePersist: true);
        }
    }

    public string BackToAndroidHomeCommand
    {
        get => _backToAndroidHomeCommand;
        set
        {
            if (!SetProperty(ref _backToAndroidHomeCommand, value))
            {
                return;
            }

            QueuePersist(forcePersist: true);
        }
    }

    public string ExitEmulatorCommand
    {
        get => _exitEmulatorCommand;
        set
        {
            if (!SetProperty(ref _exitEmulatorCommand, value))
            {
                return;
            }

            QueuePersist(forcePersist: true);
        }
    }

    public string ExitSelfCommand
    {
        get => _exitSelfCommand;
        set
        {
            if (!SetProperty(ref _exitSelfCommand, value))
            {
                return;
            }

            QueuePersist(forcePersist: true);
        }
    }

    public string StatusMessage
    {
        get => _statusMessage;
        private set => SetProperty(ref _statusMessage, value);
    }

    public string LastErrorMessage
    {
        get => _lastErrorMessage;
        private set => SetProperty(ref _lastErrorMessage, value);
    }

    public async Task InitializeAsync(CancellationToken cancellationToken = default)
    {
        _suppressPersist = true;
        var result = await _runtime.PostActionFeatureService.LoadAsync(cancellationToken);
        if (!result.Success || result.Value is null)
        {
            LastErrorMessage = result.Message;
            _suppressPersist = false;
            return;
        }

        var config = result.Value;
        _persistentConfig = config.Clone();
        ExitArknights = _persistentConfig.ExitArknights;
        BackToAndroidHome = _persistentConfig.BackToAndroidHome;
        ExitEmulator = _persistentConfig.ExitEmulator;
        ExitSelf = _persistentConfig.ExitSelf;
        IfNoOtherMaa = _persistentConfig.IfNoOtherMaa;
        Hibernate = _persistentConfig.Hibernate;
        Shutdown = _persistentConfig.Shutdown;
        Sleep = _persistentConfig.Sleep;
        ExitArknightsCommand = _persistentConfig.Commands.ExitArknights;
        BackToAndroidHomeCommand = _persistentConfig.Commands.BackToAndroidHome;
        ExitEmulatorCommand = _persistentConfig.Commands.ExitEmulator;
        ExitSelfCommand = _persistentConfig.Commands.ExitSelf;
        _hasPendingCommandPersist = false;
        _once = false;
        OnPropertyChanged(nameof(Once));

        _suppressPersist = false;
        await ValidateAndUpdateWarningAsync(cancellationToken);
    }

    public async Task ReloadPersistentConfigAsync(CancellationToken cancellationToken = default)
    {
        _suppressPersist = true;
        ExitArknights = _persistentConfig.ExitArknights;
        BackToAndroidHome = _persistentConfig.BackToAndroidHome;
        ExitEmulator = _persistentConfig.ExitEmulator;
        ExitSelf = _persistentConfig.ExitSelf;
        IfNoOtherMaa = _persistentConfig.IfNoOtherMaa;
        Hibernate = _persistentConfig.Hibernate;
        Shutdown = _persistentConfig.Shutdown;
        Sleep = _persistentConfig.Sleep;
        ExitArknightsCommand = _persistentConfig.Commands.ExitArknights;
        BackToAndroidHomeCommand = _persistentConfig.Commands.BackToAndroidHome;
        ExitEmulatorCommand = _persistentConfig.Commands.ExitEmulator;
        ExitSelfCommand = _persistentConfig.Commands.ExitSelf;
        _hasPendingCommandPersist = false;
        _once = false;
        OnPropertyChanged(nameof(Once));
        _suppressPersist = false;
        await ValidateAndUpdateWarningAsync(cancellationToken);
    }

    public PostActionConfig BuildRuntimeConfig()
    {
        return new PostActionConfig
        {
            Once = Once,
            ExitArknights = ExitArknights,
            BackToAndroidHome = BackToAndroidHome,
            ExitEmulator = ExitEmulator,
            ExitSelf = ExitSelf,
            IfNoOtherMaa = IfNoOtherMaa,
            Hibernate = Hibernate,
            Shutdown = Shutdown,
            Sleep = Sleep,
            Commands = new PostActionCommandConfig
            {
                ExitArknights = ExitArknightsCommand.Trim(),
                BackToAndroidHome = BackToAndroidHomeCommand.Trim(),
                ExitEmulator = ExitEmulatorCommand.Trim(),
                ExitSelf = ExitSelfCommand.Trim(),
            },
        };
    }

    public void ClearActions()
    {
        ExitArknights = false;
        BackToAndroidHome = false;
        ExitEmulator = false;
        ExitSelf = false;
        IfNoOtherMaa = false;
        Hibernate = false;
        Shutdown = false;
        Sleep = false;
        Once = false;
        QueuePersist();
    }

    private void QueuePersist(bool forcePersist = false)
    {
        if (_suppressPersist)
        {
            return;
        }

        _ = ValidateAndUpdateWarningAsync(CancellationToken.None);
        if (forcePersist)
        {
            _hasPendingCommandPersist = true;
        }

        if (Once && !forcePersist)
        {
            return;
        }

        _persistDebounceCts?.Cancel();
        _persistDebounceCts?.Dispose();
        _persistDebounceCts = new CancellationTokenSource();
        var token = _persistDebounceCts.Token;

        _ = PersistDebouncedAsync(token);
    }

    public async Task<bool> FlushPendingChangesAsync(CancellationToken cancellationToken = default)
    {
        _persistDebounceCts?.Cancel();
        if (Once && !_hasPendingCommandPersist)
        {
            await ValidateAndUpdateWarningAsync(cancellationToken);
            return true;
        }

        return await PersistNowAsync(cancellationToken);
    }

    private async Task PersistDebouncedAsync(CancellationToken cancellationToken)
    {
        try
        {
            await Task.Delay(180, cancellationToken);
            await PersistNowAsync(cancellationToken);
        }
        catch (OperationCanceledException)
        {
            // ignore debounce cancellation
        }
    }

    private async Task<bool> PersistNowAsync(CancellationToken cancellationToken)
    {
        try
        {
            var persistent = BuildPersistentConfigForSave();

            var save = await _runtime.PostActionFeatureService.SaveAsync(persistent, cancellationToken);
            if (!save.Success)
            {
                LastErrorMessage = save.Message;
                await _runtime.DiagnosticsService.RecordFailedResultAsync("PostAction.Save", save, cancellationToken);
                return false;
            }

            _persistentConfig = persistent.Clone();
            _hasPendingCommandPersist = false;
            await ValidateAndUpdateWarningAsync(cancellationToken);
            StatusMessage = save.Message;
            LastErrorMessage = string.Empty;
            return true;
        }
        catch (Exception ex)
        {
            LastErrorMessage = ex.Message;
            await _runtime.DiagnosticsService.RecordErrorAsync("PostAction.Persist", "Unexpected post action persistence failure.", ex, cancellationToken);
            return false;
        }
    }

    private async Task ValidateAndUpdateWarningAsync(CancellationToken cancellationToken)
    {
        var previewResult = await _runtime.PostActionFeatureService.ValidateSelectionAsync(BuildRuntimeConfig(), cancellationToken);
        if (!previewResult.Success || previewResult.Value is null)
        {
            LastErrorMessage = previewResult.Message;
            return;
        }

        if (previewResult.Value.Warnings.Count == 0)
        {
            StatusMessage = string.Empty;
            return;
        }

        var warningText = string.Join(
            " ",
            previewResult.Value.Warnings
                .Select(LocalizeMessageToken)
                .Where(text => !string.IsNullOrWhiteSpace(text)));

        if (previewResult.Value.UnsupportedActions.Count > 0)
        {
            var unsupported = string.Join(", ", previewResult.Value.UnsupportedActions);
            warningText = $"{warningText} {_texts["PostAction.Unsupported"]} ({unsupported})".Trim();
        }

        StatusMessage = warningText;
    }

    private PostActionConfig BuildPersistentConfigForSave()
    {
        var persistent = _persistentConfig.Clone();
        persistent.Commands = new PostActionCommandConfig
        {
            ExitArknights = ExitArknightsCommand.Trim(),
            BackToAndroidHome = BackToAndroidHomeCommand.Trim(),
            ExitEmulator = ExitEmulatorCommand.Trim(),
            ExitSelf = ExitSelfCommand.Trim(),
        };

        if (!Once)
        {
            persistent.ExitArknights = ExitArknights;
            persistent.BackToAndroidHome = BackToAndroidHome;
            persistent.ExitEmulator = ExitEmulator;
            persistent.ExitSelf = ExitSelf;
            persistent.IfNoOtherMaa = IfNoOtherMaa;
            persistent.Hibernate = Hibernate;
            persistent.Shutdown = Shutdown;
            persistent.Sleep = Sleep;
        }

        persistent.Once = false;
        return persistent;
    }

    private string LocalizeMessageToken(string token)
    {
        if (string.IsNullOrWhiteSpace(token))
        {
            return string.Empty;
        }

        return _texts.GetOrDefault(token, token);
    }
}
