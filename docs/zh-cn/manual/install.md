# 下载与安装

::: tip
你正在查阅 MAA GUI 的下载与安装说明文档。若你需要查阅 maa-cli 的下载与安装说明，请访问 maa-cli 的 [安装及编译](manual/cli/install.md) 文档。
:::

## 下载 MAA

MAA 提供多种下载方式，包括官网下载，从包管理器安装，群文件下载等方式。请选择适合你的方式进行下载。

### Windows

#### 通过 [官网](https://maa.plus) 页面下载最新的 MAA 安装包

官网一般会自动选择正确的版本架构，对于大多数阅读本文的用户来说，应为 Windows x64。

#### 通过 Windows 包管理器 [Winget](https://github.com/microsoft/winget-pkgs/tree/master/manifests/m/MaaAssistantArknights/maa-cli/)

```bash
winget install maa
```

#### 通过 QQ 群文件下载最新的 MAA 安装包

1. [点此加入 MAA 官方 QQ 群](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html) ;
2. 在群文件中找到最新的 MAA 安装包进行下载。

#### 通过 [github releases 页面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 下载最新的 MAA 安装包

请确认系统架构并下载对应的安装包。对于大多数阅读本文的Windows用户来说，应为 Windows x64。

#### 通过 [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maagh-release) 下载最新的 MAA 安装包：

请确认系统架构并下载对应的安装包。对于大多数阅读本文的Windows用户来说，应为 Windows x64。

:::tip
在 Mirror酱 需要提供 CDK 才能下载 MAA 安装包。该 CDK 需要付费订阅，但连通性良好且速度较快。关于订阅价格和方式，请访问 [Mirror酱](https://mirrorchyan.com/zh/) 了解更多信息。
:::

### macOS

#### 通过 [官网](https://maa.plus) 页面下载最新的 MAA 安装包

官网一般会自动选择正确的版本架构，对于阅读本文的Mac用户来说，应下载 macOS 通用版。

#### 通过 QQ 群文件下载最新的 MAA 安装包

1. [点此加入 MAA 官方 QQ 群](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html) ;
2. 在群文件中找到最新的 MAA 安装包进行下载。

#### 通过 [github releases 页面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 下载最新的 MAA 安装包

请确认系统架构并下载对应的安装包。对于阅读本文的Mac用户来说，应为 macos universal。

#### 通过 [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maagh-release) 下载最新的 MAA 安装包：

请确认您的芯片类型 (arm/x86) 并下载对应的安装包。

:::tip
在 Mirror酱 需要提供 CDK 才能下载 MAA 安装包。该 CDK 需要付费订阅，但连通性良好且速度较快。关于订阅价格和方式，请访问 [Mirror酱](https://mirrorchyan.com/zh/) 了解更多信息。
:::

### Linux 和其他操作系统

MAA GUI **暂未支持** Linux 和其他操作系统。你可以使用 **maa-cli** 来在这些系统上使用 MAA 的功能。请访问 maa-cli 的 [安装及编译](manual/cli/install.md) 文档了解更多信息。

## 安装 MAA

下载完成后，你可以看到一个压缩包，双击解压即可。解压后，你会看到一个文件夹，里面包含 MAA 的所有文件。
双击 `maa.exe` 即可启动 MAA。

::: tip
通过包管理器安装的用户，可以直接在命令行中输入 `maa` 来启动 MAA 而无需额外操作。
:::

## 后续步骤

安装完成啦？快来看看 [新手上路](manual/newbie.md) 和 [用户手册](./manual/) ，了解如何设置和使用 MAA 吧！
