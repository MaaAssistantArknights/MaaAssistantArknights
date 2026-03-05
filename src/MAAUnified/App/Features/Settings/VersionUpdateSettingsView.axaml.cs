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

    private async void OnSaveVersionUpdateClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveVersionUpdateSettingsAsync();
        }
    }

    private async void OnCheckUpdateClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.CheckVersionUpdateAsync();
        }
    }
}
