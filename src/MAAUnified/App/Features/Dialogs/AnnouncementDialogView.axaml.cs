using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Dialogs;

namespace MAAUnified.App.Features.Dialogs;

public partial class AnnouncementDialogView : UserControl
{
    public AnnouncementDialogView()
    {
        InitializeComponent();
    }

    private AnnouncementDialogViewModel? VM => DataContext as AnnouncementDialogViewModel;

    private async void OnSaveClick(object? sender, RoutedEventArgs e)
    {
        if (VM is not null)
        {
            await VM.SaveAsync();
        }
    }
}
