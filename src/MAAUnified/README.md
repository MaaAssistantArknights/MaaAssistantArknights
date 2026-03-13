# MAAUnified

Single-directory implementation root for the new Avalonia frontend replacement.

## Directory map

- `App/`: Avalonia startup, shell, module navigation, feature views.
- `Application/`: orchestration, config facade, session state machine, feature services.
- `CoreBridge/`: MaaCore bridge interface and stub implementation.
- `Platform/`: tray/notification/hotkey/autostart/file-dialog/overlay capability layer.
- `Compat/`: WPF baseline compatibility mapping and legacy configuration keys.
- `Tests/`: config import and contract tests.
- `CI/`: standalone CI workflow template.
- `Docs/`: parity matrix, migration, preview, rollback, and import report sample.

## Locked behavior

- Config write target: `config/avalonia.json`
- Auto import only when `avalonia.json` does not exist
- Import order: `gui.new.json` -> `gui.json` -> defaults
- Manual import supports: auto / gui.new only / gui only
- Legacy config files are read-only and never overwritten


**任务背景**
项目正在把 MaaWpfGui 的 Windows 前端迁移到 MAAUnified（Avalonia + Bridge）跨平台前端。当前框架、目录和部分页面已搭好，但大多数页面仍是“结构占位 + 轻功能”，离可发布状态还有明显距离。下一阶段不再只做 UI 复刻，而是要求 UI 与功能一体落地，形成可稳定运行、可测试、可维护的完整前端。

本轮统一目标是：

1. 以 WPF 现有行为为基线，完成 UI 布局、默认值、交互、配置语义、错误反馈的一致化迁移。所有功能、界面要求与 WPF 保持一致
2. 保持现有平台降级策略（fallback）不变，但必须做到“可见、可记录、可定位”。
3. 建立可持续交付能力：模块化任务拆分、可并行开发、统一验收口径、自动化门禁。

统一约束是：

