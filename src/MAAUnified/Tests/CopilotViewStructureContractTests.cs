namespace MAAUnified.Tests;

public sealed class CopilotViewStructureContractTests
{
    [Fact]
    public void CopilotView_ShouldKeepVisibleStartButtonAndExpandableFileDropdown()
    {
        var root = GetMaaUnifiedRoot();
        var xaml = File.ReadAllText(Path.Combine(root, "App", "Features", "Advanced", "CopilotView.axaml"));

        Assert.Contains("Content=\"开始\"", xaml, StringComparison.Ordinal);
        Assert.Contains("IsVisible=\"{Binding CanEdit}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("<controls:CheckComboBox", xaml, StringComparison.Ordinal);
        Assert.Contains("IsEditable=\"True\"", xaml, StringComparison.Ordinal);
        Assert.Contains("IsTreeMode=\"True\"", xaml, StringComparison.Ordinal);
        Assert.Contains("ItemsSource=\"{Binding FileItems}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("EditorCommitted=\"OnFileSelectorEditorCommitted\"", xaml, StringComparison.Ordinal);
        Assert.Contains("SelectionCommitted=\"OnFileSelectorSelectionCommitted\"", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("OnToggleFileNodeClick", xaml, StringComparison.Ordinal);
        Assert.DoesNotContain("OnFileNodeClick", xaml, StringComparison.Ordinal);
    }

    [Fact]
    public void ConnectSettingsView_ShouldUseSharedCheckComboBox_ForConnectConfigSelection()
    {
        var root = GetMaaUnifiedRoot();
        var xaml = File.ReadAllText(Path.Combine(root, "App", "Features", "Settings", "ConnectSettingsView.axaml"));

        Assert.Contains("<controls:CheckComboBox", xaml, StringComparison.Ordinal);
        Assert.Contains("HeaderText=\"{Binding SelectedConnectConfigOption.DisplayName}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("ItemsSource=\"{Binding ConnectConfigOptions}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("SelectedItem=\"{Binding SelectedConnectConfigOption}\"", xaml, StringComparison.Ordinal);
    }

    [Fact]
    public void CheckComboBox_ShouldUseOutlinedArrowAndSolidDropdownPanel()
    {
        var root = GetMaaUnifiedRoot();
        var xaml = File.ReadAllText(Path.Combine(root, "App", "Controls", "CheckComboBox.axaml"));

        Assert.Contains("Data=\"M 1 1 L 5 5 L 9 1\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Stroke=\"{DynamicResource MAA.Brush.Text.Secondary}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Classes=\"combo-dropdown-panel\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Value=\"{DynamicResource MAA.Brush.Surface.SectionStrong}\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Placement=\"BottomEdgeAlignedLeft\"", xaml, StringComparison.Ordinal);
        Assert.Contains("HorizontalAlignment=\"Left\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"Button.combo-arrow:pointerover\"", xaml, StringComparison.Ordinal);
        Assert.Contains("Style Selector=\"TreeView.combo-dropdown ToggleButton:pointerover\"", xaml, StringComparison.Ordinal);
        Assert.Contains("<Setter Property=\"MinWidth\" Value=\"36\" />", xaml, StringComparison.Ordinal);
        Assert.Contains("<Setter Property=\"MinWidth\" Value=\"24\" />", xaml, StringComparison.Ordinal);
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
