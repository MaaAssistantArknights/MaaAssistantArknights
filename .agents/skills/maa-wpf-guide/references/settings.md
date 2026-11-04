# 设置、定时与更新

## 配置与运行

- ｢设置 → 切换配置｣ 管理不同配置，各自保留任务队列和对应设置。快捷方式可追加 `--config <配置名>`；含空格的配置名按 Windows 命令行规则加引号。换目录或迁移时退出 MAA 后复制 `config`，需要保留识别结果和现场时也考虑 `data`、`debug`。新版本配置主文件是 `config/gui.new.json`，老版本曾用 `config/gui.json`。
- ｢设置 → 启动设置｣ 管理启动 MAA 后是否运行、启动模拟器路径与附加命令、启动等待等。｢设置 → 运行设置｣ 管理客户端类型、任务期间阻止休眠、开始前/结束后脚本、划火柴模式。脚本会按指定路径执行，使用前确认其内容与运行影响。
- 一键长草下方的 ｢完成后｣ 可配置退出游戏、模拟器或 MAA，以及锁屏、睡眠、休眠或关机。按需启用 ｢任务出错时跳过｣；系统休眠需在 Windows 开启相应功能。关注 ｢仅当次*｣ 的临时选项。
- ｢设置 → 定时执行｣ 最多 8 组，设启用、时间、配置。普通定时由运行中的 MAA 处理，按本机时钟。｢自定义配置选择｣ 可在触发前两分钟切换配置；｢强制定时启动｣ 会打断当前任务并重启游戏。与 ｢理智作战｣ 任务里的周计划分别配置。
- ｢定时启动前尝试唤醒计算机并启动 MAA｣ 通过 Windows 任务计划程序在每个定时项前约 5 分钟尝试唤醒并启动 MAA。睡眠、休眠能否唤醒受设备和系统设置影响。唤醒启动会跳过启动设置里直接开跑/开模拟器的自动运行项；需要在定时运行时拉起模拟器，使用连接失败时尝试启动的链路。

依据：[其他功能](https://docs.maa.plus/zh-cn/manual/introduction/others.html) · [定时触发与切换配置](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UI/TaskQueueViewModel.cs) · [Windows 唤醒注册](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Utilities/ScheduledWakeUp.cs) · [配置读写](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Configuration/Factory/ConfigFactory.cs)。

## 其他设置入口

| 设置分类 | 常见需求 |
| --- | --- |
| 界面设置 | 显示语言、外观、主题与托盘相关选项。游戏内主界面主题通过任务 ｢更换主题｣ 设置。 |
| 性能设置 | ｢使用 GPU 加速推理｣ 控制识别推理，使用 DirectML。识别异常时可关闭做对照。模拟器截图增强在 ｢连接设置｣。 |
| 通知设置 | 系统通知和外部通知。外部通知支持多个渠道，包括自定义 Webhook，按页面选事件和渠道。 |
| 三方服务 | 掉落与公招数据上报设置、从一图流读取干员数据等。凭据由用户在本机填写。 |
| 热键设置 | 配置显示/收起 MAA 与任务启停快捷键。重复或被占用时会提示注册失败。 |
| 远程控制 | 连接按 MAA 远程控制协议实现的服务，核对获取任务和汇报状态的端点，配置由用户掌握。 |
| 成就设置／问题反馈／关于我们 | 管理成就展示、打开日志或生成报告、查看官方入口。 |

依据：[WPF 设置面板](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Views/UI/SettingsView.xaml) · [简体中文设置文案](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Res/Localizations/zh-cn.xaml) · [远程控制协议](https://docs.maa.plus/zh-cn/protocol/remote-control-schema.html)。

## 软件、资源和导航热更

- ｢设置 → 更新设置 → 软件更新｣ 更新程序及功能，通常在重启时应用。正式版、公测版和内测版渠道的功能与风险不同，具体可见选项随版本变化。
- ｢资源更新｣ 更新作业地图、掉落图标、公招标签等素材。任务期间下载后在空闲时加载，通常无需重启。右上角 ｢热更数据获取成功｣ 属于导航/活动数据通路。
- v6.21.3 的海外源软件从 GitHub 拉取，资源可检测后手动下载；Mirror酱可用 CDK 做自动更新。关于 GitHub Token 等选项，先核对用户版本的更新页和发布说明。
- 手动拖入支持 GitHub Releases 的 Windows 完整软件包或匹配当前版本与架构的 OTA，以及 GitHub 直接下载的 MaaResource ZIP。保留原文件名和目录结构。软件包重启应用；资源包在适用时合并并重载。完整包应用前需要确认，会清理安装目录内未列入保留范围的旧文件，务必使用 MAA 独立目录。

依据：[更新说明](https://docs.maa.plus/zh-cn/manual/introduction/others.html#更新) · [当前正式版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest) · [WPF 拖入更新入口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UI/RootViewModel.cs)。
