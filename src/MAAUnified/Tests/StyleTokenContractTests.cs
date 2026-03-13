using System.Text.RegularExpressions;

namespace MAAUnified.Tests;

public sealed class StyleTokenContractTests
{
    private static readonly string[] CoreEntryViews =
    [
        "App/Views/MainWindow.axaml",
        "App/Features/Root/TaskQueueView.axaml",
        "App/Features/Root/SettingsView.axaml",
        "App/Features/Advanced/CopilotView.axaml",
        "App/Features/Advanced/ToolboxView.axaml",
    ];

    [Fact]
    public void GlobalStyleEntry_ShouldBeSingleSource()
    {
        var root = GetMaaUnifiedRoot();
        var appAxaml = Path.Combine(root, "App", "App.axaml");
        var appText = File.ReadAllText(appAxaml);

        Assert.Contains("StyleInclude Source=\"avares://MAAUnified/Styles/ColorTokens.axaml\"", appText, StringComparison.Ordinal);
        Assert.Contains("StyleInclude Source=\"avares://MAAUnified/Styles/ControlStyles.axaml\"", appText, StringComparison.Ordinal);

        var allAxamlFiles = Directory.EnumerateFiles(Path.Combine(root, "App"), "*.axaml", SearchOption.AllDirectories)
            .Where(path => !path.EndsWith(Path.Combine("App", "App.axaml"), StringComparison.Ordinal))
            .ToList();

        foreach (var file in allAxamlFiles)
        {
            var text = File.ReadAllText(file);
            Assert.DoesNotContain("StyleInclude Source=\"avares://MAAUnified/Styles/", text, StringComparison.Ordinal);
        }
    }

    [Fact]
    public void ColorTokens_ShouldContainRequiredSemanticKeys_ForLightAndDark()
    {
        var root = GetMaaUnifiedRoot();
        var text = File.ReadAllText(Path.Combine(root, "App", "Styles", "ColorTokens.axaml"));

        Assert.Contains("<ResourceDictionary x:Key=\"Light\">", text, StringComparison.Ordinal);
        Assert.Contains("<ResourceDictionary x:Key=\"Dark\">", text, StringComparison.Ordinal);

        var requiredKeys =
            new[]
            {
                "MAA.Color.Surface.Window",
                "MAA.Color.Surface.Section",
                "MAA.Color.Surface.SectionStrong",
                "MAA.Color.Border.Default",
                "MAA.Color.Text.Primary",
                "MAA.Color.State.Warning",
                "MAA.Color.State.Error",
                "MAA.Color.State.Success",
                "MAA.Color.State.Running",
                "MAA.Color.State.Skipped",
                "MAA.Color.State.Idle",
                "MAA.Color.Action.Background",
                "MAA.Color.Action.BackgroundHover",
                "MAA.Color.Action.BackgroundPressed",
                "MAA.Color.Action.Border",
                "MAA.Color.Action.Foreground",
            };

        foreach (var key in requiredKeys)
        {
            Assert.Contains($"x:Key=\"{key}\"", text, StringComparison.Ordinal);
        }
    }

    [Fact]
    public void ControlStyles_ShouldContainSectionTitleActionContracts()
    {
        var root = GetMaaUnifiedRoot();
        var text = File.ReadAllText(Path.Combine(root, "App", "Styles", "ControlStyles.axaml"));

        Assert.Contains("Style Selector=\"Border.wpf-panel\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Button.wpf-button\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Border.wpf-card.status-running\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Border.wpf-card.status-success\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Border.wpf-card.status-error\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Border.wpf-card.status-skipped\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Border.wpf-card.status-idle\"", text, StringComparison.Ordinal);

        Assert.Contains("Style Selector=\"Border.section\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"TextBlock.section-title\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Button.action\"", text, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Border.item-card.status-running\"", text, StringComparison.Ordinal);

        Assert.Contains("{DynamicResource MAA.Brush.Wpf.Region25}", text, StringComparison.Ordinal);
        Assert.Contains("{DynamicResource MAA.FontSize.SectionTitle}", text, StringComparison.Ordinal);
        Assert.Contains("{DynamicResource MAA.Size.Action.Height}", text, StringComparison.Ordinal);
        Assert.Contains("{DynamicResource MAA.Brush.Wpf.TaskStatus.Running}", text, StringComparison.Ordinal);
        Assert.Contains("{DynamicResource MAA.Brush.Wpf.TaskStatus.Success}", text, StringComparison.Ordinal);
        Assert.Contains("{DynamicResource MAA.Brush.Wpf.TaskStatus.Error}", text, StringComparison.Ordinal);
    }

