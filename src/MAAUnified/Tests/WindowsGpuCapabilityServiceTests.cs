using MAAUnified.Platform;

namespace MAAUnified.Tests;

public sealed class WindowsGpuCapabilityServiceTests
{
    [Fact]
    public void BuildOptionList_FirstAcceptedGpuBecomesSystemDefault_EvenWhenAdapterIndexIsNotZero()
    {
        var options = WindowsGpuCapabilityService.BuildOptionList(
            allowDeprecatedGpu: false,
            candidates:
            [
                new WindowsGpuCapabilityService.WindowsGpuCandidate(
                    AdapterIndex: 2,
                    Description: "NVIDIA GeForce RTX 4060",
                    InstancePath: "PCI#2",
                    IsDeprecated: false,
                    DriverDate: new DateTime(2024, 6, 1),
                    DriverVersion: "555.99"),
            ]);

        Assert.Equal(3, options.Count);
        Assert.Equal(GpuOptionKind.Disabled, options[0].Kind);
        Assert.Equal(GpuOptionKind.SystemDefault, options[1].Kind);
        Assert.Equal("NVIDIA GeForce RTX 4060", options[1].Description);
        Assert.Equal(GpuOptionKind.SpecificGpu, options[2].Kind);
        Assert.True(options[2].GpuIndex.HasValue);
        Assert.Equal((uint)2, options[2].GpuIndex.Value);
        Assert.Equal("PCI#2", options[2].InstancePath);
    }

    [Fact]
    public void BuildOptionList_MissingInstancePathStillProducesSpecificGpuOption()
    {
        var options = WindowsGpuCapabilityService.BuildOptionList(
            allowDeprecatedGpu: false,
            candidates:
            [
                new WindowsGpuCapabilityService.WindowsGpuCandidate(
                    AdapterIndex: 3,
                    Description: "AMD Radeon RX 7800 XT",
                    InstancePath: string.Empty,
                    IsDeprecated: false,
                    DriverDate: null,
                    DriverVersion: null),
            ]);

        var specific = Assert.Single(options.Where(option => option.Kind == GpuOptionKind.SpecificGpu));
        Assert.Equal(string.Empty, specific.InstancePath);
        Assert.True(specific.GpuIndex.HasValue);
        Assert.Equal((uint)3, specific.GpuIndex.Value);
        Assert.True(specific.Id.StartsWith("DXGI#3#AMD Radeon RX 7800 XT", StringComparison.Ordinal));
    }

    [Fact]
    public void BuildOptionList_FiltersDeprecatedGpuAndPromotesFirstSupportedCandidateToDefault()
    {
        var options = WindowsGpuCapabilityService.BuildOptionList(
            allowDeprecatedGpu: false,
            candidates:
            [
                new WindowsGpuCapabilityService.WindowsGpuCandidate(
                    AdapterIndex: 0,
                    Description: "Legacy GPU",
                    InstancePath: "PCI#0",
                    IsDeprecated: true,
                    DriverDate: new DateTime(2018, 1, 1),
                    DriverVersion: "1.0"),
                new WindowsGpuCapabilityService.WindowsGpuCandidate(
                    AdapterIndex: 1,
                    Description: "Intel Arc A770",
                    InstancePath: "PCI#1",
                    IsDeprecated: false,
                    DriverDate: new DateTime(2024, 1, 1),
                    DriverVersion: "31.0"),
            ]);

        Assert.Equal(3, options.Count);
        Assert.DoesNotContain("Legacy GPU", options.Select(option => option.Description));
        Assert.Equal(GpuOptionKind.SystemDefault, options[1].Kind);
        Assert.Equal("Intel Arc A770", options[1].Description);
        Assert.Equal(
            "PCI#1",
            Assert.Single(options.Where(option => option.Kind == GpuOptionKind.SpecificGpu)).InstancePath);
    }
}
