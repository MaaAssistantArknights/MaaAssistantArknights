using MAAUnified.Application.Models;
using MAAUnified.Compat.Constants;
using MAAUnified.CoreBridge;
using MAAUnified.Platform;
using System.Runtime.InteropServices;
using System.Text.Json.Nodes;

namespace MAAUnified.Application.Services;

public sealed class ResourceWorkflowService
{
    private readonly string _baseDirectory;
    private readonly IMaaCoreBridge _bridge;
    private readonly IGpuCapabilityService _gpuCapabilityService;
    private readonly UiLogService _logService;

    public ResourceWorkflowService(
        string baseDirectory,
        IMaaCoreBridge bridge,
        UiLogService logService,
        IGpuCapabilityService? gpuCapabilityService = null)
    {
        _baseDirectory = baseDirectory;
        _bridge = bridge;
        _logService = logService;
        _gpuCapabilityService = gpuCapabilityService ?? new UnsupportedGpuCapabilityService();
    }

    public async Task<CoreResult<CoreInitializeInfo>> InitializeCoreAsync(
        UnifiedConfig config,
        CancellationToken cancellationToken = default)
    {
        var clientType = ResolveClientType(config);
        var gpuPreference = ReadGpuPreference(config);
        var gpuResolution = _gpuCapabilityService.Resolve(gpuPreference);
        var gpuRequest = BuildGpuRequest(gpuPreference, gpuResolution);
        var libraryPath = Path.Combine(_baseDirectory, ResolveLibraryName());
        var resourceDirectory = Path.Combine(_baseDirectory, "resource");
        _logService.Debug(
            $"Core init request: base={_baseDirectory}, lib={libraryPath}, libExists={File.Exists(libraryPath)}, resource={resourceDirectory}, resourceExists={Directory.Exists(resourceDirectory)}, client={clientType ?? "<default>"}");
        LogGpuSelection(gpuPreference, gpuResolution);

        var result = await _bridge.InitializeAsync(
            new CoreInitializeRequest(_baseDirectory, clientType, gpuRequest),
            cancellationToken);

        if (result.Success)
        {
            _logService.Info($"Core initialized. lib={result.Value?.LibraryPath}, version={result.Value?.CoreVersion}, client={clientType}");
            LogGpuInitializeResult(result.Value?.Gpu, gpuResolution);
        }
        else
        {
            _logService.Error($"Core initialize failed: {result.Error?.Code} {result.Error?.Message}");
        }

        return result;
    }

    private static string? ResolveClientType(UnifiedConfig config)
    {
        if (config.Profiles.TryGetValue(config.CurrentProfile, out var profile)
            && profile.Values.TryGetValue("ClientType", out var clientTypeNode))
        {
            if (clientTypeNode is JsonValue value && value.TryGetValue(out string? clientType))
            {
                return clientType;
            }
        }

        return null;
    }

    private static GpuPreference ReadGpuPreference(UnifiedConfig config)
    {
        return new GpuPreference(
            UseGpu: ReadProfileBoolFlexible(config, ConfigurationKeys.PerformanceUseGpu, false),
            AllowDeprecatedGpu: ReadProfileBoolFlexible(config, ConfigurationKeys.PerformanceAllowDeprecatedGpu, false),
            PreferredGpuDescription: ReadProfileString(config, ConfigurationKeys.PerformancePreferredGpuDescription, string.Empty).Trim(),
            PreferredGpuInstancePath: ReadProfileString(config, ConfigurationKeys.PerformancePreferredGpuInstancePath, string.Empty).Trim());
    }

    private static CoreGpuInitializeRequest? BuildGpuRequest(GpuPreference preference, GpuSelectionResolution resolution)
    {
        if (resolution.Snapshot.SupportMode != GpuPlatformSupportMode.WindowsSupported)
        {
            return preference.UseGpu || preference.HasSpecificSelection
                ? new CoreGpuInitializeRequest(CoreGpuRequestMode.Cpu)
                : null;
        }

        return resolution.SelectedOption.Kind == GpuOptionKind.Disabled
            ? new CoreGpuInitializeRequest(CoreGpuRequestMode.Cpu)
            : new CoreGpuInitializeRequest(
                CoreGpuRequestMode.Gpu,
                resolution.SelectedOption.GpuIndex,
                resolution.SelectedOption.Description);
    }

