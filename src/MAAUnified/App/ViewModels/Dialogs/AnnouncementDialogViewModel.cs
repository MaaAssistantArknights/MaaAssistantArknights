using MAAUnified.App.ViewModels.Infrastructure;
using MAAUnified.Application.Models;
using MAAUnified.Application.Services;

namespace MAAUnified.App.ViewModels.Dialogs;

public sealed class AnnouncementDialogViewModel : PageViewModelBase
{
    private string _announcementInfo = string.Empty;
    private bool _doNotRemindThisAnnouncementAgain;
    private bool _doNotShowAnnouncement;

    public AnnouncementDialogViewModel(MAAUnifiedRuntime runtime)
        : base(runtime)
    {
    }

    public string AnnouncementInfo
    {
        get => _announcementInfo;
        set => SetProperty(ref _announcementInfo, value ?? string.Empty);
    }

    public bool DoNotRemindThisAnnouncementAgain
    {
        get => _doNotRemindThisAnnouncementAgain;
        set => SetProperty(ref _doNotRemindThisAnnouncementAgain, value);
    }

    public bool DoNotShowAnnouncement
    {
        get => _doNotShowAnnouncement;
        set => SetProperty(ref _doNotShowAnnouncement, value);
    }

    public async Task LoadAsync(CancellationToken cancellationToken = default)
    {
        var result = await Runtime.AnnouncementFeatureService.LoadStateAsync(cancellationToken);
        var state = await ApplyResultAsync(result, "Dialog.Announcement.Load", cancellationToken);
        if (state is null)
        {
            return;
        }

        AnnouncementInfo = state.AnnouncementInfo;
        DoNotRemindThisAnnouncementAgain = state.DoNotRemindThisAnnouncementAgain;
        DoNotShowAnnouncement = state.DoNotShowAnnouncement;
    }

    public async Task SaveAsync(CancellationToken cancellationToken = default)
    {
        var state = new AnnouncementState(
            AnnouncementInfo: AnnouncementInfo,
            DoNotRemindThisAnnouncementAgain: DoNotRemindThisAnnouncementAgain,
            DoNotShowAnnouncement: DoNotShowAnnouncement);
        await ApplyResultAsync(
            await Runtime.AnnouncementFeatureService.SaveStateAsync(state, cancellationToken),
            "Dialog.Announcement.Save",
            cancellationToken);
    }
}
