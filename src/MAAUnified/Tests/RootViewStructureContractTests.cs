namespace MAAUnified.Tests;

public sealed class RootViewStructureContractTests
{
    [Fact]
    public void MainWindow_ShouldUseRootLocalizationBindings_ForTabsAndMenus()
    {
        var root = GetMaaUnifiedRoot();
        var text = File.ReadAllText(Path.Combine(root, "App", "Views", "MainWindow.axaml"));

        Assert.Contains("{Binding RootTexts[Main.Tab.TaskQueue]}", text, StringComparison.Ordinal);
        Assert.Contains("{Binding RootTexts[Main.Tab.Copilot]}", text, StringComparison.Ordinal);
        Assert.Contains("{Binding RootTexts[Main.Tab.Toolbox]}", text, StringComparison.Ordinal);
        Assert.Contains("{Binding RootTexts[Main.Tab.Settings]}", text, StringComparison.Ordinal);
        Assert.DoesNotContain("{Binding RootTexts[Main.Tab.Advanced]}", text, StringComparison.Ordinal);
        Assert.DoesNotContain("{Binding RootTexts[Main.Menu.Start]}", text, StringComparison.Ordinal);
        Assert.DoesNotContain("{Binding RootTexts[Main.Menu.SwitchLanguage]}", text, StringComparison.Ordinal);
        Assert.Contains("Title=\"{Binding WindowTitle}\"", text, StringComparison.Ordinal);
    }

    [Fact]
    public void TaskQueueView_ShouldBindRootTextsAndKeepCoreRunContracts()
    {
        var root = GetMaaUnifiedRoot();
        var text = File.ReadAllText(Path.Combine(root, "App", "Features", "Root", "TaskQueueView.axaml"));

        Assert.Contains("{Binding RootTexts[TaskQueue.Root.TaskListTitle]}", text, StringComparison.Ordinal);
        Assert.Contains("{Binding RootTexts[TaskQueue.Root.TaskConfigTitle]}", text, StringComparison.Ordinal);
        Assert.Contains("Content=\"{Binding SelectedTaskSettingsViewModel}\"", text, StringComparison.Ordinal);
        Assert.Contains("IsEnabled=\"{Binding CanToggleRun}\"", text, StringComparison.Ordinal);
        Assert.Contains("Classes=\"wpf-list-no-highlight\"", text, StringComparison.Ordinal);
        Assert.Contains("{Binding RootTexts[TaskQueue.Root.LogsTitle]}", text, StringComparison.Ordinal);
        Assert.Contains("ItemsSource=\"{Binding LogCards}\"", text, StringComparison.Ordinal);
        Assert.Contains("ItemsSource=\"{Binding Items}\"", text, StringComparison.Ordinal);
        Assert.Contains("ToolTip.ShowDelay=\"200\"", text, StringComparison.Ordinal);
        Assert.Contains("TextWrapping=\"NoWrap\"", text, StringComparison.Ordinal);
        Assert.Contains("MaxWidth=\"960\"", text, StringComparison.Ordinal);
        Assert.Contains("MaxHeight=\"540\"", text, StringComparison.Ordinal);
        Assert.Contains("IsVisible=\"{Binding ShowTimeOnlyLayout}\"", text, StringComparison.Ordinal);
        Assert.Contains("VerticalAlignment=\"Center\"", text, StringComparison.Ordinal);
        Assert.Contains("Click=\"OnOpenPostActionClick\"", text, StringComparison.Ordinal);
        Assert.Contains("Click=\"OnAddTaskModuleClick\"", text, StringComparison.Ordinal);
        Assert.Contains("Click=\"OnBatchActionClick\"", text, StringComparison.Ordinal);
        Assert.Contains("Click=\"OnToggleBatchModeClick\"", text, StringComparison.Ordinal);
        Assert.Contains("<DataTemplate DataType=\"taskVm:StartUpTaskModuleViewModel\">", text, StringComparison.Ordinal);
        Assert.Contains("<DataTemplate DataType=\"taskVm:FightTaskModuleViewModel\">", text, StringComparison.Ordinal);
        Assert.DoesNotContain("TaskQueue.Root.AutoReload", text, StringComparison.Ordinal);
        Assert.DoesNotContain("IsChecked=\"{Binding AutoReload}\"", text, StringComparison.Ordinal);
        Assert.DoesNotContain("Click=\"OnInverseClick\"", text, StringComparison.Ordinal);
        Assert.DoesNotContain("TaskQueue.Root.AdvancedMode", text, StringComparison.Ordinal);
        Assert.DoesNotContain("ShowAdvanced", text, StringComparison.Ordinal);
        Assert.DoesNotContain("TaskQueue.Root.OverlayButton", text, StringComparison.Ordinal);
        Assert.DoesNotContain("Click=\"OnToggleOverlayClick\"", text, StringComparison.Ordinal);
    }

