## Simulator Support

### Bluestack

Fully compatible. Need to turn on `Settings` - `Engine Settings` - `Allow ADB connection`.

### Bluestack International Version (Recommended)

Fully compatible. Need to turn on `Settings` - `Advanced` - `Android Debug Bridge`.

### Bluestack Hyper-V Version

Compatible.

- Turn on `Settings` - `Advanced` - `Android Debug Bridge`.
- Bluestack Hyper-V port changes frequently. Here is a simple hack:
  1. Find `bluestacks.conf` in the installation location of the simulator.
  2. Launch MAA, which generates `gui.json`.  Open it.
  3. Add a new field `Bluestacks.Config.Path`, with the value of the full path of `bluestacks.conf` (backslash should be escaped like `\\`).
  4. LinkStart!

### Nox

Fully compatible.

### Nox Android 9

Fully compatible.

### MuMu

Compatible but:

- Requires MAA to "Run as Administrator" to get ADB path and address (since MuMu runs as admin).
- You can also fill in the ADB path and address if you do not wish to run as admin.
- It has a chance that MAA may stuck at the main screen and prompt mission failed, which is probably related to the rendering method of MuMu. Recommend to change other simulator.

### MuMu Mobile Game Assistant  

Incompatible. ADB port is not open.

### MuMu Android 9

Incompatible. ADB screenshot is black.

### Leidian

Compatible.

But the connection address will change between `127.0.0.1:5555` and `emulator-5554`, varies from device to device, or may change after restarting. Try another when one address is not available.

### Xiaoyao

Compatible, but some recognition error may occur.

### Tencent Mobile Game Assistant

Incompatible. ADB port is not open.

### Win11 WSA

Partially compatible.

- Need to connect with [Custom Connection](#custom-connection). Try `Legacy WSA` or `General Configuration` to connect.
- Since WSA does not support changing resolution, please resize the window manually because this program supports 720p or higher `16:9` resolution better. (Or you can simply maximize the window if your monitor is 16:9.)
- Sometimes the screenshot of WSA may be blank, causing recognition failure. So it is not recommended to use WSA.

### AVD

Compatible.

### Custom Connection

1. Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip.
2. Go to `Settings` - `Connection Settings`, and fill in ADB path and address (IP+port is required, e.g. `127.0.0.1:5555`)  

**Note:** Some features on mobile phone with non-`16:9` resolution is not stable, especially the Infrastructure. We are working in progress to make it compatible. `16:9` is the most recommended resolution since it is tested stable in most of the time.
You can set resolution with `adb shell wm size` on your phone and change it back later.
