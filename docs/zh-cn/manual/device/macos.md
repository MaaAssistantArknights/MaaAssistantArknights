---
order: 2
icon: basil:apple-solid
---

# Mac 模拟器

## Apple Silicon 芯片

### ✅ [PlayCover](https://playcover.io)（原生运行最流畅 🚀）

试验性支持，遇到问题请多多提 Issue，并在标题中提及 `macOS`。

注意：由于 `macOS` 本身机制的问题，将游戏窗口最小化、台前调度状态下切换到别的窗口、将窗口移动到别的桌面/屏幕之后，截图会出现问题，导致无法正确运行。相关 Issue [#4371](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/4371#issuecomment-1527977512)

0. 要求：MAA 版本 v4.13.0-rc.1 以上

1. 下载 [fork 版本的 PlayCover](https://github.com/hguandl/PlayCover/releases) 并安装。

2. 下载 [脱壳的明日方舟客户端安装包](https://decrypt.day/app/id1454663939)，并在 PlayCover 中安装。

3. 在 PlayCover 中右键明日方舟，选择 `设置` - `绕过`，勾选 `启用 PlayChain`、`启用绕过越狱检测`、`插入内省库`、`MaaTools`，然后点击 `好`。

4. 此时再启动明日方舟，即可正常运行。标题栏结尾会有 `[localhost:端口号]`，说明已经成功启用。

5. 在 MAA 中，点击 `设置` - `连接设置`，`触控模式` 选择 `MacPlayTools`。`连接地址` 填入上面标题栏 `[]` 里的内容。

6. 设置完成，MAA 可以连接了。如果遇到图像识别出错，可以尝试在 PlayCover 内将分辨率设置为 1080P。

7. 3-5 步骤只需要做一次，之后只需要启动明日方舟即可。在明日方舟每次更新客户端之后，需要重新做第 2 步。

### ✅ [MuMu 模拟器 Pro](https://mumu.163.com/mac/)

支持，但测试较少，需使用除 `MacPlayTools` 以外的触控模式。相关 Issue [#8098](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8098)

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

支持，但从 Android 10 开始，Minitouch 在 SELinux 为 `Enforcing` 模式时不再可用。请切换至其他触控模式，或将 SELinux **临时**切换为 `Permissive` 模式。

### ✅ [蓝叠模拟器air版](https://www.bluestacks.com/mac) （免费，针对 Apple M 系列芯片进行优化的版本）

支持，经测试，可用 maatouch 经`127.0.0.1:5555`连接。

需要在模拟器 `设置` - `高级` 中打开 `Android调试（ADB）`。

## Intel 芯片

::: tip
由于 Mac 版开发缺少人手，更新速度相对较慢，更推荐使用 Mac 自带的多系统安装 Windows，并使用 Windows 版 MAA。
:::

### ✅ [蓝叠模拟器](https://www.bluestacks.cn/)

完美支持。需要在模拟器 `设置` - `引擎设置` 中打开 `允许ADB连接`。

### ✅ [蓝叠模拟器国际版](https://www.bluestacks.com/tw/index.html)

完美支持。需要在模拟器 `设定` - `进阶` 中打开 `Android调试桥`。

### ✅ [夜神模拟器](https://www.yeshen.com/)

完美支持。

补充：mac 下夜神模拟器的 ADB 二进制文件的位置为 `/Applications/NoxAppPlayer.app/Contents/MacOS/adb` ，在父目录 `MacOS` 下可使用 `adb devices` 命令查看 ADB 端口。

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

支持，但从 Android 10 开始，Minitouch 在 SELinux 为 `Enforcing` 模式时不再可用。请切换至其他触控模式，或将 SELinux **临时**切换为 `Permissive` 模式。
