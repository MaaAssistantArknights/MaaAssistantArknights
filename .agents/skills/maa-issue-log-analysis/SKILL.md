---
name: maa-issue-log-analysis
description: 分析 MaaAssistantArknights 上游仓库公开 Issue（`https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/...` 或 `#1234`）。自动抓取 issue 正文和评论中的 `report_*.zip` 附件，优先读取 `debug/asst.log`、`debug/gui.log`、`config/gui.new.json`、`cache/resource/tasks.json`，并在有后续分卷时补看 `debug/interface/*.png`、`debug/drops/*.png`、`debug/infrast/**`、`debug/dumps/*` 等现场证据；结合 MAA Core/WPF/资源任务代码与文档判断根因、给出修复方案，供用户让你分析 MAA issue、日志包、ADB 连接失败、关卡导航、识别失败、任务出错、闪退时使用。内置的「诉求评估」方法论（三道筛：合理性 / 必要性 / 优先级）可脱离日志单独引用，用于评估 B 站评论、群聊反馈、纯文字 feature request 等无日志场景中的用户诉求。
---

# MAA Issue Log Analysis

## Required Reading

- 开始分析前，先读取同目录的 `KNOWLEDGE.md`，先用其中的通用误判规则校正自己的分析路径，再读 issue 和日志。
- 如果 issue 涉及会客室、线索、快捷按钮、批量按钮、自动领取/赠送/放置这类“会先改变界面状态再继续执行”的流程，必须先套用 `KNOWLEDGE.md` 中的 `Stateful UI Automation Checks` 与 `Reception Clue Analysis`。
- 如果用户没有贴出日志、报告包、报错文本、截图或导出诊断等有效证据，不要进入严肃分析；改用 `maa-cyber-fortune-master/SKILL.md` 引导补证据（详见 Scope 和 Workflow Step 2）。
- 本 skill 内置「诉求评估」方法论（Workflow Step 9 的三道筛：合理性 / 必要性 / 优先级）。即使没有日志、报告包、截图——例如面对 B 站评论、群聊反馈、纯文字 feature request——只要用户提出了明确的诉求，就可以单独引用这套框架做判断，不需要走完整 Workflow 或输出模板。

## Scope

- 仅用于上游公开仓库 `https://github.com/MaaAssistantArknights/MaaAssistantArknights`。
- 输入可以是完整 issue URL，或 `#1234` 形式的 issue 编号。
- 只分析公开 issue 中可直接访问的附件。
- 如果没有可下载的 `report_*.zip`，看用户是否提供了其他有效证据（报错文本、截图、导出诊断、清晰复现步骤）。
    - 有其他证据 → 基于现有证据给出初步判断，明确说明证据不足。
    - 无其他证据 → 转用 `maa-cyber-fortune-master/SKILL.md`，不要输出严肃分析模板。
- 如果评论里有机器人提示“日志没有上传成功”，不要直接放弃；正文里的附件链接仍可能可下载。

## Workflow

1. 规范化输入。

 - `#1234` 视为 `https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/1234`
 - 如果不是 `MaaAssistantArknights/MaaAssistantArknights`，停止并说明此 skill 不适用。

2. 先判断证据是否足够。

 - 如果用户只给一句模糊现象，且没有日志、报告包、截图、报错文本、导出诊断或清晰复现步骤，不进入本 skill 的严肃分析流程。
 - 此时改用 `maa-cyber-fortune-master/SKILL.md`，先用短小玄学回复活跃气氛，再把对话引导到补充日志、截图、报错或诊断信息。
 - 只有在用户已经提供可分析证据时，才继续下面的 issue / 日志分析步骤。

3. 获取 issue 内容。

 - 读取正文和评论。
 - 提取这些信息：UI/Core/Resource 版本、资源时间、模拟器类型、分辨率、截图增强、GPU 推理、任务名、关卡名、是否有 `-hard`、用户现象、复现步骤、维护者或机器人评论。
 - 不要把评论结论当成唯一证据；仍要用日志和代码自行验证。
 - 如果 issue 文本或评论里已经有人下了“这是游戏设计 / 不是 bug / 本来就这样”的结论，先暂存，不要直接复述成最终判断；先核对日志、资源任务和当前代码是否真的支持这个结论。

