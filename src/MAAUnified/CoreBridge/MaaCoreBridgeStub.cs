using System.Runtime.CompilerServices;
using System.Threading.Channels;

namespace MAAUnified.CoreBridge;

public sealed class MaaCoreBridgeStub : IMaaCoreBridge
{
    private readonly Channel<CoreCallbackEvent> _callbackChannel = Channel.CreateUnbounded<CoreCallbackEvent>();

    private bool _initialized;
    private bool _connected;
    private bool _running;
    private bool _disposed;
    private int _taskId;

    public bool SupportsBackToHome => true;

    public bool SupportsStartCloseDown => true;

    public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
        CoreInitializeRequest request,
        CancellationToken cancellationToken = default)
    {
        if (_disposed)
        {
            return Task.FromResult(CoreResult<CoreInitializeInfo>.Fail(new CoreError(CoreErrorCode.Disposed, "Bridge is disposed.")));
        }

        _initialized = true;
        return Task.FromResult(
            CoreResult<CoreInitializeInfo>.Ok(
                new CoreInitializeInfo(
                    request.BaseDirectory,
                    Path.Combine(request.BaseDirectory, "stub-core"),
                    "stub",
                    request.ClientType)));
    }

    public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
    {
        if (!_initialized)
        {
            return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotInitialized, "Bridge not initialized.")));
        }

        _connected = !string.IsNullOrWhiteSpace(connectionInfo.Address);
        return Task.FromResult(
            _connected
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.ConnectFailed, "Stub connect failed.")));
    }

    public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
    {
        if (!_connected)
        {
            return Task.FromResult(CoreResult<int>.Fail(new CoreError(CoreErrorCode.AppendTaskFailed, "Stub is not connected.")));
        }

        _taskId += 1;
        return Task.FromResult(CoreResult<int>.Ok(_taskId));
    }

    public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
    {
        _running = _connected;
        return Task.FromResult(
            _running
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StartFailed, "Stub start failed.")));
    }

    public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
    {
        var wasRunning = _running;
        _running = false;
        return Task.FromResult(
            wasRunning
                ? CoreResult<bool>.Ok(true)
                : CoreResult<bool>.Fail(new CoreError(CoreErrorCode.StopFailed, "Stub was not running.")));
    }

    public Task<CoreResult<bool>> BackToHomeAsync(CancellationToken cancellationToken = default)
    {
        if (!_connected)
        {
            return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotInitialized, "Stub is not connected.")));
        }

        return Task.FromResult(CoreResult<bool>.Ok(true));
    }

    public async Task<CoreResult<bool>> StartCloseDownAsync(string clientType, CancellationToken cancellationToken = default)
    {
        if (!_connected)
        {
            return CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotInitialized, "Stub is not connected."));
        }

        var append = await AppendTaskAsync(
            new CoreTaskRequest(
                "CloseDown",
                "CloseDown",
                true,
                $"{{\"client_type\":\"{clientType?.Trim() ?? string.Empty}\"}}"),
            cancellationToken);
        if (!append.Success)
        {
            return CoreResult<bool>.Fail(append.Error!);
        }

        return await StartAsync(cancellationToken);
    }

    public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(_initialized, _connected, _running)));
    }

    public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
    {
        return Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "Stub does not support AttachWindow.")));
    }

    public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
    {
        return Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.GetImageFailed, "Stub has no image source.")));
    }

    public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
    {
        await foreach (var callback in _callbackChannel.Reader.ReadAllAsync(cancellationToken))
        {
            yield return callback;
        }
    }

    public ValueTask DisposeAsync()
    {
        _disposed = true;
        _callbackChannel.Writer.TryComplete();
        return ValueTask.CompletedTask;
    }
}
