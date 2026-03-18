using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Threading;
using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;
using MAAUnified.Platform;

namespace MAAUnified.App.Services;

public sealed class AvaloniaPostActionPromptService : IPostActionPromptService
{
    private readonly IClassicDesktopStyleApplicationLifetime _desktopLifetime;

    public AvaloniaPostActionPromptService(IClassicDesktopStyleApplicationLifetime desktopLifetime)
    {
        _desktopLifetime = desktopLifetime;
    }

    public async Task<UiOperationResult> ConfirmPowerActionAsync(
        PostActionPromptRequest request,
        CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();

        if (Dispatcher.UIThread.CheckAccess())
        {
            return await ShowDialogAsync(request, cancellationToken);
        }

        return await await Dispatcher.UIThread.InvokeAsync(
            () => ShowDialogAsync(request, cancellationToken),
            DispatcherPriority.Normal,
            cancellationToken);
    }

    private async Task<UiOperationResult> ShowDialogAsync(
        PostActionPromptRequest request,
        CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var owner = ResolveOwnerWindow();
        if (owner is null)
        {
            return UiOperationResult.Fail(
                UiErrorCode.PostActionExecutionFailed,
                "Unable to show power action confirmation dialog because no desktop window is available.");
        }

        var dialog = new WarningConfirmDialogView();
        var seconds = Math.Max(1, (int)Math.Ceiling(request.Countdown.TotalSeconds));
        dialog.ApplyRequest(
            BuildTitle(request.Action, request.Language),
            BuildMessage(request.Action, seconds, request.Language),
            confirmText: BuildConfirmText(request.Language),
            cancelText: DialogTextCatalog.WarningDialogCancelButton(request.Language),
            language: request.Language,
            countdownSeconds: seconds);

        var confirmed = await dialog.ShowDialog<bool>(owner);
        return confirmed
            ? UiOperationResult.Ok($"{request.Action} confirmed.")
            : UiOperationResult.Cancelled($"{request.Action} cancelled.");
    }

    private Window? ResolveOwnerWindow()
    {
        return _desktopLifetime.Windows.LastOrDefault(window => window.IsActive)
               ?? _desktopLifetime.MainWindow;
    }

    private static string BuildTitle(PostActionType action, string? language)
    {
        return action switch
        {
            PostActionType.Shutdown => DialogTextCatalog.Select(language, "确认关机", "Confirm Shutdown"),
            PostActionType.Hibernate => DialogTextCatalog.Select(language, "确认休眠", "Confirm Hibernate"),
            PostActionType.Sleep => DialogTextCatalog.Select(language, "确认睡眠", "Confirm Sleep"),
            _ => DialogTextCatalog.WarningDialogTitle(language),
        };
    }

    private static string BuildMessage(PostActionType action, int seconds, string? language)
    {
        return action switch
        {
            PostActionType.Shutdown => DialogTextCatalog.Select(
                language,
                $"任务完成后将在 {seconds} 秒后执行关机，期间可以取消。",
                $"MAA will shut down this computer in {seconds} seconds unless you cancel."),
            PostActionType.Hibernate => DialogTextCatalog.Select(
                language,
                $"任务完成后将在 {seconds} 秒后执行休眠，期间可以取消。",
                $"MAA will hibernate this computer in {seconds} seconds unless you cancel."),
            PostActionType.Sleep => DialogTextCatalog.Select(
                language,
                $"任务完成后将在 {seconds} 秒后执行睡眠，期间可以取消。",
                $"MAA will put this computer to sleep in {seconds} seconds unless you cancel."),
            _ => DialogTextCatalog.WarningDialogPrompt(language),
        };
    }

    private static string BuildConfirmText(string? language)
    {
        return DialogTextCatalog.Select(language, "立即执行", "Run Now");
    }
}
