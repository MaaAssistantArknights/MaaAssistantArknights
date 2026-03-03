# WPF -> Avalonia 字段/命令映射（P0 冻结）

## 页面映射

| Windows (WPF) | Avalonia |
| --- | --- |
| `RootView` | `App/Views/MainWindow.axaml` |
| `TaskQueueView` | `App/Features/Root/TaskQueueView.axaml` |
| `CopilotView` | `App/Features/Advanced/CopilotView.axaml` |
| `ToolboxView` | `App/Features/Advanced/ToolboxView.axaml` |
| `SettingsView` | `App/Features/Root/SettingsView.axaml` |
| `NotifyIcon` | `MainWindow` 托盘菜单入口 + `Platform/ITrayService` |
| `OverlayWindow` | `TaskQueuePageViewModel` Overlay 控制区 + `IOverlayCapabilityService` |

## 任务模块映射（10）

| WPF 模块 | Avalonia 模块视图 |
| --- | --- |
| StartUp | `Features/TaskQueue/StartUpTaskView.axaml` |
| Fight | `Features/TaskQueue/FightSettingsView.axaml` |
| Recruit | `Features/TaskQueue/RecruitSettingsView.axaml` |
| Infrast | `Features/TaskQueue/InfrastSettingsView.axaml` |
| Mall | `Features/TaskQueue/MallSettingsView.axaml` |
| Award | `Features/TaskQueue/AwardSettingsView.axaml` |
| Roguelike | `Features/TaskQueue/RoguelikeSettingsView.axaml` |
| Reclamation | `Features/TaskQueue/ReclamationSettingsView.axaml` |
| Custom | `Features/TaskQueue/CustomSettingsView.axaml` |
| PostAction | `Features/TaskQueue/PostActionSettingsView.axaml` |

## 设置段映射（15）

| WPF 设置段 | Avalonia 视图 |
| --- | --- |
| Configuration Manager | `Settings/ConfigurationManagerView.axaml` |
| Timer | `Settings/TimerSettingsView.axaml` |
| Performance | `Settings/PerformanceSettingsView.axaml` |
| Game | `Settings/GameSettingsView.axaml` |
| Connect | `Settings/ConnectSettingsView.axaml` |
| Start | `Settings/StartSettingsView.axaml` |
| Remote Control | `Settings/RemoteControlSettingsView.axaml` |
| GUI | `Settings/GuiSettingsView.axaml` |
| Background | `Settings/BackgroundSettingsView.axaml` |
| External Notification | `Settings/ExternalNotificationSettingsView.axaml` |
| HotKey | `Settings/HotKeySettingsView.axaml` |
| Achievement | `Settings/AchievementSettingsView.axaml` |
| Version Update | `Settings/VersionUpdateSettingsView.axaml` |
| Issue Report | `Settings/IssueReportView.axaml` |
| About | `Settings/AboutSettingsView.axaml` |

## 对话框映射（7）

| WPF Dialog | Avalonia Dialog View |
| --- | --- |
| Announcement | `Dialogs/AnnouncementDialogView.axaml` |
| VersionUpdate | `Dialogs/VersionUpdateDialogView.axaml` |
| ProcessPicker | `Dialogs/ProcessPickerDialogView.axaml` |
| EmulatorPathSelection | `Dialogs/EmulatorPathSelectionDialogView.axaml` |
| ErrorDialog | `Dialogs/ErrorDialogView.axaml` |
| AchievementList | `Dialogs/AchievementListDialogView.axaml` |
| TextDialog | `Dialogs/TextDialogView.axaml` |

## 配置键映射（示例）

| Legacy key (`ConfigurationKeys`) | Avalonia 落点 |
| --- | --- |
| `GUI.Localization` | `SettingsPageViewModel.Language` |
| `GUI.UseTray` | `SettingsPageViewModel.UseTray` |
| `GUI.MinimizeToTray` | `SettingsPageViewModel.MinimizeToTray` |
| `GUI.WindowTitleScrollable` | `SettingsPageViewModel.WindowTitleScrollable` |
| `GUI.Background.ImagePath` | `SettingsPageViewModel.BackgroundImagePath` |
| `GUI.Background.Opacity` | `SettingsPageViewModel.BackgroundOpacity` |
| `GUI.Background.BlurEffectRadius` | `SettingsPageViewModel.BackgroundBlur` |
| `RemoteControl.RemoteControlGetTaskEndpointUri` | `SettingsPageViewModel.RemoteGetTaskEndpoint` |
| `RemoteControl.RemoteControlReportStatusUri` | `SettingsPageViewModel.RemoteReportEndpoint` |
| `RemoteControl.RemoteControlPollIntervalMs` | `SettingsPageViewModel.RemotePollInterval` |

## 命令映射（关键）

| Windows 命令 | Avalonia 实现 |
| --- | --- |
| LinkStart | `TaskQueuePageViewModel.StartAsync` |
| Stop | `TaskQueuePageViewModel.StopAsync` |
| WaitAndStop | `TaskQueuePageViewModel.WaitAndStopAsync` |
| 手动导入旧配置 | `MainShellViewModel.ManualImportAsync` |
| 生成支持包 | `SettingsPageViewModel.BuildIssueReportAsync` |
| Overlay 目标选择 | `TaskQueuePageViewModel.ReloadOverlayTargetsAsync + SelectOverlayTargetAsync` |
