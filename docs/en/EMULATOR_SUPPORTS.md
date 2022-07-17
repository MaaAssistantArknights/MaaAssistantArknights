## Emulator Supports

### ✅ Bluestacks

Fully compatible. Need to turn on `Settings` - `Engine Settings` - `Allow ADB connection`.

### ✅ Bluestacks Global Version (Recommended 👍)

Fully compatible. Need to turn on `Settings` - `Advanced` - `Android Debug Bridge`.

### ✅ Bluestacks Hyper-V Version

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

### ✅ Nox

Fully compatible.

### ✅ Nox Android 9

Fully compatible.

### ⚠️ MuMu

Compatible but:

- Requires MAA to "Run as Administrator" to get ADB path and address (since MuMu runs as admin).
- You can also fill in the ADB path and address if you do not wish to run as admin.
- It has a chance that MAA may stuck at the main screen and prompt mission failed, which is probably related to the rendering method of MuMu. Recommend to change other emulator.

### 🚫 MuMu Mobile Game Assistant  

Incompatible. ADB port is not open.

### 🚫 MuMu Android 9

Incompatible. ADB screenshot is black.

### ⚠️ Leidian

Compatible. But the emulator may not be able to get the resolution. Please change to other emulators if it prompts warning like too low resolution.

### ⚠️ MEmu

Compatible, but some recognition error may occur.

### 🚫 Tencent Mobile Game Assistant

Incompatible. ADB port is not open.

### ⚠️ Win11 WSA

Partially compatible.

- Need to connect with [Custom Connection](#custom-connection).
- For WSA 2204 or higher (version is in the `About` window of system settings), try `General Configuration` to connect.
- For WSA 2203 or older (version is in the top of the system settings window), try `Legacy WSA` to connect.
- Since WSA does not support changing resolution, please resize the window manually because this program supports 720p or higher `16:9` resolution better. (Or you can simply maximize the window with `F11` if your monitor is 16:9.)
- Please ensure that your emulator is at the top of other windows in most of the time and there are no other android applications running. Otherwise the game may pause or the recognition may fail.
- Sometimes the screenshot of WSA may be blank, causing recognition failure. So it is not recommended to use WSA.

### ✅ AVD

Compatible.

### ⚙️ Custom Connection

1. Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip.
2. Go to `Settings` - `Connection Settings`, and fill in ADB path and address (IP+port is required, e.g. `127.0.0.1:5555`)  

**Note:** If your device is non-`16:9` resolution like mobile phone or Android pad, You can set resolution with `adb shell wm size` on your phone and change it back later.
