---
order: 2
icon: basil:apple-solid
---

# Mac Emulator

## Apple Silicon Chips

### ✅ [PlayCover](https://playcover.io) (The software runs most fluently for its nativity 🚀)

Compatible in beta, plz submit issue marked with `MacOS` when encounter errors.

P.S.: For the reason of `MacOS` mechanism itself, errors might occur when `PlayCover` is minimized and you switch to others windows. The errors of screenshot might crash `MAA`. Reference 👉🏻️[issue](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/4371#issuecomment-1527977512)

1. Requests: MAA version above v4.13.0-rc.1

2. Download and install [fork version of PlayCover](https://github.com/hguandl/PlayCover/releases)

3. Download [decrypted version of Arknights](https://decrypt.day/app/id1454663939), and then install in PlayCover.

4. Right-click Arknights in PlayCover, choose `Setting` - `Jailbreak Bypass`, enable `Enable PlayChain`, `Enable Jailbreak Bypass (Alpha)`, `Insert Introspection Library`, `MaaTools`, then click `OK`.

5. Relaunch Arknights, `[localhost:${port number}]` will appear above, meaning launching is successful.

6. In MAA, click `Settings` - `Connection Settings`, `Minitouch` choose `MacPlayTools`. `Connection Address` filling the content in `[]` .

7. Finish, MAA is able to build connection. If errors occur in image recognition, you could try to set resolution to 1080P in PlayCover.

8. 3-5 do not need to be repeated, you could just launch Arknights after that. While you have to repeat step 2 if Arknights is updated.

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

Compatible

## Intel Chips

::: tip
Due to a lack of manpower for Mac version development, updates are relatively slower. It is recommended to use Mac's built-in multi-system feature to install Windows and use the Windows version of MAA.
:::

### ✅ [Bluestacks-CN 5](https://www.bluestacks.cn/)

Fully compatible. Need to turn on `Settings` - `Engine Settings` - `Allow ADB connection`.

### ✅ [Bluestacks 5](https://www.bluestacks.com/tw/index.html)

Fully compatible. Need to turn on `Settings` - `Advanced` - `Android Debug Bridge`.

### ✅ [Nox](https://www.yeshen.com/)

Fully compatible. MAAX cannot auto-detect adb port now, so you need to fill in adb port `127.0.0.1:62001` in MAA `Setting` - `Connect Setting`. Notice that port is not default `5555` , you can get more info in [Common ADB ports for popular Android emulators](../faq.md#common-adb-ports-for-popular-android-emulators)

P.S.: Nox adb bin file path in Mac is `/Applications/NoxAppPlayer.app/Contents/MacOS/adb` , in parent path `MacOS` you are able to use `adb devices` command to get adb path.

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

Compatible.
