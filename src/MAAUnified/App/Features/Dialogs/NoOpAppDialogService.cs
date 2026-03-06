using MAAUnified.Application.Models;

namespace MAAUnified.App.Features.Dialogs;

public sealed class NoOpAppDialogService : IAppDialogService
{
    public static NoOpAppDialogService Instance { get; } = new();

    private NoOpAppDialogService()
    {
    }

    public Task<DialogCompletion<AnnouncementDialogPayload>> ShowAnnouncementAsync(
        AnnouncementDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new DialogCompletion<AnnouncementDialogPayload>(
            DialogReturnSemantic.Close,
            null,
            "dialog-service-unavailable"));
    }

    public Task<DialogCompletion<VersionUpdateDialogPayload>> ShowVersionUpdateAsync(
        VersionUpdateDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new DialogCompletion<VersionUpdateDialogPayload>(
            DialogReturnSemantic.Close,
            null,
            "dialog-service-unavailable"));
    }

    public Task<DialogCompletion<ProcessPickerDialogPayload>> ShowProcessPickerAsync(
        ProcessPickerDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new DialogCompletion<ProcessPickerDialogPayload>(
            DialogReturnSemantic.Close,
            null,
            "dialog-service-unavailable"));
    }

    public Task<DialogCompletion<EmulatorPathDialogPayload>> ShowEmulatorPathAsync(
        EmulatorPathDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new DialogCompletion<EmulatorPathDialogPayload>(
            DialogReturnSemantic.Close,
            null,
            "dialog-service-unavailable"));
    }

    public Task<DialogCompletion<ErrorDialogPayload>> ShowErrorAsync(
        ErrorDialogRequest request,
        string sourceScope,
        Func<CancellationToken, Task<UiOperationResult>>? openIssueReportAsync = null,
        CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new DialogCompletion<ErrorDialogPayload>(
            DialogReturnSemantic.Close,
            null,
            "dialog-service-unavailable"));
    }

    public Task<DialogCompletion<AchievementListDialogPayload>> ShowAchievementListAsync(
        AchievementListDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new DialogCompletion<AchievementListDialogPayload>(
            DialogReturnSemantic.Close,
            null,
            "dialog-service-unavailable"));
    }

    public Task<DialogCompletion<TextDialogPayload>> ShowTextAsync(
        TextDialogRequest request,
        string sourceScope,
        CancellationToken cancellationToken = default)
    {
        return Task.FromResult(new DialogCompletion<TextDialogPayload>(
            DialogReturnSemantic.Close,
            null,
            "dialog-service-unavailable"));
    }
}
