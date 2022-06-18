## Frequently Asked Questions

### The program crashes immediately when I try to run it

- Possible solution 0: The downloaded archive is incomplete.
  - If you are the first time using this software, please do not download the compressed package with the word `OTA` in the file name, this is for incremental update and cannot be used alone.
  - If you can't use it after an automatic update, there may be some bugs in the automatic update function, you can try to manually download the complete package and use it again.
- Possible solution 1: you are missing some runtime libraries.
  Please try installing [Visual C++ Redistributable](https://docs.microsoft.com/en/cpp/windows/latest-supported-vc-redist?view=msvc-160#visual-studio-2015-2017-2019-and-2022), [.NET Framework 4.8](https://dotnet.microsoft.com/download/dotnet-framework/net48) and restart the program.
- Possible solution 2: your CPU instruction set is incompatible.
  This project requires `PaddleOCR` to do the image recognition, which requires `AVX` instruction set that is supported by only some of the newly-released CPUs, while others may not support it.
  You can also try downloading `PaddleOCR` [NoAVX](../3rdparty/ppocr_noavx.zip) version, unzip and replace the files with the same names. Since it may cause performance drop, please do not use it unless it is necessary.  
  ([CPU-Z](https://www.cpuid.com/softwares/cpu-z.html) is a tool that provides check of existence of `AVX` instruction set.)  
- If those solutions above do not work, please submit an issue to us.

### Connection error/not knowing how to fill in ADB path

Tips: please refer to the [Usage](../README-en.md#Usage) section to ensure that you are using your simulator correctly.

#### Approach 1

1. Make sure that MAA `Settings` - `Connection Settings` - `adb path` is automatically filled in, if it has been filled in, please ignore this step. If not filled in:

    - Option 1: Find the installation path of your simulator, where there may be a file named `adb.exe` (or something similar, e.g. `nox_adb.exe`, `HD-adb.exe`, `adb_server.exe`, etc., EXE files with `adb`). Simply choose the file in the connection settings of MAA!
    - Option 2: Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it, select the `adb.exe`

2. Confirm that your connection address is filled in correctly, you can Google what the adb address of the emulator you are using is generally in a format like `127.0.0.1:5555`

#### Approach 2

Change another simulator, such as [Bluestacks international version](https://www.bluestacks.com/download.html) Nougat 64 bit

#### Approach 3

Try restarting your computer

### Wrong Recognition/Program freezes after starting

Tip 1: The `Current Stage` of auto battle that costs Sanity requires you to go to the screen with the start button. Please confirm they are not related.
Tip 2: Follow the steps below until the problem is solved.

1. Confirm that your simulator is supported in the [List of the Supported Simulators](List of the Supported Simulators.md).
2. Change the DPI to `320 dpi`.
3. Change the resolution to landscape, `1280 * 720`.
4. Try with another simulator, such as [Bluestacks international version](https://www.bluestacks.com/download.html) Nougat 64 bit. (Please note that you are required to switch on ADB in Bluestack simulator.)
5. Submit an issue to us if the problem still exists.

### Custom Connection

- Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it.
- Go to `Settings` - `Connection Settings`, and select the location of `adb.exe`, fill in the address of ADB (with the format of IP+port, e.g. `127.0.0.1:5555`), and choose the type of your simulator.
