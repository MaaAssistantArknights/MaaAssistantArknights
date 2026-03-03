namespace MAAUnified.Tests;

public sealed class AvaloniaParityCoverageTests
{
    private static readonly string[] RequiredViews =
    [
        "App/Views/MainWindow.axaml",
        "App/Features/Root/TaskQueueView.axaml",
        "App/Features/Advanced/CopilotView.axaml",
        "App/Features/Advanced/ToolboxView.axaml",
        "App/Features/Root/SettingsView.axaml",
        "App/Features/TaskQueue/StartUpTaskView.axaml",
        "App/Features/TaskQueue/FightSettingsView.axaml",
        "App/Features/TaskQueue/RecruitSettingsView.axaml",
        "App/Features/TaskQueue/InfrastSettingsView.axaml",
        "App/Features/TaskQueue/MallSettingsView.axaml",
        "App/Features/TaskQueue/AwardSettingsView.axaml",
        "App/Features/TaskQueue/RoguelikeSettingsView.axaml",
        "App/Features/TaskQueue/ReclamationSettingsView.axaml",
        "App/Features/TaskQueue/CustomSettingsView.axaml",
        "App/Features/TaskQueue/PostActionSettingsView.axaml",
        "App/Features/Settings/ConfigurationManagerView.axaml",
        "App/Features/Settings/TimerSettingsView.axaml",
        "App/Features/Settings/PerformanceSettingsView.axaml",
        "App/Features/Settings/GameSettingsView.axaml",
        "App/Features/Settings/ConnectSettingsView.axaml",
        "App/Features/Settings/StartSettingsView.axaml",
        "App/Features/Settings/RemoteControlSettingsView.axaml",
        "App/Features/Settings/GuiSettingsView.axaml",
        "App/Features/Settings/BackgroundSettingsView.axaml",
        "App/Features/Settings/ExternalNotificationSettingsView.axaml",
        "App/Features/Settings/HotKeySettingsView.axaml",
        "App/Features/Settings/AchievementSettingsView.axaml",
        "App/Features/Settings/VersionUpdateSettingsView.axaml",
        "App/Features/Settings/IssueReportView.axaml",
        "App/Features/Settings/AboutSettingsView.axaml",
        "App/Features/Dialogs/AnnouncementDialogView.axaml",
        "App/Features/Dialogs/VersionUpdateDialogView.axaml",
        "App/Features/Dialogs/ProcessPickerDialogView.axaml",
        "App/Features/Dialogs/EmulatorPathSelectionDialogView.axaml",
        "App/Features/Dialogs/ErrorDialogView.axaml",
        "App/Features/Dialogs/AchievementListDialogView.axaml",
        "App/Features/Dialogs/TextDialogView.axaml",
    ];

    [Fact]
    public void RequiredParityViews_ShouldExist()
    {
        var root = GetMaaUnifiedRoot();

        foreach (var relative in RequiredViews)
        {
            var fullPath = Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar));
            Assert.True(File.Exists(fullPath), $"Missing view: {relative}");
        }
    }

    [Fact]
    public void FeatureViews_ShouldNotContainLegacyPlaceholderSentence()
    {
        var root = GetMaaUnifiedRoot();
        var featuresRoot = Path.Combine(root, "App", "Features");
        var xamlFiles = Directory.EnumerateFiles(featuresRoot, "*.axaml", SearchOption.AllDirectories);

        foreach (var file in xamlFiles)
        {
            var text = File.ReadAllText(file);
            Assert.DoesNotContain("目标：与现有前端功能保持一致。", text);
        }
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
