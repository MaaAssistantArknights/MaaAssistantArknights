# 下载与安装

::: tip
你正在查阅 MAA GUI 的下载与安装说明文档。若你需要查阅 maa-cli 的下载与安装说明，请访问 maa-cli 的 [安装及编译](./manual/cli/install.md) 文档。
:::

## 下载 MAA

MAA 提供多种下载方式，包括官网下载，从包管理器安装，群文件下载等方式。请选择适合你的方式进行下载。

### Windows

#### 通过 [官网](https://maa.plus) 页面下载最新的 MAA 安装包

官网一般会自动选择正确的版本架构，对于大多数阅读本文的用户来说，应为 Windows x64。

#### 通过 [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maagh-release) 下载最新的 MAA 安装包：

请确认系统架构并下载对应的安装包。对于大多数阅读本文的Windows用户来说，应为 Windows x64。

:::tip
在 Mirror酱 需要提供 CDK 才能下载 MAA 安装包。该 CDK 需要付费订阅，但连通性良好且速度较快。关于订阅价格和方式，请访问 [Mirror酱](https://mirrorchyan.com/zh/) 了解更多信息。
:::

#### 通过 Windows 包管理器 [Winget](https://github.com/microsoft/winget-pkgs/tree/master/manifests/m/MaaAssistantArknights/MaaAssistantArknights/)

```bash
winget install maa
```

通过 Winget 安装的 MAA 安装位置默认为 `C:\Users\用户名\AppData\Local\Microsoft\WinGet\Packages` 。

#### 通过 QQ 群文件下载最新的 MAA 安装包

1. [点此加入 MAA 官方 QQ 群](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)
2. 在群文件中找到最新的 MAA 安装包进行下载。

#### 通过 [GitHub Releases 页面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 下载最新的 MAA 安装包

请确认系统架构并下载对应的安装包。对于大多数阅读本文的 Windows 用户来说，应为 Windows x64。

### macOS

#### 通过 [官网](https://maa.plus) 页面下载最新的 MAA 安装包

官网一般会自动选择正确的版本架构，对于阅读本文的 macOS 用户来说，应下载 macOS 通用版。

#### 通过 [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maagh-release) 下载最新的 MAA 安装包：

请确认您的芯片架构（arm/x86）并下载对应的安装包。

:::tip
在 Mirror酱 需要提供 CDK 才能下载 MAA 安装包。该 CDK 需要付费订阅，但连通性良好且速度较快。关于订阅价格和方式，请访问 [Mirror酱](https://mirrorchyan.com/zh/) 了解更多信息。
:::

#### 通过 QQ 群文件下载最新的 MAA 安装包

1. [点此加入 MAA 官方 QQ 群](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)
2. 在群文件中找到最新的 MAA 安装包进行下载。

#### 通过 [GitHub Releases 页面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 下载最新的 MAA 安装包

请确认系统架构并下载对应的安装包。对于阅读本文的 macOS 用户来说，应选择 `MAA-v*.*.*-macos-universal.dmg`。

### Linux 和其他操作系统

MAA GUI **暂未支持** Linux 和其他操作系统。你可以使用 **maa-cli** 来在这些系统上使用 MAA 的功能。请访问 maa-cli 的 [安装及编译](./manual/cli/install.md) 文档了解更多信息。

## 安装 MAA

### Windows

下载完成后，你会得到一个 `.zip` 文件。用解压缩软件将其完全解压后，会得到一个包含 MAA 所有文件的文件夹。

::: warning
MAA 需要 Visual C++ Redistributable x64（VCRedist x64）和 .NET 10 。请在解压后的 MAA 目录中以管理员身份运行 `DependencySetup_依赖库安装.bat` 来安装这些依赖。安装完成后再运行 `maa.exe`。
:::

双击 `maa.exe` 即可启动 MAA。

::: tip
通过 Windows 包管理器 [Winget](https://github.com/microsoft/winget-pkgs/tree/master/manifests/m/MaaAssistantArknights/MaaAssistantArknights/) 安装的用户，可以直接在命令行中输入 `maa` 来启动 MAA 而无需额外操作。若同时通过 Winget 安装了 GUI 与 CLI，可能需要区分二者。
:::

### macOS

下载完成后，你会得到一个 `.dmg` 文件。双击打开该 `.dmg`，将 `MAA.app` 拖入 `/Applications`（或你希望放置的其他目录）以完成安装。

首次打开可能会被 macOS 的 Gatekeeper 阻止，解决方法如下：

::: details
- 推荐（安全）：在 Finder 中对 `MAA.app` 右键 -> 选择 “打开”，在弹窗中再次选择 “打开” 来允许该应用运行（仅绕过一次性阻止）。
- 若仍无法打开，可在 **终端** 使用以下命令（仅在确认来源可信时使用）：
  - 单个应用：`xattr -d com.apple.quarantine /Applications/MAA.app`
  - 递归删除（若需要）：`sudo xattr -r -d com.apple.quarantine /Applications/MAA.app`
- 不推荐也不安全的方法：全局禁用 Gatekeeper（`sudo spctl --master-disable`），请勿在常规环境中使用。
:::

## 后续步骤

安装完成啦？快来看看 [新手上路](./manual/newbie.md) 和 [用户手册](./manual/) ，了解如何设置和使用 MAA 吧！
