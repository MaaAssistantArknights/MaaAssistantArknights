using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class HotKeySettingsView : UserControl
{
    public HotKeySettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnApplyHotkeysClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.RegisterHotkeysAsync();
        }
    }
}
