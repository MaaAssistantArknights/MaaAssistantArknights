using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class BackgroundSettingsView : UserControl
{
    public BackgroundSettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnBackgroundPathLostFocus(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveGuiSettingsAsync();
        }
    }

    private async void OnBackgroundPathKeyDown(object? sender, KeyEventArgs e)
    {
        if (e.Key != Key.Enter || VM is null)
        {
            return;
        }

        await VM.SaveGuiSettingsAsync();
    }
}
