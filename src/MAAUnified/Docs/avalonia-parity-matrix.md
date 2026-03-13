# MAAUnified 功能对齐矩阵（完整清单）

## 说明

- 基线：`src/MaaWpfGui` 现有功能。
- 目标：`src/MAAUnified` 全量复刻。
- 状态：`Implemented` / `InProgress` / `Pending`。
- 本表需与 `Docs/baseline.freeze.v1.md` 保持同步；若冲突，以冻结基线为准。

## A. 主框架与主流程

| 模块 | 状态 |
| --- | --- |
| Root 主框架 | Implemented |
| 连接设置 | InProgress |
| 任务队列 | InProgress |
| Start/Stop 状态机 | InProgress |
| 实时日志 | Implemented |

## B. 设置页

| 模块 | 状态 |
| --- | --- |
| About | Implemented |
| Achievement | Implemented |
| Background | Implemented |
| Configuration Manager | Implemented |
| Connect Settings | Implemented |
| External Notification | Implemented |
| Game Settings | Implemented |
| GUI Settings | Implemented |
| HotKey Settings | Implemented |
| HotKey Editor | Implemented |
| Issue Report | Implemented |
| Performance | Implemented |
| Remote Control | Implemented |
| Start Settings | Implemented |
| Timer Settings | Implemented |
| Version Update Settings | Implemented |

## C. 任务配置页

| 模块 | 状态 |
| --- | --- |
| StartUp | Implemented |
| Fight | Implemented |
| Recruit | Implemented |
| Infrast | Implemented |
| Mall | Implemented |
| Award | Implemented |
| Roguelike | Implemented |
| Reclamation | Implemented |
| Custom | Implemented |
| PostAction | Implemented |

## D. 高级模块

| 模块 | 状态 |
| --- | --- |
| Copilot | InProgress |
| Toolbox | Implemented |
| Remote Control Center | Implemented |
| Overlay | Implemented |
| Tray Integration | Implemented |
| StageManager / Web API / Notification Providers | Implemented |

Toolbox note: Avalonia 侧当前为 WPF 对齐的 6-tab 运行工具页，风险提示仅保留在抽卡页。

## E. 对话框

| 模块 | 状态 |
| --- | --- |
| AnnouncementDialog | Implemented |
| VersionUpdateDialog | Implemented |
| ProcessPickerDialog | Implemented |
| EmulatorPathSelectionDialog | Implemented |
| ErrorDialog | Implemented |
| AchievementListDialog | Implemented |
| TextDialog | Implemented |
