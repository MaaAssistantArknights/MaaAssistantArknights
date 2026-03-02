namespace MAAUnified.CoreBridge;

public enum CoreErrorCode
{
    LibraryNotFound = 1,
    LibraryLoadFailed = 2,
    SymbolMissing = 3,
    ResourceNotFound = 4,
    ResourceLoadFailed = 5,
    CoreInstanceCreateFailed = 6,
    NotInitialized = 7,
    Disposed = 8,
    InvalidRequest = 9,
    ConnectFailed = 10,
    ConnectTimeout = 11,
    AppendTaskFailed = 12,
    StartFailed = 13,
    StopFailed = 14,
    GetImageFailed = 15,
    PlatformNotSupported = 16,
    NotSupported = 17,
    NotImplemented = 18,
}

public sealed record CoreError(
    CoreErrorCode Code,
    string Message,
    string? NativeDetails = null,
    string? Exception = null);

public sealed record CoreResult<T>(bool Success, T? Value, CoreError? Error)
{
    public static CoreResult<T> Ok(T value) => new(true, value, null);

    public static CoreResult<T> Fail(CoreError error) => new(false, default, error);
}

public sealed record CoreInitializeRequest(string BaseDirectory, string? ClientType = null);

public sealed record CoreInitializeInfo(
    string BaseDirectory,
    string LibraryPath,
    string CoreVersion,
    string? ClientType);

public sealed record CoreConnectionInfo(
    string Address,
    string ConnectConfig,
    string? AdbPath,
    TimeSpan? Timeout = null);

public sealed record CoreTaskRequest(string Type, string Name, bool IsEnabled, string ParamsJson);

public sealed record CoreRuntimeStatus(bool Initialized, bool Connected, bool Running);

public sealed record CoreAttachWindowRequest(
    nint WindowHandle,
    ulong ScreencapMethod,
    ulong MouseMethod,
    ulong KeyboardMethod);

public sealed record CoreCallbackEvent(
    int MsgId,
    string MsgName,
    string PayloadJson,
    DateTimeOffset Timestamp);
