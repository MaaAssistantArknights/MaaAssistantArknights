using MAAUnified.App.ViewModels;

namespace MAAUnified.Tests;

public sealed class BaselineCoverageTests
{
    [Fact]
    public void BaselineCoverage_ShouldMatchFeatureManifest44()
    {
        var baseline = BaselineTestSupport.LoadBaseline();

        var featureItems = baseline.Items
            .Where(i => i.Kind == "Feature")
            .Select(i => i.ItemId)
            .ToList();

        Assert.Equal(44, featureItems.Count);

        var duplicates = featureItems.GroupBy(i => i).Where(g => g.Count() > 1).Select(g => g.Key).ToList();
        Assert.Empty(duplicates);

        var manifestKeys = FeatureManifest.All.Select(m => m.Key).ToHashSet(StringComparer.Ordinal);
        var baselineKeys = featureItems.ToHashSet(StringComparer.Ordinal);

        Assert.True(manifestKeys.SetEquals(baselineKeys),
            $"Baseline feature keys mismatch. Missing: {string.Join(", ", manifestKeys.Except(baselineKeys))}; Extra: {string.Join(", ", baselineKeys.Except(manifestKeys))}");
    }

    [Fact]
    public void BaselineConfigKeys_ShouldCoverAll290LegacyKeys()
    {
        var baseline = BaselineTestSupport.LoadBaseline();

        var legacyKeys = BaselineTestSupport.GetLegacyConfigurationKeys();
        var baselineKeys = baseline.ConfigKeyMappings.Select(k => k.Key).ToList();

        Assert.Equal(290, legacyKeys.Count);
        Assert.Equal(290, baselineKeys.Count);

        var legacySet = legacyKeys.ToHashSet(StringComparer.Ordinal);
        var baselineSet = baselineKeys.ToHashSet(StringComparer.Ordinal);

        Assert.True(legacySet.SetEquals(baselineSet),
            $"Config key coverage mismatch. Missing: {string.Join(", ", legacySet.Except(baselineSet))}; Extra: {string.Join(", ", baselineSet.Except(legacySet))}");

        var duplicates = baselineKeys.GroupBy(k => k).Where(g => g.Count() > 1).Select(g => g.Key).ToList();
        Assert.Empty(duplicates);
    }

    [Fact]
    public void BaselineFallback_ShouldCover5CapabilitiesAcross3Platforms()
    {
        var baseline = BaselineTestSupport.LoadBaseline();

        var expectedCapabilities = new HashSet<string>(StringComparer.Ordinal)
        {
            "Tray",
            "Notification",
            "Hotkey",
            "Autostart",
            "Overlay",
        };

        var expectedPlatforms = new HashSet<string>(StringComparer.Ordinal)
        {
            "windows",
            "macos",
            "linux",
        };

        var capabilities = baseline.FallbackCapabilities
            .Select(f => f.CapabilityId)
            .ToHashSet(StringComparer.Ordinal);

        Assert.True(expectedCapabilities.SetEquals(capabilities));

        foreach (var capability in expectedCapabilities)
        {
            var platforms = baseline.FallbackCapabilities
                .Where(f => f.CapabilityId == capability)
                .Select(f => f.Platform)
                .ToHashSet(StringComparer.Ordinal);

            Assert.True(expectedPlatforms.SetEquals(platforms),
                $"Capability `{capability}` platform coverage mismatch: {string.Join(", ", platforms)}");
        }

        Assert.Equal(15, baseline.FallbackCapabilities.Count);
    }

    [Fact]
    public void BaselineLocaleTheme_ShouldCoverRequiredMatrix()
    {
        var baseline = BaselineTestSupport.LoadBaseline();

        var expectedThemes = new[] { "Light", "Dark" };
        var expectedLocales = new[] { "zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr", "pallas" };

        Assert.Equal(expectedThemes, baseline.Themes);
        Assert.Equal(expectedLocales, baseline.Locales);
    }

    [Fact]
    public void AcceptanceTemplate_ShouldReferenceExistingBaselineItems()
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var acceptance = BaselineTestSupport.LoadAcceptanceTemplate();

        var itemIds = baseline.Items.Select(i => i.ItemId).ToHashSet(StringComparer.Ordinal);

        var duplicateCaseIds = acceptance.Cases.GroupBy(c => c.CaseId).Where(g => g.Count() > 1).Select(g => g.Key).ToList();
        Assert.Empty(duplicateCaseIds);

        Assert.All(acceptance.Cases, testCase =>
        {
            Assert.True(itemIds.Contains(testCase.ItemId), $"Case `{testCase.CaseId}` references unknown item `{testCase.ItemId}`.");
            Assert.NotEmpty(testCase.Platforms);
            Assert.NotEmpty(testCase.Themes);
            Assert.NotEmpty(testCase.Locales);
            Assert.NotEmpty(testCase.Steps);
            Assert.NotEmpty(testCase.Expected);
            Assert.True(testCase.NonCrashRequired);
        });

        var covered = acceptance.Cases.Select(c => c.ItemId).ToHashSet(StringComparer.Ordinal);
        Assert.True(itemIds.SetEquals(covered),
            $"Acceptance case coverage mismatch. Missing: {string.Join(", ", itemIds.Except(covered))}");
    }
}
