---
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

### The port number of BlueStack emulator Hyper-V is different every time it is started

_TODO I can't find the original document_

## Connection Present

You need to select the configuration of the simulator you are using. If it is not in the list, select General Mode. If General Mode is not available please try and select any of the other available presents.

For specific differences, you can read the [source code](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/resource/config.json#L68).

## Touch Mode

1. [Minitouch](https://github.com/DeviceFarmer/minitouch)：An Android touch eventer written in C provides a Socket interface for external programs to trigger touch events and gestures. Starting with Android 10, Minitouch no longer works when SELinux is `Enforcing`.
2. [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)：A Java-based reimplementation of Minitouch by MAA. Availability of higher versions of Android is yet to be tested.
3. Adb Input：Directly calling ADB commands for touch operations. It has the highest compatibility and the slowest speed.

## ADB Lite

The ADB Client independently implemented by MAA can avoid continuously opening multiple ADB processes and reduce performance loss compared to the original ADB, but some screenshot methods are not available.

It is recommended to enable it, but the specific advantages and disadvantages need feedback. ~~Help us do the testing plz~~
