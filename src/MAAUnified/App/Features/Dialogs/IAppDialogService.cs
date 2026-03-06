using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public interface IAppDialogService
{
    Task<DialogCompletion<AnnouncementDialogPayload>> ShowAnnouncementAsync(
        AnnouncementDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default);

    Task<DialogCompletion<VersionUpdateDialogPayload>> ShowVersionUpdateAsync(
        VersionUpdateDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default);

    Task<DialogCompletion<ProcessPickerDialogPayload>> ShowProcessPickerAsync(
        ProcessPickerDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default);

    Task<DialogCompletion<EmulatorPathDialogPayload>> ShowEmulatorPathAsync(
        EmulatorPathDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default);

    Task<DialogCompletion<ErrorDialogPayload>> ShowErrorAsync(
        ErrorDialogRequest request,
        string sourceScope,
        Func<CancellationToken, Task<UiOperationResult>>? openIssueReportAsync = null,
        CancellationToken cancellationToken = default);

    Task<DialogCompletion<AchievementListDialogPayload>> ShowAchievementListAsync(
        AchievementListDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default);

    Task<DialogCompletion<TextDialogPayload>> ShowTextAsync(
        TextDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default);
}
