namespace MAAUnified.App.ViewModels;

public static class FeatureManifest
{
    public static IReadOnlyList<FeatureModule> All { get; } =
    [
        new("RootDashboard", "Root", "Root 主框架", "导航、状态栏、全局命令", "RootDashboard", "主框架"),
        new("TaskQueueRoot", "Root", "任务队列主页", "任务队列入口与总览", "TaskQueue", "主流程"),
        new("SettingsRoot", "Root", "设置主页", "设置导航与总览", "Settings", "主流程"),

        new("Settings.About", "Settings", "About", "关于信息", "AboutSettings", "设置"),
        new("Settings.Achievement", "Settings", "Achievement", "成就设置", "AchievementSettings", "设置"),
        new("Settings.Background", "Settings", "Background", "背景设置", "BackgroundSettings", "设置"),
        new("Settings.ConfigurationManager", "Settings", "Configuration Manager", "配置管理与导入入口", "ConfigurationManager", "设置"),
        new("Settings.Connect", "Settings", "Connect Settings", "连接配置", "ConnectSettings", "设置"),
        new("Settings.ExternalNotification", "Settings", "External Notification", "外部通知配置", "ExternalNotificationSettings", "设置"),
        new("Settings.Game", "Settings", "Game Settings", "游戏相关配置", "GameSettings", "设置"),
        new("Settings.Gui", "Settings", "GUI Settings", "界面行为与显示", "GuiSettings", "设置"),
        new("Settings.HotKey", "Settings", "HotKey Settings", "快捷键设置", "HotKeySettings", "设置"),
        new("Settings.HotKeyEditor", "Settings", "HotKey Editor", "快捷键编辑器", "HotKeyEditor", "设置"),
        new("Settings.IssueReport", "Settings", "Issue Report", "问题报告", "IssueReport", "设置"),
        new("Settings.Performance", "Settings", "Performance", "性能配置", "PerformanceSettings", "设置"),
        new("Settings.RemoteControl", "Settings", "Remote Control", "远程控制配置", "RemoteControlSettings", "设置"),
        new("Settings.Start", "Settings", "Start Settings", "启动行为", "StartSettings", "设置"),
        new("Settings.Timer", "Settings", "Timer Settings", "定时设置", "TimerSettings", "设置"),
        new("Settings.VersionUpdate", "Settings", "Version Update", "版本更新设置", "VersionUpdateSettings", "设置"),

        new("Task.StartUp", "TaskQueue", "StartUp", "启动任务配置", "StartUpTask", "任务配置"),
        new("Task.Fight", "TaskQueue", "Fight", "作战任务配置", "FightSettings", "任务配置"),
        new("Task.Recruit", "TaskQueue", "Recruit", "招募任务配置", "RecruitSettings", "任务配置"),
        new("Task.Infrast", "TaskQueue", "Infrast", "基建任务配置", "InfrastSettings", "任务配置"),
        new("Task.Mall", "TaskQueue", "Mall", "商店任务配置", "MallSettings", "任务配置"),
        new("Task.Award", "TaskQueue", "Award", "奖励任务配置", "AwardSettings", "任务配置"),
        new("Task.Roguelike", "TaskQueue", "Roguelike", "肉鸽任务配置", "RoguelikeSettings", "任务配置"),
        new("Task.Reclamation", "TaskQueue", "Reclamation", "生息演算配置", "ReclamationSettings", "任务配置"),
        new("Task.Custom", "TaskQueue", "Custom", "自定义任务配置", "CustomSettings", "任务配置"),
        new("Task.PostAction", "TaskQueue", "PostAction", "后置动作配置", "PostActionSettings", "任务配置"),

        new("Advanced.Copilot", "Advanced", "Copilot", "作业导入与执行", "Copilot", "高级功能"),
        new("Advanced.Toolbox", "Advanced", "Toolbox", "工具箱", "Toolbox", "高级功能"),
        new("Advanced.RemoteControlCenter", "Advanced", "Remote Control Center", "远程控制中心", "RemoteControlCenter", "高级功能"),
        new("Advanced.Overlay", "Advanced", "Overlay", "窗口附着/覆盖层", "Overlay", "高级功能"),
        new("Advanced.TrayIntegration", "Advanced", "Tray Integration", "托盘交互", "TrayIntegration", "高级功能"),
        new("Advanced.StageManager", "Advanced", "Stage Manager", "关卡管理", "StageManager", "高级功能"),
        new("Advanced.WebApi", "Advanced", "Web API", "网络接口", "WebApi", "高级功能"),
        new("Advanced.ExternalNotificationProviders", "Advanced", "Notification Providers", "外部通知 Provider", "ExternalNotificationProviders", "高级功能"),

        new("Dialog.Announcement", "Dialogs", "AnnouncementDialog", "公告弹窗", "AnnouncementDialog", "对话框"),
        new("Dialog.VersionUpdate", "Dialogs", "VersionUpdateDialog", "版本更新弹窗", "VersionUpdateDialog", "对话框"),
        new("Dialog.ProcessPicker", "Dialogs", "ProcessPickerDialog", "进程选择弹窗", "ProcessPickerDialog", "对话框"),
        new("Dialog.EmulatorPath", "Dialogs", "EmulatorPathSelectionDialog", "模拟器路径弹窗", "EmulatorPathSelectionDialog", "对话框"),
        new("Dialog.Error", "Dialogs", "ErrorDialog", "错误弹窗", "ErrorDialog", "对话框"),
        new("Dialog.AchievementList", "Dialogs", "AchievementListDialog", "成就列表弹窗", "AchievementListDialog", "对话框"),
        new("Dialog.TextDialog", "Dialogs", "TextDialog", "文本弹窗", "TextDialog", "对话框"),
    ];
}
