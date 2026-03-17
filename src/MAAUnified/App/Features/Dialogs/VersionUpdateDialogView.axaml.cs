using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.Infrastructure;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public partial class VersionUpdateDialogView : Window
{
    public VersionUpdateDialogView()
    {
        InitializeComponent();
        WindowVisuals.ApplyDefaultIcon(this);
    }

    public void ApplyRequest(VersionUpdateDialogRequest request)
    {
        Title = request.Title;
        VersionLine.Text = $"{request.CurrentVersion} -> {request.TargetVersion}";
        SummaryLine.Text = request.Summary;
        BodyBox.Text = request.Body;
        ConfirmButton.Content = request.ConfirmText;
        CancelButton.Content = request.CancelText;
    }

    public VersionUpdateDialogPayload BuildPayload()
    {
        var versionText = VersionLine.Text ?? string.Empty;
        var split = versionText.Split("->", StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);
        var currentVersion = split.Length > 0 ? split[0] : string.Empty;
        var targetVersion = split.Length > 1 ? split[1] : string.Empty;
        return new VersionUpdateDialogPayload(
            Action: "confirm",
            CurrentVersion: currentVersion,
            TargetVersion: targetVersion,
            Summary: SummaryLine.Text ?? string.Empty);
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