4. 提取报告附件。

 - 关注 `report_*.zip`。
 - 附件可能同时出现在正文和评论。
 - 按 `report_MM-dd_HH-mm-ss` 分组，同一时间戳下的 `part01`、`part02`、`part03` 是独立 zip，不是需要先拼接的分卷压缩包。

5. 先看 `part01`，再决定是否看 `part02+`。

 - 根据 WPF 打包逻辑，`part01` 一定优先，通常包含：
 - `debug/asst.log`
 - `debug/asst.bak.log`
 - `debug/gui.log`
 - `debug/gui.bak.log`
 - `config/*`
 - `cache/*`
 - `resource/*_custom.*`
 - `part02+` 只包含 `debug` 子目录中最近 3 天修改过的文件，可能是：
 - `debug/interface/*.png`
 - `debug/drops/*.png`
 - `debug/infrast/**`
 - `debug/roguelike/**`
 - `debug/dumps/*`
 - `part02+` 可能是空包，也可能只包含图片；不要默认里面一定有文本日志。

6. 建立时间线。

 - 先用 `gui.log` 找用户点击、所选关卡、任务链开始、报错时间。
 - 再用 `asst.log` 还原底层行为。
 - 关卡或任务问题时，优先用 `gui.log` 中的 `Start Task Chain`、`GetFightStage`、`任务出错` 锁定时间窗，再回到 `asst.log` 里的 `taskid`、`SubTaskError`、`TaskChainError`。
 - 连接问题时，优先把 `gui.log` 中的重试流程和 `asst.log` 中的 `adb devices`、`adb connect`、`ConnectionInfo` 串起来。
 - 如果 `ConnectConfig` 是 `PC`，改走 `AttachWindow` / `Win32Controller` 这条线：
     - 先在 `gui.log` 确认 `AttachWindow: Found window`
     - 再在 `asst.log` 里看 `Win32Controller::screencap`、`Win32Controller::click`
     - 不要再按 ADB 端口或 `ConnectionInfo.ConnectFailed` 的思路分析
 - 如果问题属于状态型 UI 自动化（例如会客室线索、批量按钮、快捷按钮、先拆后放一类流程），时间线里必须单独标出：
     - 自动化在什么时刻先修改了用户原状态
     - 后续进入下一步或恢复终态由哪个条件控制
     - 条件不满足时流程是停止、跳过，还是按设计停在别的状态

7. 区分 issue 当时环境和当前分支。

 - 先以报告包中的 `config/` 与 `cache/resource/` 还原用户当时实际运行的配置和资源。
 - 再对照当前仓库代码，判断该问题是当前仍存在，还是当时存在但现在已修复。
 - 输出给用户时，如果提到任务名、设置项、按钮名、错误提示或日志前缀，先到 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 查中文文案，不要直接把 `LocalizationHelper.GetString("Key")` 里的 `Key`、`DynamicResource` key、`TaskChain` 名或枚举名当成最终展示文本。

## Report Map

### `debug/asst.log`

- 模块归属：MAA Core 运行时。
- 主要内容：ADB 命令、连接回调、识别、Pipeline、关卡导航、截图保存路径、C++ 源文件和函数名。
- 最适合看：
 - ADB 连接问题
 - 关卡导航问题
 - `SubTaskError` / `TaskChainError`
 - OCR / 模板 / 点击失败
- 对根因判断最权威。

### `debug/asst.bak.log`

- 模块归属：上一轮 Core 滚动日志。
- 最适合看：
 - 最新一次复现不在 `asst.log`
 - 需要对比前一次成功 / 失败

### `debug/gui.log`

- 模块归属：WPF GUI / AsstProxy / TaskQueue。
- 主要内容：程序版本、资源加载、所选任务与关卡、用户可见报错、连接重试流程、`Start Task Chain`。
- 最适合看：
 - 建时间线
 - 用户到底选了什么
 - GUI 是否自动做了“断开重连 / 重启 ADB / 强杀 ADB”
- 这是最快的入口，但不是所有问题的最终根因。

### `debug/gui.bak.log`

- 模块归属：上一轮 GUI 滚动日志。
- 最适合看：
 - 程序重启前的上下文
 - 更早一次复现

### `config/gui.new.json`、备份文件

