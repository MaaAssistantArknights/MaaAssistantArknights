using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Advanced;

namespace MAAUnified.App.Features.Advanced;

public partial class WebApiView : UserControl
{
    public WebApiView()
    {
        InitializeComponent();
    }

    private WebApiPageViewModel? VM => DataContext as WebApiPageViewModel;

    private async void OnSaveClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveAsync();
        }
    }

    private async void OnStartClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StartAsync();
        }
    }

    private async void OnStopClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.StopAsync();
        }
    }

    private async void OnRefreshStatusClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RefreshRunningStatusAsync();
        }
    }
}
