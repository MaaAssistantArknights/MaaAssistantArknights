# 开始使用

基线为 Windows WPF v6.21.3。系统架构看 Windows ｢设置 → 系统 → 关于 → 系统类型｣。

## 下载、安装和首跑

1. 使用 [MAA 官网](https://maa.plus/) 或 [GitHub 正式发布](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)。下载名为 `MAA-<版本>-win-x64.zip` 或 `MAA-<版本>-win-arm64.zip` 的完整包，分别对应 x64 与 ARM64。文件名以 `MAAComponent-` 开头的增量包、符号包等组件依赖完整安装。
2. 完整解压到独立、可写的目录，例如用户目录里的 `Apps\MAA`。启动该目录下的 `MAA.exe`；要从桌面打开就创建快捷方式。正式包自带 .NET 运行时。缺 Visual C++ 运行库时，使用随包提供的 `DependencySetup_依赖库安装.bat`。系统要求为 64 位 Windows 10/11。
3. Winget 用户可以使用 `winget install --id MaaAssistantArknights.MaaAssistantArknights --exact --source winget`，安装后可运行 `maa`。若也安装了 maa-cli，确认实际打开的是 WPF 窗口。
4. 先启动一个模拟器，启用其 ADB 调试，断开其他待调试设备。首次设置指引或 ｢设置 → 连接设置｣ 可选 ｢自动检测连接｣。分辨率先用横屏 1280×720 或 1920×1080，美服用 1920×1080。非官服在 ｢设置 → 运行设置 → 客户端类型｣ 选择对应区服。
5. 用 ｢设置 → 连接设置 → 截图测试｣ 确认目标实例和画面正确。回 ｢一键长草｣ 先只勾选一个简单任务，点 ｢Link Start!｣，以该任务真正完成作为首跑验证。

依据：[下载与安装](https://docs.maa.plus/zh-cn/manual/install.html) · [新手上路](https://docs.maa.plus/zh-cn/manual/newbie.html) · [WPF 发布工程](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/MaaWpfGui.csproj)。

## 设备和连接方式

| 用户场景 | 设置入口与要点 |
| --- | --- |
| 本机安卓模拟器 | ｢连接设置｣ 优先选对应品牌的 ｢连接配置｣。自动检测会尝试填 ADB 路径、地址和配置。需要手动设置时，在模拟器侧确认当前实例的 ADB 端口，使用 `127.0.0.1:<端口>` 或 ADB 列出的 `emulator-<编号>`。可用模拟器的 ADB 或 Google Platform Tools 的 `adb.exe`，若在 PATH 中可填 `adb`。品牌的常见端口仅供排查。 |
| 蓝叠 Hyper-V 动态端口 | 选 ｢蓝叠模拟器｣，启用 ｢自动检测连接｣ 和 ｢每次重新检测｣。多核心时按连接手册核对当前配置的 `Gui.ConnectSettings.Extras.BluestacksExtra.ConfigKeyword` 与 `ConfigPath`。手改 JSON 前先退出并备份；截图测试确认连到了目标核心。 |
| MuMu、雷电、AVD 截图增强 | 先选对应连接配置，再按官方文档核对模拟器版本、发行版、安装路径与实例编号。MuMu 的 Windows ARM 预设是独立入口，可用 ｢自动检测并测试｣，Windows x64 增强说明只适用于对应 x64 版本。AVD 截图增强使用 MaaFwAdb。 |
| Android 实体机 | USB 调试授权后，`adb devices` 中状态为 `device` 的序列号可直接填作 ｢连接地址｣。无线调试使用设备实际连接端口，Android 11+ 的配对端口只用于配对。设备分辨率、触控和账号切换有额外限制，优先查看实体设备文档。 |
| 多开模拟器 | 需要同时跑多个实例时，给每个实例一份独立的 MAA 安装目录，使用相同的 ADB 程序、不同的连接地址。启动模拟器时可在 ｢启动设置｣ 选对应实例快捷方式，或填写模拟器本体加实例启动参数。用截图测试逐个确认。 |
| 明日方舟 PC 客户端 | WPF 有 ｢明日方舟 PC 端（社区维护）｣ 连接配置，走窗口绑定而非 ADB。支持范围以对应版本的界面提示为准。 |

依据：[连接设置](https://docs.maa.plus/zh-cn/manual/connection.html) · [Windows 模拟器](https://docs.maa.plus/zh-cn/manual/device/windows.html) · [Android 实体设备](https://docs.maa.plus/zh-cn/manual/device/android.html) · [WPF 连接配置源码](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UserControl/Settings/ConnectSettingsUserControlModel.cs)。
