---
order: 2
icon: material-symbols:download-2-rounded
---

# 下载与安装

::: tip

你正在查阅 MAA GUI 的下载与安装说明文档。若你需要查阅 maa-cli 的下载与安装说明，请访问 maa-cli 的 [安装及编译](./cli/install.md) 文档。

:::

## 下载 MAA

MAA 提供多种下载方式，包括官网下载，从包管理器安装，群文件下载等方式。请选择适合你的方式进行下载。

### Windows

#### 通过 [官网](https://maa.plus) 页面下载最新的 MAA 安装包

官网一般会自动选择正确的版本架构，对于大多数阅读本文的用户来说，应为 Windows x64。

#### 通过 [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maagh-release) 下载最新的 MAA 安装包

请确认系统架构并下载对应的安装包。对于大多数阅读本文的Windows用户来说，应为 Windows x64。

::: tip

需要提供 CDK 才能在 Mirror酱 下载 MAA 的安装包。该 CDK 需要付费订阅，但连通性良好且速度较快。这并不代表 MAA 本体需要付费使用。关于订阅价格和方式，请访问 [Mirror酱](https://mirrorchyan.com/zh/) 了解更多信息。

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

#### 通过 [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maagh-release) 下载最新的 MAA 安装包

请确认您的芯片架构（arm/x86）并下载对应的安装包。

::: tip

需要提供 CDK 才能在 Mirror酱 下载 MAA 的安装包。该 CDK 需要付费订阅，但连通性良好且速度较快。这并不代表 MAA 本体需要付费使用。关于订阅价格和方式，请访问 [Mirror酱](https://mirrorchyan.com/zh/) 了解更多信息。

:::

#### 通过 QQ 群文件下载最新的 MAA 安装包

1. [点此加入 MAA 官方 QQ 群](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)
2. 在群文件中找到最新的 MAA 安装包进行下载。

#### 通过 [GitHub Releases 页面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 下载最新的 MAA 安装包

请确认系统架构并下载对应的安装包。对于阅读本文的 macOS 用户来说，应选择 `MAA-v*.*.*-macos-universal.dmg`。

### Linux 和其他操作系统

MAA GUI **暂未支持** Linux 和其他操作系统。你可以使用 **maa-cli** 来在这些系统上使用 MAA 的功能。请访问 maa-cli 的 [安装及编译](./cli/install.md) 文档了解更多信息。

## 安装 MAA

### Windows

下载完成后，你会得到一个 `.zip` 文件。用解压缩软件将其完全解压后，会得到一个包含 MAA 所有文件的文件夹。

::: warning 

1.请不要将 MAA 解压到如 `C:\`、`C:\Program Files\` 等需要 UAC 权限的路径。

2.MAA 需要 Visual C++ Redistributable x64（VCRedist x64）和 .NET 10 。请在解压后的 MAA 目录中以管理员身份运行 `DependencySetup_依赖库安装.bat` 来安装这些依赖。安装完成后再运行 `maa.exe`。

更多信息请参考[常见问题](./faq.md)置顶。

:::

双击 `maa.exe` 即可启动 MAA。

::: tip

通过 Windows 包管理器 [Winget](https://github.com/microsoft/winget-pkgs/tree/master/manifests/m/MaaAssistantArknights/MaaAssistantArknights/) 安装的用户，可以直接在命令行中输入 `maa` 来启动 MAA 而无需额外操作。若同时通过 Winget 安装了 GUI 与 CLI，可能需要区分二者。

:::

### macOS

下载完成后，你会得到一个 `.dmg` 文件。双击打开该 `.dmg`，将 `MAA.app` 拖入 `/Applications`（或你希望放置的其他目录）以完成安装。

首次打开可能会被 macOS 的 Gatekeeper 阻止，解决方法如下：

::: details

#### 现象

- 提示：“应用程序” 已损坏，无法打开。您应该将它移到废纸篓。

- 提示： 无法打开 “应用程序”，因为无法验证开发者。macOS 无法验证此 App 不包含恶意软件。

- 提示：“应用程序” 将对您的电脑造成伤害。您应该将它移到废纸篓。

#### 解决方法

一般情况下，只需要 1 和 2 两步即可。

1. 允许 “任何来源” 下载的 App 运行
   
   - 打开 “终端” 执行如下命令（根据提示输入您的密码即可）：

   ```bash
   sudo spctl --master-disable
   ```

   - 打开 “系统偏好设置…” - “安全性与隐私”，“通用” 标签页，勾选：

   `任何来源`
   
   并点按左下角的锁图标以保存更改。

   ::: tip

   macOS Ventura 略有变化，位于：系统设置 – 隐私和安全性，“安全性”。
   
   :::

2. 移除应用的安全隔离属性
   
   - 打开 “终端” 执行如下命令（根据提示输入您的密码即可）：

   ```bash
   sudo xattr -dr com.apple.quarantine /Applications/name.app
   ```

   `/Applications/name.app` 如果不知道该如何输入，将 App 直接拖拽 `sudo xattr -rd com.apple.quarantine`（中间有个空格）后面即可。

   一般情况下，多余的扩展属性都可以清除（个别应用例外）：

   可以直接输入：`sudo xattr -cr /Applications/*`

3. macOS Ventura 额外步骤

   - macOS Ventura 的系统安全性又上升到一个新的高度，上述两个步骤后，需要在 “系统设置” 中打开应用。
   系统设置 – 隐私和安全性，“安全性” 下面出现提示，点击 “仍要打开”，当然该操作仅需要一次，以后可以正常打开。

   ::: tip

   以上同样适用于 macOS Sonoma 和 macOS Sequoia。

   :::

4. 重新签名
   
   - 多数情况下，App 已经可以正常运行了，很多和谐软件因为证书问题，仍然无法打开。此时可以重新签名。

   - 打开 “终端”，执行如下命令：

   ``` bash
   codesign --force --deep --sign - /Applications/name.app
   ```

   **前提条件**: 安装 Command Line Tools for Xcode
   
   此时会提示安装，或者使用终端命令在线安装：
   
   ```bash
   xcode-select --install
   ```

   也可以到这里下载 [Command Line Tools for Xcode：Xcode 14.3.1 (14E300c) - Apple 平台 IDE](https://sysin.org/blog/apple-xcode-14/)

5. 覆盖恶意软件保护

   - 还有一种情况，

     提示：“应用程序” 将对您的电脑造成伤害。您应该将它移到废纸篓。

   以上步骤操作后还是无法打开。

   - 解决办法：

   访达 -> 应用程序 -> 右击软件 -> 显示简介，勾选 “覆盖恶意软件保护”。

   ::: tip

   正常应用不会出现这个选项
   
   :::
   
本解决办法引用于 [macOS 提示：“应用程序” 已损坏，无法打开的解决方法总结](https://sysin.org/blog/macos-if-crashes-when-opening/)

:::

## 后续步骤

安装完成后，可返回[新手上路](./newbie.md)继续配置，或前往[功能介绍](./introduction/)查看 MAA 支持的各种功能吧！
