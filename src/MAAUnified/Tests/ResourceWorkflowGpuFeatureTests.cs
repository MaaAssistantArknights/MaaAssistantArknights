using System.Runtime.CompilerServices;
using System.Text.Json.Nodes;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class ResourceWorkflowGpuFeatureTests
{
    [Fact]
    public async Task InitializeCoreAsync_WindowsSpecificGpu_PassesGpuRequestToBridge()
    {
        var bridge = new CapturingBridge();
        var log = new UiLogService();
        var service = new ResourceWorkflowService("/tmp/maa", bridge, log, new ScriptedWindowsGpuCapabilityService());
        var config = BuildConfig(new Dictionary<string, string>
        {
            [ConfigurationKeys.PerformanceUseGpu] = "True",
            [ConfigurationKeys.PerformancePreferredGpuDescription] = "RTX",
            [ConfigurationKeys.PerformancePreferredGpuInstancePath] = "PCI#0",
        });

        var result = await service.InitializeCoreAsync(config);

        Assert.True(result.Success);
        Assert.NotNull(bridge.LastRequest);
        Assert.Equal(CoreGpuRequestMode.Gpu, bridge.LastRequest!.Gpu?.Mode);
        Assert.Equal((uint)1, bridge.LastRequest.Gpu?.GpuIndex);
    }

    [Fact]
    public async Task InitializeCoreAsync_WindowsCpuMode_PassesCpuRequestToBridge()
    {
        var bridge = new CapturingBridge();
        var log = new UiLogService();
        var service = new ResourceWorkflowService("/tmp/maa", bridge, log, new ScriptedWindowsGpuCapabilityService());
        var config = BuildConfig(new Dictionary<string, string>
        {
            [ConfigurationKeys.PerformanceUseGpu] = "False",
        });

        var result = await service.InitializeCoreAsync(config);

        Assert.True(result.Success);
        Assert.Equal(CoreGpuRequestMode.Cpu, bridge.LastRequest?.Gpu?.Mode);
    }

    [Fact]
    public async Task InitializeCoreAsync_UnsupportedGpuConfig_ForcesCpuRequestAndLogsWarning()
    {
        var bridge = new CapturingBridge();
        var log = new UiLogService();
        var service = new ResourceWorkflowService("/tmp/maa", bridge, log, new UnsupportedGpuCapabilityService());
        var config = BuildConfig(new Dictionary<string, string>
        {
            [ConfigurationKeys.PerformanceUseGpu] = "True",
            [ConfigurationKeys.PerformancePreferredGpuDescription] = "Legacy GPU",
            [ConfigurationKeys.PerformancePreferredGpuInstancePath] = "PCI#0",
        });

        var result = await service.InitializeCoreAsync(config);

        Assert.True(result.Success);
        Assert.Equal(CoreGpuRequestMode.Cpu, bridge.LastRequest?.Gpu?.Mode);
        Assert.Contains(
            log.Snapshot,
            entry => entry.Level == "WARN"
                && entry.Message.Contains("unsupported", StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public async Task InitializeCoreAsync_GpuProbeFailure_FallsBackToCpuAndLogsDiagnostics()
    {
        var bridge = new CapturingBridge();
        var log = new UiLogService();
        var service = new ResourceWorkflowService("/tmp/maa", bridge, log, new ThrowingGpuCapabilityService());
        var config = BuildConfig(new Dictionary<string, string>
        {
            [ConfigurationKeys.PerformanceUseGpu] = "True",
            [ConfigurationKeys.PerformancePreferredGpuDescription] = "RTX",
            [ConfigurationKeys.PerformancePreferredGpuInstancePath] = "PCI#0",
        });

        var result = await service.InitializeCoreAsync(config);

        Assert.True(result.Success);
        Assert.Equal(CoreGpuRequestMode.Cpu, bridge.LastRequest?.Gpu?.Mode);
        Assert.Contains(
            log.Snapshot,
            entry => entry.Level == "ERROR"
                && entry.Message.Contains("GPU capability probe failed during core initialization", StringComparison.OrdinalIgnoreCase));
        Assert.Contains(
            log.Snapshot,
            entry => entry.Level == "WARN"
                && entry.Message.Contains("Falling back to CPU OCR for this session", StringComparison.OrdinalIgnoreCase));
    }

    private static UnifiedConfig BuildConfig(IReadOnlyDictionary<string, string> profileValues)
    {
        var profile = new UnifiedProfile();
        foreach (var (key, value) in profileValues)
        {
            profile.Values[key] = JsonValue.Create(value);
        }

        return new UnifiedConfig
        {
            CurrentProfile = "Default",
            Profiles = new Dictionary<string, UnifiedProfile>(StringComparer.OrdinalIgnoreCase)
            {
                ["Default"] = profile,
            },
        };
    }

    private sealed class CapturingBridge : IMaaCoreBridge
    {
        public CoreInitializeRequest? LastRequest { get; private set; }

        public Task<CoreResult<CoreInitializeInfo>> InitializeAsync(
            CoreInitializeRequest request,
            CancellationToken cancellationToken = default)
        {
            LastRequest = request;
            return Task.FromResult(
                CoreResult<CoreInitializeInfo>.Ok(
                    new CoreInitializeInfo(
                        request.BaseDirectory,
                        "captured",
                        "fake",
                        request.ClientType,
                        request.Gpu is null
                            ? null
                            : new CoreGpuInitializeInfo(
                                request.Gpu.Mode,
                                request.Gpu.Mode == CoreGpuRequestMode.Gpu ? CoreGpuAppliedMode.Gpu : CoreGpuAppliedMode.Cpu,
                                request.Gpu.GpuIndex,
                                request.Gpu.GpuIndex))));
        }

        public Task<CoreResult<bool>> ConnectAsync(CoreConnectionInfo connectionInfo, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<int>> AppendTaskAsync(CoreTaskRequest task, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<int>.Ok(1));

        public Task<CoreResult<bool>> StartAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<bool>> StopAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Ok(true));

        public Task<CoreResult<CoreRuntimeStatus>> GetRuntimeStatusAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<CoreRuntimeStatus>.Ok(new CoreRuntimeStatus(true, true, false)));

        public Task<CoreResult<bool>> AttachWindowAsync(CoreAttachWindowRequest request, CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<bool>.Fail(new CoreError(CoreErrorCode.NotSupported, "unsupported")));

        public Task<CoreResult<byte[]>> GetImageAsync(CancellationToken cancellationToken = default)
            => Task.FromResult(CoreResult<byte[]>.Fail(new CoreError(CoreErrorCode.NotSupported, "unsupported")));

        public async IAsyncEnumerable<CoreCallbackEvent> CallbackStreamAsync([EnumeratorCancellation] CancellationToken cancellationToken = default)
        {
            await Task.CompletedTask;
            yield break;
        }

        public ValueTask DisposeAsync() => ValueTask.CompletedTask;
    }

    private sealed class ScriptedWindowsGpuCapabilityService : IGpuCapabilityService
    {
        public GpuSelectionResolution Resolve(GpuPreference preference)
        {
            var options = new List<GpuOptionDescriptor>
            {
                GpuOptionDescriptor.Disabled,
                GpuOptionDescriptor.SystemDefault("RTX"),
                new(
                    Id: "PCI#0",
                    Kind: GpuOptionKind.SpecificGpu,
                    DisplayName: "RTX",
                    Description: "RTX",
                    InstancePath: "PCI#0",
                    GpuIndex: 1),
            };

            var selected = !preference.UseGpu
                ? GpuOptionDescriptor.Disabled
                : options.FirstOrDefault(option => option.InstancePath == preference.PreferredGpuInstancePath)
                    ?? options.First(option => option.Kind == GpuOptionKind.SystemDefault);

            return new GpuSelectionResolution(
                Snapshot: new GpuCapabilitySnapshot(
                    SupportMode: GpuPlatformSupportMode.WindowsSupported,
                    IsEditable: true,
                    AppliesToCore: true,
                    SupportsDeprecatedToggle: true,
                    Options: options,
                    StatusTextKey: "Settings.Performance.Gpu.Status.WindowsReady",
                    Provider: "scripted-windows"),
                SelectedOption: selected);
        }
    }

    private sealed class ThrowingGpuCapabilityService : IGpuCapabilityService
    {
        public GpuSelectionResolution Resolve(GpuPreference preference)
        {
            throw new InvalidOperationException("Synthetic GPU probe failure for test coverage.");
        }
    }
}
