using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.Features.Dialogs;

public partial class WarningConfirmDialogView : Window
{
    public WarningConfirmDialogView()
    {
        InitializeComponent();
    }

    public void ApplyRequest(
        string title,
        string message,
        string confirmText = "",
        string cancelText = "",
        string? language = null)
    {
        Title = string.IsNullOrWhiteSpace(title) ? DialogTextCatalog.WarningDialogTitle(language) : title.Trim();
        TitleText.Text = Title;
        PromptText.Text = string.IsNullOrWhiteSpace(message) ? DialogTextCatalog.WarningDialogPrompt(language) : message.Trim();
        ConfirmButton.Content = string.IsNullOrWhiteSpace(confirmText)
            ? DialogTextCatalog.WarningDialogConfirmButton(language)
            : confirmText;
        CancelButton.Content = string.IsNullOrWhiteSpace(cancelText)
            ? DialogTextCatalog.WarningDialogCancelButton(language)
            : cancelText;
    }

    private void OnConfirmClick(object? sender, RoutedEventArgs e)
    {
        Close(true);
    }

    private void OnCancelClick(object? sender, RoutedEventArgs e)
    {
        Close(false);
    }
}
