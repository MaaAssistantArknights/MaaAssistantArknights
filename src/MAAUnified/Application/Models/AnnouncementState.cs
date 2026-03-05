using MAAUnified.Compat.Constants;

namespace MAAUnified.Application.Models;

public sealed record AnnouncementState(
    string AnnouncementInfo,
    bool DoNotRemindThisAnnouncementAgain,
    bool DoNotShowAnnouncement)
{
    public static AnnouncementState Default { get; } = new(
        AnnouncementInfo: string.Empty,
        DoNotRemindThisAnnouncementAgain: false,
        DoNotShowAnnouncement: false);

    public IReadOnlyDictionary<string, string> ToGlobalSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.AnnouncementInfo] = AnnouncementInfo,
            [ConfigurationKeys.DoNotRemindThisAnnouncementAgain] = DoNotRemindThisAnnouncementAgain.ToString(),
            [ConfigurationKeys.DoNotShowAnnouncement] = DoNotShowAnnouncement.ToString(),
        };
    }
}