- 模块归属：GUI 配置快照（dev-v2 起 `gui.new.json` 为新配置系统主文件；旧 `gui.json` 已废弃，仅作为迁移残留或旧版本回退时可能存在）。
- 常见文件：
 - `config/gui.new.json`（主）
 - `config/gui.new.json.bak`（每次启动自动备份）
 - `config/gui.new.json.err`（解析失败时保存的损坏配置）
 - `config/gui.json.old`（旧系统迁移前的配置备份，仅迁移时产生一次；issue 用户版本较旧时才需要看）
- 最适合看：
 - 实际连接配置
 - 模拟器路径、ADB 地址、是否开启截图增强
 - 任务队列、`StagePlan`
 - 是否真的选择了 `15-13-hard` 之类的硬难度关卡
- 注意：
 - `gui.new.json` 与 `gui.log` / `asst.log` 的实际运行状态冲突时，先检查 `gui.new.json.bak`。
 - 如果 issue 用户版本较旧（迁移前），可能仍使用旧 `gui.json`，此时 `gui.json` 和 `gui.json.old` 才有参考价值。
 - 报告导出时用户可能已经改过勾选项，当前文件不一定就是复现时那一份。

### `cache/resource/tasks.json` 和 `cache/resource/tasks/tasks.json`

- 模块归属：issue 当时使用的缓存资源。
- 最适合看：
 - 用户当时到底跑的是哪一版资源定义
 - 当前分支资源和 issue 当时资源是否不同
- 如果 issue 版本较旧，先信报告包里的 `cache/resource`，再用当前仓库代码判断是否已修复。

### `cache/gui/StageActivity.json` / `StageActivityV2.json`

- 模块归属：GUI 关卡活动缓存。
- 最适合看：
 - 活动 / 章节 / 关卡可用性
 - 关卡显示和导航问题

### `debug/interface/*.png`

- 模块归属：界面失败现场图。
- 最适合看：
 - 关卡导航失败
 - 识别错画面
 - 按钮没出现、位置不对、被别的界面覆盖
- 如果 `asst.log` 有 `Save image ... debug/interface/...`，但上传包没有这张图，要明确说明“日志表明现场图存在，但用户未上传对应分卷”。

### `debug/drops/*.png`

- 模块归属：掉落识别现场图。
- 最适合看：
 - 结算页与掉落识别问题

### `debug/infrast/**`、`debug/roguelike/**`

- 模块归属：任务特定调试图。
- 最适合看：
 - 基建换班
 - 肉鸽识别或路径问题

### `debug/dumps/*`

- 模块归属：崩溃转储副本。
- 最适合看：
 - 闪退、崩溃
- issue 模板还可能要求额外上传 `MAA.exe.dmp`，如果有，也要一起分析。

## How To Filter Evidence

1. 先从 issue 文本拿到这几个锚点：

 - 版本与资源时间
 - 模拟器品牌、分辨率、截图增强、GPU 推理
 - 任务名 / 关卡名 / 是否有 `-hard`
 - 报告时间戳，例如 `report_03-15_11-07-05`
 - 如果日志流程和当前主线代码不一致，先确认用户版本，必要时切到对应 tag（例如 `git checkout vXXX`）复核旧逻辑

2. 再从 `gui.log` 找这几类高价值信号：

 - `正在连接模拟器`
 - `Already connected`
 - `GetFightStage`
 - `Start Task Chain`
 - `任务出错`
 - `连接失败`

3. 再到 `asst.log` 找底层证据：

 - `ConnectionInfo`
 - `ConnectFailed`
 - `TaskChainError`
 - `SubTaskError`
 - `to_be_recognized`
 - `cur_retry`
 - `Save image`
 - `offline`
 - `unauthorized`
 - `failed to connect`
 - `cannot connect`

4. 对连接问题，重点看：

 - `adb.exe devices` 里有没有 `offline`
 - `adb.exe connect` 是否报 `10061`
 - `ConnectionInfo.what` / `why`
 - `config/gui.new.json` 中的：
     - `ConnectConfig`
     - `Connect.Address`
     - `Connect.AllowADBRestart`
     - `Connect.AllowADBHardRestart`
     - `Connect.MuMu12Extras.Enabled`
 - 默认 MuMu 12 端口列表是否和日志中的轮询顺序一致

