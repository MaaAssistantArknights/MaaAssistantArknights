using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Advanced;

namespace MAAUnified.App.Features.Advanced;

public partial class OverlayView : UserControl
{
    public OverlayView()
    {
        InitializeComponent();
    }

    private OverlayAdvancedPageViewModel? VM => DataContext as OverlayAdvancedPageViewModel;

    private async void OnReloadTargetsClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ReloadTargetsAsync();
        }
    }

    private async void OnToggleOverlayClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ToggleOverlayAsync();
        }
    }
}
