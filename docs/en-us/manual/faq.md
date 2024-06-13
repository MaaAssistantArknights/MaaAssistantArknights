---
icon: ph:question-fill
---

# Frequently Asked Questions

::: warning
MAA has been updated to .NET 8 in version 5.0. For end users, the impact is as follows:
1. MAA now requires the .NET 8 runtime library, which will automatically prompt the user to install it when starting. If the installation fails, please read the following and download the installation package to install manually.
2. MAA will no longer be falsely reported by Windows Defender.
3. [.NET 8 does not support Windows 7/8/8.1](https://github.com/dotnet/core/issues/7556), so MAA is also no longer supported, even though it can still run normally.
4. When running MAA on Windows 7, an abnormal memory usage problem occurs. Please refer to the Windows 7 section below to implement mitigation measures. Windows 8/8.1 has not been tested. If you have the same problem, please send an Issue to remind us to supplement the documentation.
:::

## The program crashes immediately when I try to run it

### Possible cause 0: incomplete file downloaded

- If you don't have a complete package of this software already, please DO NOT download the zip files marked with `OTA` in the file name, which are for incremental update and shall not be used alone. In most cases, Windows users should download `MAA-vX.X.X-win-x64.zip`.
- If the application does not run properly after an automatic update, it may be due to some bugs within the autoupdater. Please try reinstalling the application and migrating `config` directory from the old install to the new install.

### Possible cause 1: architecture mismatch

- In most cases, you need x64 operating system and x64 variant of MAA, i.e. `MAA-*-win-x64.zip`. There are no support for 32-bit (x86) operating systems.

### Possible cause 2: missing runtime libraries

> Only official sources listed here. We can't gurantee whether some random third-party all-in-one pack can work.

- Please try installing [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) and [.NET8](https://dotnet.microsoft.com/download/dotnet/thank-you/runtime-desktop-8.0.1-windows-x64-installer), and restart your computer and the application.

#### Notes on Windows N/KN

- If you are using Windows 8/8.1/10/11 N/KN（Europe/Korea）editions，you also need [Media Feature Pack](https://support.microsoft.com/en-us/topic/c1c6fffa-d052-8338-7a79-a4bb980a700a)。

#### Notes on Windows 7

- If you are using Windows 7，you need to check following updates before installing runtime libraries：

  1. [Windows 7 Service Pack 1](https://support.microsoft.com/en-us/windows/b3da2c0f-cdb6-0572-8596-bab972897f61)
  2. SHA-2 code-signing update：
     - KB4474419：[link 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu), [link 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu)
     - KB4490628：[link 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu), [link 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu)
  3. Platform Update for Windows 7 (DXGI 1.2 & Direct3D 11.1, KB2670838)：[link 1](https://catalog.s.download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu), [link 2](http://download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu)

Mitigation measures for .NET 8 applications running abnormally on Windows 7 [#8238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8238)

   1. Open `Computer`, right-click a blank space, click Properties, click `Advanced System Settings` on the left, and click `Environment Variables`.
   2. Create a new system variable with variable name `DOTNET_EnableWriteXorExecute` and variable value `0`.
   3. Restart the computer.

We cannot guarantee compatibility of future versions with Windows 7, ~~it's all Microsoft's fault~~.

#### Alternative approach: install development tools

- Install [Microsoft C++ Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) to configure a complete development environment (only .NET and C++ environments are required).

- **Please note that you may need about 10GB disk space to install these tools, so make sure that your free space is enough.**

### Possible cause 3: broken system

- Installation of runtime libraries above requires the Component-Based Servicing (CBS) infrastructure (i.e. TrustedInstaller/TiWorker, WinSxS). Installation may fail if CBS is broken.

- We have no suggestion other than reinstalling Windows. Please avoid using so-called "lite" editions.

## Connection error

- Tips: please refer to the [List of the Supported Emulators](1.3-EMULATOR_SUPPORTS.md) section to ensure that the emulator you are using is officially supported and that your configuration is correct.
- If you are using softwares such as a game accelerator, please close the softwares, RESTART your computer and try again.
- Please check your decompression software - in some cases, using uncommon software such as `7z` or _other niche decompression software_ may cause errors in Minitouch related files.

### Approach 1: make sure ADB and address are correct

- Make sure that MAA `Settings` - `Connection Settings` - `adb path` is automatically filled in. If so, skip to the next step. Otherwise:
  - Option 1: find the installation path of your emulator, where there may be a file named `adb.exe` (or something similar, e.g. `nox_adb.exe`, `HD-adb.exe`, `adb_server.exe`, etc., any EXE files with `adb`). Simply choose the file in the connection settings of MAA!
  - Option 2: download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it. Select the `adb.exe` file.

- Confirm that your connection address is filled in correctly. The ADB address is usually like `127.0.0.1:5555`, depending on the emulators (except Leidian emulator).

#### Common ADB ports for popular Android emulators

- Single instance / first instance in multi-instance mode

  For single instance mode, please refer to the documentation for each emulator and the blog post by NetEase senior game development engineer @Zhao Qingqing to find the default ADB ports for common Android emulators:

    |Emulator|Default ADB port|
    |-|:-:|
    |NetEase MuMu emulator 6/X|7555|
    |NetEase MuMu emulator 12|16384|
    |NoxPlayer emulator|62001|
    |BlueStacks emulator|5555|
    |LDPlayer emulator 9|5555 / emulator-5554|

    You can connect to emulators with purely numeric ports using `127.0.0.1:[port]`. LDPlayer emulator has its own wrapper, so you can also use `emulator-5554` to connect.

    If you need to modify the connection settings in the `Settings` - `Connection Settings` - `Connection Address` on Windows or Mac, please refer to the table above.

- Multi-instance mode

  - For NoxPlayer emulator, the port of the first device is `62001`, and the ports for subsequent devices start from `62025`.
  - For NetEase MuMu emulator 12, the ADB ports for multi-instance mode are irregular. To find the ADB port for a running emulator, launch the emulator from MuMu Multi-instance Manager 12, then click on the ADB icon in the upper-right corner.
  - For LDPlayer emulator 9, the local ADB port starts from `5555`, and subsequent ports increment by 2. For example, the second emulator has a local port of `5557`.

### Approach 2: change emulator

Change to another emulator, such as [Bluestacks international version](https://www.bluestacks.com/download.html) Nougat 64 bit.

_After installation of Bluestack, you need to enable `Android Debug Bridge` in the settings._

### Approach 3: close extra adb processes

- Close MAA and check for any adb processes in the task manager. If there are any, please close them and retry.

### Approach 4: restart computer

Try restarting your computer.

## Connected successfully, then stuck, not operating at all

The adb version packaged with some emulators is too old and does not support minitouch. Please open MAA with administrator priviledge and navigate to `Settings` - `Connection Settings` - `Forced Replace ADB`. (It is recommended to close the emulator and restart MAA before proceeding, otherwise the replacement may not be sussessful.)

The emulator will reset the ADB version after updating. If the problems occurs again, please repeat the above steps. Or you can try to use the [custom connection](1.1-USER_MANUAL.md#custom-connection) method to solve it once and for all.

If it still doesn't work, please retry after switching to `MaaTouch` from `Minitouch` in `Settings` - `Connection Settings`.

## Connected successfully, but actions are slow or error are frequent

- The auto battle requires you to go to the screen with the `Start` button. Please confirm they are not related.
- If you are using a non-CN client, please go to MAA Settings - Start Up Settings - Select your client version. And not all features are supported for non-CN client, please refer to docs.
- If you are running auto-IS function, please PIN UP the IS theme you want in the game, and select IS theme in MAA `Task Settings` - `Auto I.S.`.
- The input method `Adb Input` is natrually slow. It is recommended to use `MaaTouch` or `Minitouch` instead.

### Prompt that the screenshot takes a long time / is too long

- MAA currently supports 3 screenshot methods: `RawByNc`, `RawWithGzip`, and `Encode`. When the average screenshot time of executing a task is >400 / >800, a prompt message will be output (a single task will only be output once)
- `Settings - Connection Settings` will display the minimum/average/maximum time taken for the last 30 screenshots, refreshed every 10 screenshots
- Automatic combat functions (such as automatic meat pigeons) are greatly affected by the time taken to take screenshots
- This time consumption is unrelated to MAA, but related to computer performance, current usage, or emulator. You can try cleaning up background processes, changing emulators, or upgrading computer configurations.

## The download speed is slow and the mirror site is not accessible

1. Go to [Download](../../README.md#Download) to get the link (non-mirror) to you want to download.
2. Find the link to your file you need to download.
3. Right-click it and select `Copy link address`.
4. Paste the link into your browser.
5. Replace the `github.com` in the link with `download.fastgit.org`.
6. Press `Enter` to download.

## Download halfway and prompt "login"/"authentication"

Please use a browser / IDM / FDM and other regular downloaders to download files, **Do NOT fucking use Thunder!**