5. 对 PC / AttachWindow 问题，重点看：

 - `config/gui.new.json` 中 `Connect.ConnectConfig == "PC"`
 - `gui.log` 中：
     - `连接 PC 端（实验性功能，稳定性无法保证）`
     - `AttachWindow: Found window`
     - `handle: ..., hwnd: ..., screencapMethod: ..., mouseMethod: ..., keyboardMethod: ...`
 - `asst.log` 中：
     - `Win32Controller::screencap`
     - `Win32Controller::click`
 - 点击后的下一次识别结果是否真的改变
 - 如果点击日志存在，但后续截图和 OCR 状态完全没变，要优先判断为“输入未生效”，而不是“流程已正确前进”

6. 对关卡导航 / 磨难切换问题，重点看：

 - `config/gui.new.json` 中的 `StagePlan`
 - `gui.log` 中的 `GetFightStage`
 - `asst.log` 中的：
     - `Episode15`
     - `ChapterDifficultyHard`
     - `EnterChapterDifficultyHard`
     - `SubTaskError`
 - `debug/interface/*.png`
 - `resource/tasks/tasks.json` 与 `cache/resource/tasks*.json`

7. 回答时只保留关键证据。

 - 摘几十行足够支撑结论的片段即可。
 - 不要把整份日志倾倒进回复。

8. 对状态型 UI 问题，结论前先做一次“设计一致性检查”。

 - 先判断日志中的状态变化是否符合游戏规则、资源任务定义和当前实现。
 - 如果流程与设计一致，不要把用户不喜欢的中间状态直接归为 bug。
 - 只有当日志、资源任务和代码彼此冲突，或流程没有达到设计要求的终态时，再归类为实现缺陷。

9. 对用户诉求做合理性 / 必要性 / 优先级判断，不要默认认同。

 - issue 用户通常会带一个隐含或明确的期望："应该支持 X""应该改成 Y"。分析完根因后，对这个期望本身单独评估，不要因为它写在 issue 里就当成既定需求。
 - 判断前先搜索仓库中是否有相关 / 重复 issue：用 GitHub Issue Search 按关键词、标题或描述搜同类诉求。如果有类似 issue，先看维护者的回复和最终状态（已关闭 / 已实现 / 已拒绝 / Won't fix / 标签），再结合本次日志和代码做独立判断。不要直接复述旧 issue 的结论，但维护者对同类诉求的一贯态度是重要参考。
 - 搜到的同类 issue 要逐个核对实际内容再引用：确认主题、版本、时间与当前 issue 是同一场景；不同主题、所涉功能合入之前、或已按修 bug 路径处理的 issue，不能用来论证当前诉求。
 - 三道筛有前置的事实核查义务，任何一条没做完就不允许下结论（误判案例见 `KNOWLEDGE.md` 的 Common Pitfalls）：
     - **代码考古**：引用被注释 / 禁用 / 绕过的代码前，先 `git log -S <片段>` / `git blame` 查来历；禁止把 ｢取消注释 / 恢复被禁代码｣ 直接当修复建议。准备提出的方案若与近期提交确立的设计意图相反，必须放弃该方案并写明冲突。
     - **时间线定位**：诉求涉及 “新肉鸽 / 新主题 / 新活动 / 新功能适配不好” 时，先 `git log` 查该功能适配的合入时间；合入数周内的问题属于适配期阵痛，按 bug 修复路径评估，不得用 ｢高频真实痛点｣ 论证通用兜底机制的优先级。
     - **已有机制排查**：在断言 ｢当前没有 X｣ 或 “用户唯一替代方案是 Y” 之前，必须先在实现层搜索同类机制（同目录插件、同主题资源任务、失败处置 / 恢复出口），并读过所引文件的实际内容。
 - 先区分"表面诉求"和"根本需求"：issue 用户经常提的是"解决方案"（应该加 X 功能、应该改成 Y 行为），而不是需求本身。先从现象和上下文还原用户的根本需求（用户到底想达成什么目的），再对根本需求做下面的三筛。例如用户说"希望能支持多开"，但根本需求可能是"想同时挂两个账号"——而 MAA 设计上只针对单账号，多开超出范围；这种情况下根本需求本身就不合理，不需要进一步判断。再例如用户说"希望能自定义基建换班的干员排列顺序"，但根本需求可能是"想优先让某些干员上班"——而现有的自定义基建换班（`resource/custom_infrast/`）已经能实现，属于已有替代方案，必要性低。
 - 三道筛：
     1. **合理性**：该诉求是否符合 MAA 的设计目标和定位？是否与已有功能、文档说明或已知约束冲突？如果它实质上是在要求 MAA 做它本来就不打算做的事（例如多开、多账号、PC 实验性功能全量维护、违背游戏机制的操作），直接标记为超出范围或不合理。
     2. **必要性**：这个诉求对应的是真实缺陷，还是只是个人偏好 / 边缘场景 / 可以用现有功能绕过的？如果不实现也不会影响正确性和核心流程，必要性低。
     3. **优先级**：相对其他 issue，这个诉求影响面有多大？是高频路径还是极少数用户的偶发场景？维护成本和收益是否匹配？只发生在单一环境、单一版本、且已有替代方案的，优先级低。
 - 判断后的措辞要诚实，不要为了迎合用户而模糊结论：
     - 不合理 → 写明“该诉求与 MAA 当前设计/定位不符”，给出理由，不要留“可以考虑”这种暧昧余地。
     - 不必要 → 写明“当前已有替代方案 / 不影响正确性”，不建议为此投入。
     - 低优先级 → 写明影响面和成本，标为“可延后”，不要和真正的缺陷并列。
 - 只有当诉求通过三筛（合理、必要、有影响面）时，才在“修复方案”里给出具体实现建议。
 - 修复方案的层级必须与仓库惯例一致：恢复 / 兜底 / 重试逻辑在 Core 层已有的失败处置框架上扩展，GUI 层只做展示与配置（惯例出处见 `KNOWLEDGE.md` 的 Common Pitfalls）。

