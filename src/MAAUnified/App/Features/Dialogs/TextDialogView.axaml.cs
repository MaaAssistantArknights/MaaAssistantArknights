using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.Infrastructure;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public partial class TextDialogView : Window
{
    public TextDialogView()
    {
        InitializeComponent();
        WindowVisuals.ApplyDefaultIcon(this);
        Opened += (_, _) =>
        {
            InputBox.Focus();
            InputBox.SelectAll();
        };
    }

    public void ApplyRequest(TextDialogRequest request)
    {
        Title = request.Title;
        PromptText.Text = request.Prompt;
        InputBox.Text = request.DefaultText;
        InputBox.AcceptsReturn = request.MultiLine;
        ConfirmButton.Content = request.ConfirmText;
        CancelButton.Content = request.CancelText;
    }

    public TextDialogPayload BuildPayload()
    {
        return new TextDialogPayload(InputBox.Text ?? string.Empty);
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
