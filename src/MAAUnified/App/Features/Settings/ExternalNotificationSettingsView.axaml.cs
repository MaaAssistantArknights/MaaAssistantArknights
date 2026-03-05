using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class ExternalNotificationSettingsView : UserControl
{
    public ExternalNotificationSettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnSaveExternalNotificationClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveExternalNotificationAsync();
        }
    }

    private async void OnValidateProviderClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ValidateExternalNotificationParametersAsync();
        }
    }

    private async void OnTestSendClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.TestExternalNotificationAsync();
        }
    }
}