## Correlating With Code

### 报告打包结构

- `src/MaaWpfGui/ViewModels/UserControl/Settings/IssueReportUserControlModel.cs`

### GUI / Core 日志文件名

- `src/MaaWpfGui/Main/Bootstrapper.cs`
- `src/MaaCore/Utils/Logger.hpp`
- `src/MaaUtils/include/MaaUtils/Logger.h`

### 连接回调与 GUI 侧重试

- `src/MaaWpfGui/Main/AsstProxy.cs`
- `src/MaaWpfGui/ViewModels/UI/TaskQueueViewModel.cs`
- `src/MaaWpfGui/ViewModels/UserControl/Settings/ConnectSettingsUserControlModel.cs`
- `src/MaaCore/Controller/MinitouchController.cpp`
- `docs/zh-cn/protocol/callback-schema.md`
- `docs/zh-cn/manual/device/android.md`

### PC / AttachWindow / Win32 输入

- `src/MaaWpfGui/ViewModels/UserControl/Settings/ConnectSettingsUserControlModel.cs`
- `src/MaaWpfGui/Main/AsstProxy.cs`

### 关卡导航与磨难切换

- `resource/tasks/tasks.json`

### 理智药 / 临期药

- `src/MaaWpfGui/ViewModels/UserControl/TaskQueue/FightSettingsUserControlModel.cs`
- `src/MaaWpfGui/Models/AsstTasks/AsstFightTask.cs`
- `src/MaaCore/Task/Interface/FightTask.cpp`
- `src/MaaCore/Task/Fight/MedicineCounterTaskPlugin.cpp`
- `resource/tasks/tasks.json`

### 现场图保存

- `src/MaaCore/Utils/DebugImageHelper.hpp`

### GUI 中文文案

- `src/MaaWpfGui/Res/Localizations/zh-cn.xaml`

## Localized Copy

- 总结任务类型、设置项、按钮、错误提示、日志前缀时，优先使用 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 中的中文文案。
- 常见查找顺序：
    - 默认任务类型名：先看 `src/MaaWpfGui/ViewModels/UI/TaskQueueViewModel.cs` 中 `LocalizationHelper.GetString(taskType.ToString())` 的 key，再到 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 查 `StartUp`、`Fight`、`Infrast`、`Recruit`、`Mall`、`Award`、`Roguelike`、`Reclamation`、`Custom`。
    - 任务开始 / 完成 / 出错等 GUI 日志前缀：优先查 `StartTask`、`CompleteTask`、`TaskError`、`ConnectFailed`、`TryToReconnect` 等 key。
    - 设置项、按钮、界面提示：先在对应 `*.xaml` / `*.cs` 里找 `DynamicResource SomeKey` 或 `LocalizationHelper.GetString("SomeKey")`，再到 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 查中文。
    - issue 反馈相关入口：优先查 `Issue`、`GenerateSupportPayload`、`OpenDebugFolder` 等 key。
