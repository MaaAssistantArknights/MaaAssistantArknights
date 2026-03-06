using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class VersionUpdateSettingsView : UserControl
{
    public VersionUpdateSettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnSaveVersionUpdateChannelClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveVersionUpdateChannelAsync();
        }
    }

    private async void OnSaveVersionUpdateProxyClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveVersionUpdateProxyAsync();
        }
    }

    private async void OnCheckUpdateClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.CheckVersionUpdateWithDialogAsync();
        }
    }
}
