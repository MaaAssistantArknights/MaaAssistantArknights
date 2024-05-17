---
icon: mdi:plug
---

# Custom Connection

- Make sure that MAA `Settings` - `Connection Settings` - `adb path` is automatically filled in. If so, skip to the next step. Otherwise:
  - Option 1: find the installation path of your emulator, where there may be a file named `adb.exe` (or something similar, e.g. `nox_adb.exe`, `HD-adb.exe`, `adb_server.exe`, etc., any EXE files with `adb`). Simply choose the file in the connection settings of MAA!
  - Option 2: download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it. Select the `adb.exe` file.

- Confirm that your connection address is filled in correctly. The ADB address is usually like `127.0.0.1:5555`, depending on the emulators (except Leidian emulator).

## Obtain adb path

- Method 1: Download adb and set up connection manually
  - (Only for Windows users) Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it. It is recommended to unzip to the MAA directory.
  - Go to the "Settings" - "Connection Settings" of the software, select the file path of `adb.exe`, fill in the adb address (IP + port need to be filled in, such as `127.0.0.1:5555`), and select the emulator type.

- Method 2: Find the adb execution port of the emulator and connect
  - For emulators that come with adb, you can find the location of the adb executable file according to the [Confirm adb address section](./faq.md#approach-1-make-sure-adb-and-address-are-correct).
  - Mac Android emulators are generally installed in the `/Application/` directory. Right-click on `[emulator name].app` and select "Show Package Contents". Find the adb executable file in the directory.

## Get the port number

### Common ADB ports for popular Android emulators

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

### Obtain Manually

- Method 1: Use the adb command to view the running port directly

  **Replace adb in the command below with the name of the found adb executable file**, and then execute:

  ```sh
  # mac/linux/windows cmd
  adb devices
  # windows PowerShell
  .\adb devices
  ```

  After most emulator adb execution commands are run, they will be output in the following form, where `[ADBPORT]` is a specific number:

  ```sh
  List of devices attached
        127.0.0.1:[ADBPORT] device
  # may be more output like the above one
  ```

  Use `127.0.0.1:[ADBPORT]` as the connection address (replace `[ADBPORT]` with the actual number).

- Method 2: Use system command to check emulator debugging port

  If the output of the adb command does not display the port information in the form of `127.0.0.1:[port]`, you can use the following method to check:
    1. First, run the adb executable program once (need to start the adb daemon process), and run the Arknights emulator application.
    2. Then use the system command to check the port information of the adb process.

    Windows Command

    You can use `win` + `R` to open the command line by entering `cmd`, and use the following command to check:

    ```sh
    for /F "tokens=1,2" %A in ('"tasklist | findstr "adb""') do ( ^
    netstat -ano | findstr "%B" |)
    ```

    Mac / Linux Command

    Enter the following command in the terminal after starting the emulator:

    ```sh
    tmp=$(sudo ps aux | grep "[a]db" | awk '{print $2}') && \
    sudo netstat -vanp tcp | grep -e "\b$tmp"
    ```

    The output will be in the following format, where `[PID]` is the actual process number of the adb, `[ADBPORT]` is the remote connection port of the target emulator adb, and `[localport]` is the local address of the process, which does not need to be concerned (there will also be other irrelevant indicators in the Mac operation result):

    ```sh
    > ( netstat -ano | findstr "[PID]" )
    TCP  127.0.0.1:[localport]   127.0.0.1:0.0.0.0     LISTENING   [PID]
    TCP  127.0.0.1:[localport]   127.0.0.1:[ADBPORT]   ESTABLISHED [PID]
    # ...
    # maybe more output like the above line
    ```

  Use `127.0.0.1:[ADBPORT]` (replace `[ADBPORT]` with the actual number) in the results as the actual connection address of the emulator adb and fill it in `Settings` - `Connection Settings` - `Connection Address`.