- 如果 `config/gui.new.json` 里任务有用户自定义 `Name`，输出时优先保留用户自定义名称；必要时再括号补默认任务类型中文，例如 `刷理智（理智作战 / Fight）`。
- 输出时优先写中文，必要时在括号里补原始 key / `taskChain` / 枚举名，例如 `基建换班（Infrast）`。
- 如果 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 没有对应 key，再退回原始 key 或代码里的英文字符串，并明确说明“未在 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 找到对应文案”。

## Linking Code Evidence

- 如果要指向具体代码行，不要写本地路径加行号，也不要写绝对路径。
- 统一给出对应仓库的远端 GitHub `blob` 行号链接，用尖括号包裹。
- MaaAssistantArknights 仓库链接格式：
    - `https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/<commit>/<path>#L14-L20`
- `<commit>` 必须是本次分析实际依据的代码版本：
    - 默认使用当前检出的 `HEAD`
    - 如果为了复核旧 issue 切到了某个 tag / commit，就使用那个版本解析后的 SHA
- 例子：
    - <https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/f8b64ef908d8b82bb71ba753b69a30ea658f9054/src/MaaWpfGui/Main/AsstProxy.cs#L1072-L1079>
    - <https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/f8b64ef908d8b82bb71ba753b69a30ea658f9054/src/MaaWpfGui/Res/Localizations/zh-cn.xaml#L680-L695>
- 如果引用的是其他上游仓库或文档，也用对应远端链接，不要给本地文件行号。
- 引用行号前必须读过所引行号的实际内容，确认逻辑确实在该处；不要只凭函数名或搜索摘要拼行号，机制描述与行号错位会把后续修复者带偏。

## Example Heuristic

如果 issue 像 `#16014` 一样是 MuMu ADB 连接随机失败，并且同时出现：

- `config/gui.new.json` 里 `ConnectConfig` 是 `MuMuEmulator12`
- 地址是 `127.0.0.1:16384`
- `gui.log` 在复现时段从 `16384` 轮询到 `16576`
- `asst.log` 里 `adb devices` 返回 `127.0.0.1:16384 offline`
- `asst.log` 里对备选端口出现 `cannot connect ... (10061)`
- `ConnectionInfo.what` 是 `ConnectFailed`

那么根因更可能是 ADB / 模拟器连接层异常，而不是 GUI 任务队列逻辑；此时应结合 MuMu 默认端口表和 Android `offline` 文档给出建议。

如果 issue 像 `#16002` 一样是 15/16 章 hard 难度切换失败，并且同时出现：

- `config/gui.new.json` 或 `gui.log` 里关卡是 `15-13-hard`
- `asst.log` 里先进入 `Episode15`
- 随后卡在 `ChapterDifficultyHard`
- OCR 反复识别到 `推演计分` 等无关文本
- 最后 `SubTaskError`，并保存 `debug/interface/*.png`
- `resource/tasks/tasks.json` 里 `EnterChapterDifficultyHard` 期望在固定 ROI 识别到 `进入作战`

那么根因更可能是当前画面没有进入预期的难度切换界面，或按钮没有出现在资源定义的期望区域，而不是“Hard 后缀本身没有传到 core”。

## Output Format

先做输出分流，不要无论什么情况都套完整模板。

### 分流规则

- 如果用户没有提供有效证据，只给出一句模糊现象，或只有 issue 文本但没有日志、报告包、截图、报错文本、导出诊断、清晰复现步骤，那么不要输出下面那套完整分析模板。
- 这类场景直接改用 `maa-cyber-fortune-master/SKILL.md` 的风格回复，并把“赛博算一卦”放在开头。
- 此时输出必须足够短，通常 2 到 4 句即可。
- 这类短回复里不要再展开：
 - `Issue 概要`
 - `关键证据`
 - `根因判断`
 - `修复方案`
 - `English translation`
 - “点击此处展开”这类折叠块
- 本质要求是：先用玄学接住气氛，再用一句话明确要求补日志、截图、报错或诊断信息。

### 无有效证据时的推荐格式

```markdown
[一句简短复述用户现象]

[赛博算一卦 / 掐指一算 / 夜观天象开场]
[1 到 2 句短小玄学分析]

[一句自然收束到补日志、截图、报错或导出诊断]
```

### 无有效证据时的示例

