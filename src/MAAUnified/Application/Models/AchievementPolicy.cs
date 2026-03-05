using MAAUnified.Compat.Constants;

namespace MAAUnified.Application.Models;

public sealed record AchievementPolicy(
    bool PopupDisabled,
    bool PopupAutoClose)
{
    public static AchievementPolicy Default { get; } = new(
        PopupDisabled: false,
        PopupAutoClose: false);

    public IReadOnlyDictionary<string, string> ToGlobalSettingUpdates()
    {
        return new Dictionary<string, string>(StringComparer.Ordinal)
        {
            [ConfigurationKeys.AchievementPopupDisabled] = PopupDisabled.ToString(),
            [ConfigurationKeys.AchievementPopupAutoClose] = PopupAutoClose.ToString(),
        };
    }
}
