---
order: 3
icon: teenyicons:linux-alt-solid
---

# Linux 模拟器与容器

## 准备工作

以下安装方式任选其一即可：

### 使用 maa-cli

[maa-cli](https://github.com/MaaAssistantArknights/maa-cli) 是一个使用 Rust 编写的简单 MAA 命令行工具。相关安装与使用教程请阅读 [CLI 使用指南](../cli/)。

### 使用 Wine

MAA WPF GUI 当前可以通过 Wine 运行。

#### 安装步骤

1. 前往 [.NET 发布页](https://dotnet.microsoft.com/en-us/download/dotnet/8.0)下载并安装 Windows 版 .NET **桌面**运行时。

2. 下载 Windows 版 MAA，解压后运行 `wine MAA.exe`。

::: info 注意
需要在连接设置中将 ADB 路径设置为 [Windows 版 `adb.exe`](https://dl.google.com/android/repository/platform-tools-latest-windows.zip)。

如果您需要通过 ADB 连接 USB 设备，请先在 Wine 外运行 `adb start-server`，即通过 Wine 连接原生 ADB server。
:::

#### 使用 Linux 原生 MaaCore（实验性功能）

下载 [MAA Wine Bridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/MaaWineBridge) 源码并构建，用生成的 `MaaCore.dll`（ELF 文件）替换 Windows 版本，并将 Linux 原生动态库（`libMaaCore.so` 以及依赖）放在同一目录下。

此时通过 Wine 运行 `MAA.exe`，将会加载 Linux 原生动态库。

::: info 注意
使用 Linux 原生 MaaCore 时，需要在连接设置中将 ADB 路径设置为 Linux 原生 ADB。
:::

#### Linux 桌面整合（实验性功能）

桌面整合提供原生桌面通知支持，以及将 fontconfig 字体配置映射到 WPF 的功能。

将 MAA Wine Bridge 生成的 `MaaDesktopIntegration.so` 放到 `MAA.exe` 同目录下即可启用。

#### 已知问题

- Wine DirectWrite 强制启用 hinting，并且不将 DPI 传递给 FreeType，导致字体显示效果不佳。
- 不使用原生桌面通知时，弹出通知会抢占全系统鼠标焦点，导致无法操作其他窗口。可以通过 `winecfg` 启用虚拟桌面模式缓解，或禁用桌面通知。
- Wine-staging 用户需要关闭 `winecfg` 中的 `隐藏 Wine 版本` 选项，以便 MAA 正确检测 Wine 环境。
- Wine 的 Light 主题会导致 WPF 中部分文字颜色异常，建议在 `winecfg` 中切换到无主题（Windows 经典主题）。
- Wine 使用旧式 XEmbed 托盘图标，在 GNOME 下可能无法正常工作。
- 使用 Linux 原生 MaaCore 时暂不支持自动更新（~~更新程序：我寻思我应该下载个 Windows 版~~）

### 使用 Python

#### 1. 安装 MAA 动态库

1. 在 [MAA 官网](https://maa.plus/) 下载 Linux 动态库并解压，或从软件源安装：

   - AUR：[maa-assistant-arknights](https://aur.archlinux.org/packages/maa-assistant-arknights)，按照安装后的提示编辑文件
   - Nixpkgs: [maa-assistant-arknights](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)

2. 进入 `./MAA-v{版本号}-linux-{架构}/Python/` 目录下打开 `sample.py` 文件

::: tip
预编译的版本包含在相对较新的 Linux 发行版 (Ubuntu 22.04) 中编译的动态库，如果您系统中的 libstdc++ 版本较老，可能遇到 ABI 不兼容的问题
可以参考 [Linux 编译教程](../../develop/linux-tutorial.md) 重新编译或使用容器运行
:::

#### 2. ADB 配置

1. 找到 [`if asst.connect('adb.exe', '127.0.0.1:5554'):`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/b4fc3528decd6777441a8aca684c22d35d2b2574/src/Python/sample.py#L62) 一栏

2. ADB 工具调用

   - 如果模拟器使用 `Android Studio` 的 `avd` ，其自带 ADB 。可以直接在 `adb.exe` 一栏填写 ADB 路径，一般在 `$HOME/Android/Sdk/platform-tools/` 里面可以找到，例如：

   ```python
   if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "模拟器的 ADB 地址"):
   ```

   - 如果使用其他模拟器须先下载 ADB ： `$ sudo apt install adb` 后填写路径或利用 `PATH` 环境变量直接填写 `adb` 即可。

3. 模拟器 ADB 路径获取

   - 可以直接使用 ADB 工具： `$ adb路径 devices` ，例如：

   ```shell
   $ /home/foo/Android/Sdk/platform-tools/adb devices
   List of devices attached
   emulator-5554 device
   ```

   - 返回的 `emulator-5554` 就是模拟器的 ADB 地址，覆盖掉 `127.0.0.1:5555` ，例如：

   ```python
   if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "emulator-5554"):
   ```

4. 这时候可以测试下： `$ python3 sample.py` ，如果返回 `连接成功` 则基本成功了。

#### 3. 任务配置

自定义任务： 根据需要参考 [集成文档](../../protocol/integration.md) 对 `sample.py` 的 [`# 任务及参数请参考 docs/integration.md`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L54) 一栏进行修改

## 模拟器支持

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

必选配置： 16:9 的屏幕分辨率，且分辨率需大于 720p

推荐配置： x86_64 的框架 (R - 30 - x86_64 - Android 11.0) 配合 MAA 的 Linux x64 动态库

注意：从 Android 10 开始，Minitouch 在 SELinux 为 `Enforcing` 模式时不再可用，请切换至其他触控模式，或将 SELinux **临时**切换为 `Permissive` 模式。

### ⚠️ [Genymotion](https://www.genymotion.com/)

高版本安卓自带 x86_64 框架，轻量但是运行明日方舟时易闪退

暂未严格测试， ADB 功能和路径获取没有问题

## 容器化安卓的支持

::: tip
以下方案通常对内核模块有一定要求, 请根据具体方案和发行版安装合适的内核模块
:::

### ✅ [Waydroid](https://waydro.id/)

安装后需要重新设置分辨率（或者大于 720P 且为 16:9 的分辨率，然后重新启动）：

```shell
waydroid prop set persist.waydroid.width 1280
waydroid prop set persist.waydroid.height 720
```

设置 ADB 的 IP 地址：打开 `设置` - `关于` - `IP地址` ，记录第一个 `IP` ，将 `${记录的IP}:5555` 填入`sample.py` 的 adb IP 一栏。

如果使用 amdgpu, `screencap` 命令可能向 stderr 输出信息导致图片解码失败.
可以运行 `adb exec-out screencap | xxd | head` 并检查输出中是否有类似 `/vendor/etc/hwdata/amdgpu.ids: No such file...` 的文本来确认这一点.
尝试将 `resource/config.json` 中的截图命令由 `adb exec-out screencap` 改为 `adb exec-out 'screencap 2>/dev/null'`.

### ✅ [redroid](https://github.com/remote-android/redroid-doc)

安卓 11 版本的镜像可正常运行游戏, 需要暴露 5555 ADB 端口.
