using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Advanced;

namespace MAAUnified.App.Features.Advanced;

public partial class TrayIntegrationView : UserControl
{
    public TrayIntegrationView()
    {
        InitializeComponent();
    }

    private TrayIntegrationPageViewModel? VM => DataContext as TrayIntegrationPageViewModel;

    private async void OnApplyTrayVisibilityClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ApplyTrayVisibilityAsync();
        }
    }

    private async void OnSendTrayMessageClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SendTrayMessageAsync();
        }
    }

    private async void OnSyncMenuStateClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SyncMenuStateAsync();
        }
    }

    private async void OnRefreshCapabilityClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RefreshCapabilitySummaryAsync();
        }
    }
}
