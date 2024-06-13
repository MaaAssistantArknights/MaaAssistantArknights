---
order: 3
icon: mdi:plug
---

# Connection

:::note
For information about physical machines, please also check [Android Physical Devices](./device/android.md).
:::

## ADB Path

:::info Technical Details
Automatic detection uses the emulator's ADB, but sometimes problems occur with automatic detection, then manual settings are required.
`Replace ADB` is to download the ADB provided by Google and then replace it. If you set up Google's ADB yourself, you can do it once and for all.
:::

### Use the ADB provided by the emulator

Go to the simulator installation path. In Windows, when the simulator is running, right-click the process in the Task Manager and click `Open the location of the file`.

There should be an exe file with `adb` in its name in the top-level or lower directory. You can search and select it.

:::details Examples
`adb.exe` `HD-adb.exe` `adb_server.exe` `nox_adb.exe`
:::

### Use ADB provided by Google

[Click](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) to download and unzip, then select `adb.exe`.

We are recommended to extract it directly to the MAA folder, so that you can directly fill in `.\platform-tools\adb.exe` in the ADB path, and move it with MAA.

## Connection Address

::: tip
The connection address of the emulator running on this computer should be `127.0.0.1:<port>` 或 `emulator-<four numbers>`。
:::

### Obtain Port Number

#### Simulator documents and ports for reference

_TODO: replace these Chinese emulators and documents with English ones_

