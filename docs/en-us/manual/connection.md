---
order: 3
icon: mdi:plug
---

# Connection Settings

## Auto Detection

MAA can automatically detect and fill in the ADB path, connection address, and connection configuration for a **single currently running emulator**.

As of MAA v5.22.3, the following emulators and connection addresses are supported for detection:

- BlueStacks 5: `127.0.0.1:5555/5556/5565/5575/5585/5595/5554`
- MuMu Player: `127.0.0.1:16384/16416/16448/16480/16512/16544/16576`
- LDPlayer 9: `emulator-5554/5556/5558/5560`, `127.0.0.1:5555/5557/5559/5561`
- NoxPlayer: `127.0.0.1:62001/59865`
- MEmu Play: `127.0.0.1:21503`

If detection fails, try launching MAA with UAC administrator privileges and detect again. If it still fails, refer to the manual setup instructions below and verify that your emulator and connection address are included in the list above.

## ADB Path

:::info Technical details
Auto detection uses the emulator's ADB. Manual configuration is only required if auto detection fails.
`Force Replace ADB` will download Google's ADB and use it automatically.
If ADB is available in the environment variables, just fill in `adb`.
:::

### Use emulator's ADB

Go to your emulator's installation directory. On Windows, you can right-click the process in Task Manager while the emulator is running and select `Open file location`.

Look for an exe file with `adb` in its name in the top-level or subdirectories. You can use search to find it.

:::details Examples
`adb.exe` `HD-adb.exe` `adb_server.exe` `nox_adb.exe`
:::

### Use Google's ADB

