# MAAUnified 基线冻结 v1

## 摘要
- Frozen at (UTC): `2026-03-04T07:48:13Z`
- WPF baseline commit: `23ddd271b40a3d0c18de8a6ae8305357cc032e19`
- Scope: `src/MAAUnified/**`
- Matrix mode: `tiered`
- Themes: `Light, Dark`
- Locales: `zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas`
- Feature items: `44`
- System items: `13`
- Config keys: `292`
- Fallback records: `15`

## Feature Parity
- Aligned: `44`
- Gap: `0`
- Waived: `0`

| Item ID | Group | Parity | Avalonia Path |
| --- | --- | --- | --- |
| `RootDashboard` | Root | Aligned | `App/Features/Root/RootDashboardView.axaml` |
| `TaskQueueRoot` | Root | Aligned | `App/Features/Root/TaskQueueView.axaml` |
| `SettingsRoot` | Root | Aligned | `App/Features/Root/SettingsView.axaml` |
| `Settings.About` | Settings | Aligned | `App/Features/Settings/AboutSettingsView.axaml` |
| `Settings.Achievement` | Settings | Aligned | `App/Features/Settings/AchievementSettingsView.axaml` |
| `Settings.Background` | Settings | Aligned | `App/Features/Settings/BackgroundSettingsView.axaml` |
| `Settings.ConfigurationManager` | Settings | Aligned | `App/Features/Settings/ConfigurationManagerView.axaml` |
| `Settings.Connect` | Settings | Aligned | `App/Features/Settings/ConnectSettingsView.axaml` |
| `Settings.ExternalNotification` | Settings | Aligned | `App/Features/Settings/ExternalNotificationSettingsView.axaml` |
| `Settings.Game` | Settings | Aligned | `App/Features/Settings/GameSettingsView.axaml` |
| `Settings.Gui` | Settings | Aligned | `App/Features/Settings/GuiSettingsView.axaml` |
| `Settings.HotKey` | Settings | Aligned | `App/Features/Settings/HotKeySettingsView.axaml` |
| `Settings.HotKeyEditor` | Settings | Aligned | `App/Features/Settings/HotKeyEditorView.axaml` |
| `Settings.IssueReport` | Settings | Aligned | `App/Features/Settings/IssueReportView.axaml` |
| `Settings.Performance` | Settings | Aligned | `App/Features/Settings/PerformanceSettingsView.axaml` |
| `Settings.RemoteControl` | Settings | Aligned | `App/Features/Settings/RemoteControlSettingsView.axaml` |
| `Settings.Start` | Settings | Aligned | `App/Features/Settings/StartSettingsView.axaml` |
| `Settings.Timer` | Settings | Aligned | `App/Features/Settings/TimerSettingsView.axaml` |
| `Settings.VersionUpdate` | Settings | Aligned | `App/Features/Settings/VersionUpdateSettingsView.axaml` |
| `Task.StartUp` | TaskQueue | Aligned | `App/Features/TaskQueue/StartUpTaskView.axaml` |
| `Task.Fight` | TaskQueue | Aligned | `App/Features/TaskQueue/FightSettingsView.axaml` |
| `Task.Recruit` | TaskQueue | Aligned | `App/Features/TaskQueue/RecruitSettingsView.axaml` |
| `Task.Infrast` | TaskQueue | Aligned | `App/Features/TaskQueue/InfrastSettingsView.axaml` |
| `Task.Mall` | TaskQueue | Aligned | `App/Features/TaskQueue/MallSettingsView.axaml` |
| `Task.Award` | TaskQueue | Aligned | `App/Features/TaskQueue/AwardSettingsView.axaml` |
| `Task.Roguelike` | TaskQueue | Aligned | `App/Features/TaskQueue/RoguelikeSettingsView.axaml` |
| `Task.Reclamation` | TaskQueue | Aligned | `App/Features/TaskQueue/ReclamationSettingsView.axaml` |
| `Task.Custom` | TaskQueue | Aligned | `App/Features/TaskQueue/CustomSettingsView.axaml` |
| `Task.PostAction` | TaskQueue | Aligned | `App/Features/TaskQueue/PostActionSettingsView.axaml` |
| `Advanced.Copilot` | Advanced | Aligned | `App/Features/Advanced/CopilotView.axaml` |
| `Advanced.Toolbox` | Advanced | Aligned | `App/Features/Advanced/ToolboxView.axaml` |
| `Advanced.RemoteControlCenter` | Advanced | Aligned | `App/Features/Advanced/RemoteControlCenterView.axaml` |
| `Advanced.Overlay` | Advanced | Aligned | `App/Features/Advanced/OverlayView.axaml` |
| `Advanced.TrayIntegration` | Advanced | Aligned | `App/Features/Advanced/TrayIntegrationView.axaml` |
| `Advanced.StageManager` | Advanced | Aligned | `App/Features/Advanced/StageManagerView.axaml` |
| `Advanced.WebApi` | Advanced | Aligned | `App/Features/Advanced/WebApiView.axaml` |
| `Advanced.ExternalNotificationProviders` | Advanced | Aligned | `App/Features/Advanced/ExternalNotificationProvidersView.axaml` |
| `Dialog.Announcement` | Dialogs | Aligned | `App/Features/Dialogs/AnnouncementDialogView.axaml` |
| `Dialog.VersionUpdate` | Dialogs | Aligned | `App/Features/Dialogs/VersionUpdateDialogView.axaml` |
| `Dialog.ProcessPicker` | Dialogs | Aligned | `App/Features/Dialogs/ProcessPickerDialogView.axaml` |
| `Dialog.EmulatorPath` | Dialogs | Aligned | `App/Features/Dialogs/EmulatorPathSelectionDialogView.axaml` |
| `Dialog.Error` | Dialogs | Aligned | `App/Features/Dialogs/ErrorDialogView.axaml` |
| `Dialog.AchievementList` | Dialogs | Aligned | `App/Features/Dialogs/AchievementListDialogView.axaml` |
| `Dialog.TextDialog` | Dialogs | Aligned | `App/Features/Dialogs/TextDialogView.axaml` |

