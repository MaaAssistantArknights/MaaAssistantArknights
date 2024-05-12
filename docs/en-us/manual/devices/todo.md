---
order: 1
icon: ri:windows-fill
---

# Windows Emulator Support TODO

::: tip
If you encounter problems, please refer to the [FAQ](../FAQ.md)
:::

The following emulators are randomly generated in no particular order

<script setup>
import MarkdownIt from ‘markdown-it’
import MarkdownItAnchor from ‘markdown-it-anchor’

const shuffleArray = (array) => {
    for (let i = array.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [array[i], array[j]] = [array[j], array[i]];
    }
    return array;
}

const fullySupport = shuffleArray([
`
### ✅ [Bluestacks 5](https://www.bluestacks.com/)
Perfect Support. Need to turn on \`Allow ADB connections\` in the emulator \`Settings\` - \`Engine Settings\`.
`.
`


- Recommended to download [Offline Installer](https://support.bluestacks.com/hc/en-us/articles/4402611273485-BlueStacks-5-offline-installer) to avoid slow and bundled installation; recommend installing [Android 11](https://support.bluestacks.com/hc/en-us/articles/4402611273485-BlueStacks-5-offline-installer#:~:text=To%20install%20BlueStacks%205%20Android%2011) version; to uninstall it, please use the official [ Uninstall Tool](https://support.bluestacks.com/hc/en-us/articles/360057724751-How-to-uninstall-BlueStacks-5-BlueStacks-X-and-BlueStacks-Services-completely-from-your-PC) to get rid of the residue.
- If the adb port number keeps changing irregularly and is different every time you start it, it may be because your computer has [Hyper-V](https://support.bluestacks.com/hc/en-us/articles/4415238471053-System-requirements-for-BlueStacks-5-on-Hyper-V-enabled-Windows-10-and-11) enabled. MAA will now try to automatically read the port number within the Blue Stacker emulator configuration file, if this does not work/you have a need to multi-open/have more than one emulator kernel installed, please refer to the [Frequently Asked Questions] (... /FAQ.html#Bluestack emulator port number is different every time you start-hyper-v) to make changes. Since Hyper-V runs as administrator, operations that don't involve adb such as auto-shutdown of the emulator, auto-detect connection, etc. also need to run MAA as administrator.
`,’
`

### ✅ [MuMu Emulator 12](https://mumu.163.com/)

Perfectly supported, with additional support for the exclusive Extreme Control Mode.

- The ‘Exit emulator when done’ function may occasionally be abnormal, if you encounter it, please contact MuMu official for feedback;
- If you are using MuMu 12 version 3.5.4 ~ 3.5.7, please disable the ‘Keep alive in the background’ function in MuMu 12 Settings - Others. ‘Keep alive while hanging in the background’ (see [Official Announcement](https://mumu.163.com/help/20230802/35047_1102450.html) for details);
- You need to check the port information of the corresponding instance through the ADB button of MuMu 12 Multiple Opener when you open more than one instance, and change the port number of the connection address in MAA \`Settings\` - \`Connection Settings\` to the corresponding port.

#### MuMu Screenshot Enhanced Mode

You need to use the official MuMu 12 3.8.13 and later versions, and close the background live. Ark Edition and International Edition are not supported at the moment.

##### Connection Settings

1. Settings → Connection Settings, tick \`Enable MuMu Screenshot Enhanced Mode\`. 2.

2. \`MuMu Emulator Path\` Fill in the path to the \`MuMuPlayer-12.0\` folder, e.g. \`C:\`Program Files\\Netease\\MuMuPlayer-12.0\`.

3. \`Instance Number\` Fill in the serial number of the corresponding emulator in MuMu Multiplayer, e.g. \`0\` for Main Multiplayer.

4. \`Instance Screen\` Fill in \`0\`.

##### About background keep alive

It is recommended to turn it off, at this time the instance screen is always \`0\`.

When it is on, the order of MuMu emulator tabs should be the serial number of the instance screen, e.g. \`0\` for emulator desktop, \`1\` for Tomorrow's Ark client.

Adaptation for backend live is very imperfect, there are always all kinds of inexplicable problems, it is very not recommended.

`,
`
### ✅ [LDPlayer Emulator](https://www.ldmnq.com/)

Perfectly supported.

- Raiden 9 is recommended to use version 9.0.57 and above; Raiden 5 is recommended to use version 5.0.67 and above;
- For versions lower than the above, you need to run \`Forced ADB Replacement\` in \`Settings\` - \`Connection\` in order to use efficient touch modes such as Minitouch, MaaTouch, and so on;
`,
`
### ✅ [Nox Emulator](https://www.yeshen.com/)

Perfectly supported, but only tested 7 and 9.
`,
`
### ✅ [MEmu Emulator](https://www.xyaz.cn/)

Perfectly supported, but less tested.
`,
])

