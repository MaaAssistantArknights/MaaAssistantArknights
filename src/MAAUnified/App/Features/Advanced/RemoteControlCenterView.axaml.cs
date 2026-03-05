using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Advanced;

namespace MAAUnified.App.Features.Advanced;

public partial class RemoteControlCenterView : UserControl
{
    public RemoteControlCenterView()
    {
        InitializeComponent();
    }

    private RemoteControlCenterPageViewModel? VM => DataContext as RemoteControlCenterPageViewModel;

    private async void OnSaveClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveAsync();
        }
    }

    private async void OnTestConnectivityClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.TestConnectivityAsync();
        }
    }
}
