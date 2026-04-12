---
name: maa-issue-log-analysis
description: 分析 MaaAssistantArknights 上游仓库公开 Issue（`https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/...` 或 `#1234`）。自动抓取 issue 正文和评论中的 `report_*.zip` 附件，优先读取 `debug/asst.log`、`debug/gui.log`、`config/gui.json` / `config/gui.new.json`、`cache/resource/tasks.json`，并在有后续分卷时补看 `debug/interface/*.png`、`debug/drops/*.png`、`debug/infrast/**`、`debug/dumps/*` 等现场证据；结合 MAA Core/WPF/资源任务代码与文档判断根因、给出修复方案，供用户让你分析 MAA issue、日志包、ADB 连接失败、关卡导航、识别失败、任务出错、闪退时使用。
---

# MAA Issue Log Analysis

## Scope

- 仅用于上游公开仓库 `https://github.com/MaaAssistantArknights/MaaAssistantArknights`。
- 输入可以是完整 issue URL，或 `#1234` 形式的 issue 编号。
- 只分析公开 issue 中可直接访问的附件。
- 如果没有可下载的 `report_*.zip`，先明确说明证据不足，再尽量基于 issue 文本、截图、代码和文档给出初步判断。
- 如果评论里有机器人提示“日志没有上传成功”，不要直接放弃；正文里的附件链接仍可能可下载。

## Workflow

1. 规范化输入。

 - `#1234` 视为 `https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/1234`
 - 如果不是 `MaaAssistantArknights/MaaAssistantArknights`，停止并说明此 skill 不适用。

2. 获取 issue 内容。

 - 读取正文和评论。
 - 提取这些信息：UI/Core/Resource 版本、资源时间、模拟器类型、分辨率、截图增强、GPU 推理、任务名、关卡名、是否有 `-hard`、用户现象、复现步骤、维护者或机器人评论。
 - 不要把评论结论当成唯一证据；仍要用日志和代码自行验证。

3. 提取报告附件。

 - 关注 `report_*.zip`。
 - 附件可能同时出现在正文和评论。
 - 按 `report_MM-dd_HH-mm-ss` 分组，同一时间戳下的 `part01`、`part02`、`part03` 是独立 zip，不是需要先拼接的分卷压缩包。

4. 先看 `part01`，再决定是否看 `part02+`。

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

5. 建立时间线。

 - 先用 `gui.log` 找用户点击、所选关卡、任务链开始、报错时间。
 - 再用 `asst.log` 还原底层行为。
 - 关卡或任务问题时，优先用 `gui.log` 中的 `Start Task Chain`、`GetFightStage`、`任务出错` 锁定时间窗，再回到 `asst.log` 里的 `taskid`、`SubTaskError`、`TaskChainError`。
 - 连接问题时，优先把 `gui.log` 中的重试流程和 `asst.log` 中的 `adb devices`、`adb connect`、`ConnectionInfo` 串起来。
- 如果 `ConnectConfig` 是 `PC`，改走 `AttachWindow` / `Win32Controller` 这条线：
- 先在 `gui.log` 确认 `AttachWindow: Found window`
- 再在 `asst.log` 里看 `Win32Controller::screencap`、`Win32Controller::click`
- 不要再按 ADB 端口或 `ConnectionInfo.ConnectFailed` 的思路分析

6. 区分 issue 当时环境和当前分支。

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

### `config/gui.json`、`config/gui.new.json`、备份文件

- 模块归属：GUI 配置快照。
- 常见文件：
 - `config/gui.json`
 - `config/gui.new.json`
 - `config/gui.json.old`
 - `config/gui.json.bak`
- 最适合看：
 - 实际连接配置
 - 模拟器路径、ADB 地址、是否开启截图增强
 - 任务队列、`StagePlan`
 - 是否真的选择了 `15-13-hard` 之类的硬难度关卡
- 注意：
 - `gui.new.json` 可能比 `gui.json` 更接近用户当前界面上的任务配置，不能只看一个文件。
- 如果 `gui.new.json` 与 `gui.log` / `asst.log` 的实际运行状态冲突，继续检查：
- `gui.new.json.bak`
- `gui.json.old`
- `gui.json.bak`
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
 - `config/gui.json` 中的：
 - `ConnectConfig`
 - `Connect.Address`
 - `Connect.AllowADBRestart`
 - `Connect.AllowADBHardRestart`
 - `Connect.MuMu12Extras.Enabled`
 - 默认 MuMu 12 端口列表是否和日志中的轮询顺序一致

