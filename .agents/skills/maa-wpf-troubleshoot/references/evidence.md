# 收集和判读证据

## 给用户的收集步骤

1. 复现一次，记下本地时间、预期、实际画面、操作顺序，记录 ｢设置 → 更新设置｣ 的界面/核心版本和资源日期。设备问题再记录模拟器、分辨率、帧率、触控、是否启用增强。任务问题再记录区服、当前配置、任务名、关卡难度。
2. 用 ｢设置 → 问题反馈 → 生成日志压缩包｣，默认在 MAA 安装目录的 `reports`。程序无法打开时，可到 `MAA.exe` 所在目录手动收集 `debug`、`config`、`cache`。若完全没有日志，提供启动报错原文、系统版本与架构，必要时补 Windows 应用程序事件和故障模块。
3. 上传前在本地查看内容，按需遮盖账号标识、内网地址和本机用户名，移除或遮盖 CDK、Token、密码、Webhook 等。生成报告会直接复制配置和缓存；手动压缩也需检查。公开反馈由用户确认并提交。只需要判断一个简单现象时，先提供脱敏的相关日志片段。

依据：[新手上路的反馈说明](https://docs.maa.plus/zh-cn/manual/newbie.html) · [官方问题模板](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/.github/ISSUE_TEMPLATE/cn-bug-report.yaml) · [WPF 报告打包](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UserControl/Settings/IssueReportUserControlModel.cs)。

## 文件定位

| 文件 | 读什么 |
| --- | --- |
| `debug/gui.log`、`debug/gui.bak.log` | WPF 启动、版本、选择的任务与关卡、连接和重试、更新、异常堆栈。用 `Start Task Chain`、`GetFightStage`、`Scheduled start`、`AttachWindow` 或失败提示定位时间和 task ID。 |
| `debug/asst.log`、`debug/asst.bak.log` | MaaCore 的 ADB、截图、识别和任务链。搜 `ConnectionInfo` 的 `what`、`why`、`details`，以及对应任务的 `SubTaskError`、`TaskChainError`、`cur_retry`、`Save image`。单次重试记录应和最终结果一起读。 |
| `debug/pending-update-applier.log` | 若走外部更新器，查看应用更新、移动文件、失败阶段。先看 `gui.log` 有无委托给外部更新器的记录。 |
| `config/gui.new.json`、`.bak`、`.err` | 当前配置名 `Current`、`Configurations[名称].Gui.ConnectSettings`、`Configurations[名称].TaskQueue` 和根级 `Timers`。旧版可能只有 `gui.json`。报告导出前可以改过配置，实际执行参数以当时日志与资源为准。 |
| `cache/resource`、`cache/gui` | 若报告中存在，结合日志判断复现时的资源任务与活动缓存。查对应版本的资源定义，开发分支的修复不能直接解释旧版本行为。 |
| `debug/interface`、`debug/drops`、`debug/infrast`、`debug/roguelike`、`debug/dumps` | 依次用于失败画面、掉落、基建、肉鸽和崩溃。用日志的文件名与时间配对；日志记了截图路径但附件中缺图时，可以请求对应图片或分卷。 |

WPF 可生成一个完整 `report_*.zip`，并在 `*_parts` 生成可独立解压的 ZIP。优先读 part01，它包含 debug 根目录、配置、缓存和选入的自定义资源；part02 起收录最近三天的 debug 子目录文件，可能包括截图与转储。按同次报告的时间戳归组。

依据：[启动与日志路径](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Main/Bootstrapper.cs) · [日志回调协议](https://docs.maa.plus/zh-cn/protocol/callback-schema.html) · [日志包分卷实现](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UserControl/Settings/IssueReportUserControlModel.cs)。

## 从界面追到执行

- 在对应版本的 `src/MaaWpfGui/Res/Localizations/<语言>.xaml` 找实际显示名，展开 `{key=...}` 引用。再用 key 找 `Views/UI`、`Views/UserControl` 的命令和可见条件，进入 ViewModel 与配置类。
- 队列问题从 `TaskQueueViewModel` 和对应 `*TaskUserControlModel` 追序列化，再到 `AsstProxy`、`MaaService` 和 MaaCore；识别与导航再查 `resource/tasks`、相应区服资源与缓存。Core 链名与 WPF 显示名可能不同。
- 连接按配置走 ADB 或 `AttachWindow`，以实际地址、截图和输入后画面为证据。处理线索或其他会先修改状态的任务，要逐步追踪修改前、修改后和结束时的状态。
- 每次结论列明版本、时间和最关键的几条观察。把源码允许的路径、报告实际走过的路径、还需验证的假设分开。仓库内的 [Issue 日志分析技能](../../maa-issue-log-analysis/SKILL.md) 可用于公开 Issue 的进一步取证。

依据：[WPF 回调入口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Main/AsstProxy.cs) · [简体中文界面字符串](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Res/Localizations/zh-cn.xaml) · [Core 任务资源](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/v6.21.3/resource/tasks)。
