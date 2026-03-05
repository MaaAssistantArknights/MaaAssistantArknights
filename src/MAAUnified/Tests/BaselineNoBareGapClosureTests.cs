namespace MAAUnified.Tests;

public sealed class BaselineNoBareGapClosureTests
{
    [Fact]
    public void Baseline_ShouldNotContainBareGapEntries()
    {
        var baseline = BaselineTestSupport.LoadBaseline();

        var itemGaps = baseline.Items
            .Where(item => string.Equals(item.ParityStatus, "Gap", StringComparison.Ordinal))
            .Select(item => item.ItemId)
            .ToList();
        var mappingGaps = baseline.ConfigKeyMappings
            .Where(mapping => string.Equals(mapping.ParityStatus, "Gap", StringComparison.Ordinal))
            .Select(mapping => $"{mapping.OwnerItemId}:{mapping.Key}")
            .ToList();
        var fallbackGaps = baseline.FallbackCapabilities
            .Where(row => string.Equals(row.ParityStatus, "Gap", StringComparison.Ordinal))
            .Select(row => $"{row.CapabilityId}:{row.Platform}")
            .ToList();

        Assert.True(itemGaps.Count == 0, $"Feature/System gaps remain: {string.Join(", ", itemGaps)}");
        Assert.True(mappingGaps.Count == 0, $"Config key gaps remain: {string.Join(", ", mappingGaps)}");
        Assert.True(fallbackGaps.Count == 0, $"Fallback gaps remain: {string.Join(", ", fallbackGaps)}");
    }
}

