namespace MAAUnified.Tests;

public sealed class WorkPackageQ1BlockingCoverageTests
{
    [Fact]
    public void RootDashboard_BlockedPath_ShouldBeDefinedWithFailureAndNonCrashEvidence()
    {
        AssertBlockedPathCoverage("RootDashboard");
    }

    [Fact]
    public void SettingsRoot_BlockedPath_ShouldBeDefinedWithFailureAndNonCrashEvidence()
    {
        AssertBlockedPathCoverage("SettingsRoot");
    }

    [Fact]
    public void SettingsAbout_ErrorPath_ShouldBeDefinedWithFailureAndNonCrashEvidence()
    {
        AssertBlockedPathCoverage("Settings.About");
    }

    [Fact]
    public void SettingsAchievement_ErrorPath_ShouldBeDefinedWithFailureAndNonCrashEvidence()
    {
        AssertBlockedPathCoverage("Settings.Achievement");
    }

    [Fact]
    public void SettingsHotKeyEditor_BlockedPath_ShouldBeDefinedWithFailureAndNonCrashEvidence()
    {
        AssertBlockedPathCoverage("Settings.HotKeyEditor");
    }

    [Fact]
    public void SettingsIssueReport_ErrorPath_ShouldBeDefinedWithFailureAndNonCrashEvidence()
    {
        AssertBlockedPathCoverage("Settings.IssueReport");
    }

    [Fact]
    public void SettingsVersionUpdate_BlockedPath_ShouldBeDefinedWithFailureAndNonCrashEvidence()
    {
        AssertBlockedPathCoverage("Settings.VersionUpdate");
    }

    private static void AssertBlockedPathCoverage(string itemId)
    {
        var baseline = BaselineTestSupport.LoadBaseline();
        var acceptance = BaselineTestSupport.LoadAcceptanceTemplate();

        var item = Assert.Single(baseline.Items, i => string.Equals(i.ItemId, itemId, StringComparison.Ordinal));
        Assert.Equal("Aligned", item.ParityStatus);
        Assert.NotEmpty(item.ErrorFeedback);
        Assert.Contains(item.ErrorFeedback, feedback =>
            feedback.NonCrash
            && !string.IsNullOrWhiteSpace(feedback.LogPath)
            && !string.IsNullOrWhiteSpace(feedback.Trigger));
        Assert.Contains(item.Interactions, interaction => !string.IsNullOrWhiteSpace(interaction.FallbackBehavior));

        var testCase = Assert.Single(acceptance.Cases, c => string.Equals(c.ItemId, itemId, StringComparison.Ordinal));
        Assert.True(testCase.NonCrashRequired);
        Assert.Contains(testCase.Steps, step => step.Contains("failure", StringComparison.OrdinalIgnoreCase));
        Assert.Contains(testCase.Expected, expected => expected.Contains("log", StringComparison.OrdinalIgnoreCase));
        Assert.Contains(testCase.Expected, expected => expected.Contains("alive", StringComparison.OrdinalIgnoreCase));
    }
}
