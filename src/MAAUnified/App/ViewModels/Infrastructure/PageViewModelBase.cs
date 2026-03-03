using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Infrastructure;

public abstract class PageViewModelBase : ObservableObject
{
    private string _statusMessage = string.Empty;
    private string _lastErrorMessage = string.Empty;

    protected PageViewModelBase(MAAUnifiedRuntime runtime)
    {
        Runtime = runtime;
    }

    protected MAAUnifiedRuntime Runtime { get; }

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

    protected async Task<bool> ApplyResultAsync(UiOperationResult result, string scope, CancellationToken cancellationToken = default)
    {
        if (result.Success)
        {
            StatusMessage = result.Message;
            LastErrorMessage = string.Empty;
            await Runtime.DiagnosticsService.RecordEventAsync(scope, result.Message, cancellationToken);
            return true;
        }

        LastErrorMessage = result.Message;
        await Runtime.DiagnosticsService.RecordFailedResultAsync(scope, result, cancellationToken);
        return false;
    }

    protected async Task<T?> ApplyResultAsync<T>(UiOperationResult<T> result, string scope, CancellationToken cancellationToken = default)
    {
        if (result.Success)
        {
            StatusMessage = result.Message;
            LastErrorMessage = string.Empty;
            await Runtime.DiagnosticsService.RecordEventAsync(scope, result.Message, cancellationToken);
            return result.Value;
        }

        LastErrorMessage = result.Message;
        await Runtime.DiagnosticsService.RecordFailedResultAsync(scope, UiOperationResult.Fail(
            result.Error?.Code ?? "UiOperationFailed",
            result.Message,
            result.Error?.Details), cancellationToken);
        return default;
    }
}
