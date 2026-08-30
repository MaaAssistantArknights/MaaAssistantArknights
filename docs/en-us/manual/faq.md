---
order: 3
icon: ph:question-fill
---

# FAQs

If this is your first time using MAA, please read [Getting Started](./newbie.md).

::: warning

If MAA fails to run after an update, or if you've arrived here from MAA's error window, it's most likely due to outdated runtime libraries.  
The most common issue is related to runtime libraries, and many people keep asking about it without reading the documentation, so we changed the pinned message to this. It's frustrating.

MAA now uses self-contained deployment with the .NET runtime included, but still requires Visual C++ runtime libraries.

Please run `DependencySetup_依赖库安装.bat` in the MAA directory, or execute the following command in terminal:

```sh
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --force --uninstall-previous --accept-package-agreements
```

Or manually download and install this runtime library to solve the problem:

- [Visual C++ Redistributable](https://aka.ms/vc14/vc_redist.x64.exe)

:::

## Software won't run/crashes/shows errors

### Download/Installation issues

- A complete MAA software package is named "MAA-`version`-`platform`-`architecture`.zip"; others are "components" that cannot be used independently. Please read carefully.
  In most cases, you need the x64 architecture of MAA, meaning you should download `MAA-*-win-x64.zip`, not `MAA-*-win-arm64.zip`.
- If you find missing or non-working features after an automatic update, the update process may have had issues. Please download and extract the complete installation package again. After extraction, copy the `config`, `data`, and `debug` folders from your old `MAA` folder into the newly extracted `MAA` folder to keep your existing settings and logs. The `cache` and `achievement` (screenshots) folders are optional to keep.

### Runtime library issues

Find the up arrow ↑ at the bottom right of the webpage and click it.

### System issues

- MAA doesn't support 32-bit operating systems or Windows 7/8/8.1.
- The runtime installations above require Component Store Service (CBS, TrustedInstaller/TiWorker, WinSxS).
  If these services are damaged, installation will fail.

We cannot provide repair solutions other than reinstalling your system. Please avoid using "slimmed" system versions without clear documentation of what was removed, or extremely outdated systems.

#### Windows N/KN

For Windows N/KN (European/Korean versions), you also need to install the [Media Feature Pack](https://support.microsoft.com/en-us/topic/c1c6fffa-d052-8338-7a79-a4bb980a700a).

#### Windows 7/8/8.1

Due to runtime libraries and system components requiring Windows 10 or above, MAA no longer supports Windows 7/8/8.1 systems.

### Flagged by Windows Defender / antivirus software (PUA / malware)

- First verify the download source. Only use official channels (official website, GitHub Releases, Winget, or official community distribution channels), and make sure you downloaded the full package (for example, `MAA-<version>-win-x64.zip`).
- Automation tools may trigger heuristic detections in some antivirus engines. A detection result does not always mean the program is malicious.
- If the source is trusted, submit a false-positive sample to the security vendor and wait for signature updates.
- While waiting, you can temporarily add the MAA installation directory to antivirus allowlists. Avoid disabling real-time protection entirely.

### "Missing installation files" prompt on startup

On startup, MAA checks installation files against the file list bundled with the installation package (filelist.txt). If this prompt appears, some files have been blocked or removed by security software, or deleted manually, and the corresponding features (such as mini-game tasks) may not work:

- If security software is the cause, restore the removed files from its quarantine area and add the MAA installation directory to its allowlist.
- You can also choose automatic repair in the dialog (re-download the full package and restore after restart), or reinstall manually following the instructions in the dialog.

### DLL injection issues

If MAA shows a compatibility warning on startup, or crashes / displays rendering anomalies, it is usually caused by third-party software (such as Nahimic) injecting DLLs into MAA. You can resolve this with any of the following methods:

**Method 1: Block DLL injection via Windows Security (recommended)**

1. Open **Windows Security** → **App & browser control** → **Exploit protection** → **Exploit protection settings** → **Program settings**
2. Click **Add program to customize**, choose **"Add by program name"** and enter `MAA.exe` (or choose **"Choose exact file path"** and locate `MAA.exe` in the MAA installation directory)
3. Find **"Disable extension points"** in MAA's settings list, check **"Override system settings"** and turn it on
4. Click **"Apply"** to save, then restart MAA

This setting blocks third-party software from injecting DLLs into MAA at the system level, without affecting other programs.

**Method 2: Rename the injected DLL**

1. Locate the corresponding DLL file at the path shown in the warning (if you cannot see file extensions or the file itself, first enable **"File name extensions"** and **"Hidden items"** under **View → Show** at the top of File Explorer)
2. Right-click the file → **Rename**, and change the `.dll` extension to an invalid one such as `.dll1`

::: warning
This method may cause the related software to malfunction. Use it with caution. For more on the root cause, see the [related post](https://t.bilibili.com/1133690423484612615) and [blog](https://blog.walterlv.com/post/wpf-renders-wrong-because-of-nahimicosd.html).
:::

**Method 3: Handle the related software**

Try adding MAA to the exclusion list of the related software, or uninstall the related software and check whether the problem persists.

## Connection errors

### Verify ADB and connection address are correct

See [Connection Settings](./connection.md).

### Close existing ADB processes

After closing MAA, check `Task Manager` - `Details` for processes containing `adb`. If found, end them and retry connecting.

### Properly use multiple ADB instances

When ADB versions differ, newly started processes will close older ones. If you need to run multiple ADB instances simultaneously (e.g., Android Studio, Alas, phone managers), ensure they use the same version.

### Avoid game accelerators

Some accelerators require restarting MAA, ADB, and the emulator after enabling or disabling acceleration.

For using UU Accelerator with MuMu, refer to the [official documentation](https://mumu.163.com/help/20240321/35047_1144608.html).

### Restart your computer

Restarting solves 97% of problems. (Trust me)

### Try a different emulator

Please refer to [Emulator and Device Support](./device/).

## Connected but no operation

Some emulators come with outdated ADB versions that don't support `Minitouch` or `MaaTouch`.

Run MAA as administrator, close the emulator, restart MAA, and click `MAA Settings` - `Connection Settings` - `Force Replace ADB`.

Emulator updates may overwrite the ADB file. If the issue returns after an update, try replacing again or use [alternative ADB](./connection.md#use-adb-provided-by-google).

## Connected but operations are laggy, abnormal, or error-prone

- If you've enabled `Notched Screen UI Adaptation`, set it to 0.
- If using a non-CN client, first select your client version in `Settings` - `Game Settings` - `Client Type`. Some features may not be fully adapted for non-CN clients; refer to the corresponding documentation.
- For Integrated Strategy automation, check the [Auto Integrated Strategy documentation](./introduction/integrated-strategy.md) and correctly select the theme in `Auto Integrated Strategy` - `Integrated Strategy Theme`.
- If auto-combat frequently pauses without deploying operators, disable `Pause Trick` in `Settings` - `Game Settings`.
- If auto-squad formation can't recognize operators correctly, remove special focus from those operators.
- `ADB Input` touch mode is naturally slow; try switching to other modes for functions like auto-combat.
- When using MuMu Emulator, do NOT set the `Graphics Memory Usage Policy` to `Lower Resource Consumption`.

### "Screenshot takes too long" message

- MAA currently supports three ADB-based screenshot methods: `RawByNc`, `RawWithGzip`, and `Encode`. When average screenshot time exceeds 400/800ms, a notification appears once per task.
- `Settings - Connection Settings` displays min/avg/max times for the last 30 screenshots, refreshed every 10 screenshots.
- Auto-combat features (like Integrated Strategy) are heavily affected by screenshot times.
- Screenshot performance is unrelated to MAA but depends on computer performance, background processes, or emulator. Try clearing background tasks, changing emulators, or upgrading your computer.

## Administrator permissions issues

MAA shouldn't need Windows UAC administrator privileges for any functionality. Current admin-related functions include:

1. `Auto-detect connection`: Administrator rights needed when the target emulator runs as administrator.
2. `Exit Emulator` in `Then`: Administrator rights needed when the target emulator runs as administrator.
3. `Auto-start MAA at boot`: Cannot be set when running as administrator.
4. When MAA is incorrectly extracted to paths requiring admin rights for writing, like `C:\` or `C:\Program Files\`.

Reports indicate that systems with disabled UAC may run applications as administrator even without explicitly selecting "Run as administrator." We recommend enabling UAC to prevent unexpected privilege escalation.

## Download stops midway with "login"/"authentication" prompt

Please use a browser / IDM / FDM or other downloaders to download files, **DO NOT use Thunder!**
