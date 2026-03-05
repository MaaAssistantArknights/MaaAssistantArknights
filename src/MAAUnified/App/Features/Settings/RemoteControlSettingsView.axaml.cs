using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class RemoteControlSettingsView : UserControl
{
    public RemoteControlSettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnSaveRemoteClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveRemoteControlAsync();
        }
    }

    private async void OnTestRemoteConnectivityClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.TestRemoteControlConnectivityAsync();
        }
    }
}