## System Entry Parity
| Item ID | Parity | Avalonia Path |
| --- | --- | --- |
| `System.Connect` | Aligned | `App/Views/MainWindow.axaml` |
| `System.ImportLegacyConfig` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.Start` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.Stop` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.SwitchLanguage` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.ForceShow` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.HideTray` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.ToggleOverlay` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.Restart` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.Exit` | Aligned | `App/Views/MainWindow.axaml` |
| `System.CapabilitySummary` | Aligned | `App/Views/MainWindow.axaml` |
| `System.GlobalErrorChannel` | Aligned | `App/Views/MainWindow.axaml` |
| `System.PageErrorChannel` | Aligned | `App/ViewModels/Infrastructure/PageViewModelBase.cs` |

## Config Key Mapping Summary
- Aligned: `292`
- Gap: `0`
- Waived: `0`

### Aligned Config Keys
- `Achievement.PopupAutoClose` -> `AchievementPolicy`
- `Achievement.PopupDisabled` -> `AchievementPolicy`
- `Announcement.AnnouncementInfo` -> `AnnouncementState`
- `Announcement.DoNotRemindThisAnnouncementAgain` -> `AnnouncementState`
- `Announcement.DoNotShowAnnouncement` -> `AnnouncementState`
- `Bluestacks.Config.Error` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Bluestacks.Config.Error]`
- `Bluestacks.Config.Keyword` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Bluestacks.Config.Keyword]`
- `Bluestacks.Config.Path` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Bluestacks.Config.Path]`
- `Configurations` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Configurations]`
- `Copilot.AddUserAdditional` -> `Copilot settings bridge`
- `Copilot.CopilotTaskList` -> `Copilot settings bridge`
- `Copilot.LoopTimes` -> `Copilot settings bridge`
- `Copilot.SelectFormation` -> `Copilot settings bridge`
- `Copilot.SupportUnitUsage` -> `Copilot settings bridge`
- `Copilot.UserAdditional` -> `Copilot settings bridge`
- `Cron` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Cron]`
- `Current` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Current]`
- `Data` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Data]`
- `Debug.TaskName` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Debug.TaskName]`
- `Default` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Default]`
- `Depot.DepotResult` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Depot.DepotResult]`
- `GUI.Background.BlurEffectRadius` -> `SettingsPageViewModel.BackgroundBlur`
- `GUI.Background.ImagePath` -> `SettingsPageViewModel.BackgroundImagePath`
- `GUI.Background.Opacity` -> `SettingsPageViewModel.BackgroundOpacity`
- `GUI.Localization` -> `SettingsPageViewModel.Language`
- `GUI.MinimizeToTray` -> `SettingsPageViewModel.MinimizeToTray`
- `GUI.UseTray` -> `SettingsPageViewModel.UseTray`
- `GUI.WindowTitleScrollable` -> `SettingsPageViewModel.WindowTitleScrollable`
- `Gacha.ShowDisclaimerNoMore` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Gacha.ShowDisclaimerNoMore]`
- `Global` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Global]`
- `Guide.StepIndex` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Guide.StepIndex]`
- `HotKeys` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[HotKeys]`
- `MiniGame.SecretFrontEnding` -> `Toolbox settings bridge`
- `MiniGame.SecretFrontEvent` -> `Toolbox settings bridge`
- `MiniGame.TaskName` -> `Toolbox settings bridge`
- `OperBox.Data` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[OperBox.Data]`
- `Overlay.PreviewPinned` -> `OverlayTargetPersistence.LoadPreviewPreference`
- `Overlay.Target` -> `OverlayAdvancedPageViewModel.SelectedTarget`
- `Peep.TargetFps` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Peep.TargetFps]`
- `Penguin.EnablePenguin` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Penguin.EnablePenguin]`
- `Penguin.Id` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Penguin.Id]`
- `Penguin.IsDrGrandet` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Penguin.IsDrGrandet]`
- `RemoteControl.RemoteControlGetTaskEndpointUri` -> `SettingsPageViewModel.RemoteGetTaskEndpoint`
- `RemoteControl.RemoteControlPollIntervalMs` -> `SettingsPageViewModel.RemotePollInterval`
- `RemoteControl.RemoteControlReportStatusUri` -> `SettingsPageViewModel.RemoteReportEndpoint`
- `TimeOut.Timer.ReminderIntervalMinutes` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[TimeOut.Timer.ReminderIntervalMinutes]`
- `TimeOut.Timer.TaskTimeoutMinutes` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[TimeOut.Timer.TaskTimeoutMinutes]`
- `Toolbox.Recruit.ChooseLevel3.Time` -> `Toolbox settings bridge`
- `Toolbox.Recruit.ChooseLevel4.Time` -> `Toolbox settings bridge`
- `Toolbox.Recruit.ChooseLevel5.Time` -> `Toolbox settings bridge`
- `VersionUpdate.AllowNightlyUpdates` -> `VersionUpdatePolicy`
- `VersionUpdate.AutoDownloadUpdatePackage` -> `VersionUpdatePolicy`
- `VersionUpdate.AutoInstallUpdatePackage` -> `VersionUpdatePolicy`
- `VersionUpdate.HasAcknowledgedNightlyWarning` -> `VersionUpdatePolicy`
- `VersionUpdate.Proxy` -> `VersionUpdatePolicy`
- `VersionUpdate.ProxyType` -> `VersionUpdatePolicy`
- `VersionUpdate.ResourceApi` -> `VersionUpdatePolicy`
- `VersionUpdate.ResourceUpdateSource` -> `VersionUpdatePolicy`
- `VersionUpdate.ResourceUpdateSource.MirrorChyanCdk` -> `VersionUpdatePolicy`
- `VersionUpdate.ScheduledUpdateCheck` -> `VersionUpdatePolicy`
- `VersionUpdate.StartupUpdateCheck` -> `VersionUpdatePolicy`
- `VersionUpdate.UpdateSource.ForceGithubGlobalSource` -> `VersionUpdatePolicy`
- `VersionUpdate.UpdateSource.MirrorChyanCdkExpired` -> `VersionUpdatePolicy`
- `VersionUpdate.UseAria2` -> `VersionUpdatePolicy`
- `VersionUpdate.VersionType` -> `VersionUpdatePolicy`
- `VersionUpdate.body` -> `VersionUpdatePolicy`
- `VersionUpdate.doNotShowUpdate` -> `VersionUpdatePolicy`
- `VersionUpdate.isfirstboot` -> `VersionUpdatePolicy`
- `VersionUpdate.name` -> `VersionUpdatePolicy`
- `VersionUpdate.package` -> `VersionUpdatePolicy`
- `Yituliu.EnableYituliu` -> `UnifiedConfigurationService.CurrentConfig.GlobalValues[Yituliu.EnableYituliu]`

## Waiver Entries
- None

## Platform Fallback Records
| Capability | Platform | Expected | Current | Parity | Visible | Recorded | Locatable |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Tray | windows | full | full | Aligned | True | True | True |
| Tray | macos | degrade-visible | fallback | Aligned | True | True | True |
| Tray | linux | degrade-visible | fallback | Aligned | True | True | True |
| Notification | windows | full | full | Aligned | True | True | True |
| Notification | macos | degrade-visible | fallback | Aligned | True | True | True |
| Notification | linux | degrade-visible | fallback | Aligned | True | True | True |
| Hotkey | windows | full | full | Aligned | True | True | True |
| Hotkey | macos | degrade-visible | fallback | Aligned | True | True | True |
| Hotkey | linux | degrade-visible | fallback | Aligned | True | True | True |
| Autostart | windows | full | full | Aligned | True | True | True |
| Autostart | macos | degrade-visible | fallback | Aligned | True | True | True |
| Autostart | linux | degrade-visible | fallback | Aligned | True | True | True |
| Overlay | windows | full | full | Aligned | True | True | True |
| Overlay | macos | degrade-visible | fallback | Aligned | True | True | True |
| Overlay | linux | degrade-visible | fallback | Aligned | True | True | True |

## Notes
- This file is generated from `baseline.freeze.v1.json` during Package A freeze.
- Any baseline change must follow `baseline-change-control.v1.md`.
