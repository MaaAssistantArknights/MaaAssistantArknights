---
order: 4
icon: mingcute:android-fill
---

# Android Physical Devices

::: warning
This method involves ADB command-line usage, has lower stability, and still requires computer connection. **Not recommended** for beginners.
:::

::: info Note

0. Please also refer to [Connection Settings](../connection.md).
1. Starting from Android 10, Minitouch is no longer available when SELinux is in `Enforcing` mode. Please switch to other touch modes, or **temporarily** switch SELinux to `Permissive` mode.
2. Due to the extreme complexity of the Android ecosystem, try changing the `Connection Configuration` in MAA `Settings` - `Connection Settings` to `General Mode`, `Compatibility Mode`, `2nd Resolution`, or `General Mode (Block Exception Output)` until one mode works properly.
3. Since MAA only supports `16:9` aspect ratios, devices with non-`16:9` or `9:16` screen ratios (including most modern devices) need to have their resolution forcibly changed. If your device's native screen ratio is `16:9` or `9:16`, you can skip the `Change Resolution` section.
4. Switch your device's navigation method to something other than `Full Screen Gestures`, such as `Classic Navigation Keys`, to avoid accidental operations.
5. Please set the `Notched Screen UI Adaptation` option in the game settings to 0 to avoid task errors.
:::

::: tip
Typical `16:9` resolutions include `3840x2160` (4K), `2560x1440` (2K), `1920x1080` (1080P), and `1280x720` (720P).
:::

## Download and Run ADB Debug Tool to Connect Device

1. Download [ADB](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and extract it.
2. Open the extracted folder, clear the address bar, type `cmd`, and press Enter.
3. In the command prompt window that appears, type `adb`. If you see extensive help text, the command ran successfully.
4. Enable `USB Debugging` on your phone. The process varies by manufacturer, so use a search engine to find instructions. Manufacturers may provide additional USB debugging options, such as MIUI's `USB Installation` and `USB Debugging (Security Settings)` - enable these too.
5. Connect your phone to the computer with a data cable and enter the following command in the command prompt:

   ```bash
   adb devices
   ```

- When executed successfully, it will show connected USB debugging devices.

  - Example of a successful connection:

    ```bash
    List of devices attached
    VFNDU1682100xxxx        device
    ```

  - **The alphanumeric combination before `device` is the device serial number, which is also your MAA `Connection Address`.**

- Modern Android devices require authorization on the device itself. Without authorization, you'll see:

  ```bash
  List of devices attached
  VFNDU1682100xxxx        unauthorized
  ```

- If you consistently get "unauthorized" or "offline" status, restart both your device and computer. If that doesn't help, delete the `.android` folder in your user's personal directory and try again after restarting. Use search to find the exact location.

## Change Resolution

::: tip
Mobile screen resolution is specified as `short edge × long edge`, not `long edge × short edge` as with computer monitors. Determine the appropriate values for your specific device.
:::

- If you only have one device in your device list, run these commands to change/restore resolution:

  ```bash
  # View current resolution
  adb shell wm size
  # Restore default resolution
  adb shell wm size reset

  # Change to 720p
  adb shell wm size 720x1280
  # Change to 1080p
  adb shell wm size 1080x1920
  ```

- If you have multiple devices, add the parameter `-s <target device serial number>` between `adb` and `shell`:

  ```bash
  # View current resolution
  adb -s VFNDU1682100xxxx shell wm size
  # Restore default resolution
  adb -s VFNDU1682100xxxx shell wm size reset

  # Change to 720p
  adb -s VFNDU1682100xxxx shell wm size 720x1280
  # Change to 1080p
  adb -s VFNDU1682100xxxx shell wm size 1080x1920
  ```

- Some apps with irregular designs may still have layout issues after resolution restoration. Usually, restarting the app or device resolves this.

::: danger Warning
Strongly recommended to restore the default resolution **before** restarting your device. Otherwise, depending on your device, unpredictable consequences may occur, ~~including layout chaos, touch misalignment, app crashes, inability to unlock, etc~~.
:::

## Automate Resolution Changes

1. Create two text files in the MAA directory with the following content:

   ```bash
   # Adjust resolution to 1080p
   adb -s <target device serial number> shell wm size 1080x1920
   # Lower screen brightness (optional)
   adb -s <target device serial number> shell settings put system screen_brightness 1
   ```

   ```bash
   # Restore resolution
   adb -s <target device serial number> shell wm size reset
   # Increase screen brightness (optional)
   adb -s <target device serial number> shell settings put system screen_brightness 20
   # Return to home screen (optional)
   adb -s <target device serial number> shell input keyevent 3
   # Lock screen (optional)
   adb -s <target device serial number> shell input keyevent 26
   ```

2. Rename the first file to `startup.bat` and the second to `finish.bat`.

   - If no confirmation dialog appears when changing the extension and the file icon doesn't change, search for "How to show file extensions in Windows."

3. In MAA's `Settings` - `Connection Settings`, set `Start Script` to `startup.bat` and `End Script` to `finish.bat`.

## Connect to MAA

### Wired Connection

::: tip
Wired connections don't need IP addresses or ports - just the device serial number from `adb devices`.
:::

1. Enter the target device serial number from above into MAA's `Settings` - `Connection Settings` - `Connection Address`.
2. Link Start!

### Wireless Connection

- Ensure your device and computer are on the same network and can communicate with each other. Router settings like `AP Isolation` or `Guest Network` can block device communication - check your router documentation.
- Wireless debugging is disabled after device restart and must be re-enabled.

#### Using `adb tcpip` for Wireless Debugging

1. In the command prompt, enable wireless debugging:

   ```bash
   adb tcpip 5555
   # Add -s parameter to specify the serial number if multiple devices are connected
   ```

2. Find your device's IP address:

   - Go to your phone's `Settings` - `Wi-Fi`, tap the connected network to view the IP address.
   - The location varies by manufacturer - search for instructions if needed.

3. Enter `<IP>:5555` in MAA's `Settings` - `Connection Settings` - `Connection Address`, e.g., `192.168.1.2:5555`.
4. Link Start!

#### Using `adb pair` for Wireless Debugging

::: tip
`adb pair` wireless pairing (available in Android 11 and later via Developer Options) allows connection without a physical USB connection, unlike `adb tcpip`.
:::

1. On your phone, go to Developer Options, tap `Wireless Debugging` and enable it. Tap `Pair device with pairing code` and keep the popup open until pairing completes.

2. Complete the pairing:

   1. In the command prompt, type `adb pair <IP address and port shown in the device popup>` and press Enter.
   2. Enter the six-digit pairing code from the device popup and press Enter.
   3. When you see `Successfully paired to <IP:port>`, the device popup will close automatically, and your computer's name will appear in the paired devices list.

3. Enter the IP address and port shown on your device screen into MAA's `Settings` - `Connection Settings` - `Connection Address`, e.g., `192.168.1.2:11451`. **This is different from the address used for pairing**.
4. Link Start!

#### Using Root to Enable Wireless ADB

~~If you have access to root, why do you need to read this document~~

1. Download, install [WADB](https://github.com/RikkaApps/WADB/releases) and grant it root privileges. 2.
2. Open WADB and start wireless adb. 3.
3. Put the IP address and port provided by WADB into MAA `Settings` - `Connection` - `Connection Address`, such as `192.168.1.2:5555`.
4. Link Start!
