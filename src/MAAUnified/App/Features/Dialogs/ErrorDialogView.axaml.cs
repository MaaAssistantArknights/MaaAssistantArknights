using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public partial class ErrorDialogView : Window
{
    private Func<CancellationToken, Task<UiOperationResult>>? _openIssueReportAsync;
    private bool _copied;
    private bool _issueOpened;
    private string _formattedText = string.Empty;

    public ErrorDialogView()
    {
        InitializeComponent();
    }

    public void ApplyRequest(
        ErrorDialogRequest request,
        Func<CancellationToken, Task<UiOperationResult>>? openIssueReportAsync = null)
    {
        Title = request.Title;
        _openIssueReportAsync = openIssueReportAsync;
        ConfirmButton.Content = request.ConfirmText;
        CancelButton.Content = request.CancelText;
        ContextLine.Text = $"[{request.Context}] {request.Result.Message}";
        _formattedText = BuildFormattedErrorText(request);
        ErrorDetailBox.Text = _formattedText;
    }

    public ErrorDialogPayload BuildPayload()
    {
        return new ErrorDialogPayload(
            FormattedErrorText: _formattedText,
            Copied: _copied,
            IssueReportOpened: _issueOpened);
    }

    private static string BuildFormattedErrorText(ErrorDialogRequest request)
    {
        var code = request.Result.Error?.Code ?? UiErrorCode.UiOperationFailed;
        var details = request.Result.Error?.Details ?? string.Empty;
        var suggestion = request.Suggestion ?? string.Empty;
        return
            $"TimestampUtc: {DateTimeOffset.UtcNow:O}{Environment.NewLine}" +
            $"Context: {request.Context}{Environment.NewLine}" +
            $"Code: {code}{Environment.NewLine}" +
            $"Message: {request.Result.Message}{Environment.NewLine}" +
            $"Details: {details}{Environment.NewLine}" +
            $"Suggestion: {suggestion}";
    }

    private async void OnCopyClick(object? sender, RoutedEventArgs e)
    {
        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel?.Clipboard is null)
        {
            return;
        }

        await topLevel.Clipboard.SetTextAsync(_formattedText);
        _copied = true;
    }

    private async void OnOpenIssueReportClick(object? sender, RoutedEventArgs e)
    {
        if (_openIssueReportAsync is null)
        {
            return;
        }

        var result = await _openIssueReportAsync(CancellationToken.None);
        if (result.Success)
        {
            _issueOpened = true;
        }
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