1. 代码主战场在 src/MAAUnified/**。
2. 默认值和字段语义优先对齐 WPF。
3. 任意失败不应导致应用进程崩溃退出。
4. 需要覆盖 Light/Dark + 多语言。
5. 验收采用功能清单制，不依赖主观“看起来差不多”。

注意，会有多个同时做不同工作包的，可能你做着做着会发现不是你做的修改，不用管；理论上应当没有影响，但实际可能在测试的时候，别的正在做的内容的测试会超时、失败，这个也请忽略

# 工作包设计（开发使用）

## 工作包A 基线冻结与验收口径（5人天）

本包负责把“参考 WPF”变成可执行的工程清单。团队需要逐页梳理 WPF 页面的字段、默认值、交互、动态可见性、错误提示和配置键映射，并形成冻结版文档。
交付物是统一的“迁移基线清单”和“验收清单模板”，并明确哪些是本期必须项、哪些可延期。
完成后，后续开发包不再讨论“要不要做”，只讨论“怎么做”。

## 工作包B UI基础设施（主题/样式/多语言）（7人天）

### B1 主题 token 与控件样式基座（2人天）
具体要求：
1. 扩展 `App/Styles/ColorTokens.axaml` 与 `App/Styles/ControlStyles.axaml`，形成颜色、边框、间距、字号、状态色（warning/error/success）的统一 token，不允许核心页面新增硬编码颜色和尺寸。
2. 在 `App/App.axaml` 保持统一样式入口，禁止页面级重复 include 样式文件；样式变更必须由全局资源驱动。
3. 统一 `Border.section`、`TextBlock.section-title`、`Button.action` 的视觉规范，覆盖 MainWindow、TaskQueue、Settings、Copilot、Toolbox 五个主入口页面。

### B2 多语言资源与切换机制（3人天）
具体要求：
1. 统一多语言最小能力，优先收口 `TaskQueueLocalization` 与 `PlatformCapabilityTextMap`，覆盖 `zh-cn/zh-tw/en-us/ja-jp/ko-kr/pallas`。
2. `MainShellViewModel.SwitchLanguageCycle`、`TaskQueuePageViewModel.SetLanguage`、`SettingsPageViewModel.Language` 三处切换链路必须保持一致，切换后能力文本、任务文案、设置提示同步刷新。
3. 缺失翻译 key 时回退到英文或默认文案，同时写入 `UiDiagnosticsService` 事件日志，保证可定位。

### B3 主题与语言组合稳定性（2人天）
具体要求：
1. 以 `MainWindow.axaml`、`TaskQueueView.axaml`、`SettingsView.axaml` 为主验收面，执行 Light/Dark + 多语言组合检查，重点关注列表、Tab、Expander 和长文案截断。
2. 维持 `AvaloniaParityCoverageTests` 对视图存在性的覆盖，并新增组合 smoke 检查，防止样式改动导致布局回退。
3. 任何组合问题若暂缓，必须登记到基线 waiver 字段并提供替代验证路径。

## 工作包C Runtime与状态机主链路（7人天）

### C1 生命周期状态机收口（3人天）
具体要求：
1. 以 `UnifiedSessionService` + `SessionStateMachine` 为唯一状态源，统一 `Idle/Connecting/Connected/Running/Stopping` 转移规则。
2. `MainShellViewModel`、`TaskQueuePageViewModel`、`CopilotPageViewModel` 不再各自定义“运行中”语义，统一消费 Session 状态并映射到按钮可用态。
3. 对连接失败、启动失败、停止失败建立显式回退状态，确保不会出现“UI显示运行中但核心已停止”的分叉状态。

### C2 回调泵、取消与恢复（2人天）
具体要求：
1. 以 `UnifiedSessionService.StartCallbackPumpAsync` 为入口，固化 `TaskChainStart/TaskChainCompleted/TaskChainStopped/AllTasksCompleted/ConnectionInfo` 的状态映射。
2. 完整联通 `TaskQueuePageViewModel.WaitAndStopAsync` 与 `ConnectFeatureService.WaitAndStopAsync`，明确“等待停止”期间的禁用态和恢复态。
3. 回调 payload 解析失败不得抛出未处理异常，必须按 warning 处理并继续泵循环。

### C3 错误语义与诊断入口统一（2人天）
具体要求：
1. 统一使用 `UiOperationResult` 与 `UiErrorCode` 表达 UI 侧失败，禁止页面直接拼接不可检索错误字符串。
2. 通过 `PageViewModelBase.ApplyResultAsync` 把成功/失败统一写入 `UiDiagnosticsService`，覆盖 `RecordEventAsync`、`RecordFailedResultAsync`、`RecordErrorAsync`。
3. 每条失败链路必须带 scope（例如 `TaskQueue.Start`、`Settings.Autostart.Set`），保证日志可以按 scope + case id 回放。

## 工作包D 配置与迁移能力（7人天）

### D1 主配置读写与校验闭环（3人天）
具体要求：
1. `UnifiedConfigurationService.SaveAsync` 继续作为唯一配置写入口，所有模块禁止直接写 `config/avalonia.json`。
2. 保持 `CurrentValidationIssues` 与 `HasBlockingValidationIssues` 的同步更新，`MainShellViewModel` 与 `TaskQueuePageViewModel` 必须在阻断时禁用 Start。
3. `ConfigLoadResult` 必须在初始化时带回 validation issues，页面首屏可见阻断详情（scope/code/field/message/suggested action）。

### D2 Legacy 导入、冲突处理与报告（2人天）
具体要求：
1. 以 `ImportLegacyAsync` 为中心，固化 `Auto/GuiNewOnly/GuiOnly` 三种导入路径与 `gui.new -> gui -> defaults` 顺序。
2. 手动导入时必须备份 `avalonia.json.bak.*`，自动导入失败时也要写 `debug/config-import-report.json`。
3. `MainShellViewModel.ManualImportAsync` 成功后必须联动刷新 TaskQueue、Settings、连接共享状态，避免导入后 UI 仍显示旧值。

### D3 损坏修复与升级迁移（2人天）
具体要求：
1. 对 `avalonia.json` 解析失败场景保持“重建默认配置 + 记录 warning + 不中断进程”。
2. 对 schema 非最新版本的配置保留告警并给出迁移策略，不允许无提示 silent overwrite。
3. 补齐 `ConfigurationImportTests` 对损坏配置、不支持任务、手动导入备份、报告生成的回归覆盖。

## 工作包E 平台能力服务实现（8人天）

本包把平台接口从“占位”升级为“可执行能力”：托盘、通知、全局热键、自启动、Overlay。
对于不支持能力的平台，继续保留 fallback，但必须提供明确提示和日志事件。
本包完成后，平台能力应具备“可检测、可调用、可回报结果”的统一调用体验。

## 工作包F 主壳与全局导航联动（8人天）

### F1 主壳结构与状态面板（4人天）
具体要求：
1. 以 `MainWindow.axaml` + `MainShellViewModel` 为唯一主壳，收口标题区、全局状态、错误区、能力摘要区和根 Tab。
2. `ConfigIssueDetails` 阻断面板必须覆盖配置校验关键信息，且 `CanStartExecution` 与托盘 Start 菜单状态保持一致。
3. 主壳顶部“连接 + 导入 + 语言切换”动作必须统一触发相同服务层命令，不允许分叉入口行为不一致。

### F2 全局命令与托盘联动（4人天）
具体要求：
1. 托盘菜单动作统一通过 `MainWindow.axaml.cs` -> `MainShellViewModel` -> `PlatformCapabilityService` 链路执行。
2. `SyncTrayMenuStateAsync` 必须与 `TaskQueuePage.IsRunning`、配置阻断态联动，避免“不可执行操作仍可点击”。
3. `SetTrayVisibleAsync`、`ToggleOverlayFromTrayAsync`、语言循环切换都需要写诊断事件并提供 growl 回显。

## 工作包G 任务队列主流程（8人天）

### G1 队列操作与持久化一致性（4人天）
具体要求：
1. 以 `TaskQueuePageViewModel` + `TaskQueueFeatureService` 收口任务增删改、改名、移动、全选/反选，所有改动可通过 `SaveAsync` 与 `FlushTaskParamWritesAsync` 落盘。
2. 对 `SelectedTask` 切换中的 `BindSelectedTaskAsync` 加强并发保护，确保切换任务时不会丢失上一个任务的脏数据。
3. 任务状态色（`TaskQueueItemViewModel.StatusBrush`）必须与运行态回调一致，不出现“状态文本和颜色不一致”。

### G2 运行控制与回调回显（4人天）
具体要求：
1. 启停链路统一走 `SaveBoundTaskModulesAsync` -> `QueueEnabledTasksAsync` -> `ConnectFeatureService.StartAsync/StopAsync`。
2. 强化编辑态/运行态边界：`CanEdit`、`CanToggleRun` 与 UI 按钮禁用逻辑一致，运行中禁止破坏性编辑。
3. 完整消费 `TaskChainStart/SubTaskStart/SubTaskCompleted/TaskChainError/AllTasksCompleted`，保证状态、日志和后置动作执行一致。

## 工作包H 任务模块A（StartUp/Fight/Recruit）（9人天）

本包优先实现高频三模块，快速形成业务可用面。重点是参数模型、默认值、联动规则、校验逻辑、执行映射与配置回写。
需要确保参数在“修改-保存-重启-执行”链路中语义一致。
完成后，这三模块应可作为回归基准模块。

## 工作包I 任务模块B（Infrast/Mall/Award/PostAction）（8人天）

本包实现中频四模块，重点是策略参数和流程联动。
要求补齐运行态反馈、错误提示和配置持久化，确保模块组合执行时行为稳定。
完成后，任务编排应覆盖绝大多数日常使用组合。


## 工作包J 任务模块C（Roguelike/Reclamation/Custom）（9人天）

### J1 参数模型与 ViewModel 落地（3人天）
具体要求：
1. 将 `RoguelikeSettingsView.axaml`、`ReclamationSettingsView.axaml`、`CustomSettingsView.axaml` 从静态控件改为绑定式页面，引入对应 `App/ViewModels/TaskQueue/*ModuleViewModel.cs`。
2. 在 `Application/Models/TaskParams` 增加三模块 DTO，并在 `TaskParamCompiler` 中实现 `Read + Compile` 双向转换。
3. 默认值来源明确：先遵循 WPF 基线键语义，再由 DTO 默认值补齐，不允许“页面默认值和配置默认值不一致”。

### J2 校验、容错与配置污染防护（3人天）
具体要求：
1. 对三模块实现字段级校验（范围、依赖、互斥），校验信息进入 `ValidationMessages` 并区分阻断/预警。
2. 复杂输入解析失败时必须阻断保存且保留原配置，不允许写入半结构化脏数据。
3. 为高风险字段（例如 Roguelike 模式组合、Reclamation 回合上限、Custom 自定义规则）提供降级策略与错误提示。

### J3 执行映射与可扩展结构（3人天）
具体要求：
1. 新模块必须接入 `TaskQueuePageViewModel.BindSelectedTaskAsync` 与 `SaveBoundTaskModulesAsync` 主链，保证“选中-编辑-保存-执行”完整闭环。
2. `TaskQueueFeatureService.ValidateTaskAsync` 能返回三模块的编译结果与 issue 列表，供 UI 与 CI 共用。
3. 新增 `TaskModuleCFeatureTests`（或同等测试集）覆盖 DTO 读写、编译、阻断校验与回归路径。

## 工作包K 设置模块A（Connect/Game/Start/Performance/Timer）（9人天）

### K1 Connect/Game 链路闭环（3人天）
具体要求：
1. 以 `ConnectionGameSharedStateViewModel` 作为连接/游戏共享状态源，收口 `ConnectSettingsView` 与 `GameSettingsView` 的读写逻辑。
2. `SettingsPageViewModel.SaveConnectionGameSettingsAsync` 与 `MainShellViewModel.SyncConnectionToProfile` 保持字段一致（ConnectAddress/ConnectConfig/AdbPath/ClientType/StartGame/TouchMode/AutoDetect）。
3. 保存后必须可在 TaskQueue StartUp 模块即时反映，避免设置页和任务页状态不一致。

### K2 Start/Performance 即时与延迟生效边界（3人天）
具体要求：
1. 明确哪些字段保存后即时生效，哪些仅在下次执行或重启时生效，并在 UI 中给出标识。
2. 对性能相关数值输入增加范围约束与错误提示，禁止非法值进入配置。
3. 补齐设置保存后的回读验证，确保“写入成功但界面仍显示旧值”的问题可被自动化测试捕获。

### K3 Timer 参数建模与持久化（3人天）
具体要求：
1. 将 `TimerSlotViewModel` 从纯 UI 状态升级为可读写配置模型，支持 8 个定时槽的保存和恢复。
2. 统一时间格式校验（`HH:mm`）和边界行为，非法值不能进入调度链路。
3. 与运行链路联动：定时触发应能正确调起 Start/Stop 并写入诊断日志。

## 工作包L 设置模块B（GUI/Background/Hotkey/RemoteControl/ExternalNotification）（9人天）

### L1 GUI/Background 一致性与回读（3人天）
具体要求：
1. 固化 `SettingsPageViewModel.SaveGuiSettingsAsync` 这条主链，确保 `GUI.Localization/UseTray/MinimizeToTray/WindowTitleScrollable/Background*` 可写可读可回显。
2. 主题、语言、背景参数变更后，主壳与页面需要即时刷新，且重启后保持一致。
3. 对背景图路径不存在、透明度越界等异常输入给出前置校验和降级提示。

### L2 Hotkey 注册反馈闭环（3人天）
具体要求：
1. `RegisterHotkeysAsync` 必须覆盖 ShowGui/LinkStart 两个默认热键，注册结果写入设置状态区和诊断日志。
2. 对 `HotkeyConflict`、`HotkeyInvalidGesture`、`HotkeyNotFound` 等错误码提供明确文案映射，避免只显示通用失败。
3. 在 fallback 平台（如 `WindowScopedHotkeyService`）保持可见降级提示，满足“可见、可记录、可定位”。

### L3 RemoteControl/ExternalNotification 可验证化（3人天）
具体要求：
1. `RemoteControlSettingsView` 中连通测试按钮需真正调用 `RemoteControlFeatureService`，并把测试结果落日志。
2. `ExternalNotificationSettingsView` 与 `NotificationProviderFeatureService` 对接，至少支持 provider 列表展示、参数校验、测试发送。
3. 失败路径须区分参数错误、网络失败、平台不支持，并提供差异化反馈。

## 工作包M 设置模块C（VersionUpdate/IssueReport/Achievement/About/ConfigurationManager）（7人天）

### M1 VersionUpdate 与 ConfigurationManager 可用化（3人天）
具体要求：
1. 将 `VersionUpdateSettingsView` 从静态控件改为可执行入口，至少打通“检查更新/通道保存/代理保存”链路。
2. `ConfigurationManagerView` 需要接入真实配置 profile 管理（新增、删除、排序、切换），并与 `UnifiedConfigurationService` 同步。
3. 所有配置管理操作必须具备失败回滚策略，避免误删导致当前 profile 不可用。

### M2 IssueReport/Achievement/About 支持能力收口（4人天）
具体要求：
1. 以 `SettingsPageViewModel.BuildIssueReportAsync` + `UiDiagnosticsService.BuildIssueReportBundleAsync` 为主线，保证问题包包含 `avalonia.json`、导入报告、UI错误/事件/平台日志。
2. `IssueReportView` 的“打开 debug 目录/清理缓存”入口需要真实行为和失败提示，不保留纯按钮占位。
3. `AchievementSettingsView`、`AboutSettingsView` 至少完成数据来源与动作路由（例如弹窗、外链、版本信息展示）的可验证实现。

## 工作包N Copilot 全功能实现（9人天）

### N1 输入接入与数据校验（3人天）
具体要求：
1. 在 `CopilotPageViewModel` 强化 `ImportFromFileAsync` 与 `ImportFromClipboardAsync`：完成文件存在性、格式、字段完整性校验。
2. 导入失败必须给出可操作提示（缺字段、格式不合法、文件不存在），并写入 `Copilot.Import*` scope 日志。
3. 兼容常见输入类型（本地路径、剪贴板 JSON、空内容），不能因为异常输入导致列表状态错乱。

### N2 列表维护与反馈动作（3人天）
具体要求：
1. 完成新增、删除、清空、选中、排序（如需要）等列表维护能力，确保 `Items` 与持久化状态一致。
2. `SendLikeAsync` 需要绑定选中项并处理未选中场景，不允许无目标提交反馈。
3. 关键动作都需更新 `StatusMessage/LastErrorMessage` 并输出诊断事件，便于回归排查。

### N3 执行联动与运行态回显（3人天）
具体要求：
1. `Copilot.StartAsync/StopAsync` 与 Runtime 状态机联动，避免与 TaskQueue 并发时互相覆盖运行态。
2. 列表项状态（Running/Stopped/Success/Error）应由执行回调驱动，不使用一次性静态赋值。
3. 新增 Copilot 端到端集成测试，验证“导入-执行-停止-反馈”完整链路。

## 工作包O Toolbox 全功能实现（9人天）

### O1 统一执行协议与状态模型（3人天）
具体要求：
1. 在 `ToolboxFeatureService` 统一六工具执行协议（入参、返回、错误码、超时/取消），避免页面层散落判断。
2. `ToolboxPageViewModel.ExecuteCurrentToolAsync` 需要支持执行中/执行完成/执行失败的完整状态迁移。
3. 统一结果对象映射到 `ResultText` 与日志，保证同类错误在不同工具页表现一致。

### O2 六大工具页参数化（3人天）
具体要求：
1. 将 `ToolboxView.axaml` 六个 Tab 从示例按钮升级为 WPF 对齐的可配置工具页，保留每个工具的关键参数与结果区。
2. 每个工具补齐成功和失败两条可复现路径，并有明确结果展示。
3. 风险提示改为仅抽卡页生效，其他工具页不再受全局免责声明约束。

### O3 结果持久化与失败复盘（3人天）
具体要求：
1. 增加最近执行历史（工具名、时间、参数摘要、结果），支持基础复盘。
2. 执行失败写入 `Toolbox.*` scope 的错误日志，包含错误码与关键上下文。
3. 新增 Toolbox 回归测试，覆盖工具不支持、参数错误、免责声明未确认三类路径。

## 工作包P 对话框与高级模块实现（8人天）

### P1 7 个对话框契约化实现（4人天）
具体要求：
1. `Announcement/VersionUpdate/ProcessPicker/EmulatorPath/Error/AchievementList/TextDialog` 全部建立统一输入/输出模型，不再只保留静态文案。
2. 每个对话框明确确认/取消/关闭三种返回语义，并通过 `IDialogFeatureService` 记录关键事件。
3. 错误弹窗要能承接真实 `UiOperationResult`，支持复制错误与跳转 IssueReport 的联动。

### P2 高级模块页联调（4人天）
具体要求：
1. `Advanced/*View.axaml`（RemoteControlCenter/Overlay/TrayIntegration/StageManager/WebApi/ExternalNotificationProviders）从占位文本升级为真实能力入口。
2. 与 `IPlatformCapabilityService`、`IRemoteControlFeatureService`、`INotificationProviderFeatureService` 打通，至少提供查询、执行、结果回显三类能力。
3. 所有高级入口失败必须可见并可定位到 `debug/avalonia-ui-errors.log` 或 `debug/avalonia-platform-events.log`。

## 工作包Q 测试门禁与发布收口（7人天）

### Q1 自动化测试补齐（2人天）
具体要求：
1. 在现有 `PlatformCapabilityContractTests`、`ConfigurationImportTests`、`SessionStateSyncTests` 基础上补齐新增模块测试。
2. 对基线文件保持 `BaselineContract/Coverage/RenderSync` 三重校验，防止文档与机读基线漂移。
3. 新增测试必须能覆盖阻断级问题，不接受只测 happy path。

### Q2 CI 门禁固化（3人天）
具体要求：
1. 以 `.github/workflows/ci-avalonia.yml` 为主，固定 restore/test/publish/package 流程，失败即阻断合并。
2. Linux runner 至少执行全量 `MAAUnified.Tests`；Windows runner执行平台能力契约测试；macOS至少执行构建与打包验证。
3. 将基线文档一致性测试纳入默认门禁，确保基线变更必须通过评审和自动化校验。

### Q3 发布收口与文档同步（2人天）
具体要求：
1. 完成最终功能清单验收，逐项回填 ACC case 结果与证据路径。
2. 同步更新 `Docs/baseline.freeze.v1.md`、`Docs/acceptance.checklist.template.v1.md`、发布说明与回滚说明。
3. 形成固定发布清单（构建产物、配置兼容、fallback 验证、日志验证），保证后续版本可重复执行。

------

## 并行分配建议（给团队拆包时用）

1. 基建组先做 A-B-C-D。
2. 平台与壳层组并行做 E-F，主流程组做 G。
3. 业务组分三路并行做 H-I-J。
4. 设置组分三路并行做 K-L-M。
5. 高级能力组并行做 N-O-P。
6. 测试与发布组最后主导 Q，并参与各包中期回归。
