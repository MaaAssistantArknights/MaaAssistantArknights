using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Advanced;

namespace MAAUnified.App.Features.Advanced;

public partial class ExternalNotificationProvidersView : UserControl
{
    public ExternalNotificationProvidersView()
    {
        InitializeComponent();
    }

    private ExternalNotificationProvidersPageViewModel? VM => DataContext as ExternalNotificationProvidersPageViewModel;

    private async void OnValidateClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.ValidateAsync();
        }
    }

    private async void OnSendTestClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SendTestAsync();
        }
    }
}
