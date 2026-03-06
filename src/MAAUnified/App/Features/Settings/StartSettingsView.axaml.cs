using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class StartSettingsView : UserControl
{
    public StartSettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnSelectEmulatorPathClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SelectEmulatorPathWithDialogAsync();
        }
    }
}
