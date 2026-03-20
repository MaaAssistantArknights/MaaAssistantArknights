using MAAUnified.App.Features.Dialogs;
using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;

namespace MAAUnified.App.Services;

internal sealed class ShellCloseConfirmationService
{
    private readonly IAppDialogService _dialogService;

    public ShellCloseConfirmationService(IAppDialogService dialogService)
    {
        _dialogService = dialogService;
    }

    public async Task<bool> ConfirmCloseAsync(
        RootLocalizationTextMap rootTexts,
        string language,
        bool isTaskRunning,
        bool isVersionUpdateRunning,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        if (!isTaskRunning && !isVersionUpdateRunning)
        {
            return true;
        }

        var request = BuildRequest(rootTexts, language, isVersionUpdateRunning);
        var dialogResult = await _dialogService.ShowWarningConfirmAsync(request, sourceScope, cancellationToken);
        return dialogResult.Return == DialogReturnSemantic.Confirm;
    }

    internal static WarningConfirmDialogRequest BuildRequest(
        RootLocalizationTextMap rootTexts,
        string language,
        bool isVersionUpdateRunning)
    {
        var titleKey = isVersionUpdateRunning
            ? "Main.CloseConfirm.Update.Title"
            : "Main.CloseConfirm.Title";
        var messageKey = isVersionUpdateRunning
            ? "Main.CloseConfirm.Update.Message"
            : "Main.CloseConfirm.Message";
        return new WarningConfirmDialogRequest(
            Title: rootTexts[titleKey],
            Message: rootTexts[messageKey],
            ConfirmText: rootTexts["Main.CloseConfirm.Confirm"],
            CancelText: rootTexts["Main.CloseConfirm.Cancel"],
            Language: language);
    }
}
