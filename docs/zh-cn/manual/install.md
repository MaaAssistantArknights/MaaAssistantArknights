---
order: 2
icon: material-symbols:download-2-rounded
---

<!-- markdownlint-disable MD024 -->

# 下载与安装

> [tip]
>
>你正在查阅 MAA GUI 的下载与安装说明文档。若你需要查阅 maa-cli 的下载与安装说明，请访问 maa-cli 的 [安装及编译](./cli/install.md) 文档。目前安卓牛牛（MAA 安卓版）已开放测试，请前往 [MAA-Meow](https://github.com/Aliothmoon/MAA-Meow) 了解更多情况。



## 下载 MAA

MAA 提供多种下载方式，包括官网下载，从包管理器安装，群文件下载等方式。请选择适合你的方式进行下载。

### 通过 [官网](https://maa.plus) 下载最新的 MAA 软件包

官网一般会自动选择正确的版本架构，对于大多数阅读本文的用户来说，应为 Windows x64。对于阅读本文的 macOS 用户来说，应下载 macOS 通用版。

### 通过 [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install) 下载最新的 MAA 软件包

请确认系统架构并下载对应的软件包。对于大多数阅读本文的 Windows 用户来说，应为 Windows x64。对于阅读本文的 Mac 用户，Mirror酱 不提供通用软件包，请确认您的芯片架构（arm/x86）后下载对应的软件包。

::: tip

[Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install) 是独立的第三方下载加速服务，需要付费使用，而非 MAA 收费。其运营成本由订阅收入支撑，部分收益将回馈项目开发者。欢迎订阅 CDK 享受高速下载，同时支持项目持续开发。

:::

### 使用 Windows 包管理器（Winget）安装

::: tip

本方法仅适用于使用 Windows 的用户

:::

请在终端中运行以下命令：

```bash
winget install maa
```

通过此方式安装的默认安装路径为 `C:\Users\用户名\AppData\Local\Microsoft\WinGet\Packages`。

### 通过 QQ 群文件下载最新的 MAA 软件包

1. 加入 [MAA 官方 QQ 群](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)
2. 在群文件中找到最新的 MAA 软件包进行下载。

### 通过 [GitHub Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 下载最新的 MAA 软件包

请确认系统架构并下载对应的软件包。对于大多数阅读本文的 Windows 用户来说，应为 `MAA-<版本号>-win-x64.zip`。对于阅读本文的 macOS 用户来说，应选择 `MAA-<版本号>-macos-universal.dmg`。

## Linux 和其他操作系统

MAA GUI **暂未支持** Linux 和其他操作系统。你可以使用 **maa-cli** 来在这些系统上使用 MAA 的功能。请访问 maa-cli 的 [安装及编译](./cli/install.md) 文档了解更多信息。

## 安装 MAA

### Windows

下载完成后，你会得到一个 `.zip` 文件。用解压缩软件将其完全解压后，会得到一个包含 MAA 所有文件的文件夹。

::: warning

1.请不要将 MAA 解压到如 `C:\`、`C:\Program Files\` 等需要 UAC 权限的路径。

2.MAA 需要 Visual C++ Redistributable x64（VCRedist x64）和 .NET 10 。请在解压后的 MAA 目录中以管理员身份运行 `DependencySetup_依赖库安装.bat` 来安装这些依赖。安装完成后再运行 `maa.exe`。

更多信息请参考[常见问题](./faq.md)置顶。

:::

双击 `MAA.exe` 即可启动 MAA。

::: tip

通过 Windows 包管理器（Winget）安装的用户，可直接在命令行中输入 `maa` 来启动 MAA 而无需解压缩、安装运行库等额外操作。若 PATH 中有 `maa-cli`，可能需要额外步骤以区分二者。

:::

### macOS

下载完成后，你会得到一个 `.dmg` 文件。双击打开该 `.dmg`，将 `MAA.app` 拖入 `/Applications` 以完成安装。

## 后续步骤

安装完成后，请返回 [新手上路](./newbie.md) 继续配置，或前往 [功能介绍](./introduction/) 查看 MAA 支持的各项功能吧！若您在安装中遇到问题，可以尝试查阅 [常见问题](./faq.md) 来解决问题。
