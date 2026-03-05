namespace MAAUnified.Tests;

public sealed class BaselineFallbackParityPolicyTests
{
    [Fact]
    public void FallbackParityPolicy_ShouldMatchExpectedVsCurrentMode()
    {
        var baseline = BaselineTestSupport.LoadBaseline();

        foreach (var row in baseline.FallbackCapabilities)
        {
            if (string.Equals(row.ExpectedMode, "full", StringComparison.OrdinalIgnoreCase)
                && !string.Equals(row.CurrentMode, "full", StringComparison.OrdinalIgnoreCase))
            {
                Assert.Equal("Waived", row.ParityStatus);
                Assert.NotNull(row.Waiver);
                continue;
            }

            if (string.Equals(row.ExpectedMode, "degrade-visible", StringComparison.OrdinalIgnoreCase)
                && string.Equals(row.CurrentMode, "fallback", StringComparison.OrdinalIgnoreCase))
            {
                Assert.True(
                    string.Equals(row.ParityStatus, "Aligned", StringComparison.Ordinal)
                    || string.Equals(row.ParityStatus, "Waived", StringComparison.Ordinal),
                    $"Unexpected parity for {row.CapabilityId}:{row.Platform} => {row.ParityStatus}");
            }
        }
    }
}