    private void LogGpuSelection(GpuPreference preference, GpuSelectionResolution resolution)
    {
        if (!string.IsNullOrWhiteSpace(resolution.SelectionWarningTextKey))
        {
            _logService.Warn(
                $"Saved GPU selection is unavailable on this machine. Falling back to `{resolution.SelectedOption.Kind}`.");
        }

        switch (resolution.Snapshot.SupportMode)
        {
            case GpuPlatformSupportMode.WindowsSupported:
                if (preference.UseGpu && resolution.SelectedOption.Kind != GpuOptionKind.Disabled)
                {
                    _logService.Info(
                        $"GPU OCR requested. option={resolution.SelectedOption.Kind}, adapter={resolution.SelectedOption.Description}, index={resolution.SelectedOption.GpuIndex?.ToString() ?? "<none>"}");
                }

                if (resolution.SelectedOption.IsDeprecated)
                {
                    _logService.Warn(
                        $"Selected GPU `{resolution.SelectedOption.Description}` is marked deprecated for DirectML OCR.");
                }

                if (resolution.SelectedOption.DriverDate.HasValue
                    && resolution.SelectedOption.DriverDate.Value < GpuCapabilityConstants.DirectMlDriverMinimumDate)
                {
                    _logService.Warn(
                        $"Selected GPU driver predates DirectML support ({resolution.SelectedOption.DriverDate:yyyy-MM-dd}).");
                }

                break;

            case GpuPlatformSupportMode.Unsupported when preference.UseGpu:
                _logService.Warn("GPU config unsupported on current platform. Falling back to CPU OCR.");
                break;
        }
    }

    private void LogGpuInitializeResult(CoreGpuInitializeInfo? gpuInfo, GpuSelectionResolution resolution)
    {
        if (gpuInfo is null)
        {
            return;
        }

        if (gpuInfo.RequestedMode == CoreGpuRequestMode.Gpu && gpuInfo.AppliedMode == CoreGpuAppliedMode.Gpu)
        {
            _logService.Info(
                $"GPU OCR applied successfully. adapter={resolution.SelectedOption.Description}, index={gpuInfo.AppliedGpuIndex?.ToString() ?? resolution.SelectedOption.GpuIndex?.ToString() ?? "<none>"}");
        }

        if (gpuInfo.Warnings is not null)
        {
            foreach (var warning in gpuInfo.Warnings.Where(static warning => !string.IsNullOrWhiteSpace(warning)))
            {
                _logService.Warn(warning);
            }
        }
    }

    private static string ReadProfileString(UnifiedConfig config, string key, string fallback)
    {
        if (TryGetProfileValue(config, key, out var value))
        {
            return value;
        }

        return config.GlobalValues.TryGetValue(key, out var globalValue) && globalValue is not null
            ? globalValue.ToString()
            : fallback;
    }

    private static bool ReadProfileBoolFlexible(UnifiedConfig config, string key, bool fallback)
    {
        var text = ReadProfileString(config, key, string.Empty);
        if (bool.TryParse(text, out var parsedBool))
        {
            return parsedBool;
        }

        if (int.TryParse(text, out var parsedInt))
        {
            return parsedInt != 0;
        }

        return fallback;
    }

    private static bool TryGetProfileValue(UnifiedConfig config, string key, out string value)
    {
        value = string.Empty;
        if (!config.Profiles.TryGetValue(config.CurrentProfile, out var profile)
            || !profile.Values.TryGetValue(key, out var node)
            || node is null)
        {
            return false;
        }

        value = node.ToString();
        return true;
    }

    private static string ResolveLibraryName()
    {
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
        {
            return "MaaCore.dll";
        }

        if (RuntimeInformation.IsOSPlatform(OSPlatform.Linux))
        {
            return "libMaaCore.so";
        }

        if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
        {
            return "libMaaCore.dylib";
        }

        return "MaaCore.unknown";
    }
}