    [Fact]
    public void TaskQueueTaskViews_ShouldUseVerticalSettingsLayout_AndHidePostActionCommandInputs()
    {
        var root = GetMaaUnifiedRoot();
        var taskViewFiles = new[]
        {
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
        };

        foreach (var file in taskViewFiles)
        {
            var fullPath = Path.Combine(root, file.Replace('/', Path.DirectorySeparatorChar));
            var text = File.ReadAllText(fullPath);
            Assert.DoesNotContain("<WrapPanel", text, StringComparison.Ordinal);
            Assert.DoesNotContain("Orientation=\"Horizontal\"", text, StringComparison.Ordinal);
        }

        var postActionText = File.ReadAllText(
            Path.Combine(
                root,
                "App",
                "Features",
                "TaskQueue",
                "PostActionSettingsView.axaml"));
        Assert.DoesNotContain("ExitArknightsCommand", postActionText, StringComparison.Ordinal);
        Assert.DoesNotContain("BackToAndroidHomeCommand", postActionText, StringComparison.Ordinal);
        Assert.DoesNotContain("ExitEmulatorCommand", postActionText, StringComparison.Ordinal);
        Assert.DoesNotContain("ExitSelfCommand", postActionText, StringComparison.Ordinal);
    }

    [Fact]
    public void SettingsView_ShouldLoadAllSectionsIntoSharedScrollSurface()
    {
        var root = GetMaaUnifiedRoot();
        var text = File.ReadAllText(Path.Combine(root, "App", "Features", "Root", "SettingsView.axaml"));

        Assert.Contains("SelectionChanged=\"OnSectionSelectionChanged\"", text, StringComparison.Ordinal);
        Assert.Contains("x:Name=\"SectionScrollViewer\"", text, StringComparison.Ordinal);
        Assert.Contains("x:Name=\"SectionContentPanel\"", text, StringComparison.Ordinal);
        Assert.Contains("ScrollChanged=\"OnSectionScrollChanged\"", text, StringComparison.Ordinal);
        Assert.Contains("settingsViews:ConfigurationManagerView", text, StringComparison.Ordinal);
        Assert.Contains("settingsViews:AboutSettingsView", text, StringComparison.Ordinal);
        Assert.DoesNotContain("x:Name=\"SectionHost\"", text, StringComparison.Ordinal);
    }

    [Fact]
    public void IssueReportView_ShouldExposeOpenRuntimeLogWindowAction()
    {
        var root = GetMaaUnifiedRoot();
        var xaml = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "IssueReportView.axaml"));
        var codeBehind = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "IssueReportView.axaml.cs"));

        Assert.Contains("Click=\"OnOpenRuntimeLogWindowClick\"", xaml, StringComparison.Ordinal);
        Assert.Contains("OnOpenRuntimeLogWindowClick", codeBehind, StringComparison.Ordinal);
        Assert.Contains("OpenRuntimeLogWindow()", codeBehind, StringComparison.Ordinal);
    }

    [Fact]
    public void ConfigurationManagerView_ShouldAutoSwitchProfiles_AndRemoveObsoleteButtons()
    {
        var root = GetMaaUnifiedRoot();
        var xaml = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "ConfigurationManagerView.axaml"));
        var codeBehind = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "ConfigurationManagerView.axaml.cs"));

        Assert.Contains("SelectionChanged=\"OnConfigurationProfileSelectionChanged\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Click=\"OnImportProfilesClick\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Watermark=\"新配置名称\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Content=\"另存为新配置\"", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("留空使用当前时间", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("切换到别的配置", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("保存当前配置的修改", xaml, StringComparison.Ordinal);
        Assert.Contains("确认删除配置“{0}”？", codeBehind, StringComparison.Ordinal);
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
