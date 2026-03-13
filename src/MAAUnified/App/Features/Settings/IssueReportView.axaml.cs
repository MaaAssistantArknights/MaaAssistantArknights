using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.Views;
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

    private async void OnOpenHelpClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.OpenIssueReportHelpAsync();
        }
    }

    private async void OnOpenIssueEntryClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.OpenIssueReportEntryAsync();
        }
    }

    private async void OnOpenDebugDirectoryClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.OpenIssueReportDebugDirectoryAsync();
        }
    }

    private async void OnClearImageCacheClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ClearIssueReportImageCacheAsync();
        }
    }

    private void OnOpenRuntimeLogWindowClick(object? sender, RoutedEventArgs e)
    {
        if (TopLevel.GetTopLevel(this) is MainWindow mainWindow)
        {
            mainWindow.OpenRuntimeLogWindow();
        }
    }
}
