namespace MAAUnified.Tests;

public sealed class SettingsViewStructureContractTests
{
    [Fact]
    public void PrimarySettingsViews_ShouldUseSharedSettingsFormLayoutClasses()
    {
        var root = GetMaaUnifiedRoot();
        var files =
            new[]
            {
                "App/Features/Settings/GuiSettingsView.axaml",
                "App/Features/Settings/BackgroundSettingsView.axaml",
                "App/Features/Settings/PerformanceSettingsView.axaml",
                "App/Features/Settings/StartSettingsView.axaml",
                "App/Features/Settings/RemoteControlSettingsView.axaml",
                "App/Features/Settings/ExternalNotificationSettingsView.axaml",
                "App/Features/Settings/HotKeySettingsView.axaml",
                "App/Features/Settings/TimerSettingsView.axaml",
                "App/Features/Settings/GameSettingsView.axaml",
                "App/Features/Settings/ConnectSettingsView.axaml",
            };

        foreach (var relative in files)
        {
            var text = File.ReadAllText(Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar)));
            Assert.Contains("settings-form", text, StringComparison.Ordinal);
        }
    }

    [Fact]
    public void DependentSettingsOptions_ShouldUseConditionalVisibilityOrEnablement()
    {
        var root = GetMaaUnifiedRoot();

        var gui = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "GuiSettingsView.axaml"));
        Assert.Contains("IsVisible=\"{Binding CanMinimizeToTray}\"", gui, StringComparison.Ordinal);

        var start = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "StartSettingsView.axaml"));
        Assert.Contains("IsVisible=\"{Binding CanEditEmulatorLaunchSettings}\"", start, StringComparison.Ordinal);

        var timer = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "TimerSettingsView.axaml"));
        Assert.Contains("IsVisible=\"{Binding ForceScheduledStart}\"", timer, StringComparison.Ordinal);

        var achievement = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "AchievementSettingsView.axaml"));
        Assert.Contains("IsVisible=\"{Binding CanEditAchievementPopupAutoClose}\"", achievement, StringComparison.Ordinal);

        var external = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "ExternalNotificationSettingsView.axaml"));
        Assert.Contains("IsEnabled=\"{Binding CanEditExternalNotification}\"", external, StringComparison.Ordinal);
        Assert.Contains("IsEnabled=\"{Binding CanEditExternalNotificationDetails}\"", external, StringComparison.Ordinal);
    }

    [Fact]
    public void TimerSettingsView_ShouldRenderValidationMessageAfterTimerSlots()
    {
        var root = GetMaaUnifiedRoot();
        var timer = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "TimerSettingsView.axaml"));

        var itemsControlIndex = timer.IndexOf("<ItemsControl", StringComparison.Ordinal);
        var validationMessageIndex = timer.IndexOf("TimerValidationMessage", StringComparison.Ordinal);

        Assert.True(itemsControlIndex >= 0, "Timer settings view should contain the timer slots list.");
        Assert.True(validationMessageIndex >= 0, "Timer settings view should contain the timer validation message binding.");
        Assert.True(
            itemsControlIndex < validationMessageIndex,
            "Timer validation message should be rendered after the timer slots list.");
    }

    [Fact]
    public void ControlStyles_ShouldExposeSharedSettingsFormResources()
    {
        var root = GetMaaUnifiedRoot();
        var text = File.ReadAllText(Path.Combine(root, "App", "Styles", "ControlStyles.axaml"));

        Assert.Contains("x:Key=\"MAA.Size.Settings.FormMaxWidth\"", text, StringComparison.Ordinal);
        Assert.Contains("x:Key=\"MAA.Layout.Settings.RowLabelField\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Grid.settings-form\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"WrapPanel.settings-wrap\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"TextBlock.settings-label\"", text, StringComparison.Ordinal);
    }

    private static string GetMaaUnifiedRoot()
    {
        var current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current is not null)
        {
            var appDir = Path.Combine(current.FullName, "App");
            var testsDir = Path.Combine(current.FullName, "Tests");
            if (Directory.Exists(appDir) && Directory.Exists(testsDir))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new DirectoryNotFoundException("Cannot locate src/MAAUnified root from test runtime path.");
    }
}
