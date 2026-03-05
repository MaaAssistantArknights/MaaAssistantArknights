namespace MAAUnified.Tests;

public sealed class BaselineNoWaiverRemainingTests
{
    [Fact]
    public void Baseline_ShouldContainNoWaivedRows()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var acceptance = BaselineTestSupport.LoadAcceptanceTemplate();

        var waivedItems = baseline.Items
            .Where(item => string.Equals(item.ParityStatus, "Waived", StringComparison.Ordinal))
            .Select(item => item.ItemId)
            .ToList();
        var waivedMappings = baseline.ConfigKeyMappings
            .Where(mapping => string.Equals(mapping.ParityStatus, "Waived", StringComparison.Ordinal))
            .Select(mapping => mapping.Key)
            .ToList();
        var waivedFallbacks = baseline.FallbackCapabilities
            .Where(row => string.Equals(row.ParityStatus, "Waived", StringComparison.Ordinal))
            .Select(row => $"{row.CapabilityId}:{row.Platform}")
            .ToList();
        var waivedCases = acceptance.Cases
            .Where(testCase => testCase.Waiver is not null)
            .Select(testCase => testCase.CaseId)
            .ToList();

        var totalWaived = waivedItems.Count + waivedMappings.Count + waivedFallbacks.Count + waivedCases.Count;
        Assert.True(
            totalWaived == 0,
            $"Waived rows remain. items=[{string.Join(", ", waivedItems)}] mappings=[{string.Join(", ", waivedMappings)}] fallback=[{string.Join(", ", waivedFallbacks)}] cases=[{string.Join(", ", waivedCases)}]");
    }
}
