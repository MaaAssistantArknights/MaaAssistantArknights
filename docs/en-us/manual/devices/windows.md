---
order: 1
icon: ri:windows-fill
---

# Windows Emulator Support

## ✅ [Bluestacks-CN 5](https://www.bluestacks.cn/)

Fully compatible. Need to turn on `Settings` - `Engine Settings` - `Allow ADB connection`.

## ✅ [Bluestacks 5](https://www.bluestacks.com/) (Recommended👍)

Fully compatible. Need to turn on `Settings` - `Advanced` - `Android Debug Bridge`.

## ✅ [Bluestacks 5 Hyper-V Version](https://support.bluestacks.com/hc/en-us/articles/4415238471053-System-requirements-for-BlueStacks-5-on-Hyper-V-enabled-Windows-10-and-11-)

Compatible.

- Turn on `Settings` - `Advanced` - `Android Debug Bridge`.
- Bluestack Hyper-V port changes frequently. Here is a simple hack:

    1. Find `bluestacks.conf` in the data location of the emulator. (Default is `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`)
    2. If you are using MAA for the first time, Launch it, which generates `gui.json`.
    3. **Exit** MAA, **then** open the `gui.json`, and add a new field `Bluestacks.Config.Path`, with the value of the full path of `bluestacks.conf` (backslashes should be escaped like `\\`).
    For example: (suppose the file is at `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`)

        ```json
        {
            "Configurations": {
                "Default": {
                    "Bluestacks.Config.Path":"C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf",
                    // ...
                }
            }
        }
        ```

    4. LinkStart!

- If you need to run multiple emulators (ignore this step if you do not need to do so), you can change the keywords for MAA to detect configuration files.
    Add field `Bluestacks.Config.Keyword` following the steps above
    Example:

    ```json
    "Bluestacks.Config.Keyword":"bst.instance.Nougat64.status.adb_port",
    ```

## ✅ [Nox](https://en.bignox.com/)

Fully compatible.

## ✅ [Nox Android 9](https://en.bignox.com/)

Fully compatible.

## ⚠️ [MuMu](https://www.mumuglobal.com/)

Compatible but:

- Requires MAA to "Run as Administrator" to get ADB path and address (since MuMu runs as admin).
- You can also fill in the ADB path and address if you do not wish to run as admin.
- It is not recommended to use the default odd resolutions of MuMu, it is better to change to mainstream resolutions such as 1280x720, 1920x1080, 2560x1440, etc.
- MuMu uses the same adb port for multi-instance, so multi-instance is not supported.

## 🚫 MuMu Mobile Game Assistant

Incompatible. The ADB port is unavailable.

## ⚠️ [MuMu emulator X (Android 12)](https://www.mumuglobal.com/) (most smooth👍)

Compatible but:

- This emulator is still in the testing phase and it is uncertain whether unknown issues will occur.
- Requires MAA to "Run as Administrator" to get ADB path and address (since MuMu runs as admin).
- You can also fill in the ADB path and address if you do not wish to run as admin.
- It is not recommended to use the default odd resolutions of MuMu, it is better to change to mainstream resolutions such as 1280x720, 1920x1080, 2560x1440, etc.
- MuMu uses the same adb port for multi-instance, so multi-instance is not supported.

## ⚠️ [LDPlayer](https://www.ldplayer.net/)

The recent official update has fixed some issues and provided corresponding support. After some time of testing, we have basically confirmed that it can be used normally.
While using it, please still pay attention to the following points, but overall, the user experience has been greatly improved.

- **For LDPlayer 9, it is recommended to use version 9.0.37 or above; for LDPlayer 5, it is recommended to use version 5.0.44 or above.**
- We cannot guarantee that the LDPlayer emulator can run perfectly on all computers, but we are working hard to optimize the adaptation.
- If you encounter any problems when using the LDPlayer emulator, please update to the latest version of the emulator and try to solve the problem by yourself first. If you encounter difficulties, please feel free to provide feedback, and we will try our best to provide support.
- Note: We welcome feedback on issues related to the LDPlayer emulator, and will try to solve them. We would be very grateful if you could provide relevant code or contributions.

## ✅ [Memu](https://www.memuplay.com)

Compatible, but there are fewer tests and there may be unknown issues

## 🚫 Tencent Mobile Game Assistant

Incompatible. The ADB port is unavailable.

## 🚫 [Google Play Games Beta](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)

Incompatible. The ADB port of [the consumer client](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)is unavailable.

## ⚠️ [Win11 WSA](https://docs.microsoft.com/en-us/windows/android/wsa/)

Partially compatible.

- Need to connect with [Custom Connection](#%EF%B8%8F-custom-connection).
- For WSA 2204 or higher (version is in the `About` window of system settings), try `General Configuration` to connect.
- For WSA 2203 or older (version is in the top of the system settings window), try `Legacy WSA` to connect.
- Since WSA does not support changing resolution, please resize the window manually because this program supports 720p or higher `16:9` resolution better. (Or you can simply maximize the window with `F11` if your monitor is 16:9.)
- Please ensure that your emulator is at the top of other windows in most of the time and there are no other android applications running. Otherwise the game may pause or the recognition may fail.
- Sometimes the screenshot of WSA may be blank, causing recognition failure. So it is not recommended to use WSA.

## ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

Compatible.
