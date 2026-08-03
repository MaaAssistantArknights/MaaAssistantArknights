# MAA Issue Log Analysis Knowledge Base

## Stateful UI Automation Checks

- 分析 issue 时，先区分三层东西，不要混为一谈：
 - 游戏规则
 - MAA 当前自动化流程
 - 用户对最终状态的预期
- 遇到“一键/快捷/批量”按钮时，先看资源任务和日志里的真实控制路径，不要只看现象。
- 对状态型 UI，重点核对三件事：
 - 前置条件是什么
 - 流程中哪些状态会被临时改写
 - 结束条件满足后应停在哪个状态

## Abort DWM

- DWM 在 Windows8 之后为常开，如遇相关问题为显卡驱动问题

## About Arknights PC Client

- 连接 PC 端为实验性功能
- 由于 MAA Team 开发人手有限，此功能由社区维护，非 MAA Team 持续支持，功能和稳定性可能不尽如人意，遇到问题时也可能无法第一时间修复。
- 在使用 PC 端的过程中，如遇到影响使用的问题，建议改用 ADB 连接 Android 模拟器或移动设备，获得更稳定的体验。
- 我们也始终欢迎有能力的开发者参与贡献提交 Pull Request，共同完善 PC 端支持。
- PC 的鼠标为客户端独立渲染的，鼠标会挡住需要识别的目标，在需要点击相同位置的图标或重试时极有可能导致下一次识别失败。
- PC 版存在根据鼠标位置产生界面偏转的效果（类似手机端的陀螺仪视差），而模拟器环境中界面通常保持在正中央，因此不会受到影响。若鼠标未位于客户端窗口正中央，UI 会发生偏转，导致图像识别失败，进而出现无法正常进入界面、循环切换主题等现象。如果使用 PC 客户端运行游戏，请将鼠标移动到游戏窗口正中央附近并保持不动，以避免界面偏转影响识别。

## Reception Clue Analysis

- 会客室线索问题先对照当前资源任务和日志，不要只凭体感下结论。
- 取下线索 -> 赠送重复线索 -> 当前线索数量够开启线索交流时才统一放置，这是 by design。
- “送完重复线索后线索板暂时为空”或“用户自己放的线索被统一取下”本身不能直接判成 bug。

## PC announcement cannot be closed

- PC 端的公告为独立的弹出窗口，现有的窗口绑定方式无法截图获取到公告窗口
- 如需使用 PC 端自动关闭公告，请使用前台的截图方式，但该方法会要求窗口必须在前台且无遮挡
- 如不是用前台模式，请手动关闭公告，或更推荐使用 adb 连接模拟器

## Guardrails For Future Analysis

- 不要把维护者评论、机器人评论、或单张截图当成最终结论；必须回到日志和代码确认状态是怎么变化的。
- 对状态型 UI，先回答“日志里的行为是否符合设计”，再回答“这个设计是否符合用户预期”。
- 不要只看最终现象；要结合配置、`gui.log`、`asst.log`、资源任务和当前代码一起判断。

## Common Pitfalls

> 从 SKILL.md 迁入：分析时容易踩的误判陷阱。

### 连接类

- `gui.log` 只显示"连接失败"，但 `asst.log` 里已经给出 `adb devices`、`adb connect`、端口轮询和 `ConnectionInfo`。连接类问题必须以 `asst.log` 为准。
- `adb devices` 显示目标地址 `offline`，随后 MuMu 备选端口都 `10061`，通常更像模拟器 / ADB 状态异常，或自动探测到的端口不可达，而不是任务逻辑问题。

### 关卡导航 / 难度切换

- `gui.log` 显示选中的关卡是 `15-13-hard` 一类 hard 代码，而 `asst.log` 长时间卡在 `ChapterDifficultyHard`，OCR 却反复识别到和按钮无关的文字，通常说明当前画面没有进入预期的难度切换界面。

### 证据完整性

- `asst.log` 明确写了 `Save image` 到 `debug/interface/*.png` 或 `debug/drops/*.png`，但上传包没有相应分卷时，要把"缺失的现场证据"单独写出来。
- `part02` 可以是空包，也可以只包含图片；不要因为没有文本日志就把它判成"无用分卷"。
- issue 机器人评论"日志没有上传成功"时，不要自动当真；先验证正文附件是否仍可下载。
- 如果 `gui.log` 说"任务出错"，但对应 `taskid` 的 `asst.log` 实际 `AllTasksCompleted`，要明确写"本次日志未复现用户描述的问题"。

### 会客室 / 线索

- 对会客室 / 线索 issue，如果 `asst.log` 里出现 `InfrastClueQuickInsert`、`remove_clue`、`SendClues` 或 `InfrastClueQuickSendDuplicates`，先对照资源任务判断这是不是当前设计流程，不要只看线索板中途是否为空。
- 如果线索流程里出现"取下线索 -> 赠送重复线索 -> 条件满足后统一放置"，默认先按 by design 处理；只有当日志显示本应统一放置却没有发生时，再继续追实现问题。

### 版本差异

- 用户日志里的任务流程与当前主线代码明显不一致，且当前代码看起来已经修掉了该问题：
    - 先确认用户版本，必要时切到对应 tag（例如 `git checkout vXXX`）核对旧逻辑。
    - 不要用当前分支否定旧日志；旧版本问题可能真实存在。
    - 如果主线已修复，再看修复 commit 是否已进入 tag / release：已发版建议升级，未发版建议等待 release。

### 配置文件

- `gui.new.json` 和实际日志不一致时，不要急着判"用户配置写错了"；先看 `gui.new.json.bak`，尤其是用户复现后又改回开关的场景。

### PC / Win32 输入

- 在 `ConnectConfig=PC` 的 issue 里，`Win32Controller::click` 正常返回不代表点击真的生效；要看点击后的下一帧中，按钮状态、数量 OCR、场景识别有没有变化。

### 理智药 / 临期药

- `gui.log` 中"已使用即将过期的理智药"这类高层提示，不一定等价于底层逐药 OCR 结论；如果 `asst.log` 明确识别到 `3天`、`NotExpiring` 等相反证据，应优先相信 `asst.log`。注意过期天数阈值现为可配置参数 `medicine_expire_days`，不再是固定 48 小时。

## MAA multi-opening and multi-account management

- MAA 在设计上仅针对单账号使用。若你需要同时管理多个游戏账号（多开），官方并未提供内置支持，但可以通过复制多份 MAA 程序到不同文件夹的方式实现变通
- 不考虑多开相关实现

## Connect.TouchMode: adb

- MAA 触控模式共三种：`minitouch`（默认）、`maatouch`（实验性）和 `adb input`（不推荐使用）。
- `maatouch` 是 `minitouch` 的 Java 实现，并额外支持按键输入，可避免 minitouch 走 adb 命令传输按钮带来的较高延迟。
- `adb input` 仅用于兼容部分系统版本过低、无法运行 `minitouch` 或 `maatouch` 的实体机设备。
- 能用前两种模式时，绝不推荐使用 `adb input`。
- `adb input` 的滑动容易拖飞，为避免此问题，滑动速度会被设置得非常慢，且滑动距离与其他两种模式不同；在需要精确控制坐标的场景下无法使用。
- 若用户反馈触控相关异常且配置为 `adb`，应优先建议切换为 `minitouch` 或 `maatouch`，排除模式本身带来的延迟与兼容性问题。

