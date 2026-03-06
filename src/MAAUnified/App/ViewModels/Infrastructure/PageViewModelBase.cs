using MAAUnified.Application.Configuration;
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

    protected Task RecordEventAsync(string scope, string message, CancellationToken cancellationToken = default)
    {
        return Runtime.DiagnosticsService.RecordEventAsync(scope, message, cancellationToken);
    }

    protected Task RecordFailedResultAsync(string scope, UiOperationResult result, CancellationToken cancellationToken = default)
    {
        return Runtime.DiagnosticsService.RecordFailedResultAsync(scope, result, cancellationToken);
    }

    protected Task RecordErrorAsync(string scope, string message, Exception? exception = null, CancellationToken cancellationToken = default)
    {
        return Runtime.DiagnosticsService.RecordErrorAsync(scope, message, exception, cancellationToken);
    }

    protected Task RecordConfigValidationFailureAsync(ConfigValidationIssue? issue, CancellationToken cancellationToken = default)
    {
        return Runtime.DiagnosticsService.RecordConfigValidationFailureAsync(issue, cancellationToken);
    }

    protected async Task RecordUnhandledExceptionAsync(
        string scope,
        Exception exception,
        string exceptionCode = UiErrorCode.UiOperationFailed,
        string? message = null,
        CancellationToken cancellationToken = default)
    {
        var code = string.IsNullOrWhiteSpace(exceptionCode)
            ? UiErrorCode.UiOperationFailed
            : exceptionCode;
        var errorMessage = string.IsNullOrWhiteSpace(message)
            ? exception.Message
            : message!;
        LastErrorMessage = exception.Message;
        await RecordErrorAsync(scope, errorMessage, exception, cancellationToken);
        await RecordFailedResultAsync(
            scope,
            UiOperationResult.Fail(code, exception.Message, exception.ToString()),
            cancellationToken);
    }

    protected async Task<bool> ApplyResultAsync(UiOperationResult result, string scope, CancellationToken cancellationToken = default)
    {
        if (result.Success)
        {
            StatusMessage = result.Message;
            LastErrorMessage = string.Empty;
            await RecordEventAsync(scope, result.Message, cancellationToken);
            return true;
        }

        LastErrorMessage = result.Message;
        await RecordFailedResultAsync(scope, result, cancellationToken);
        await Runtime.DialogFeatureService.ReportErrorAsync(scope, result, cancellationToken);
        return false;
    }

    protected async Task<T?> ApplyResultAsync<T>(UiOperationResult<T> result, string scope, CancellationToken cancellationToken = default)
    {
        if (result.Success)
        {
            StatusMessage = result.Message;
            LastErrorMessage = string.Empty;
            await RecordEventAsync(scope, result.Message, cancellationToken);
            return result.Value;
        }

        LastErrorMessage = result.Message;
        var failed = UiOperationResult.Fail(
            result.Error?.Code ?? UiErrorCode.UiOperationFailed,
            result.Message,
            result.Error?.Details);
        await RecordFailedResultAsync(scope, failed, cancellationToken);
        await Runtime.DialogFeatureService.ReportErrorAsync(scope, failed, cancellationToken);
        return default;
    }

    protected async Task<bool> ApplyResultAsync(
        Func<CancellationToken, Task<UiOperationResult>> operation,
        string scope,
        string exceptionCode,
        CancellationToken cancellationToken = default)
    {
        try
        {
            var result = await operation(cancellationToken);
            return await ApplyResultAsync(result, scope, cancellationToken);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                scope,
                ex,
                exceptionCode,
                $"Unhandled exception in `{scope}`.",
                cancellationToken);
            return false;
        }
    }

    protected async Task<T?> ApplyResultAsync<T>(
        Func<CancellationToken, Task<UiOperationResult<T>>> operation,
        string scope,
        string exceptionCode,
        CancellationToken cancellationToken = default)
    {
        try
        {
            var result = await operation(cancellationToken);
            return await ApplyResultAsync(result, scope, cancellationToken);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception ex)
        {
            await RecordUnhandledExceptionAsync(
                scope,
                ex,
                exceptionCode,
                $"Unhandled exception in `{scope}`.",
                cancellationToken);
            return default;
        }
    }
}