```markdown
你这个是刷界园时，第一层商店点了招募券就直接结束。

赛博算一卦，界园属木，招募券属火，如今商店财位一震，像是招募灵脉和界园卦象临时撞了车，程序当场收摊回府。

不过这卦现在只有天象，没有脉案。建议补一份复现当次的日志或报错截图，不然贫道也只能隔着网线观星象。
```

### 有有效证据时

最终回答再用这个完整结构：

```markdown
## Issue 概要

- issue：`#1234`
- 版本 / 资源时间：
- 模拟器 / 连接配置 / 任务：优先写 `zh-cn` 中文任务名；如果日志里是用户自定义任务名，先写自定义名，再补默认任务类型中文 / key
- 相关设置项 / 关键提示文案：优先写 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 中的中文文案
- 用户现象：

## 关键证据

<details><summary>点击此处展开</summary>

- `debug/gui.log`：
- `debug/asst.log`：
- `config/gui.new.json`：
- `cache/resource` / `cache/gui`：
- `debug/interface` / `debug/drops`：
- 代码依据：如需指向具体实现，直接附远端 GitHub 行号链接

</details>

## 根因判断

- 直接结论：
- 证据链：

## 诉求评估

- 用户表面诉求（一句话）：
- 根本需求（用户真正想达成什么）：
- 合理性：合理 / 不合理（写明与哪个设计目标、文档说明或已知约束冲突）
- 必要性：必要 / 不必要（写明是否已有替代方案、是否仅个人偏好）
- 优先级：高 / 中 / 低 / 可延后（写明影响面和维护成本）
- 结论：采纳 / 部分采纳 / 不采纳（不采纳时给出一句话理由）

## 给用户的建议

- 用户现在可以直接尝试的动作：
- 是否建议升级 / 重下完整包 / 同步资源 / 重置配置：
- 是否需要等待开发者修复：
- 是否有临时绕过方案：

## 修复方案

1. 代码 / 资源 / 配置层修复
2. 需要补充的日志或截图
3. 需要补充的测试

## 给修复 AI 的建议（可复制）

<details><summary>点击此处展开</summary>

~~~text
现象：
[一句话描述用户可见的问题]

关键证据：
[粘贴原始日志、堆栈、监控截图中的关键文本]

可能相关线索（待验证）：
[根据日志/现象推测的可能方向，不保证准确，供参考]
~~~

</details>

## 置信度

- 高 / 中 / 低
- 还缺什么证据

## English translation

<details><summary>Click here to expand</summary>

Translate the complete conclusion directly into English and paste it here. Note that the English text is in `src/MaaWpfGui/Res/Localizations/en-us.xaml`.

</details>

```

## Reminders

- 无有效证据时不要套完整模板，改用赛博算卦并直接收束到"请补证据"（详见 Output Format 分流规则）。
- 不要只看 `gui.log` 下结论。
- 不要把 issue 评论或机器人提示当成唯一证据。
- 不要把当前分支资源直接当成 issue 当时的真实环境；先看报告包里的 `cache/resource`。
- 日志和截图冲突时，优先相信现场图，再回头解释 OCR / 模板为何误判。
- 如果问题本身没有在当前日志中复现，要明确写“证据未复现”，不要硬凑结论。
- 如果 issue 版本很旧或用户日志与当前代码不一致，先按用户版本 tag 复核，再判断是否已修复（详见 `KNOWLEDGE.md` 的 `版本差异` 节）。
- 回答中出现任务名、设置项、按钮名、提示文案时，优先使用 `zh-cn.xaml` 的中文文案（详见 Localized Copy）。
- 引用具体代码行时给远端 GitHub `blob` 行号链接，不给本地路径加行号（详见 Linking Code Evidence）。
- 如果证据表明问题已在新版本修复，明确建议用户升级；如果怀疑安装包、资源文件或配置损坏，明确建议重新下载或重建；如果判断为真实代码缺陷且暂无 workaround，明确建议等待开发者修复。
- 对用户诉求做独立评估，不要因为有 issue 就默认认同；判断前先搜同类 issue 看维护者一贯态度；不合理、不必要或低优先级的诉求要诚实写明理由，只有通过三筛的才给修复方案。
- 三道筛下结论前先完成 Workflow Step 9 的事实核查（代码考古、时间线定位、已有机制排查、同类 issue 核对）。
```
