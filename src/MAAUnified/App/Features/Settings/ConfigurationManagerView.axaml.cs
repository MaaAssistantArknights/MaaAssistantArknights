using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class ConfigurationManagerView : UserControl
{
    public ConfigurationManagerView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnRefreshProfilesClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RefreshConfigurationProfilesAsync();
        }
    }

    private async void OnSwitchProfileClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SwitchConfigurationProfileAsync();
        }
    }

    private async void OnAddProfileClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.AddConfigurationProfileAsync();
        }
    }

    private async void OnDeleteProfileClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.DeleteConfigurationProfileAsync();
        }
    }

    private async void OnMoveProfileUpClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.MoveConfigurationProfileUpAsync();
        }
    }

    private async void OnMoveProfileDownClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.MoveConfigurationProfileDownAsync();
        }
    }
}
