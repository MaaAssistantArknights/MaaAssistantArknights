using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace MAAUnified.CoreBridge;

public interface IMaaCoreBridge : IAsyncDisposable
{
    bool SupportsBackToHome => false;

    bool SupportsStartCloseDown => false;

    Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
        CoreInitializeRequest request,
        CancellationToken cancellationToken = default);

    Task<CoreResult<bool>> ConnectAsync(
        CoreConnectionInfo connectionInfo,
        CancellationToken cancellationToken = default);

    Task<CoreResult<int>> AppendTaskAsync(
        CoreTaskRequest task,
        CancellationToken cancellationToken = default);

    Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default);

    Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default);

    Task<CoreResult<bool>> BackToHomeAsync(CancellationToken cancellationToken = default)
        => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "BackToHome is unsupported by current bridge.")));

    Task<CoreResult<bool>> StartCloseDownAsync(string clientType, CancellationToken cancellationToken = default)
        => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "CloseDown is unsupported by current bridge.")));

    Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default);

    Task<CoreResult<bool>> AttachWindowAsync(
        CoreAttachWindowRequest request,
        CancellationToken cancellationToken = default);

    Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default);

    IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync(CancellationToken cancellationToken = default);
}
