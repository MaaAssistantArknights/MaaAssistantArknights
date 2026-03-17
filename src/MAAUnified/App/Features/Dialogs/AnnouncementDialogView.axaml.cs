using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.Infrastructure;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public partial class AnnouncementDialogView : Window
{
    public AnnouncementDialogView()
    {
        InitializeComponent();
        WindowVisuals.ApplyDefaultIcon(this);
    }

    public void ApplyRequest(AnnouncementDialogRequest request)
    {
        Title = request.Title;
        AnnouncementInfoBox.Text = request.AnnouncementInfo;
        DoNotRemindBox.IsChecked = request.DoNotRemindThisAnnouncementAgain;
        DoNotShowBox.IsChecked = request.DoNotShowAnnouncement;
        ConfirmButton.Content = request.ConfirmText;
        CancelButton.Content = request.CancelText;
    }

    public AnnouncementDialogPayload BuildPayload()
    {
        return new AnnouncementDialogPayload(
            AnnouncementInfo: AnnouncementInfoBox.Text ?? string.Empty,
            DoNotRemindThisAnnouncementAgain: DoNotRemindBox.IsChecked ?? false,
            DoNotShowAnnouncement: DoNotShowBox.IsChecked ?? false);
    }

    private void OnConfirmClick(object? sender, RoutedEventArgs e)
    {
        Close(DialogReturnSemantic.Confirm);
    }

    private void OnCancelClick(object? sender, RoutedEventArgs e)
    {
        Close(DialogReturnSemantic.Cancel);
    }
}
