using System.Text.Json.Nodes;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.TaskQueue;

public abstract class TaskModuleSettingsViewModelBase : ObservableObject
{
    private readonly SemaphoreSlim _persistLock = new(1, 1);
    private CancellationTokenSource? _persistDebounceCts;
    private bool _isTaskBound;
    private bool _isAdvancedMode;
    private string _statusMessage = string.Empty;
    private string _lastErrorMessage = string.Empty;
    private int _boundTaskIndex = -1;
    private bool _suppressPersist = true;

    protected TaskModuleSettingsViewModelBase(MAAUnifiedRuntime runtime, LocalizedTextMap texts, string moduleType)
    {
        Runtime = runtime;
        Texts = texts;
        ModuleType = moduleType;
    }

    protected MAAUnifiedRuntime Runtime { get; }

    public LocalizedTextMap Texts { get; }

    protected string ModuleType { get; }

    public bool IsTaskBound
    {
        get => _isTaskBound;
        private set => SetProperty(ref _isTaskBound, value);
    }

    public bool IsAdvancedMode
    {
        get => _isAdvancedMode;
        set
        {
            if (!SetProperty(ref _isAdvancedMode, value))
            {
                return;
            }

            OnPropertyChanged(nameof(IsGeneralMode));
        }
    }

    public bool IsGeneralMode => !IsAdvancedMode;

    public string StatusMessage
    {
        get => _statusMessage;
        protected set => SetProperty(ref _statusMessage, value);
    }

    public string LastErrorMessage
    {
        get => _lastErrorMessage;
        protected set => SetProperty(ref _lastErrorMessage, value);
    }

    public async Task BindAsync(int taskIndex, JsonObject parameters, CancellationToken cancellationToken = default)
    {
        _boundTaskIndex = taskIndex;
        _suppressPersist = true;
        IsTaskBound = true;
        LastErrorMessage = string.Empty;
        await LoadFromParametersAsync(parameters, cancellationToken);
        _suppressPersist = false;
    }

    public virtual void ClearBinding()
    {
        _boundTaskIndex = -1;
        _suppressPersist = true;
        IsTaskBound = false;
    }

    public async Task<bool> FlushPendingChangesAsync(CancellationToken cancellationToken = default)
    {
        _persistDebounceCts?.Cancel();
        return await PersistNowAsync(cancellationToken);
    }

    protected void QueuePersist()
    {
        if (_suppressPersist || _boundTaskIndex < 0)
        {
            return;
        }

        _persistDebounceCts?.Cancel();
        _persistDebounceCts?.Dispose();
        _persistDebounceCts = new CancellationTokenSource();
        var token = _persistDebounceCts.Token;

        _ = PersistDebouncedAsync(token);
    }

    protected abstract Task LoadFromParametersAsync(JsonObject parameters, CancellationToken cancellationToken);

    protected abstract JsonObject BuildParameters();

    private async Task PersistDebouncedAsync(CancellationToken cancellationToken)
    {
        try
        {
            await Task.Delay(180, cancellationToken);
            await PersistNowAsync(cancellationToken);
        }
        catch (OperationCanceledException)
        {
            // ignore
        }
    }

    private async Task<bool> PersistNowAsync(CancellationToken cancellationToken)
    {
        await _persistLock.WaitAsync(cancellationToken);
        try
        {
            if (_boundTaskIndex < 0 || _suppressPersist)
            {
                return true;
            }

            var parameters = BuildParameters();
            var update = await Runtime.TaskQueueFeatureService.UpdateTaskParamsAsync(
                _boundTaskIndex,
                parameters,
                persistImmediately: true,
                cancellationToken: cancellationToken);
            if (!update.Success)
            {
                LastErrorMessage = update.Message;
                await Runtime.DiagnosticsService.RecordFailedResultAsync("TaskModule.UpdateParams", update, cancellationToken);
                return false;
            }

            LastErrorMessage = string.Empty;
            StatusMessage = update.Message;
            return true;
        }
        catch (Exception ex)
        {
            LastErrorMessage = ex.Message;
            await Runtime.DiagnosticsService.RecordErrorAsync("TaskModule.Persist", "Unexpected module persist failure.", ex, cancellationToken);
            return false;
        }
        finally
        {
            _persistLock.Release();
        }
    }
}
