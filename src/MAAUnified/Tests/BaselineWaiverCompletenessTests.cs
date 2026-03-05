using MAAUnified.Compat.Mapping.Baseline;

namespace MAAUnified.Tests;

public sealed class BaselineWaiverCompletenessTests
{
    [Fact]
    public void WaivedRows_ShouldProvideCompleteWaiverAndBeUnexpired()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var today = DateOnly.FromDateTime(DateTime.UtcNow.Date);

        foreach (var item in baseline.Items.Where(item => string.Equals(item.ParityStatus, "Waived", StringComparison.Ordinal)))
        {
            AssertWaiver(item.Waiver, today, $"item:{item.ItemId}");
            AssertScope(item.WaiverScope, $"item:{item.ItemId}");
        }

        foreach (var mapping in baseline.ConfigKeyMappings.Where(mapping => string.Equals(mapping.ParityStatus, "Waived", StringComparison.Ordinal)))
        {
            AssertWaiver(mapping.Waiver, today, $"mapping:{mapping.OwnerItemId}:{mapping.Key}");
        }

        foreach (var fallback in baseline.FallbackCapabilities.Where(row => string.Equals(row.ParityStatus, "Waived", StringComparison.Ordinal)))
        {
            AssertWaiver(fallback.Waiver, today, $"fallback:{fallback.CapabilityId}:{fallback.Platform}");
            AssertScope(fallback.WaiverScope, $"fallback:{fallback.CapabilityId}:{fallback.Platform}");
        }
    }

    private static void AssertWaiver(
        WaiverSpec? waiver,
        DateOnly today,
        string subject)
    {
        Assert.NotNull(waiver);
        Assert.False(string.IsNullOrWhiteSpace(waiver!.Owner), $"Missing owner => {subject}");
        Assert.False(string.IsNullOrWhiteSpace(waiver.Reason), $"Missing reason => {subject}");
        Assert.False(string.IsNullOrWhiteSpace(waiver.AlternativeValidation), $"Missing alternative validation => {subject}");
        Assert.True(DateOnly.TryParse(waiver.ExpiresOn, out var expiresOn), $"Invalid expires_on => {subject}");
        Assert.True(expiresOn >= today, $"Expired waiver => {subject}");
    }

    private static void AssertScope(
        WaiverScope? scope,
        string subject)
    {
        Assert.NotNull(scope);
        Assert.True(
            scope!.Platforms.Count > 0
            || scope.Themes.Count > 0
            || scope.Locales.Count > 0,
            $"Missing waiver scope dimensions => {subject}");
    }
}
