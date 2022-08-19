## Emulator Supports

### ✅ [Bluestacks-CN](https://www.bluestacks.cn/)

Fully compatible. Need to turn on `Settings` - `Engine Settings` - `Allow ADB connection`.

### ✅ [Bluestacks](https://www.bluestacks.com/) (Recommended 👍)

Fully compatible. Need to turn on `Settings` - `Advanced` - `Android Debug Bridge`.

### ✅ [Bluestacks Hyper-V Version](https://support.bluestacks.com/hc/en-us/articles/4415238471053-System-requirements-for-BlueStacks-5-on-Hyper-V-enabled-Windows-10-and-11-)

Compatible.

- Turn on `Settings` - `Advanced` - `Android Debug Bridge`.
- Bluestack Hyper-V port changes frequently. Here is a simple hack:

  1. Find `bluestacks.conf` in the data location of the emulator. (Default is `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`)
  2. If you are using MAA for the first time, Launch it, which generates `gui.json`.
  3. **Exit** MAA, **then** open the `gui.json`, and add a new field `Bluestacks.Config.Path`, with the value of the full path of `bluestacks.conf` (backslashes should be escaped like `\\`).
  For example: (suppose the file is at `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`) 

    ```jsonc
    "Bluestacks.Config.Path":"C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf",
    ```

  4. LinkStart!

- If you need to run multiple emulators (ignore this step if you do not need to do so), you can change the keywords for MAA to detect configuration files.
    Add field `Bluestacks.Config.Keyword` following the steps above
    Example:

    ```jsonc
    "Bluestacks.Config.Keyword":"bst.instance.Nougat64.status.adb_port",
    ```

### ✅ [Nox](https://en.bignox.com/)

Fully compatible.

### ✅ [Nox Android 9](https://en.bignox.com/)

Fully compatible.

### ⚠️ [MuMu](https://www.mumuglobal.com/)

Compatible but:

- Requires MAA to "Run as Administrator" to get ADB path and address (since MuMu runs as admin).
- You can also fill in the ADB path and address if you do not wish to run as admin.
- It has a chance that MAA may stuck at the main screen and prompt mission failed, which is probably related to the rendering method of MuMu. Recommend to change other emulator.

### 🚫 MuMu Mobile Game Assistant  

Incompatible. ADB port is not open.

### 🚫 MuMu Android 9

Incompatible. ADB screenshot is black.

### 🚫 Leidian

Compatible but not recommended. There is a small chance that the game will be displayed upside down or blackscreen, and with other problems.
For example, when it clicks the "Back" button at the top left corner, the screen rotated 180°, causing the application to click the button of using Originium by mistake (reported by users).

### ⚠️ [Memu](https://www.memuplay.com)

Compatible, but some recognition error may occur.

### 🚫 Tencent Mobile Game Assistant

Incompatible. ADB port is not open.

### ⚠️ [Win11 WSA](https://docs.microsoft.com/en-us/windows/android/wsa/)

Partially compatible.

- Need to connect with [Custom Connection](#⚙️ Custom Connection).
- For WSA 2204 or higher (version is in the `About` window of system settings), try `General Configuration` to connect.
- For WSA 2203 or older (version is in the top of the system settings window), try `Legacy WSA` to connect.
- Since WSA does not support changing resolution, please resize the window manually because this program supports 720p or higher `16:9` resolution better. (Or you can simply maximize the window with `F11` if your monitor is 16:9.)
- Please ensure that your emulator is at the top of other windows in most of the time and there are no other android applications running. Otherwise the game may pause or the recognition may fail.
- Sometimes the screenshot of WSA may be blank, causing recognition failure. So it is not recommended to use WSA.

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

Compatible.

### ⚙️ Custom Connection

1. Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip.
2. Go to `Settings` - `Connection Settings`, and fill in ADB path and address (IP + port are required, e.g. `127.0.0.1:5555`)

### ⚙️ Non-`16:9` devices like smartphones or tablets

You may need to change the resolution manually since MAA supports only `16:9` resolution.

1. Turn on USB debugging mode and connect your device to the computer with a cable, or debug with ADB remotely.
2. Run `Command Prompt (CMD)`, check the device address and connect.

    - Use the following command to check device ID if you are using USB cable to connect:

    ```bash
    adb devices                          # Checks the connection status of the current device, with the first column to be the device ID
    ```

    - If you are using remote ADB connection: go to `Settings -> WLAN -> View` to find the corresponding IP address, and the port will usually be 5555 or 5037.

    ```bash
    adb connect <IP Address + Port>        # E.g. 192.168.0.10:5555
    ```

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