const particallySupport = shuffleArray([
`
### ⚠️ [MuMu Emulator 6](https://mumu.163.com/update/win/)

Support has been dropped since MAA v5.1.0 and NetEase has stopped maintaining it on 15-08-2023.

- No longer support auto-detect connection, need to use generic connection configuration and manually configure adb path and connection address;
- Need to run \`Forced Replacement of ADB\` in \`Settings\` - \`Connection\` to use efficient touch modes such as Minitouch, MaaTouch;
- You need to run MAA with administrator privileges to use the ‘Exit Emulator When Done’ function;
- MuMu 6 default resolution is not supported, you need to change it to \`1280x720\`, \`1920x1080\`, \`2560x1440\` and other 16:9 ratio;
- MuMu 6 multi-open uses the same adb port, so it can't support multi-open MuMu 6.
`,
`
### ⚠️ [Windows Subsystem for Android™️](https://learn.microsoft.com/en-us/windows/android/wsa/)

Support has been dropped since MAA v5.2.0 and will be discontinued by Microsoft on 05-03-2025.

- Requires the use of [custom connection](../details.html#Custom connection) is required;
- WSA 2204 or later (the version number is in the \`About\` page of the subsystem settings), select \`Common Configuration\` for the connection configuration;
- WSA 2203 or older (the version number is at the top of the subsystem setup page), for the connection configuration select \`WSA Older Versions\`;
- Since this software only supports 720p or higher \`16:9\` resolution better, please manually drag the window size as close to the 16:9 ratio as possible. (If your monitor is 16:9, you can press \`F11\` to go full screen);
- Please try to make sure that Tomorrow's Ark is in the foreground and no other Android apps are running in the foreground at the same time, otherwise it may cause the game to pause or the task recognition error;
- WSA's screenshots often somehow capture a white screen, resulting in recognition of abnormalities, or not recommended to use.
`,
`,
### ⚠️ [AVD](https://developer.android.com/studio/run/managing-avds)

Theoretical support.

- Starting from Android 10, Minitouch is no longer available when SELinux is in \`Enforcing\` mode, please switch to other touch modes, or switch SELinux **temporary** to \`Permissive\` mode.
- AVD is made for debugging, it is more recommended to use other emulators designed for gaming.
`.
])



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

### ⚙️ Non-`16:9` devices like smartphones or tablets

You may need to change the resolution manually since MAA supports only `16:9` resolution.

1. Turn on USB debugging mode and connect your device to the computer with a cable, or debug with ADB remotely.
2. Run `Command Prompt (CMD)`, check the device address and connect.

    - Use the following command to check device ID if you are using USB cable to connect:

    ```bash
    adb devices                          # Checks the connection status of the current device, with the first column to be the device ID
    ```

    After successful connection, you will see the following messages:

    ```bash
    C:\Users\username>adb devices
    List of devices attached
    VFNDU1682100xxxx        device       # The data before "device" is the device ID
    ```

    - If you are using remote ADB connection: go to `Settings -> WLAN -> View` to find the corresponding IP address, and the port will usually be 5555 or 5037.

    ```bash
    adb connect <IP Address + Port>        # E.g. 192.168.0.10:5555
    ```

    - If it prompts `'adb' is not recognized as an internal or external command`, it is because the environment variable is not configured correctly. You need to give the full path of ADB to run it. For example:

    ```bash
    D:\adb_unzip_path\adb.exe devices          # Or you can simply drag the adb.exe to CMD window and type [SPACE] devices
    ```

    It is suggested that you configure the environment variable for ADB if you need to run it frequently. Please refer to the articles on the Internet about how to configure it for help.

3. Enter the command prompt to proceed

   ```bash
   adb -s <Device ID or IP + Port> shell  # Enters the command prompt of the device
   wm size                               # Checks the resolution of the current device
   wm size 720x1280                      # Changes the resolution to 720p
   ```

4. Fill in the ADB path and the address of your smartphone (device ID or IP + port) in MAA
5. Set the `Special-shaped screen adaptation` to 0 (off) in the game settings.
    Otherwise, your phone UI and click response may be dislocated if you change the resolution with ADB later.
6. Use MAA (≧∇≦)ﾉ
7. Before exiting MAA, reset the resolution of your phone.

   ```bash
   wm size reset                         # Resets resolution
   ```
#### Using `Starts/End with Script` Options

After MAA version 4.13.0, you're able to utilize `Starts/End with Script` options to apply resolution changes on startup and revert the changes on finishing.

Create two script files: `startup.bat` & `finish.bat` in appropriate path and edit them using your favored text editor.

In `startup.bat`:

```bash
adb shell wm size 1080x1920
::If long-time operation is required, add the following line to save the power and protect your screen.
adb shell settings put system screen_brightness 1 
```

In `finish.bat`:

```bash
adb shell wm size reset
::Add the following line to brighten your screen
adb shell settings put system screen_brightness 20
::Add the following line to lock (Command may differ in different device, test it first)
adb shell input keyevent 26
```

Then go to `Settings` - `Connection Settings` and add the paths of files above to `Starts with Script` & `End with Script` options, the script will be executed at corresponding timing. 

