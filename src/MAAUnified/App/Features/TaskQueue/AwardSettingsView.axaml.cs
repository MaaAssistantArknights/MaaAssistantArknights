using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.TaskQueue;

namespace MAAUnified.App.Features.TaskQueue;

public partial class AwardSettingsView : UserControl
{
    public AwardSettingsView()
    {
        InitializeComponent();
    }

    private AwardModuleViewModel? VM => DataContext as AwardModuleViewModel;

    private void OnConfirmRecruitClick(object? sender, RoutedEventArgs e)
    {
        VM?.ConfirmRecruitEnable();
    }

    private void OnCancelRecruitClick(object? sender, RoutedEventArgs e)
    {
        VM?.CancelRecruitEnable();
    }
}
