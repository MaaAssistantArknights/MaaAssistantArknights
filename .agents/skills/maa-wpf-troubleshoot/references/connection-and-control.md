# ADB、模拟器、截图和触控

## 先分开看三个结果

1. ｢设置 → 连接设置 → 截图测试｣ 记录返回提示、实际画面和截图耗时。画面须确实属于目标模拟器实例。可在 PowerShell 用 MAA 实际配置的 ADB 运行 `& 'C:\实际路径\adb.exe' devices -l`。核对序列号、地址和状态。
2. 设备列表有目标，表示 ADB 能枚举它；截图测试显示预期画面，表示已取得图像。输入链路须另用简单任务验证实际游戏画面有变化。
3. 分清初始连接失败、运行中设备断开和游戏内掉线。初始 ADB 连接失败时 WPF 会尝试断开重连。启用相应选项时，会在这之前尝试启动模拟器，仍失败时再尝试重启 ADB Server、关闭并重启 ADB 进程。重启 ADB 可能影响同机其他 ADB 工具。运行中 Core 有设备重连链路，失败可能停止任务；游戏内网络错误则看当时游戏画面与具体任务流程。

## 常见提示

| 现象、报错或日志 | 检查和下一步 |
| --- | --- |
| ｢自动检测连接｣ 找不到模拟器或认错目标 | 先只开一个目标实例、断开其他设备，并开启目标模拟器的 ADB 调试。若模拟器提升运行，检查权限差异。手填时核对 ADB 路径、当前实例地址和连接配置。蓝叠 Hyper-V 端口可能变动，见连接手册。 |
| `10061`、`connection refused` | 目标 TCP 地址未接受连接，先核对模拟器已启动、ADB 调试已开启、该实例实际端口及监听地址。本机普通模拟器通常用 `127.0.0.1:<端口>`，不要把局域网网关当模拟器。MuMu 网络桥接和远端设备按各自地址处理。 |
| `unauthorized`、`offline`、列表空 | `unauthorized` 先解锁实体机并确认 USB 调试授权；`offline` 核对实例是否仍在线、地址是否过期、线缆/无线链路与 ADB 版本；列表空先查模拟器端口或 USB 连接与调试。无线配对成功后仍要使用设备显示的连接端口。 |
| `ResolutionError`／｢模拟器分辨率获取失败｣ | 先看 `ConnectionInfo.why` 是否为显示命令执行失败或无法解析。用同一 ADB 对目标设备检查分辨率，例如 `adb -s <序列号> shell wm size`。 |
| `UnsupportedResolution`／｢模拟器分辨率不支持｣ | 已拿到分辨率但不符合缩放要求。设置横屏 16:9、720p 或更高，美服按手册用 1920×1080；重新连接再测。 |
| `ScreencapFailed`、｢截图为空｣、黑/白/冻结画面 | 先看前台是否是游戏、是否截图到正确实例。核对模拟器渲染配置和截图增强适用版本、安装路径、实例号。可暂时关闭增强做对照，若禁用后截图恢复，再排对应增强分支。 |
| ｢截图用时较长／过长｣ | v6.21.3 的 WPF 平均耗时警告阈值为 400/800 ms（含阈值）。检查实际截图方法、模拟器版本、CPU/GPU 占用、后台降帧和渲染配置。支持的 MuMu、雷电可测试截图增强。自动战斗对延迟更敏感。 |
| `TouchModeNotAvailable`、有图但无操作 | 先检查触控模式和输入后的画面变化。Android 10+ 的 Minitouch 可能受 SELinux 限制，可尝试其他兼容模式。旧 ADB 兼容问题优先用官方 Platform Tools 做对照，v6.21.3 的 ｢强制替换 ADB｣ 会把 Platform Tools 解压到 MAA 的 `adb/platform-tools` 目录并改填当前 ADB 路径。ADB Input 慢，主要作为兼容手段。 |
| MuMu 启用应用保活后无法点击 | 截图增强与触控增强分开。此基线的触控增强要求 Windows x64 官方版 MuMu 6.3.2+，方舟专版和国际版需另查版本支持。在 MuMu 连接配置下依次开启截图和触控增强，再看 ｢完整 MuMu 增强已就位｣ 或 Core 的 `MuMuExtrasInputStatus`（`available`、`deferred`），最后确认实际输入能改变画面。增强不可用时可先关闭 MuMu 的 ｢应用保活｣，用前台画面重测。 |
| 雷电 + MaaTouch 滑动缓慢、任务抢跑 | WPF 有这个组合的专项提示。换兼容触控模式后重测同一任务的滑动与识别；若截图也慢，单独检查雷电增强。 |
| ｢未找到窗口｣ 或 ｢AttachWindow 绑定窗口失败｣ | ｢明日方舟 PC 端（社区维护）｣ 使用窗口连接。先检查目标窗口是否存在可见，再看 `AttachWindow` 和 `Win32Controller` 的截图/输入；有点击调用记录仍要看画面是否变化。 |

可用两条历史记录校正误判。[#18197](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/18197) 用了网关地址，模拟器能启动但 ADB 仍连接失败。[#17441](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17441) 的 `ArgumentOutOfRangeException` 出在 WPF 连接地址历史更新，修复提交已在 v6.21.3 内；只有堆栈匹配才考虑同类问题。

依据：[连接设置](https://docs.maa.plus/zh-cn/manual/connection.html) · [设备兼容](https://docs.maa.plus/zh-cn/manual/device/windows.html) · [FAQ](https://docs.maa.plus/zh-cn/manual/faq.html) · [WPF 回调与连接分支](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Main/AsstProxy.cs) · [Core ADB 控制器](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaCore/Controller/AdbController.cpp) · [分辨率校验](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaCore/Controller/ControlScaleProxy.cpp)。
