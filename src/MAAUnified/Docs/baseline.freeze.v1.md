# MAAUnified 基线冻结 v1

## 摘要
- Frozen at (UTC): `2026-03-03T03:50:59Z`
- WPF baseline commit: `23ddd271b40a3d0c18de8a6ae8305357cc032e19`
- Scope: `src/MAAUnified/**`
- Matrix mode: `tiered`
- Themes: `Light, Dark`
- Locales: `zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas`
- Feature items: `44`
- System items: `13`
- Config keys: `290`
- Fallback records: `15`

## Feature Parity
- Aligned: `35`
- Gap: `9`

| Item ID | Group | Parity | Avalonia Path |
| --- | --- | --- | --- |
| `RootDashboard` | Root | Aligned | `App/Features/Root/RootDashboardView.axaml` |
| `TaskQueueRoot` | Root | Gap | `App/Features/Root/TaskQueueView.axaml` |
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
| `Advanced.Copilot` | Advanced | Gap | `App/Features/Advanced/CopilotView.axaml` |
| `Advanced.Toolbox` | Advanced | Gap | `App/Features/Advanced/ToolboxView.axaml` |
| `Advanced.RemoteControlCenter` | Advanced | Gap | `App/Features/Advanced/RemoteControlCenterView.axaml` |
| `Advanced.Overlay` | Advanced | Gap | `App/Features/Advanced/OverlayView.axaml` |
| `Advanced.TrayIntegration` | Advanced | Gap | `App/Features/Advanced/TrayIntegrationView.axaml` |
| `Advanced.StageManager` | Advanced | Gap | `App/Features/Advanced/StageManagerView.axaml` |
| `Advanced.WebApi` | Advanced | Gap | `App/Features/Advanced/WebApiView.axaml` |
| `Advanced.ExternalNotificationProviders` | Advanced | Gap | `App/Features/Advanced/ExternalNotificationProvidersView.axaml` |
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
| `System.Connect` | Gap | `App/Views/MainWindow.axaml` |
| `System.ImportLegacyConfig` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.Start` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.Stop` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.SwitchLanguage` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.ForceShow` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.HideTray` | Gap | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.ToggleOverlay` | Aligned | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.Restart` | Gap | `App/Views/MainWindow.axaml` |
| `System.TrayMenu.Exit` | Aligned | `App/Views/MainWindow.axaml` |
| `System.CapabilitySummary` | Aligned | `App/Views/MainWindow.axaml` |
| `System.GlobalErrorChannel` | Aligned | `App/Views/MainWindow.axaml` |
| `System.PageErrorChannel` | Aligned | `App/ViewModels/Infrastructure/PageViewModelBase.cs` |

## Config Key Mapping Summary
- Aligned: `10`
- Gap: `280`
- Waived: `0`

### Aligned Config Keys
- `GUI.Background.BlurEffectRadius` -> `SettingsPageViewModel.BackgroundBlur`
- `GUI.Background.ImagePath` -> `SettingsPageViewModel.BackgroundImagePath`
- `GUI.Background.Opacity` -> `SettingsPageViewModel.BackgroundOpacity`
- `GUI.Localization` -> `SettingsPageViewModel.Language`
- `GUI.MinimizeToTray` -> `SettingsPageViewModel.MinimizeToTray`
- `GUI.UseTray` -> `SettingsPageViewModel.UseTray`
- `GUI.WindowTitleScrollable` -> `SettingsPageViewModel.WindowTitleScrollable`
- `RemoteControl.RemoteControlGetTaskEndpointUri` -> `SettingsPageViewModel.RemoteGetTaskEndpoint`
- `RemoteControl.RemoteControlPollIntervalMs` -> `SettingsPageViewModel.RemotePollInterval`
- `RemoteControl.RemoteControlReportStatusUri` -> `SettingsPageViewModel.RemoteReportEndpoint`

## Platform Fallback Records
| Capability | Platform | Expected | Current | Parity | Visible | Recorded | Locatable |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Tray | windows | full | fallback | Gap | True | True | True |
| Tray | macos | degrade-visible | fallback | Aligned | True | True | True |
| Tray | linux | degrade-visible | fallback | Aligned | True | True | True |
| Notification | windows | full | fallback | Gap | True | True | True |
| Notification | macos | degrade-visible | fallback | Aligned | True | True | True |
| Notification | linux | degrade-visible | fallback | Aligned | True | True | True |
| Hotkey | windows | full | fallback | Gap | True | True | True |
| Hotkey | macos | degrade-visible | fallback | Aligned | True | True | True |
| Hotkey | linux | degrade-visible | fallback | Aligned | True | True | True |
| Autostart | windows | full | fallback | Gap | True | True | True |
| Autostart | macos | degrade-visible | fallback | Aligned | True | True | True |
| Autostart | linux | degrade-visible | fallback | Aligned | True | True | True |
| Overlay | windows | full | fallback | Gap | True | True | True |
| Overlay | macos | degrade-visible | fallback | Aligned | True | True | True |
| Overlay | linux | degrade-visible | fallback | Aligned | True | True | True |

## Notes
- This file is generated from `baseline.freeze.v1.json` during Package A freeze.
- Any baseline change must follow `baseline-change-control.v1.md`.
