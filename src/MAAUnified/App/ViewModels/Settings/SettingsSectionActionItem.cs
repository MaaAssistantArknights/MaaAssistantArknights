namespace MAAUnified.App.ViewModels.Settings;

public sealed record SettingsSectionActionItem(
    string ActionId,
    string Label,
    bool IsPrimary = false,
    bool IsSubtle = false,
    bool IsEnabled = true)
{
    public bool IsSecondary => !IsPrimary && !IsSubtle;
}
