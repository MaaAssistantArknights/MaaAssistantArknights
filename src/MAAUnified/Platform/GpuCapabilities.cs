namespace MAAUnified.Platform;

public enum GpuPlatformSupportMode
{
    WindowsSupported = 0,
    Unsupported = 1,
}

public enum GpuOptionKind
{
    Disabled = 0,
    SystemDefault = 1,
    SpecificGpu = 2,
}

public static class GpuCapabilityConstants
{
    public static readonly DateTime DirectMlDriverMinimumDate = new(2019, 5, 21);
}

public sealed record GpuPreference(
    bool UseGpu,
    bool AllowDeprecatedGpu,
    string PreferredGpuDescription,
    string PreferredGpuInstancePath)
{
    public bool HasSpecificSelection =>
        !string.IsNullOrWhiteSpace(PreferredGpuDescription)
        || !string.IsNullOrWhiteSpace(PreferredGpuInstancePath);
}

public sealed record GpuOptionDescriptor(
    string Id,
    GpuOptionKind Kind,
    string DisplayName,
    string Description,
    string InstancePath,
    uint? GpuIndex = null,
    bool IsDeprecated = false,
    bool IsCustomEntry = false,
    DateTime? DriverDate = null,
    string? DriverVersion = null)
{
    public static GpuOptionDescriptor Disabled { get; } = new(
        Id: "disabled",
        Kind: GpuOptionKind.Disabled,
        DisplayName: string.Empty,
        Description: string.Empty,
        InstancePath: string.Empty);

    public static GpuOptionDescriptor SystemDefault(
        string displayName,
        bool isDeprecated = false,
        DateTime? driverDate = null,
        string? driverVersion = null)
        => new(
            Id: "system-default",
            Kind: GpuOptionKind.SystemDefault,
            DisplayName: string.Empty,
            Description: displayName,
            InstancePath: string.Empty,
            GpuIndex: 0,
            IsDeprecated: isDeprecated,
            DriverDate: driverDate,
            DriverVersion: driverVersion);
}

public sealed record GpuCapabilitySnapshot(
    GpuPlatformSupportMode SupportMode,
    bool IsEditable,
    bool AppliesToCore,
    bool SupportsDeprecatedToggle,
    IReadOnlyList<GpuOptionDescriptor> Options,
    string StatusTextKey,
    string? WarningTextKey = null,
    string Provider = "unknown");

public sealed record GpuSelectionResolution(
    GpuCapabilitySnapshot Snapshot,
    GpuOptionDescriptor SelectedOption,
    bool SelectionChanged = false,
    string? SelectionWarningTextKey = null);

public interface IGpuCapabilityService
{
    GpuSelectionResolution Resolve(GpuPreference preference);
}

public sealed class UnsupportedGpuCapabilityService : IGpuCapabilityService
{
    public GpuSelectionResolution Resolve(GpuPreference preference)
    {
        var selected = GpuOptionDescriptor.Disabled;
        return new GpuSelectionResolution(
            Snapshot: new GpuCapabilitySnapshot(
                SupportMode: GpuPlatformSupportMode.Unsupported,
                IsEditable: false,
                AppliesToCore: false,
                SupportsDeprecatedToggle: false,
                Options: [GpuOptionDescriptor.Disabled],
                StatusTextKey: "Settings.Performance.Gpu.Status.Unsupported",
                WarningTextKey: preference.UseGpu ? "Settings.Performance.Gpu.Warning.Unsupported" : null,
                Provider: "unsupported"),
            SelectedOption: selected);
    }
}
