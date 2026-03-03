using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Root;

public partial class SettingsView : UserControl
{
    public SettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnSaveGuiClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveGuiSettingsAsync();
        }
    }

    private async void OnSaveRemoteClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveRemoteControlAsync();
        }
    }

    private async void OnRegisterHotkeysClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RegisterHotkeysAsync();
        }
    }

    private async void OnTestNotificationClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.TestNotificationAsync();
        }
    }

    private async void OnApplyAutostartClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ApplyAutostartAsync();
        }
    }

    private async void OnSaveConnectionGameClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveConnectionGameSettingsAsync();
        }
    }
}
