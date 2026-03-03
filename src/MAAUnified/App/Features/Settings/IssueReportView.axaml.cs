using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class IssueReportView : UserControl
{
    public IssueReportView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnBuildIssueReportClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.BuildIssueReportAsync();
        }
    }
}
