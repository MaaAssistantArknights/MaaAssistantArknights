---
order: 2
icon: ph:question-fill
---

# FAQs

If this is your first time using MAA, please read [Getting Started](./newbie.md).

::: warning

If MAA fails to run after an update, or if you've arrived here from MAA's error window, it's most likely due to outdated runtime libraries.  
The most common issue is related to runtime libraries, and many people keep asking about it without reading the documentation, so we changed the pinned message to this. It's frustrating.

Please run `DependencySetup_依赖库安装.bat` in the MAA directory, or execute the following command in terminal:

```sh
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --force --uninstall-previous --accept-package-agreements && winget install "Microsoft.DotNet.DesktopRuntime.10" --override "/repair /passive /norestart" --force --uninstall-previous --accept-package-agreements
```

Or manually download and install these <u>**two**</u> runtime libraries to solve the problem:

- [Visual C++ Redistributable](https://aka.ms/vc14/vc_redist.x64.exe)
- [.NET Desktop Runtime 10](https://aka.ms/dotnet/10.0/windowsdesktop-runtime-win-x64.exe)

:::

## Software won't run/crashes/shows errors

### Download/Installation issues

- A complete MAA software package is named "MAA-`version`-`platform`-`architecture`.zip"; others are "components" that cannot be used independently. Please read carefully.
  In most cases, you need the x64 architecture of MAA, meaning you should download `MAA-*-win-x64.zip`, not `MAA-*-win-arm64.zip`.
- If you find missing or non-working features after an automatic update, the update process may have had issues. Please download and extract the complete installation package again. After extraction, drag the `config` folder from your old `MAA` folder into the newly extracted `MAA` folder.

### Runtime library issues

Find the up arrow ↑ at the bottom right of the webpage and click it.

### System issues

- MAA doesn't support 32-bit operating systems or Windows 7/8/8.1.
- The runtime installations above require Component Store Service (CBS, TrustedInstaller/TiWorker, WinSxS).
  If these services are damaged, installation will fail.

We cannot provide repair solutions other than reinstalling your system. Please avoid using "slimmed" system versions without clear documentation of what was removed, or extremely outdated systems.

#### Windows N/KN

For Windows N/KN (European/Korean versions), you also need to install the [Media Feature Pack](https://support.microsoft.com/en-us/topic/c1c6fffa-d052-8338-7a79-a4bb980a700a).

#### Windows 7

.NET 10 doesn't support Windows 7/8/8.1 systems<sup>[source](https://github.com/dotnet/core/issues/7556)</sup>, so MAA no longer supports them either. The last usable .NET 8 version is [`v5.4.0-beta.1.d035.gd2e5001e7`](https://github.com/MaaAssistantArknights/MaaRelease/releases/tag/v5.4.0-beta.1.d035.gd2e5001e7); the last usable .NET 4.8 version is [`v4.28.8`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/tag/v4.28.8). Self-compilation feasibility remains undetermined.

For Windows 7, before installing the two runtime libraries mentioned above, check if these patches are installed:

1. [Windows 7 Service Pack 1](https://support.microsoft.com/en-us/windows/b3da2c0f-cdb6-0572-8596-bab972897f61)
2. SHA-2 code signing patches:
   - KB4474419: [Download link 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu), [Download link 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu)
   - KB4490628: [Download link 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu), [Download link 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu)
3. Platform Update for Windows 7 (DXGI 1.2, Direct3D 11.1, KB2670838): [Download link 1](https://catalog.s.download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu), [Download link 2](http://download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu)

##### Workaround for .NET 8 applications running abnormally on Windows 7 [#8238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8238)

When running .NET 8 applications on Windows 7, abnormal memory usage can occur. Follow these steps to mitigate it. Windows 8/8.1 hasn't been tested; if similar issues occur, please open an Issue to remind us to update the documentation.

1. Open `Computer`, right-click in empty space, click Properties, click `Advanced system settings` on the left, and click `Environment Variables`.
2. Create a new system variable with name `DOTNET_EnableWriteXorExecute` and value `0`.
3. Restart your computer.

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
- If auto-combat frequently pauses without deploying operators, disable `Matchstick Mode` in `Settings` - `Operation Settings`.
- If auto-squad formation can't recognize operators correctly, remove special focus from those operators.
- `Adb Input` touch mode is naturally slow; try switching to other modes for functions like auto-combat.
- When using MuMu Emulator, do NOT set the `Graphics Memory Usage Policy` to `Lower Resource Consumption`.

### "Screenshot takes too long" message

- MAA currently supports three ADB-based screenshot methods: `RawByNc`, `RawWithGzip`, and `Encode`. When average screenshot time exceeds 400/800ms, a notification appears once per task.
- `Settings - Connection Settings` displays min/avg/max times for the last 30 screenshots, refreshed every 10 screenshots.
- Auto-combat features (like Integrated Strategy) are heavily affected by screenshot times.
- Screenshot performance is unrelated to MAA but depends on computer performance, background processes, or emulator. Try clearing background tasks, changing emulators, or upgrading your computer.

## Administrator permissions issues

MAA shouldn't need Windows UAC administrator privileges for any functionality. Current admin-related functions include:

1. `Auto-detect connection`: Administrator rights needed when the target emulator runs as administrator.
2. `Close emulator when done`: Administrator rights needed when the target emulator runs as administrator.
3. `Auto-start MAA at boot`: Cannot be set when running as administrator.
4. When MAA is incorrectly extracted to paths requiring admin rights for writing, like `C:\` or `C:\Program Files\`.

Reports indicate that systems with disabled UAC may run applications as administrator even without explicitly selecting "Run as administrator." We recommend enabling UAC to prevent unexpected privilege escalation.

## Download stops midway with "login"/"authentication" prompt

Please use a browser / IDM / FDM or other downloaders to download files, **DO NOT use Thunder!**
