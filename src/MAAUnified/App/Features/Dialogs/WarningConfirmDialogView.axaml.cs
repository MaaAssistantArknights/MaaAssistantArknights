using Avalonia.Controls;
using Avalonia.Interactivity;
using MAAUnified.App.Infrastructure;
using MAAUnified.App.ViewModels.Infrastructure;

namespace MAAUnified.App.Features.Dialogs;

public partial class WarningConfirmDialogView : Window
{
    private CancellationTokenSource? _countdownCts;
    private string _confirmText = string.Empty;
    private string? _language;
    private int _countdownSeconds;

    public WarningConfirmDialogView()
    {
        InitializeComponent();
        WindowVisuals.ApplyDefaultIcon(this);
        Opened += OnOpened;
        Closed += OnClosed;
    }

    public void ApplyRequest(
        string title,
        string message,
        string confirmText = "",
        string cancelText = "",
        string? language = null,
        int countdownSeconds = 0)
    {
        StopCountdown();
        _language = language;
        Title = string.IsNullOrWhiteSpace(title) ? DialogTextCatalog.WarningDialogTitle(language) : title.Trim();
        TitleText.Text = Title;
        PromptText.Text = string.IsNullOrWhiteSpace(message) ? DialogTextCatalog.WarningDialogPrompt(language) : message.Trim();
        _confirmText = string.IsNullOrWhiteSpace(confirmText)
            ? DialogTextCatalog.WarningDialogConfirmButton(language)
            : confirmText;
        _countdownSeconds = Math.Max(0, countdownSeconds);
        UpdateConfirmButtonText(_countdownSeconds);
        CancelButton.Content = string.IsNullOrWhiteSpace(cancelText)
            ? DialogTextCatalog.WarningDialogCancelButton(language)
            : cancelText;
    }

    private void OnConfirmClick(object? sender, RoutedEventArgs e)
    {
        StopCountdown();
        Close(true);
    }

    private void OnCancelClick(object? sender, RoutedEventArgs e)
    {
        StopCountdown();
        Close(false);
    }

    private void OnOpened(object? sender, EventArgs e)
    {
        if (_countdownSeconds > 0)
        {
            StartCountdown();
        }
    }

    private void OnClosed(object? sender, EventArgs e)
    {
        StopCountdown();
    }

    private async void StartCountdown()
    {
        StopCountdown();
        _countdownCts = new CancellationTokenSource();

        try
        {
            for (var remaining = _countdownSeconds; remaining > 0; remaining--)
            {
                UpdateConfirmButtonText(remaining);
                await Task.Delay(TimeSpan.FromSeconds(1), _countdownCts.Token);
            }

            if (_countdownCts.IsCancellationRequested)
            {
                return;
            }

            UpdateConfirmButtonText(0);
            Close(true);
        }
        catch (OperationCanceledException)
        {
            // Countdown cancelled by user interaction or dialog close.
        }
    }

    private void StopCountdown()
    {
        _countdownCts?.Cancel();
        _countdownCts?.Dispose();
        _countdownCts = null;
    }

    private void UpdateConfirmButtonText(int remainingSeconds)
    {
        ConfirmButton.Content = remainingSeconds > 0
            ? DialogTextCatalog.Select(_language, $"{_confirmText}（{remainingSeconds}s）", $"{_confirmText} ({remainingSeconds}s)")
            : _confirmText;
    }
}
