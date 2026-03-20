using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Threading.Channels;

namespace MAAUnified.CoreBridge;

public sealed class MaaCoreBridgeNative : IMaaCoreBridge
{
    private const int MsgInitFailed = 1;
    private const int MsgConnectionInfo = 2;
    private const int MsgAsyncCallInfo = 4;
    private const int AsstStaticOptionCpuOcr = 1;
    private const int AsstStaticOptionGpuOcr = 2;
    private static readonly HashSet<string> DefaultClientTypes = new(StringComparer.OrdinalIgnoreCase)
    {
        string.Empty,
        "Official",
        "Bilibili",
    };
    private static readonly IReadOnlyDictionary<string, string> ClientTypeAliasMap =
        new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["Official"] = "Official",
            ["Bilibili"] = "Bilibili",
            ["Txwy"] = "txwy",
            ["YoStarEN"] = "YoStarEN",
            ["YoStarJP"] = "YoStarJP",
            ["YoStarKR"] = "YoStarKR",
        };

    private static readonly TimeSpan _defaultConnectTimeout = TimeSpan.FromSeconds(30);

    private readonly Channel<CoreCallbackEvent> _callbackChannel = Channel.CreateUnbounded<CoreCallbackEvent>(
        new UnboundedChannelOptions
        {
            SingleReader = false,
            SingleWriter = false,
        });

    private readonly SemaphoreSlim _lifecycleLock = new(1, 1);
    private readonly SemaphoreSlim _connectLock = new(1, 1);
    private readonly object _sync = new();

    private nint _nativeLibrary;
    private nint _instance;
    private string? _libraryPath;
    private string? _baseDirectory;
    private string? _loadedClientType;
    private CoreGpuInitializeInfo? _gpuInitializeInfo;
    private bool _disposed;
    private AsstExports? _exports;
    private AsstApiCallbackDelegate? _callbackDelegate;
    private ConnectPendingState? _pendingConnect;

    public bool SupportsBackToHome => true;

    public bool SupportsStartCloseDown => true;

    public async Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
        CoreInitializeRequest request,
        CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(request.BaseDirectory))
        {
            return Fail<CoreInitializeInfo>(CoreErrorCode.InvalidRequest, "Base directory is empty.");
        }

        await _lifecycleLock.WaitAsync(cancellationToken);
        try
        {
            if (_disposed)
            {
                return Fail<CoreInitializeInfo>(CoreErrorCode.Disposed, "Bridge is already disposed.");
            }

            if (_instance != nint.Zero && _exports is not null && _libraryPath is not null && _baseDirectory is not null)
            {
                if (!string.IsNullOrWhiteSpace(request.ClientType)
                    && !string.Equals(_loadedClientType, request.ClientType, StringComparison.OrdinalIgnoreCase))
                {
                    var clientLoad = LoadClientResource(request.ClientType, _baseDirectory, _exports);
                    if (!clientLoad.Success)
                    {
                        return CoreResult<CoreInitializeInfo>.Fail(clientLoad.Error!);
                    }
                }

                return CoreResult<CoreInitializeInfo>.Ok(BuildInitializeInfo(_baseDirectory, _libraryPath, request.ClientType, _gpuInitializeInfo));
            }

            var libraryNameResult = ResolveLibraryName();
            if (!libraryNameResult.Success)
            {
                return CoreResult<CoreInitializeInfo>.Fail(libraryNameResult.Error!);
            }

            var libraryPath = Path.Combine(request.BaseDirectory, libraryNameResult.Value!);
            if (!File.Exists(libraryPath))
            {
                return Fail<CoreInitializeInfo>(CoreErrorCode.LibraryNotFound, $"MaaCore library was not found: {libraryPath}");
            }

            var resourceDir = Path.Combine(request.BaseDirectory, "resource");
            if (!Directory.Exists(resourceDir))
            {
                return Fail<CoreInitializeInfo>(CoreErrorCode.ResourceNotFound, $"Resource directory was not found: {resourceDir}");
            }

            nint loadedLibrary;
            try
            {
                loadedLibrary = NativeLibrary.Load(libraryPath);
            }
            catch (Exception ex)
            {
                return Fail<CoreInitializeInfo>(CoreErrorCode.LibraryLoadFailed, $"Failed to load MaaCore library at `{libraryPath}`.", ex: ex);
            }

            if (!TryLoadExports(loadedLibrary, out var exports, out var missingSymbol))
            {
                NativeLibrary.Free(loadedLibrary);
                return Fail<CoreInitializeInfo>(CoreErrorCode.SymbolMissing, $"Required MaaCore symbol is missing: {missingSymbol}");
            }

            _callbackDelegate = OnNativeCallback;
            var gpuInitializeInfo = ApplyGpuInitialization(request.Gpu, exports);

            if (!AsBool(exports.AsstSetUserDir(request.BaseDirectory)))
            {
                NativeLibrary.Free(loadedLibrary);
                return Fail<CoreInitializeInfo>(CoreErrorCode.ResourceLoadFailed, "AsstSetUserDir returned false.");
            }

            if (!AsBool(exports.AsstLoadResource(request.BaseDirectory)))
            {
                NativeLibrary.Free(loadedLibrary);
                return Fail<CoreInitializeInfo>(CoreErrorCode.ResourceLoadFailed, "AsstLoadResource(baseDir) returned false.");
            }

            if (!string.IsNullOrWhiteSpace(request.ClientType))
            {
                var clientLoad = LoadClientResource(request.ClientType, request.BaseDirectory, exports);
                if (!clientLoad.Success)
                {
                    NativeLibrary.Free(loadedLibrary);
                    return CoreResult<CoreInitializeInfo>.Fail(clientLoad.Error!);
                }
            }

            var instance = exports.AsstCreateEx(_callbackDelegate, nint.Zero);
            if (instance == nint.Zero)
            {
                NativeLibrary.Free(loadedLibrary);
                return Fail<CoreInitializeInfo>(CoreErrorCode.CoreInstanceCreateFailed, "AsstCreateEx returned null.");
            }

            _nativeLibrary = loadedLibrary;
            _exports = exports;
            _instance = instance;
            _libraryPath = libraryPath;
            _baseDirectory = request.BaseDirectory;
            _loadedClientType = request.ClientType;
            _gpuInitializeInfo = gpuInitializeInfo;

            return CoreResult<CoreInitializeInfo>.Ok(BuildInitializeInfo(request.BaseDirectory, libraryPath, request.ClientType, gpuInitializeInfo));
        }
        finally
        {
            _lifecycleLock.Release();
        }
    }

    public async Task<CoreResult<bool>> ConnectAsync(
        CoreConnectionInfo connectionInfo,
        CancellationToken cancellationToken = default)
    {
        if (string.IsNullOrWhiteSpace(connectionInfo.Address))
        {
            return Fail<bool>(CoreErrorCode.InvalidRequest, "Connection address is empty.");
        }

        if (string.IsNullOrWhiteSpace(connectionInfo.ConnectConfig))
        {
            return Fail<bool>(CoreErrorCode.InvalidRequest, "Connection config is empty.");
        }

        var status = EnsureReady();
        if (!status.Success)
        {
            return CoreResult<bool>.Fail(status.Error!);
        }

        await _connectLock.WaitAsync(cancellationToken);
        try
        {
            var exports = _exports!;
            var handle = _instance;

            var asyncCallId = exports.AsstAsyncConnect(
                handle,
                connectionInfo.AdbPath ?? string.Empty,
                connectionInfo.Address,
                connectionInfo.ConnectConfig,
                0);

            if (asyncCallId <= 0)
            {
                return Fail<bool>(CoreErrorCode.ConnectFailed, "AsstAsyncConnect returned invalid async call id.");
            }

            var pending = new ConnectPendingState(asyncCallId);
            lock (_sync)
            {
                _pendingConnect = pending;
            }

            var timeout = connectionInfo.Timeout is null || connectionInfo.Timeout <= TimeSpan.Zero
                ? _defaultConnectTimeout
                : connectionInfo.Timeout.Value;

            using var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeoutCts.CancelAfter(timeout);

            try
            {
                return await pending.Completion.Task.WaitAsync(timeoutCts.Token);
            }
            catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
            {
                return Fail<bool>(CoreErrorCode.ConnectTimeout, $"Connect timeout after {timeout.TotalSeconds:0.#} seconds.");
            }
            finally
            {
                lock (_sync)
                {
                    if (ReferenceEquals(_pendingConnect, pending))
                    {
                        _pendingConnect = null;
                    }
                }
            }
        }
        finally
        {
            _connectLock.Release();
        }
    }

    public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (string.IsNullOrWhiteSpace(task.Type))
        {
            return Task.FromResult(Fail<int>(CoreErrorCode.InvalidRequest, "Task type is empty."));
        }

        if (string.IsNullOrWhiteSpace(task.ParamsJson))
        {
            return Task.FromResult(Fail<int>(CoreErrorCode.InvalidRequest, "Task params are empty."));
        }

        var status = EnsureReady();
        if (!status.Success)
        {
            return Task.FromResult(CoreResult<int>.Fail(status.Error!));
        }

        try
        {
            using var _ = JsonDocument.Parse(task.ParamsJson);
        }
        catch (Exception ex)
        {
            return Task.FromResult(Fail<int>(CoreErrorCode.InvalidRequest, "Task params json is invalid.", ex: ex));
        }

        var taskId = _exports!.AsstAppendTask(_instance, task.Type, task.ParamsJson);
        if (taskId <= 0)
        {
            return Task.FromResult(Fail<int>(CoreErrorCode.AppendTaskFailed, $"AsstAppendTask returned invalid task id for `{task.Type}`."));
        }

        return Task.FromResult(CoreResult<int>.Ok(taskId));
    }

    public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var status = EnsureReady();
        if (!status.Success)
        {
            return Task.FromResult(CoreResult<bool>.Fail(status.Error!));
        }

        var ok = AsBool(_exports!.AsstStart(_instance));
        if (!ok)
        {
            return Task.FromResult(Fail<bool>(CoreErrorCode.StartFailed, "AsstStart returned false."));
        }

        return Task.FromResult(CoreResult<bool>.Ok(true));
    }

    public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var status = EnsureReady();
        if (!status.Success)
        {
            return Task.FromResult(CoreResult<bool>.Fail(status.Error!));
        }

        var ok = AsBool(_exports!.AsstStop(_instance));
        if (!ok)
        {
            return Task.FromResult(Fail<bool>(CoreErrorCode.StopFailed, "AsstStop returned false."));
        }

        return Task.FromResult(CoreResult<bool>.Ok(true));
    }

    public Task<CoreResult<bool>> BackToHomeAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var status = EnsureReady();
        if (!status.Success)
        {
            return Task.FromResult(CoreResult<bool>.Fail(status.Error!));
        }

        var ok = AsBool(_exports!.AsstBackToHome(_instance));
        if (!ok)
        {
            return Task.FromResult(Fail<bool>(CoreErrorCode.NotImplemented, "AsstBackToHome returned false."));
        }

        return Task.FromResult(CoreResult<bool>.Ok(true));
    }

    public async Task<CoreResult<bool>> StartCloseDownAsync(string clientType, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var status = EnsureReady();
        if (!status.Success)
        {
            return CoreResult<bool>.Fail(status.Error!);
        }

        var payload = JsonSerializer.Serialize(new Dictionary<string, string>
        {
            ["client_type"] = clientType?.Trim() ?? string.Empty,
        });
        var append = await AppendTaskAsync(new CoreTaskRequest("CloseDown", "CloseDown", true, payload), cancellationToken);
        if (!append.Success)
        {
            return CoreResult<bool>.Fail(append.Error!);
        }

        return await StartAsync(cancellationToken);
    }

    public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_disposed)
        {
            return Task.FromResult(Fail<CoreRuntimeStatus>(CoreErrorCode.Disposed, "Bridge is already disposed."));
        }

        if (_exports is null || _instance == nint.Zero)
        {
            return Task.FromResult(Fail<CoreRuntimeStatus>(CoreErrorCode.NotInitialized, "Bridge is not initialized."));
        }

        var connected = AsBool(_exports.AsstConnected(_instance));
        var running = AsBool(_exports.AsstRunning(_instance));
        return Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, connected, running)));
    }

    public Task<CoreResult<bool>> AttachWindowAsync(
        CoreAttachWindowRequest request,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.FromResult(Fail<bool>(CoreErrorCode.NotSupported, "AttachWindow is not implemented in MAAUnified bridge yet."));
    }

    public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var status = EnsureReady();
        if (!status.Success)
        {
            return Task.FromResult(CoreResult<byte[]>.Fail(status.Error!));
        }

        var exports = _exports!;
        var nullSize = exports.AsstGetNullSize();
        // Align with the WPF path: reserve a full-size BGR frame upfront to avoid NullSize on large PNG payloads.
        ulong bufferSize = 1280UL * 720UL * 3UL;

        for (var retry = 0; retry < 6; retry++)
        {
            if (bufferSize > int.MaxValue)
            {
                return Task.FromResult(Fail<byte[]>(CoreErrorCode.GetImageFailed, "Image buffer size exceeds supported limit."));
            }

            var buffer = Marshal.AllocHGlobal((nint)bufferSize);
            try
            {
                var imageSize = exports.AsstGetImage(_instance, buffer, bufferSize);
                if (imageSize == nullSize || imageSize == 0)
                {
                    return Task.FromResult(Fail<byte[]>(CoreErrorCode.GetImageFailed, "AsstGetImage returned null image."));
                }

                if (imageSize > bufferSize)
                {
                    bufferSize = imageSize;
                    continue;
                }

                if (imageSize > int.MaxValue)
                {
                    return Task.FromResult(Fail<byte[]>(CoreErrorCode.GetImageFailed, "Image size is too large."));
                }

                var data = new byte[(int)imageSize];
                Marshal.Copy(buffer, data, 0, data.Length);
                return Task.FromResult(CoreResult<byte[]>.Ok(data));
            }
            finally
            {
                Marshal.FreeHGlobal(buffer);
            }
        }

        return Task.FromResult(Fail<byte[]>(CoreErrorCode.GetImageFailed, "AsstGetImage did not fit buffer after retries."));
    }

    public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
    {
        await foreach (var callback in _callbackChannel.Reader.ReadAllAsync(cancellationToken))
        {
            yield return callback;
        }
    }

    public async ValueTask DisposeAsync()
    {
        await _lifecycleLock.WaitAsync();
        try
        {
            if (_disposed)
            {
                return;
            }

            _disposed = true;

            ConnectPendingState? pending;
            lock (_sync)
            {
                pending = _pendingConnect;
                _pendingConnect = null;
            }

            pending?.TryComplete(
                CoreResult<bool>.Fail(new CoreError(CoreErrorCode.Disposed, "Bridge disposed while waiting for connect.")));

            if (_instance != nint.Zero && _exports is not null)
            {
                try
                {
                    _exports.AsstDestroy(_instance);
                }
                catch
                {
                    // ignored during dispose
                }

                _instance = nint.Zero;
            }

            _exports = null;
            _callbackDelegate = null;
            _gpuInitializeInfo = null;

            if (_nativeLibrary != nint.Zero)
            {
                try
                {
                    NativeLibrary.Free(_nativeLibrary);
                }
                catch
                {
                    // ignored during dispose
                }

                _nativeLibrary = nint.Zero;
            }

            _callbackChannel.Writer.TryComplete();
        }
        finally
        {
            _lifecycleLock.Release();
        }
    }

    private void OnNativeCallback(int msg, nint detailsJson, nint customArg)
    {
        var payloadJson = Marshal.PtrToStringUTF8(detailsJson) ?? "{}";
        var callback = new CoreCallbackEvent(msg, ResolveMessageName(msg), payloadJson, DateTimeOffset.UtcNow);
        _callbackChannel.Writer.TryWrite(callback);
        TryHandlePendingConnect(callback);
    }

    private void TryHandlePendingConnect(CoreCallbackEvent callback)
    {
        ConnectPendingState? pending;
        lock (_sync)
        {
            pending = _pendingConnect;
        }

        if (pending is null)
        {
            return;
        }

        if (!TryParsePayload(callback.PayloadJson, out var root))
        {
            if (callback.MsgId == MsgInitFailed)
            {
                pending.TryComplete(
                    Fail<bool>(CoreErrorCode.ConnectFailed, "InitFailed callback received while connecting.", callback.PayloadJson));
            }

            return;
        }

        if (callback.MsgId == MsgAsyncCallInfo)
        {
            var asyncCallId = GetInt(root, "async_call_id");
            var what = GetString(root, "what");
            if (asyncCallId != pending.AsyncCallId || !string.Equals(what, "Connect", StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            var details = GetObject(root, "details");
            var ret = details is not null && GetBool(details.Value, "ret");

            pending.MarkAsyncCall(ret);
            if (!ret)
            {
                pending.TryComplete(Fail<bool>(CoreErrorCode.ConnectFailed, "AsstAsyncConnect callback reported ret=false.", callback.PayloadJson));
                return;
            }

            if (pending.CanCompleteSuccess)
            {
                pending.TryComplete(CoreResult<bool>.Ok(true));
            }

            return;
        }

        if (callback.MsgId == MsgConnectionInfo)
        {
            var what = GetString(root, "what");
            if (string.Equals(what, "Connected", StringComparison.OrdinalIgnoreCase)
                || string.Equals(what, "Reconnected", StringComparison.OrdinalIgnoreCase))
            {
                pending.MarkConnected();
                if (pending.CanCompleteSuccess)
                {
                    pending.TryComplete(CoreResult<bool>.Ok(true));
                }

                return;
            }

            if (string.Equals(what, "ConnectFailed", StringComparison.OrdinalIgnoreCase)
                || string.Equals(what, "Disconnect", StringComparison.OrdinalIgnoreCase))
            {
                pending.TryComplete(Fail<bool>(
                    CoreErrorCode.ConnectFailed,
                    BuildConnectionFailureMessage(root, what),
                    callback.PayloadJson));
            }
        }
    }

    private CoreResult<bool> EnsureReady()
    {
        if (_disposed)
        {
            return Fail<bool>(CoreErrorCode.Disposed, "Bridge is already disposed.");
        }

        if (_exports is null || _instance == nint.Zero)
        {
            return Fail<bool>(CoreErrorCode.NotInitialized, "Bridge is not initialized.");
        }

        return CoreResult<bool>.Ok(true);
    }

    private CoreResult<bool> LoadClientResource(string clientType, string baseDirectory, AsstExports exports)
    {
        var normalizedClientType = NormalizeClientType(clientType);
        if (DefaultClientTypes.Contains(normalizedClientType))
        {
            _loadedClientType = normalizedClientType;
            return CoreResult<bool>.Ok(true);
        }

        var expectedPath = Path.Combine(baseDirectory, "resource", "global", normalizedClientType, "resource");
        if (!TryResolveClientResourcePath(baseDirectory, normalizedClientType, out var resolvedClientType, out var clientResourcePath))
        {
            return Fail<bool>(CoreErrorCode.ResourceNotFound, $"Client resource directory was not found: {expectedPath}");
        }

        if (!AsBool(exports.AsstLoadResource(clientResourcePath)))
        {
            return Fail<bool>(CoreErrorCode.ResourceLoadFailed, $"AsstLoadResource failed for client resource: {clientResourcePath}");
        }

        _loadedClientType = resolvedClientType;
        return CoreResult<bool>.Ok(true);
    }

    private static string NormalizeClientType(string? clientType)
    {
        var normalized = (clientType ?? string.Empty).Trim();
        if (string.IsNullOrWhiteSpace(normalized))
        {
            return string.Empty;
        }

        return ClientTypeAliasMap.TryGetValue(normalized, out var canonical)
            ? canonical
            : normalized;
    }

    private static bool TryResolveClientResourcePath(
        string baseDirectory,
        string normalizedClientType,
        out string resolvedClientType,
        out string clientResourcePath)
    {
        var globalRoot = Path.Combine(baseDirectory, "resource", "global");
        resolvedClientType = normalizedClientType;
        clientResourcePath = Path.Combine(globalRoot, normalizedClientType, "resource");
        if (Directory.Exists(clientResourcePath))
        {
            return true;
        }

        if (!Directory.Exists(globalRoot))
        {
            return false;
        }

        foreach (var candidateDirectory in Directory.EnumerateDirectories(globalRoot))
        {
            var candidateClientType = Path.GetFileName(candidateDirectory);
            if (!string.Equals(candidateClientType, normalizedClientType, StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var candidateResourcePath = Path.Combine(candidateDirectory, "resource");
            if (!Directory.Exists(candidateResourcePath))
            {
                continue;
            }

            resolvedClientType = candidateClientType;
            clientResourcePath = candidateResourcePath;
            return true;
        }

        return false;
    }

    private static CoreGpuInitializeInfo? ApplyGpuInitialization(
        CoreGpuInitializeRequest? request,
        AsstExports exports)
    {
        if (request is null || request.Mode == CoreGpuRequestMode.Default)
        {
            return null;
        }

        var warnings = new List<string>();
        var appliedMode = CoreGpuAppliedMode.Default;
        uint? appliedGpuIndex = null;

        if (exports.AsstSetStaticOption is null)
        {
            warnings.Add("AsstSetStaticOption is unavailable; continuing with MaaCore default OCR backend.");
            return new CoreGpuInitializeInfo(request.Mode, appliedMode, request.GpuIndex, appliedGpuIndex, warnings);
        }

        if (request.Mode == CoreGpuRequestMode.Cpu)
        {
            if (AsBool(exports.AsstSetStaticOption(AsstStaticOptionCpuOcr, string.Empty)))
            {
                appliedMode = CoreGpuAppliedMode.Cpu;
            }
            else
            {
                warnings.Add("Failed to apply CpuOCR static option; continuing with MaaCore default OCR backend.");
            }

            return new CoreGpuInitializeInfo(request.Mode, appliedMode, request.GpuIndex, appliedGpuIndex, warnings);
        }

        if (request.GpuIndex is uint gpuIndex
            && AsBool(exports.AsstSetStaticOption(AsstStaticOptionGpuOcr, gpuIndex.ToString())))
        {
            appliedMode = CoreGpuAppliedMode.Gpu;
            appliedGpuIndex = gpuIndex;
            return new CoreGpuInitializeInfo(request.Mode, appliedMode, request.GpuIndex, appliedGpuIndex, warnings);
        }

        warnings.Add(request.GpuIndex is null
            ? "GPU OCR requested without a valid GPU index; falling back to CPU OCR."
            : $"Failed to apply GpuOCR static option for GPU index {request.GpuIndex.Value}; falling back to CPU OCR.");

        if (AsBool(exports.AsstSetStaticOption(AsstStaticOptionCpuOcr, string.Empty)))
        {
            appliedMode = CoreGpuAppliedMode.Cpu;
        }
        else
        {
            warnings.Add("Failed to apply CpuOCR fallback static option; continuing with MaaCore default OCR backend.");
        }

        return new CoreGpuInitializeInfo(request.Mode, appliedMode, request.GpuIndex, appliedGpuIndex, warnings);
    }

    private CoreInitializeInfo BuildInitializeInfo(
        string baseDirectory,
        string libraryPath,
        string? clientType,
        CoreGpuInitializeInfo? gpuInitializeInfo)
    {
        var version = _exports is null
            ? string.Empty
            : Marshal.PtrToStringUTF8(_exports.AsstGetVersion()) ?? string.Empty;
        return new CoreInitializeInfo(baseDirectory, libraryPath, version, clientType, gpuInitializeInfo);
    }

    private static CoreResult<string> ResolveLibraryName()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return CoreResult<string>.Ok("MaaCore.dll");
        }

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            return CoreResult<string>.Ok("libMaaCore.so");
        }

        if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
        {
            return CoreResult<string>.Ok("libMaaCore.dylib");
        }

        return Fail<string>(CoreErrorCode.PlatformNotSupported, $"Unsupported platform: {RuntimeInformation.OSDescription}");
    }

    private static bool TryLoadExports(nint library, out AsstExports exports, out string missingSymbol)
    {
        missingSymbol = string.Empty;
        TryLoadExport<AsstSetStaticOptionDelegate>(library, "AsstSetStaticOption", out var asstSetStaticOption);

        if (!TryLoadExport<AsstSetUserDirDelegate>(library, "AsstSetUserDir", out var asstSetUserDir))
        {
            missingSymbol = "AsstSetUserDir";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstLoadResourceDelegate>(library, "AsstLoadResource", out var asstLoadResource))
        {
            missingSymbol = "AsstLoadResource";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstCreateExDelegate>(library, "AsstCreateEx", out var asstCreateEx))
        {
            missingSymbol = "AsstCreateEx";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstDestroyDelegate>(library, "AsstDestroy", out var asstDestroy))
        {
            missingSymbol = "AsstDestroy";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstAsyncConnectDelegate>(library, "AsstAsyncConnect", out var asstAsyncConnect))
        {
            missingSymbol = "AsstAsyncConnect";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstAppendTaskDelegate>(library, "AsstAppendTask", out var asstAppendTask))
        {
            missingSymbol = "AsstAppendTask";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstStartDelegate>(library, "AsstStart", out var asstStart))
        {
            missingSymbol = "AsstStart";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstBackToHomeDelegate>(library, "AsstBackToHome", out var asstBackToHome))
        {
            missingSymbol = "AsstBackToHome";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstStopDelegate>(library, "AsstStop", out var asstStop))
        {
            missingSymbol = "AsstStop";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstRunningDelegate>(library, "AsstRunning", out var asstRunning))
        {
            missingSymbol = "AsstRunning";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstConnectedDelegate>(library, "AsstConnected", out var asstConnected))
        {
            missingSymbol = "AsstConnected";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstGetImageDelegate>(library, "AsstGetImage", out var asstGetImage))
        {
            missingSymbol = "AsstGetImage";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstGetNullSizeDelegate>(library, "AsstGetNullSize", out var asstGetNullSize))
        {
            missingSymbol = "AsstGetNullSize";
            exports = null!;
            return false;
        }

        if (!TryLoadExport<AsstGetVersionDelegate>(library, "AsstGetVersion", out var asstGetVersion))
        {
            missingSymbol = "AsstGetVersion";
            exports = null!;
            return false;
        }

        exports = new AsstExports(
            asstSetStaticOption,
            asstSetUserDir!,
            asstLoadResource!,
            asstCreateEx!,
            asstDestroy!,
            asstAsyncConnect!,
            asstAppendTask!,
            asstStart!,
            asstBackToHome!,
            asstStop!,
            asstRunning!,
            asstConnected!,
            asstGetImage!,
            asstGetNullSize!,
            asstGetVersion!);

        return true;
    }

    private static bool TryLoadExport<T>(nint library, string symbol, out T? function)
        where T : Delegate
    {
        function = null;
        if (!NativeLibrary.TryGetExport(library, symbol, out var export))
        {
            return false;
        }

        function = Marshal.GetDelegateForFunctionPointer<T>(export);
        return true;
    }

    private static bool AsBool(byte value) => value != 0;

    private static CoreResult<T> Fail<T>(
        CoreErrorCode code,
        string message,
        string? nativeDetails = null,
        Exception? ex = null)
    {
        return CoreResult<T>.Fail(new CoreError(code, message, nativeDetails, ex?.ToString()));
    }

    private static string ResolveMessageName(int msgId)
    {
        return msgId switch
        {
            0 => "InternalError",
            1 => "InitFailed",
            2 => "ConnectionInfo",
            3 => "AllTasksCompleted",
            4 => "AsyncCallInfo",
            5 => "Destroyed",
            10000 => "TaskChainError",
            10001 => "TaskChainStart",
            10002 => "TaskChainCompleted",
            10003 => "TaskChainExtraInfo",
            10004 => "TaskChainStopped",
            20000 => "SubTaskError",
            20001 => "SubTaskStart",
            20002 => "SubTaskCompleted",
            20003 => "SubTaskExtraInfo",
            20004 => "SubTaskStopped",
            30000 => "ReportRequest",
            _ => $"Unknown({msgId})",
        };
    }

    private static bool TryParsePayload(string payload, out JsonElement root)
    {
        root = default;
        if (string.IsNullOrWhiteSpace(payload))
        {
            return false;
        }

        try
        {
            using var doc = JsonDocument.Parse(payload);
            root = doc.RootElement.Clone();
            return true;
        }
        catch
        {
            return false;
        }
    }

    private static int? GetInt(JsonElement element, string name)
    {
        if (!element.TryGetProperty(name, out var value))
        {
            return null;
        }

        return value.ValueKind switch
        {
            JsonValueKind.Number when value.TryGetInt32(out var number) => number,
            JsonValueKind.String when int.TryParse(value.GetString(), out var number) => number,
            _ => null,
        };
    }

    private static bool GetBool(JsonElement element, string name)
    {
        if (!element.TryGetProperty(name, out var value))
        {
            return false;
        }

        return value.ValueKind switch
        {
            JsonValueKind.True => true,
            JsonValueKind.False => false,
            JsonValueKind.String when bool.TryParse(value.GetString(), out var b) => b,
            JsonValueKind.Number when value.TryGetInt32(out var number) => number != 0,
            _ => false,
        };
    }

    private static string? GetString(JsonElement element, string name)
    {
        if (!element.TryGetProperty(name, out var value))
        {
            return null;
        }

        return value.ValueKind switch
        {
            JsonValueKind.String => value.GetString(),
            JsonValueKind.Number => value.GetRawText(),
            JsonValueKind.True => bool.TrueString,
            JsonValueKind.False => bool.FalseString,
            _ => null,
        };
    }

    private static JsonElement? GetObject(JsonElement element, string name)
    {
        if (!element.TryGetProperty(name, out var value))
        {
            return null;
        }

        return value.ValueKind is JsonValueKind.Object ? value : null;
    }

    private static string BuildConnectionFailureMessage(JsonElement root, string? what)
    {
        var details = GetObject(root, "details");
        var rawOutput = details is JsonElement detailObject
            ? NormalizeFailureOutput(GetString(detailObject, "raw_output"))
            : null;

        if (!string.IsNullOrWhiteSpace(rawOutput))
        {
            return rawOutput;
        }

        var reason = GetString(root, "why");
        if (!string.IsNullOrWhiteSpace(reason))
        {
            return reason.Trim();
        }

        return string.IsNullOrWhiteSpace(what) ? "ConnectionInfo" : what.Trim();
    }

    private static string? NormalizeFailureOutput(string? rawOutput)
    {
        if (string.IsNullOrWhiteSpace(rawOutput))
        {
            return null;
        }

        return rawOutput
            .Replace("\r\n", "\n", StringComparison.Ordinal)
            .Replace('\r', '\n')
            .Trim('\n');
    }

    private sealed class ConnectPendingState
    {
        private int _completed;

        public ConnectPendingState(int asyncCallId)
        {
            AsyncCallId = asyncCallId;
            Completion = new TaskCompletionSource<CoreResult<bool>>(TaskCreationOptions.RunContinuationsAsynchronously);
        }

        public int AsyncCallId { get; }

        public bool AsyncCallReceived { get; private set; }

        public bool AsyncCallSucceeded { get; private set; }

        public bool ConnectionEstablished { get; private set; }

        public bool CanCompleteSuccess => AsyncCallReceived && AsyncCallSucceeded && ConnectionEstablished;

        public TaskCompletionSource<CoreResult<bool>> Completion { get; }

        public void MarkAsyncCall(bool succeeded)
        {
            AsyncCallReceived = true;
            AsyncCallSucceeded = succeeded;
        }

        public void MarkConnected()
        {
            ConnectionEstablished = true;
        }

        public void TryComplete(CoreResult<bool> result)
        {
            if (Interlocked.CompareExchange(ref _completed, 1, 0) != 0)
            {
                return;
            }

            Completion.TrySetResult(result);
        }
    }

    private sealed record AsstExports(
        AsstSetStaticOptionDelegate? AsstSetStaticOption,
        AsstSetUserDirDelegate AsstSetUserDir,
        AsstLoadResourceDelegate AsstLoadResource,
        AsstCreateExDelegate AsstCreateEx,
        AsstDestroyDelegate AsstDestroy,
        AsstAsyncConnectDelegate AsstAsyncConnect,
        AsstAppendTaskDelegate AsstAppendTask,
        AsstStartDelegate AsstStart,
        AsstBackToHomeDelegate AsstBackToHome,
        AsstStopDelegate AsstStop,
        AsstRunningDelegate AsstRunning,
        AsstConnectedDelegate AsstConnected,
        AsstGetImageDelegate AsstGetImage,
        AsstGetNullSizeDelegate AsstGetNullSize,
        AsstGetVersionDelegate AsstGetVersion);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate byte AsstSetStaticOptionDelegate(int key, [MarshalAs(UnmanagedType.LPUTF8Str)] string value);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate byte AsstSetUserDirDelegate([MarshalAs(UnmanagedType.LPUTF8Str)] string path);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate byte AsstLoadResourceDelegate([MarshalAs(UnmanagedType.LPUTF8Str)] string path);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate nint AsstCreateExDelegate(AsstApiCallbackDelegate callback, nint customArg);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate void AsstDestroyDelegate(nint handle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate int AsstAsyncConnectDelegate(
        nint handle,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string adbPath,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string address,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string config,
        byte block);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate int AsstAppendTaskDelegate(
        nint handle,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string type,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string @params);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate byte AsstStartDelegate(nint handle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate byte AsstBackToHomeDelegate(nint handle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate byte AsstStopDelegate(nint handle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate byte AsstRunningDelegate(nint handle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate byte AsstConnectedDelegate(nint handle);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate ulong AsstGetImageDelegate(nint handle, nint buffer, ulong bufferSize);

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate ulong AsstGetNullSizeDelegate();

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate nint AsstGetVersionDelegate();

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate void AsstApiCallbackDelegate(int msg, nint detailsJson, nint customArg);
}
