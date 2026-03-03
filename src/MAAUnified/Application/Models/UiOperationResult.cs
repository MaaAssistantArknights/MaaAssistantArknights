using MAAUnified.CoreBridge;

namespace MAAUnified.Application.Models;

public sealed record UiOperationError(string Code, string Message, string? Details = null);

public sealed record UiOperationResult(
    bool Success,
    string Message,
    bool UserCancelled,
    UiOperationError? Error = null)
{
    public static UiOperationResult Ok(string message) => new(true, message, false);

    public static UiOperationResult Fail(string code, string message, string? details = null) =>
        new(false, message, false, new UiOperationError(code, message, details));

    public static UiOperationResult Cancelled(string message) => new(false, message, true);

    public static UiOperationResult FromCore(CoreResult<bool> core, string successMessage)
    {
        if (core.Success)
        {
            return Ok(successMessage);
        }

        var code = core.Error?.Code.ToString() ?? "CoreUnknown";
        var message = core.Error?.Message ?? "Core operation failed.";
        var details = core.Error?.NativeDetails ?? core.Error?.Exception;
        return Fail(code, message, details);
    }
}

public sealed record UiOperationResult<T>(
    bool Success,
    T? Value,
    string Message,
    bool UserCancelled,
    UiOperationError? Error = null)
{
    public static UiOperationResult<T> Ok(T value, string message) => new(true, value, message, false);

    public static UiOperationResult<T> Fail(string code, string message, string? details = null) =>
        new(false, default, message, false, new UiOperationError(code, message, details));

    public static UiOperationResult<T> Cancelled(string message) => new(false, default, message, true);

    public static UiOperationResult<T> FromCore(CoreResult<T> core, string successMessage)
    {
        if (core.Success && core.Value is not null)
        {
            return Ok(core.Value, successMessage);
        }

        var code = core.Error?.Code.ToString() ?? "CoreUnknown";
        var message = core.Error?.Message ?? "Core operation failed.";
        var details = core.Error?.NativeDetails ?? core.Error?.Exception;
        return Fail(code, message, details);
    }
}