- [Bluestacks 5](https://support.bluestacks.com/hc/zh-tw/articles/360061342631-%E5%A6%82%E4%BD%95%E5%B0%87%E6%82%A8%E7%9A%84%E6%87%89%E7%94%A8%E5%BE%9EBlueStacks-4%E8%BD%89%E7%A7%BB%E5%88%B0BlueStacks-5#%E2%80%9C2%E2%80%9D) `5555`
- [MuMu Pro](https://mumu.163.com/mac/function/20240126/40028_1134600.html) `16384`
- [MuMu 12](https://mumu.163.com/help/20230214/35047_1073151.html) `16384`
- [MuMu 6](https://mumu.163.com/help/20210531/35047_951108.html) `7555`
- [Memu](https://bbs.xyaz.cn/forum.php?mod=viewthread&tid=365537) `21503`
- [Nox](https://support.yeshen.com/zh-CN/qt/ml) `62001`

For other simulators, please check [Zhao Qingqing’s blog](https://www.cnblogs.com/zhaoqingqing/p/15238464.html).

#### Multiple Emulators

- You can view the running multi-open ports in the upper right corner of the MuMu 12 multi-open device.
- The current multi-open ports can be viewed in the Bluestacks 5 emulator settings.
- _TODO & These sentences should not start with "you"_

#### Alternatives

- Method 1: Use the adb command to view the running port directly

  1. Launch **one** emulator and make sure no other Android devices are connected to this computer.
  2. Launch a terminal in the folder where the adb executable is.
  3. Execute the following command.

  ```sh
  # Windows CMD
  adb devices
  # Windows PowerShell
  .\adb devices
  ```

  An example of output:

  ```text
  List of devices attached
  127.0.0.1:<port>   device
  emulator-<four numbers>  device
  ```

  Use `127.0.0.1:<port>` or `emulator-<four numbers>` as the connection address.

- Method 2: Find established adb connections

  1. Do method 1.
  2. Press `Win+S` to open the search bar, type `Resource Monitor` and open it.
  3. Switch to the `Network` tab and look for the emulator process name in the name column of `Listening Port`, such as `HD-Player.exe`.
  4. Make a note of all listening ports for the emulator process.
  5. Find `adb.exe` in the name column of `TCP connection`. The port in the remote port column that is consistent with the simulator listening port is the simulator debugging port.

### Automatically Start Multiple Emulators

- If you need to operate multiple emulators simultaneously, you can copy the MAA folder multiple times, and use **different MAAs**, **the same adb.exe**, and **different connection addresses** to connect.
- Taking `BlueStacks International Version` as an example, two ways to start multiple emulators are introduced.

  - Perform multiple operations by attaching commands to `HD-Player.exe`.

    1. Start the corresponding emulator separately.
    2. Open the Task Manager, find the corresponding emulator process, go to the Details tab, right-click the column above, click `Select Columns`, and check `Command Line`.
    3. In the newly added `Command Line` column, find the content after `"...\Bluestacks_nxt\HD-Player.exe"`.
    4. Fill in the found content, similar to `--instance Nougat32`, in `Startup Settings` - `Additional Commands`.

    Note: After the operation is completed, it is recommended to hide the `Command Line` column opened in `Step 2` to prevent freezing.

    - Example:

      ```bash
      Multi-instance 1:
      Emulator Path: C:\Program Files\BlueStacks_nxt\HD-Player.exe
      Additional Commands: --instance Nougat32 --cmd launchApp --package "com.hypergryph.arknights"
      Multi-instance 2:
      Emulator Path: C:\Program Files\BlueStacks_nxt\HD-Player.exe
      Additional Commands: --instance Nougat32_1 --cmd launchApp --package "com.hypergryph.arknights.bilibili"
      ```

      The `--cmd launchApp --package` part starts and automatically runs the specified package name application after startup, which can be changed as needed.

  - Perform multi-instance operation by using the shortcut of emulators or apps.

    1. Open the multi-instance manager and add the corresponding emulator's shortcut.
    2. Fill in the path of the emulator shortcut in `Startup Settings` - `Emulator Path`.

    Note: Some emulators support creating app shortcuts, which can directly launch the emulator and open Arknights with the app shortcut.

    - Example:

      ```bash
      Multi-instance 1:
      Emulator Path: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\Multi-instance 1.lnk
      Multi-instance 2:
      Emulator Path: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\Multi-instance 2 - Arknights.lnk
      ```

    If using `Emulator Path` for multi-instance operation, it is recommended to leave `Additional Commands` in `Startup Settings` empty.

### The port number of BlueStack emulator Hyper-V is different every time it is started

_TODO I can't find the original document_

## Connection Present

You need to select the configuration of the simulator you are using. If it is not in the list, select General Mode. If General Mode is not available please try and select any of the other available presents.

For specific differences, you can read the [source code](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/321347fa6bf1d29648c8ec3eaaa27d10c1245e35/resource/config.json#L68).

### MuMu Screenshot Enhanced Mode

You need to use the official China MuMu 12 3.8.13 and later versions, and close the background keep alive. Ark Edition and Global Edition are not supported now.

1. Settings → Connection Settings, tick `Enable MuMu Screenshot Enhanced Mode`.

2. `MuMu Emulator Path` Fill in the path to the `MuMuPlayer-12.0` folder, e.g. `C:`Program Files\Netease\MuMuPlayer-12.0`.

3. `Instance Number` Fill in the serial number of the corresponding emulator in MuMu Multiplayer, e.g. `0` for Main Multiplayer.

4. `Instance Screen` Fill in `0`.

#### About background keep alive

It is recommended to turn it off, at this time the instance screen is always `0`.

When it is on, the order of MuMu emulator tabs should be the serial number of the instance screen, e.g. `0` for emulator desktop, `1` for Tomorrow's Ark client.

Adaptation for backend live is very imperfect, there are always all kinds of inexplicable problems, it is very not recommended.

## Touch Mode

1. [Minitouch](https://github.com/DeviceFarmer/minitouch)：An Android touch eventer written in C uses `evdev` to provide a Socket interface for external programs to trigger touch events and gestures. Starting with Android 10, Minitouch no longer works when SELinux is `Enforcing`.<sup>[Source](https://github.com/DeviceFarmer/minitouch?tab=readme-ov-file#for-android-10-and-up)</sup>
2. [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)：A Java-based reimplementation of Minitouch by MAA, which uses Android's `InputDevice`, and added additional features. Availability of higher versions of Android is yet to be tested. ~~Help us do some testing~~
3. Adb Input：Directly call ADB to use Android's `input` command to perform touch operations. It has the highest compatibility and the slowest speed.

## ADB Lite

The ADB Client independently implemented by MAA can avoid continuously opening multiple ADB processes and reduce performance loss compared to the original ADB, but some screenshot methods are not available.

It is recommended to enable it, but the specific advantages and disadvantages need feedback. ~~Help us do some testing x2~~
