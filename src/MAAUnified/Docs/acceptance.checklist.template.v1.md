# MAAUnified 验收清单模板 v1

## 摘要
- Schema version: `1.0.0`
- Baseline ref: `src/MAAUnified/Compat/Mapping/Baseline/baseline.freeze.v1.json`
- Case count: `58`

## Matrix Strategy
- Tier-1: Root + Settings + System entries, full platform/theme/locale matrix.
- Tier-2: Other feature pages, main path plus all-language text/error key validation.

| Tier | Platforms | Themes | Locales |
| --- | --- | --- | --- |
| Tier-1 | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| Tier-2 | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |

## Global Requirements
- Any failure must not crash the process.
- Any fallback must be visible to users, recorded in logs, and locatable by scope/case id.
- All checklist items remain P0 for Package A.

## Acceptance Cases
| Case ID | Tier | Item ID | Platforms | Themes | Locales |
| --- | --- | --- | --- | --- | --- |
| `ACC-001` | Tier-1 | `RootDashboard` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-002` | Tier-1 | `TaskQueueRoot` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-003` | Tier-1 | `SettingsRoot` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-004` | Tier-1 | `Settings.About` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-005` | Tier-1 | `Settings.Achievement` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-006` | Tier-1 | `Settings.Background` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-007` | Tier-1 | `Settings.ConfigurationManager` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-008` | Tier-1 | `Settings.Connect` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-009` | Tier-1 | `Settings.ExternalNotification` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-010` | Tier-1 | `Settings.Game` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-011` | Tier-1 | `Settings.Gui` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-012` | Tier-1 | `Settings.HotKey` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-013` | Tier-1 | `Settings.HotKeyEditor` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-014` | Tier-1 | `Settings.IssueReport` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-015` | Tier-1 | `Settings.Performance` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-016` | Tier-1 | `Settings.RemoteControl` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-017` | Tier-1 | `Settings.Start` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-018` | Tier-1 | `Settings.Timer` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-019` | Tier-1 | `Settings.VersionUpdate` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-020` | Tier-2 | `Task.StartUp` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-021` | Tier-2 | `Task.Fight` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-022` | Tier-2 | `Task.Recruit` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-023` | Tier-2 | `Task.Infrast` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-024` | Tier-2 | `Task.Mall` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-025` | Tier-2 | `Task.Award` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-026` | Tier-2 | `Task.Roguelike` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-027` | Tier-2 | `Task.Reclamation` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-028` | Tier-2 | `Task.Custom` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-029` | Tier-2 | `Task.PostAction` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-030` | Tier-2 | `Advanced.Copilot` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-031` | Tier-2 | `Advanced.Toolbox` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-032` | Tier-2 | `Advanced.RemoteControlCenter` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-033` | Tier-2 | `Advanced.Overlay` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-034` | Tier-2 | `Advanced.TrayIntegration` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-035` | Tier-2 | `Advanced.StageManager` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-036` | Tier-2 | `Advanced.WebApi` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-037` | Tier-2 | `Advanced.ExternalNotificationProviders` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-038` | Tier-2 | `Dialog.Announcement` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-039` | Tier-2 | `Dialog.VersionUpdate` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-040` | Tier-2 | `Dialog.ProcessPicker` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-041` | Tier-2 | `Dialog.EmulatorPath` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-042` | Tier-2 | `Dialog.Error` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-043` | Tier-2 | `Dialog.AchievementList` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-044` | Tier-2 | `Dialog.TextDialog` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-045` | Tier-1 | `System.Connect` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-046` | Tier-1 | `System.ImportLegacyConfig` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-047` | Tier-1 | `System.TrayMenu.Start` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-048` | Tier-1 | `System.TrayMenu.Stop` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-049` | Tier-1 | `System.TrayMenu.SwitchLanguage` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-050` | Tier-1 | `System.TrayMenu.ForceShow` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-051` | Tier-1 | `System.TrayMenu.HideTray` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-052` | Tier-1 | `System.TrayMenu.ToggleOverlay` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-053` | Tier-1 | `System.TrayMenu.Restart` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-054` | Tier-1 | `System.TrayMenu.Exit` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-055` | Tier-1 | `System.CapabilitySummary` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-056` | Tier-1 | `System.GlobalErrorChannel` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-057` | Tier-1 | `System.PageErrorChannel` | windows, macos, linux | Light, Dark | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |
| `ACC-058` | Tier-2 | `TaskQueueRoot` | windows | Light | zh-cn, zh-tw, en-us, ja-jp, ko-kr, pallas |

## Case Waivers
- None

## Waiver Policy
- Allowed: `True`
- Required fields: `owner, reason, expires_on, alternative_validation`
- Rule: Waiver is allowed only for blockers and must include expiry and replacement validation plan.

## Notes
- This checklist file is generated from `acceptance.template.v1.json`.