[Download here](https://dl.google.com/android/repository/platform-tools-latest-windows.zip), extract it, and select `adb.exe`.

We recommend extracting it directly to the MAA folder so you can enter `.\platform-tools\adb.exe` as the ADB path, allowing it to move with the MAA folder.

## Connection Address

::: tip
Emulators running on your local machine should use addresses like `127.0.0.1:<port number>` or `emulator-<four digits>`.
:::

### Emulator documentation and reference addresses

- [Bluestacks 5](https://support.bluestacks.com/hc/zh-tw/articles/360061342631-%E5%A6%82%E4%BD%95%E5%B0%87%E6%82%A8%E7%9A%84%E6%87%89%E7%94%A8%E5%BE%9EBlueStacks-4%E8%BD%89%E7%A7%BB%E5%88%B0BlueStacks-5#%E2%80%9C2%E2%80%9D): Emulator settings show current multi-instance ports.
- [MuMu Player](https://mumu.163.com/help/20240807/40912_1073151.html?maa): Multi-instance manager shows running ports in the top-right corner.
- [MuMu Player Pro](https://mumu.163.com/mac/function/20240126/40028_1134600.html) `127.0.0.1:16384`
- [LDPlayer 9](https://help.ldmnq.com/docs/LD9adbserver) `emulator-5554`
- [NoxPlayer](https://support.yeshen.com/zh-CN/qt/ml) `127.0.0.1:62001`
- [MEmu Play](https://bbs.xyaz.cn/forum.php?mod=viewthread&tid=365537) `127.0.0.1:21503`

For other emulators, refer to [Zhao Qingqing's blog](https://www.cnblogs.com/zhaoqingqing/p/15238464.html).

::: details Alternative methods

- Method 1: Check emulator ports using ADB commands

  1. Launch **one** emulator and ensure no other Android devices are connected to your computer.
  2. Open a terminal in the folder containing the ADB executable.
  3. Run the following command:

  ```sh
  # Windows command prompt
  adb devices
  # Windows PowerShell
  .\adb devices
  ```

  Example output:

  ```text
  List of devices attached
  127.0.0.1:<port number>   device
  emulator-<four digits>  device
  ```

  Use `127.0.0.1:<port>` or `emulator-<four digits>` as your connection address.

- Method 2: Find established ADB connections

  1. Follow Method 1.
  2. Press `Windows key+S` to open search, type `Resource Monitor` and open it.
  3. Go to the `Network` tab and find the emulator process name in the `Listening Ports` name column, such as `HD-Player.exe`.
  4. Note all listening ports for the emulator process.
  5. In the `TCP Connections` name column, find `adb.exe` and identify the remote port matching the emulator's listening port - this is the emulator's debug port.

:::

### BlueStacks with Hyper-V port changes on every startup

In `Connection Settings`, set `Connection Configuration` to `BlueStacks`, then check both `Auto Detect Connection` and `Re-detect Each Time`.

This should work in most cases. If connection still fails, you may have multiple emulator cores or issues requiring additional settings as explained below.

#### Specifying `Bluestacks.Config.Keyword`

::: info Note
If you're using multi-instance or have multiple emulator cores installed, you'll need additional settings to specify which emulator number to use
:::

In `.\config\gui.json`, search for the `Bluestacks.Config.Keyword` field containing `"bst.instance.<emulator number>.status.adb_port"`. Find the emulator number in the `BlueStacks_nxt\Engine` folder in the emulator path.

::: details Examples
Nougat64 core:

```json
"Bluestacks.Config.Keyword":"bst.instance.Nougat64.status.adb_port",
```

Pie64_2 core: (the number after the core name indicates a multi-instance core)

```json
"Bluestacks.Config.Keyword": "bst.instance.Pie64_2.status.adb_port",
```

:::

#### Specifying `Bluestacks.Config.Path`

::: info Note
MAA now attempts to read the `bluestacks.conf` storage location from the registry. When this fails, you need to manually specify the configuration file path
:::

1. Find the `bluestacks.conf` file in the BlueStacks data directory

   - International version default: `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`
   - Chinese version default: `C:\ProgramData\BlueStacks_nxt_cn\bluestacks.conf`

2. If this is your first time, run MAA once so it automatically generates the configuration file.

3. **First close** MAA, **then** open `gui.json`, find your current configuration name under `Configurations` (viewable in Settings-Switch Configuration, default is `Default`), search for the `Bluestacks.Config.Path` field, and enter the full path to `bluestacks.conf`. (Note: use escaped backslashes `\\`)

::: details Example
For `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`:

```json
{
  "Configurations": {
    "Default": {
      "Bluestacks.Config.Path": "C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf"
      // Other configuration fields, don't manually modify
    }
  }
}
```

:::

## Connection Configuration

Select the configuration matching your emulator. If not listed, choose General Configuration. If that doesn't work, try any other available configuration.

For specific differences, see the [source code](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/resource/config.json#L65).

### MuMu Screenshot Enhanced Mode

Requires official or Arknights-specific MuMu V4.1.26 or newer. <!-- Official V3.8.13 supports initial enhanced screenshots -->

1. In `Settings` - `Connection Settings`, check `Enable MuMu Screenshot Enhanced Mode`. MAA will automatically try to get the installation path from the registry when you check this option.

2. Enter the path to the `MuMu Player` or `MuMuPlayerGlobal-12.0` or `YXArkNights-12.0` folder in `MuMu Installation Path`, e.g., `C:\Program Files\Netease\MuMu Player`.

3. If using MuMu Network Bridge, check `MuMu Network Bridge Mode` and manually enter the number of the corresponding emulator in the MuMu multi-instance manager, such as `0` for the main instance.

### LD Screenshot Enhanced Mode

Requires official or international LDPlayer 9 V9.1.32 or newer. <!-- Official V9.0.78 supports initial enhanced screenshots but has high-resolution issues that V9.1.29 fixes -->

1. In `Settings` - `Connection Settings`, check `Enable LD Screenshot Enhanced Mode`. MAA will automatically try to get the installation path from the registry when you check this option.

2. Enter the path to the `LDPlayer9` folder in `LD Installation Path`, e.g., `C:\leidian\LDPlayer9\`.

3. Enter the number (ID) of the corresponding emulator in the LDPlayer multi-instance manager in `Instance Number`, such as `0` for the main instance.

## Touch Mode

1. [Minitouch](https://github.com/DeviceFarmer/minitouch): An Android touch event handler written in C that operates on `evdev` devices and provides a Socket interface for external programs to trigger touch events and gestures. Starting with Android 10, Minitouch is no longer available when SELinux is in `Enforcing` mode.<sup>[source](https://github.com/DeviceFarmer/minitouch?tab=readme-ov-file#for-android-10-and-up)</sup>
2. [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch): MAA's Java reimplementation of Minitouch that uses Android's native `InputDevice` and adds extra features. Compatibility with newer Android versions is still being tested. ~~Help us test it~~
3. Adb Input: Directly calls ADB to use Android's `input` command for touch operations. Most compatible but slowest.

## ADB Lite

MAA's independent ADB Client implementation that communicates directly with the ADB Server via TCP. Compared to the original ADB, it avoids constantly launching multiple ADB processes, reducing performance overhead, but some screenshot methods aren't available.<sup>[PR](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/3315)</sup>

We recommend enabling it, but specific pros and cons need feedback. ~~Help us test it x2~~

## Running multiple MAA and emulator instances

::: info How to do it
To operate multiple emulators simultaneously, copy the MAA folder multiple times and use **different MAA instances**, **the same adb.exe**, and **different connection addresses** to connect.
:::

### Auto-starting multiple emulator instances

Using [BlueStacks International](./device/windows.md) as an example, here are two ways to auto-start multiple emulator instances:

#### Start via additional commands

1. Launch a single multi-instance emulator.
2. Open Task Manager, find the corresponding emulator process, go to the Details tab, right-click the column header, click `Select Columns`, and check `Command line`.
3. In the new `Command line` column, find the content after `...\Bluestacks_nxt\HD-Player.exe"`.
4. Add the content you found (similar to `--instance Nougat32`) to `Startup Settings` - `Additional Commands`.

::: note
After finishing, we recommend hiding the `Command line` column opened in `Step 2` to prevent slowdowns
:::

::: details Example

```text
Instance 1:
Emulator Path: C:\Program Files\BlueStacks_nxt\HD-Player.exe
Additional Commands: --instance Nougat32 --cmd launchApp --package "com.hypergryph.arknights"
Instance 2:
Emulator Path: C:\Program Files\BlueStacks_nxt\HD-Player.exe
Additional Commands: --instance Nougat32_1 --cmd launchApp --package "com.hypergryph.arknights.bilibili"
```

The `--cmd launchApp --package` part auto-launches the specified package after startup; modify as needed.
:::

#### Start via emulator shortcuts

Some emulators support creating application shortcuts that directly launch the emulator and open Arknights.

1. Open the multi-instance manager and create shortcuts for the corresponding emulators.
2. Enter the emulator shortcut path in `Startup Settings` - `Emulator Path`

::: details Example

```text
Instance 1:
Emulator Path: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\Instance 1.lnk
Instance 2:
Emulator Path: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\Instance 2-Arknights.lnk
```

:::

When using `Emulator Path` for multi-instance operation, we recommend leaving `Startup Settings` -