5. 对 PC / AttachWindow 问题，重点看：

 - `config/gui.json` 中 `Connect.ConnectConfig == "PC"`
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

 - `config/gui.new.json` / `gui.json` 中的 `StagePlan`
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

## Common Patterns

- `gui.log` 只显示“连接失败”，但 `asst.log` 里已经给出 `adb devices`、`adb connect`、端口轮询和 `ConnectionInfo`。连接类问题必须以 `asst.log` 为准。
- `adb devices` 显示目标地址 `offline`，随后 MuMu 备选端口都 `10061`，通常更像模拟器 / ADB 状态异常，或自动探测到的端口不可达，而不是任务逻辑问题。
- `gui.log` 显示选中的关卡是 `15-13-hard` 一类 hard 代码，而 `asst.log` 长时间卡在 `ChapterDifficultyHard`，OCR 却反复识别到和按钮无关的文字，通常说明当前画面没有进入预期的难度切换界面。
- `asst.log` 明确写了 `Save image` 到 `debug/interface/*.png` 或 `debug/drops/*.png`，但上传包没有相应分卷时，要把“缺失的现场证据”单独写出来。
- `part02` 可以是空包，也可以只包含图片；不要因为没有文本日志就把它判成“无用分卷”。
- issue 机器人评论“日志没有上传成功”时，不要自动当真；先验证正文附件是否仍可下载。
- 如果 `gui.log` 说“任务出错”，但对应 `taskid` 的 `asst.log` 实际 `AllTasksCompleted`，要明确写“本次日志未复现用户描述的问题”。
- 用户日志里的任务流程与当前主线代码明显不一致，且当前代码看起来已经修掉了该问题：

    - 先确认用户版本，必要时切到对应 tag（例如 `git checkout vXXX`）核对旧逻辑。
    - 不要用当前分支否定旧日志；旧版本问题可能真实存在。
    - 如果主线已修复，再看修复 commit 是否已进入 tag / release：已发版建议升级，未发版建议等待 release。
- `gui.new.json`、`gui.json` 和实际日志不一致时，不要急着判“用户配置写错了”；先看 `gui.new.json.bak` 和 `gui.json.old`，尤其是用户复现后又改回开关的场景。
- 在 `ConnectConfig=PC` 的 issue 里，`Win32Controller::click` 正常返回不代表点击真的生效；要看点击后的下一帧中，按钮状态、数量 OCR、场景识别有没有变化。
- `gui.log` 中“已使用 48 小时内过期的理智药”这类高层提示，不一定等价于底层逐药 OCR 结论；如果 `asst.log` 明确识别到 `9天`、`NotExpiring` 等相反证据，应优先相信 `asst.log`。

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
- 如果 `config/gui*.json` 里任务有用户自定义 `Name`，输出时优先保留用户自定义名称；必要时再括号补默认任务类型中文，例如 `刷理智（理智作战 / Fight）`。
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

## Example Heuristic

如果 issue 像 `#16014` 一样是 MuMu ADB 连接随机失败，并且同时出现：

- `config/gui.json` 里 `ConnectConfig` 是 `MuMuEmulator12`
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

最终回答用这个结构：

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
- `config/gui.json` / `config/gui.new.json`：
- `cache/resource` / `cache/gui`：
- `debug/interface` / `debug/drops`：
- 代码依据：如需指向具体实现，直接附远端 GitHub 行号链接

</details>

## 根因判断

- 直接结论：
- 证据链：

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

- 不要只看 `gui.log` 下结论。
- 不要把 issue 评论或机器人提示当成唯一证据。
- 不要把当前分支资源直接当成 issue 当时的真实环境；先看报告包里的 `cache/resource`。
- 日志和截图冲突时，优先相信现场图，再回头解释 OCR / 模板为何误判。
- 如果问题本身没有在当前日志中复现，要明确写“证据未复现”，不要硬凑结论。
- 如果 issue 版本很旧，要明确区分“当时的根因”和“当前分支是否已修复”。
- 如果用户日志与当前代码不一致，先按用户版本 tag 复核；若确认已修，再看修复是否已进入 tag / release：已发版建议升级，未发版建议等待 release。
- 如果回答里出现任务名、设置项、按钮名、提示文案，优先使用 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 的中文文案；必要时才在括号里补原始 key / `taskChain` / 枚举名。
- 如果回答里引用了具体代码行，直接给远端 GitHub `blob` 行号链接，用尖括号包裹，不要给本地路径加行号。
- 如果证据表明问题已在新版本修复，明确建议用户升级；如果怀疑安装包、资源文件或配置损坏，明确建议重新下载或重建；如果判断为真实代码缺陷且暂无 workaround，明确建议等待开发者修复。
