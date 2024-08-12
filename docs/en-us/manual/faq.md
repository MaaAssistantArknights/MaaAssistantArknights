---
order: 4
icon: ph:question-fill
---

# FAQs

::: warning
MAA has been updated to .NET 8 in version 5.0. For end users, the impact is as follows:

1. MAA now requires the .NET 8 runtime library, which will automatically prompt the user to install it when starting. If the installation fails, please read the following and download the installation package to install manually.
2. MAA will no longer be falsely reported by Windows Defender.
3. .NET 8 does not support Windows 7/8/8.1<sup>[Src](https://github.com/dotnet/core/issues/7556)</sup>, so MAA is also no longer supported. The last available .NET 8 version is [`v5.4.0-beta.1.d035.gd2e5001e7`](https://github.com/MaaAssistantArknights/MaaRelease/releases/tag/v5.4.0-beta.1.d035.gd2e5001e7); the last available .NET 4.8 version is [`v4.28.8`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/tag/v4.28.8). The feasibility of self-compilation has not yet been determined.
4. When running .NET 8 applications on Windows 7, an abnormal memory usage problem will occur. Please refer to the Windows 7 section below to implement mitigation measures. Windows 8/8.1 has not been tested. If you have the same problem, please send an Issue to remind us to supplement the documentation.
:::

## The program crashes immediately when I try to run it

### Incomplete file downloaded

- If you don't have a complete package of this software already, please DO NOT download the zip files marked with `OTA` in the file name, which are for incremental updates and shall not be used alone. In most cases, Windows users should download `MAA-vX.X.X-win-x64.zip`.
- In most cases, you need x64 operating system and x64 variant of MAA, i.e. `MAA-*-win-x64.zip`. There is no support for 32-bit (x86) operating systems.
- If the application does not run properly after an automatic update, it may be due to some bugs within the auto-updater. Please try reinstalling the application and migrating `config` directory from the old install to the new install.

### Missing runtime libraries

Only official sources are listed here. We can't guarantee whether some random third-party all-in-one pack can work.

- Please try installing [Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) and [.NET Desktop Runtime 8](https://dotnet.microsoft.com/en-us/download/dotnet/8.0#:~:text=Binaries-,Windows,-Arm64), then restart computer.
  Windows 10 or 11 users can also install using Winget by running the following command in the terminal.

  ```sh
  winget install Microsoft.VCRedist.2015+.x64 Microsoft.DotNet.DesktopRuntime.8
  ```

- If MAA cannot run after an update, it may be caused by the runtime too. You can also try to install or update the runtime again.

#### Notes on Windows N/KN

- If you are using Windows 8/8.1/10/11 N/KN (Europe/Korea)editions, you also need [Media Feature Pack](https://support.microsoft.com/en-us/topic/c1c6fffa-d052-8338-7a79-a4bb980a700a)。

#### Notes on Windows 7

- If you are using Windows 7, you need to check the following updates before installing runtime libraries：

  1. [Windows 7 Service Pack 1](https://support.microsoft.com/en-us/windows/b3da2c0f-cdb6-0572-8596-bab972897f61)
  2. SHA-2 code-signing update：
     - KB4474419：[link 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu), [link 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/09/windows6.1-kb4474419-v3-x64_b5614c6cea5cb4e198717789633dca16308ef79c.msu)
     - KB4490628：[link 1](https://catalog.s.download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu), [link 2](http://download.windowsupdate.com/c/msdownload/update/software/secu/2019/03/windows6.1-kb4490628-x64_d3de52d6987f7c8bdc2c015dca69eac96047c76e.msu)
  3. Platform Update for Windows 7 (DXGI 1.2 & Direct3D 11.1, KB2670838)：[link 1](https://catalog.s.download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu), [link 2](http://download.windowsupdate.com/msdownload/update/software/ftpk/2013/02/windows6.1-kb2670838-x64_9f667ff60e80b64cbed2774681302baeaf0fc6a6.msu)

##### Mitigation measures for .NET 8 applications running abnormally on Windows 7 [#8238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8238)

1. Open `Computer`, right-click a blank space, click Properties, click `Advanced System Settings` on the left, and click `Environment Variables`.
2. Create a new system variable with variable name `DOTNET_EnableWriteXorExecute` and variable value `0`.
3. Restart the computer.

We cannot guarantee compatibility of future versions with Windows 7, ~~it's all Microsoft's fault~~.

### System broken

- Installation of runtime libraries above requires the Component-Based Servicing (CBS) infrastructure (i.e. TrustedInstaller/TiWorker, WinSxS). Installation may fail if CBS is broken.

- We have no suggestion other than reinstalling Windows. Please avoid using so-called "lite" editions, or some old versions of Windows from thousands of years ago (e.g. 1809).

## Connection Error

- Tips: please refer to the [List of the Supported Emulators](./device/) section to ensure that the emulator you are using is officially supported and that your configuration is correct.
- If you are using software such as a game accelerator, please close the software, RESTART your computer and try again.
- Please check your decompression software - in some cases, using uncommon software such as `7z` or _other niche decompression software_ may cause errors in Minitouch-related files.

### Make sure ADB and address are correct

Check [Connection](./connection.md)

### Change emulator

Change to another emulator, such as [Bluestacks international version](https://www.bluestacks.com/download.html) Nougat 64 bit.

_After installation of Bluestack, you need to enable `Android Debug Bridge` in the settings._

### Close extra adb processes

- Close MAA and check for any adb processes in the task manager. If there are any, please close them and retry.

### Restart computer

Try restarting your computer.

## Connected successfully, then stuck, not operating at all

The adb version packaged with some emulators is too old and does not support minitouch. Please open MAA with administrator privileges and navigate to `Settings` - `Connection Settings` - `Forced Replace ADB`. (It is recommended to close the emulator and restart MAA before proceeding, otherwise, the replacement may not be successful.)

The emulator will reset the ADB version after updating. If the problems occur again, please repeat the above steps. Or you can try to use the [custom connection](./connection.md) method to solve it once and for all.

If it still doesn't work, please retry after switching to `MaaTouch` from `Minitouch` in `Settings` - `Connection Settings`.

## Connected successfully, but actions are slow or errors are frequent

- The auto-battle requires you to go to the screen with the `Start` button. Please confirm they are not related.
- If you are using a non-CN client, please go to MAA Settings - Start-Up Settings - Select your client version. Not all features are currently supported for non-CN clients, please refer to docs.
- If you are running the auto-I.S. function, please PIN UP the IS theme you want in the game, and select I.S. theme in MAA `Task Settings` - `Auto I.S.`.
- The input method `Adb Input` is naturally slow. It is recommended to use `MaaTouch` or `Minitouch` instead.

### Prompt that the screenshot takes a long time / is too long

- MAA currently supports 3 screenshot methods: `RawByNc`, `RawWithGzip`, and `Encode`. When the average screenshot time of executing a task is >400 / >800, a prompt message will be output (a single task will only be output once)
- `Settings - Connection Settings` will display the minimum/average/maximum time taken for the last 30 screenshots, refreshed every 10 screenshots
- Automatic combat functions (such as I.S.) are greatly affected by the time taken to take screenshots
- This time consumption is unrelated to MAA, but related to computer performance, current usage, or emulator. You can try cleaning up background processes, changing emulators, or upgrading computer configurations.

## The download speed is slow and the mirror site is not accessible

1. Go to [Download](../readme.md) to get the link (non-mirror) you want to download.
2. Find the link to the file you need to download.
3. Right-click it and select `Copy link address`.
4. Paste the link into your browser.
5. Replace the `github.com` in the link with `download.fastgit.org`.
6. Press `Enter` to download.

## Download halfway and prompt "login"/"authentication"

Please use a browser / IDM / FDM and other regular downloaders to download files, **Do NOT fucking use Thunder!**