    [Fact]
    public void TaskQueueView_ShouldUseStatusClassBindingInsteadOfStatusBrush()
    {
        var root = GetMaaUnifiedRoot();
        var taskQueuePath = Path.Combine(root, "App", "Features", "Root", "TaskQueueView.axaml");
        var text = File.ReadAllText(taskQueuePath);

        Assert.DoesNotContain("StatusBrush", text, StringComparison.Ordinal);
        Assert.Contains("Classes.status-running=\"{Binding IsStatusRunning}\"", text, StringComparison.Ordinal);
        Assert.Contains("Classes.status-success=\"{Binding IsStatusSuccess}\"", text, StringComparison.Ordinal);
        Assert.Contains("Classes.status-error=\"{Binding IsStatusError}\"", text, StringComparison.Ordinal);
        Assert.Contains("Classes.status-skipped=\"{Binding IsStatusSkipped}\"", text, StringComparison.Ordinal);
        Assert.Contains("Classes.status-idle=\"{Binding IsStatusIdle}\"", text, StringComparison.Ordinal);
    }

    [Fact]
    public void TaskQueueView_ShouldBindCanEditAndPrimaryRunToggle()
    {
        var root = GetMaaUnifiedRoot();
        var taskQueuePath = Path.Combine(root, "App", "Features", "Root", "TaskQueueView.axaml");
        var text = File.ReadAllText(taskQueuePath);

        Assert.Contains("ListBox Grid.Row=\"1\"", text, StringComparison.Ordinal);
        Assert.Contains("Classes=\"wpf-list-no-highlight\"", text, StringComparison.Ordinal);
        Assert.Contains("IsEnabled=\"{Binding CanEdit}\"", text, StringComparison.Ordinal);
        Assert.Contains("IsEnabled=\"{Binding CanToggleRun}\"", text, StringComparison.Ordinal);
        Assert.Contains("Content=\"{Binding BatchActionText}\"", text, StringComparison.Ordinal);
        Assert.DoesNotContain("StatusDisplayName", text, StringComparison.Ordinal);
        Assert.DoesNotContain("TabControl Grid.Row=\"2\"", text, StringComparison.Ordinal);
        Assert.Contains("ScrollViewer Grid.Row=\"1\"", text, StringComparison.Ordinal);
    }

    [Fact]
    public void TooltipHintControl_ShouldUseFrameworkTooltipServiceWithHalfSecondDelay()
    {
        var root = GetMaaUnifiedRoot();
        var xaml = File.ReadAllText(Path.Combine(root, "App", "Controls", "TooltipHint.axaml"));
        var codeBehind = File.ReadAllText(Path.Combine(root, "App", "Controls", "TooltipHint.axaml.cs"));

        Assert.Contains("ToolTip.ShowDelay=\"500\"", xaml, StringComparison.Ordinal);
        Assert.Contains("<ToolTip.Tip>", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("<Popup", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("DispatcherTimer", codeBehind, StringComparison.Ordinal);
    }

    [Fact]
    public void CoreEntryViews_ShouldNotContainHardcodedColorOrSizeLiterals()
    {
        var root = GetMaaUnifiedRoot();
        var colorPattern = new Regex("#[0-9A-Fa-f]{6,8}", RegexOptions.Compiled);
        var sizePattern = new Regex(
            "(Width|Height|MinWidth|MinHeight|MaxWidth|MaxHeight|Margin|Padding|Spacing|CornerRadius|FontSize|BorderThickness|ColumnDefinitions|RowDefinitions)=\"[^\"]*[0-9][^\"]*\"",
            RegexOptions.Compiled);

        foreach (var relative in CoreEntryViews)
        {
            var fullPath = Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar));
            var text = File.ReadAllText(fullPath);
            Assert.DoesNotMatch(colorPattern, text);
            Assert.DoesNotMatch(sizePattern, text);
        }
    }

    [Fact]
    public void CoreEntryViews_AllButtons_ShouldUseWpfButtonClass()
    {
        var root = GetMaaUnifiedRoot();
        var buttonPattern = new Regex("<Button\\b(?!\\.ContextMenu)([^>]*)>", RegexOptions.Compiled | RegexOptions.Singleline);
        var totalButtons = 0;

        foreach (var relative in CoreEntryViews)
        {
            var fullPath = Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar));
            var text = File.ReadAllText(fullPath);
            var matches = buttonPattern.Matches(text);

            foreach (Match match in matches)
            {
                totalButtons++;
                var attrs = match.Groups[1].Value;
                Assert.Matches("Classes=\"[^\"]*\\bwpf-button\\b[^\"]*\"", attrs);
            }
        }

        Assert.True(totalButtons > 0);
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
