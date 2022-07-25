# Frequently Asked Questions

## The program crashes immediately when I try to run it

### Possible cause 0: incomplete file downloaded

- If you are the first time using this software, please DO NOT download the zip file with `OTA` in the file name, which is for incremental update and shall not be used alone.
- If the application does not run properly after an automatic update, there may be some bugs in the automatic update function, you can try to manually download the full version and try again.

### Possible cause 1: missing runtime libraries

 Please try installing [Visual C++ Redistributable](https://docs.microsoft.com/en/cpp/windows/latest-supported-vc-redist?view=msvc-160#visual-studio-2015-2017-2019-and-2022) and [.NET Framework 4.8](https://dotnet.microsoft.com/download/dotnet-framework/net48), and restart your computer and the application.

### Possible cause 2: incompatible CPU instruction set

 This project requires `PaddleOCR` for image recognition, which depends on the `AVX` instruction set supported only by some of the newly-released CPUs, while old versions may not support it.
 
 You can also try downloading `PaddleOCR` of [NoAVX](../3rdparty/ppocr_noavx.zip) version, unzip and replace the files with the same names. Since it may cause performance drop, please do not use it unless it is necessary.

 _([CPU-Z](https://www.cpuid.com/softwares/cpu-z.html) is a tool that provides check of existence of `AVX` instruction set.)_

### Possible cause 3: other runtime library problems

If you are the user of Windows Server or other lite versions, you can try [Microsoft C++ Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) to configure complete development environment (only .NET and C++ environments are required).

**Please note that you may need about 10GB disk space to install these tools. Please make sure that your free space is enough.**

## Connection error

Tips: please refer to the [List of the Supported Emulators](EMULATOR_SUPPORTS.md) section to ensure that you are using your emulator correctly. If you are using Bluestack, please enable `Android Debug Bridge` in the settings.

#### Approach 1: make sure ADB and address are correct

- Make sure that MAA `Settings` - `Connection Settings` - `adb path` is automatically filled in. If so, skip to the next step. Otherwise:
    * Option 1: find the installation path of your emulator, where there may be a file named `adb.exe` (or something similar, e.g. `nox_adb.exe`, `HD-adb.exe`, `adb_server.exe`, etc., any EXE files with `adb`). Simply choose the file in the connection settings of MAA!
    * Option 2: download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it. Select the `adb.exe` file.

- Confirm that your connection address is filled in correctly. The ADB address is usually like `127.0.0.1:5555`, depending on the emulators (except Leidian emulator).

### Approach 2: change emulator

Change to another emulator, such as [Bluestacks international version](https://www.bluestacks.com/download.html) Nougat 64 bit.

_After installation of Bluestack, you need to enable `Android Debug Bridge` in the settings._

### Approach 3: restart computer

Try restarting your computer.

## Wrong recognition/program freezes after starting

Tip 1: The auto battle requires you to go to the screen with the `Start` button. Please confirm they are not related.
Tip 2: Follow the steps below until the problem is solved.

1. Confirm that your emulator is supported in the [List of the Supported Emulators](EMULATOR_SUPPORTS.md).
2. Change the DPI to `320 dpi`.
3. Change the resolution to landscape, `1280 * 720`.
4. Try with another emulator, such as [Bluestacks international version](https://www.bluestacks.com/download.html) Nougat 64 bit. (Please note that you are required to switch on ADB in Bluestack emulator.)
5. Submit an issue to us if the problem still exists.

## Custom Connection

- Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it.
- Go to `Settings` - `Connection Settings`, and select the location of `adb.exe`, fill in the address of ADB (with the format of IP+port, e.g. `127.0.0.1:5555`), and choose the type of your emulator.
