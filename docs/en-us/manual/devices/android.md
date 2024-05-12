---
order: 4
icon: mingcute:android-fill
---

# Android physical device

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

## Using `Starts/End with Script` Options

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
