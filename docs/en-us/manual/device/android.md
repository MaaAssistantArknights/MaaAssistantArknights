---
order: 4
icon: mingcute:android-fill
---

# Android Physical Device

::: info Note

0. Please also check [Connection](../connection.md).
1. Starting from Android 10, Minitouch is no longer available when SELinux is in `Enforcing` mode, please switch to other touch modes or switch SELinux **temporary** to `Permissive` mode.
2. Due to the extreme complexity of the Android ecosystem, try to change the `Connection Configuration` in MAA `Settings` - `Connection` to `General Mode` or `Compatibility Mode` or `2nd Resolution` or `General Mode (Blocked exception output)` until one of the modes is working properly.
3. Since MAA only supports `16:9` ratio resolutions, devices with non-`16:9` or `9:16` screen ratios will need to be forced to change their resolution, which includes most modern devices. If the connected device has a native `16:9` or `9:16` screen resolution ratio, you can skip the `Change resolution' section. 4.
4. Switch your device's navigation method to something other than `Full Screen Gestures`, such as the `Classic Navigation Keys` to avoid misuse.
5. Please adjust the `Shaped Screen UI Adaptation` section in the in-game settings to 0 to avoid task errors.
:::

::: tip
Typical `16:9` ratio resolutions are `3840*2160` (4K), `2560*1440` (2K), `1920*1080` (1080P), `1280*720` (720P).
:::

## Download, run adb debugging tool and connect the device

1. Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it.
2. Open the extracted folder, clear the address bar and type `cmd` and enter. 3.
3. Type `adb` in the command prompt window that pops up, if it gives you a lot of help text in English, it will run successfully.
4. Turn on `USB debugging` on your mobile phone, this may be different for each brand of mobile phone, so please use a search engine. Manufacturers may provide additional options for USB debugging, such as `USB Installation` and `USB Debugging (Security Settings)` in MIUI, please enable them at the same time.
5. Connect your phone to your computer via data cable and enter the following command in the command prompt window you just got.

   ```bash
   adb devices
   ```

- If executed successfully, it will give you a message that the ``USB debugging`` device is connected.

  - Example of a successful connection:

    ```bash
    List of devices attached
    VFNDU1682100xxxx device
    ```

  - **The alphanumeric combination in front of `device` is the serial number of the device, which also serves as the `connection address` of the MAA.**

- For modern Android devices to perform `USB debugging`, you need to click the pop-up window on the debugged device to authorise it, if not authorised, the example is as follows:

  ```bash
  List of devices attached
  VFNDU1682100xxxx unauthorized
  Unauthorised
  ```

- If you are prompted for unauthorised or the serial number of the device shows `offline`, you need to reboot the device and computer and try again. If the problem is still not solved, you can delete the `.android` folder under the current user's personal folder and reboot again to retry, please search for the exact location.

## Change resolution

::: tip
Mobile phone screen resolution is `short side*long side`, not `long side*short side` of computer monitor. Please determine the exact value according to your target device.
:::

- If there is only one device within the device list above, you can change/restore the resolution directly by running the following command.

  ```bash
  # View current resolution
  adb shell wm size
  # Restore the default resolution
  adb shell wm size reset

  # Change resolution to 720p
  adb shell wm size 720x1280
  # Change resolution to 1080p
  adb shell wm size 1080x1920
  ```

- If there are multiple devices, you need to add the parameter `-s <target device serial number>` between `adb` and `shell`, as shown in the following example.

  ```bash
  # View current resolution
  adb -s VFNDU1682100xxxx shell wm size
  # Restore the default resolution
  adb -s VFNDU1682100xxxx shell wm size reset

  # Change resolution to 720p
  adb -s VFNDU1682100xxxx shell wm size 720x1280
  # Change resolution to 1080p
  adb -s VFNDU1682100xxxx shell wm size 1080x1920
  ```

- Some applications with irregular design may still have wrong layout after restoring the resolution, usually restarting the corresponding application or device can solve the problem.

::: danger Note
It is strongly recommended to restore the resolution **before** restarting the device next time, otherwise it may lead to unpredictable consequences depending on the device, ~~including but not limited to chaotic layout, touch misalignment, application flashback, unlocking, etc~~.
:::

## Automating resolution changes

1. Create two new text files in the MAA directory and fill them with the following contents.

   ```bash
   # Adjust resolution to 1080p
   adb -s <target device serial number> shell wm size 1080x1920
   # Reduce screen brightness (optional)
   adb -s <target device serial number> shell settings put system screen_brightness 1
   ```

   ```bash
   # Restore resolution
   adb -s <target device serial number> shell wm size reset
   # Increase screen brightness (optional)
   adb -s <target device serial number> shell settings put system screen_brightness 20
   # Return to desktop (optional)
   adb -s <target device serial number> shell input keyevent 3
   # Lock screen (optional)
   adb -s <target device serial number> shell input keyevent 26
   ```

2. Rename the first file to `startup.bat` and the second file to `finish.bat`.

   - If you don't see the double confirmation dialogue box for changing extension after renaming, and the file icon doesn't change, please search for "How to display file extensions in Windows".

3. In `Settings` - `Connection` - `Starts with Script` and `Ends with Script` of MAA, fill in `startup.bat` and `finish.bat` respectively.

## Connecting to MAA

### Wired connection

::: tip
A wired connection does not require any IP address or port, only the device serial number given by `adb devices`.
:::

1. Fill in the MAA `Settings` - `Connection` - `Connection address` with the serial number of the target device obtained above.
2. Link Start!

### Wireless Connection

- Make sure that the device and the computer are on the same LAN and can communicate with each other. Settings such as `AP Isolation`, `Guest Network`, etc. can prevent communication between devices, please refer to the documentation of the corresponding router.
- Wireless debugging is disabled after a device reboot and needs to be reset.

#### To open a wireless port using `adb tcpip`

1. Enable wireless debugging by entering the following command in the command prompt window you just opened.

   ```bash
   adb tcpip 5555
   # Add the -s parameter to specify the serial number if multiple devices are present.
   ```

2. View the device IP address.

   - Go to `Settings` - `WLAN` on your mobile phone and click on the currently connected wireless network to view the IP address.
   - The IP address is different for different brands of devices, so please find it yourself.

3. Put `<IP>:5555` into MAA `Settings` - `Connection` - `Connection Address`, such as `192.168.1.2:5555`.
4. Link Start!

#### Use `adb pair` to open the wireless port

::: tip
`adb pair` wireless pairing, i.e. connecting after pairing using `wireless debugging` in Developer Options on Android 11 and newer, avoids the need for a wired connection compared to `adb tcpip`.
:::

1. Go to your phone's developer options, tap `Wireless debugging` and turn it on, tap OK, tap `Pair device using pairing code` and don't close the pop-up window that appears until pairing is complete. 2.

2. To perform pairing.

   1. At the command prompt, type `adb pair <IP address and port given in the device pop-up>` and enter.
   2. Type `<six-digit pairing code given in the device pop-up window>` and enter.
   3. The window appears with something like `Successfully paired to <IP:port>` and the pop-up window on the device disappears automatically, and the computer name appears at the bottom in the paired devices.

3. Put the `<IP address and port>` given on the current device's screen into MAA `Settings` - `Connection` - `Connection Address`, e.g. `192.168.1.2:11451`, **must be different from the one you have just put in**.
4. Link Start!

#### Enable the wireless port with root privileges

~~If you have access to root, why do you need to read this document~~

1. Download, install [WADB](https://github.com/RikkaApps/WADB/releases) and grant it root privileges. 2.
2. Open WADB and start wireless adb. 3.
3. Put the IP address and port provided by WADB into MAA `Settings` - `Connection` - `Connection Address`, such as `192.168.1.2:5555`.
4. Link Start!
