using MAAUnified.App.ViewModels;

namespace MAAUnified.Tests;

public sealed class FeatureManifestTests
{
    [Fact]
    public void FeatureManifest_HasExpectedCoverage()
    {
        var all = FeatureManifest.All;
        Assert.True(all.Count >= 44);

        var duplicateKeys = all.GroupBy(m => m.Key).Where(g => g.Count() > 1).Select(g => g.Key).ToList();
        Assert.Empty(duplicateKeys);

        Assert.Contains(all, m => m.Key == "Settings.ConfigurationManager");
        Assert.Contains(all, m => m.Key == "Task.PostAction");
        Assert.Contains(all, m => m.Key == "Advanced.Copilot");
        Assert.Contains(all, m => m.Key == "Dialog.Error");
    }
}
