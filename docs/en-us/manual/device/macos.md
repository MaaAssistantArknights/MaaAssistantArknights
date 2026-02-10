---
order: 2
icon: basil:apple-solid
---

# macOS Emulators

## Apple Silicon Chips

### ✅ [PlayCover](https://playcover.io) (Runs most fluently as it's native 🚀)

Experimental support. Please submit issues if you encounter problems, and include `macOS` in the title.

Note: Due to macOS system limitations, screenshot issues may occur when minimizing the game window, switching to other windows while in Stage Manager, or moving the window to other desktops/screens. Related issue: [#4371](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/4371#issuecomment-1527977512)

0. Requirements: MAA version v4.13.0-rc.1 or newer

1. Download and install the [forked version of PlayCover](https://github.com/hguandl/PlayCover/releases).

2. Download the [decrypted Arknights client package](https://decrypt.day/app/id1454663939) and install it in PlayCover.

3. Right-click on Arknights in PlayCover, select `Settings` - `Bypasses`, check `Enable PlayChain`, `Enable Jailbreak Detection Bypass`, `Insert Introspection Libraries`, `MaaTools`, then click `OK`.

4. Now launch Arknights, which should run normally. The title bar will end with `[localhost:port]`, indicating successful activation.

5. In MAA, go to `Settings` - `Connection Settings`, set `Touch Mode` to `MacPlayTools`. For `Connection Address`, enter the content inside the `[]` from the title bar.

6. Setup complete! MAA should now connect successfully. If you encounter image recognition errors, try setting the resolution to 1080P in PlayCover.

7. Steps 3-5 only need to be done once. After that, just launch Arknights. After each Arknights client update, you'll need to repeat step 2.

### ✅ [MuMu Emulator Pro](https://mumu.163.com/mac/)

Supported, but less thoroughly tested. Requires using a touch mode other than `MacPlayTools`. Related issue: [#8098](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8098)

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

Supported, but starting from Android 10, Minitouch is no longer available when SELinux is in `Enforcing` mode. Please switch to other touch modes, or **temporarily** switch SELinux to `Permissive` mode.

### ✅ [BlueStacks Air](https://www.bluestacks.com/mac) (Free, optimized for Apple M-series chips)

Supported and tested. Can be connected using MaaTouch via `127.0.0.1:5555`.

You need to enable `Android Debugging (ADB)` in the emulator's `Settings` - `Advanced` section.

## Intel Chips

::: tip
Due to limited development resources for the Mac version, updates are relatively slower. We recommend using Boot Camp to install Windows and using the Windows version of MAA instead.
:::

### ✅ [BlueStacks](https://www.bluestacks.com/)

Fully compatible. You need to enable `Android Debug Bridge` in the emulator's `Settings` - `Advanced`.

### ✅ [Nox Player](https://www.bignox.com/)

Fully compatible.

Note: On macOS, the Nox ADB binary is located at `/Applications/NoxAppPlayer.app/Contents/MacOS/adb`. In the `MacOS` parent directory, you can use the `adb devices` command to check the ADB port.

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

Supported, but starting from Android 10, Minitouch is no longer available when SELinux is in `Enforcing` mode. Please switch to other touch modes, or **temporarily** switch SELinux to `Permissive` mode.
