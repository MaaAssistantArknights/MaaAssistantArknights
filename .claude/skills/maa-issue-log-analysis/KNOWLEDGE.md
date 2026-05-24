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

