namespace MAAUnified.Compat.Mapping;

public static class WpfFeatureBaseline
{
    public static readonly string[] SettingsModules =
    [
        "About", "Achievement", "Background", "Configuration Manager", "Connect Settings", "External Notification",
        "Game Settings", "GUI Settings", "HotKey Settings", "HotKey Editor", "Issue Report", "Performance",
        "Remote Control", "Start Settings", "Timer Settings", "Version Update Settings",
    ];

    public static readonly string[] TaskModules =
    [
        "StartUp", "Fight", "Recruit", "Infrast", "Mall", "Award", "Roguelike", "Reclamation", "Custom", "PostAction",
    ];

    public static readonly string[] AdvancedModules =
    [
        "Copilot", "Toolbox", "Remote Control", "Overlay", "Tray Integration", "StageManager", "Web API", "Notification Providers",
    ];

    public static readonly string[] DialogModules =
    [
        "AnnouncementDialog", "VersionUpdateDialog", "ProcessPickerDialog", "EmulatorPathSelectionDialog", "ErrorDialog", "AchievementListDialog", "TextDialog",
    ];
}
