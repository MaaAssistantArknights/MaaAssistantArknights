using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Settings;

namespace MAAUnified.App.Features.Settings;

public partial class AchievementSettingsView : UserControl
{
    public AchievementSettingsView()
    {
        InitializeComponent();
    }

    private SettingsPageViewModel? VM => DataContext as SettingsPageViewModel;

    private async void OnSaveAchievementClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveAchievementSettingsAsync();
        }
    }
}
