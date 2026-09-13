---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux Compilation Tutorial

::: warning
**This tutorial requires readers to have some Linux environment configuration ability and programming foundation!** If you just want to run MAA instead of compiling it yourself, please read [User Manual - Linux Emulators and Containers](../manual/device/linux.md).
:::

::: info Note
MAA's build method is still under discussion. The content of this tutorial may be outdated. Please refer to the scripts in [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/.github/workflows/ci.yml#L264#:~:text=ubuntu%3A).  
You can also refer to [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) or [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix).
:::

::: info
Mac can use the `tools/build_macos_universal.zsh` script for compilation. It's recommended to refer to the README.md of the [MaaAssistantArknights/MaaMacGui](https://github.com/MaaAssistantArknights/MaaMacGui) project.
:::

## Compiling MaaCore

:::: steps

1. Download compilation dependencies
   ::: code-tabs
   @tab:active Ubuntu/Debian

   ```bash :no-line-numbers
   sudo apt-get install cmake ninja-build
   ```

   @tab Arch

   ```bash :no-line-numbers
   sudo pacman -S --needed cmake ninja
   ```

   :::

2. Build third-party libraries

   Choose one of the following methods:

   - Download pre-built third-party libraries (recommended)

     ```bash
     python tools/maadeps-download.py
     ```

     ::: info
     Pre-built third-party libraries are cross-compiled using the [MaaLinuxToolchain](https://github.com/MaaXYZ/MaaLinuxToolchain) toolchain and require glibc version as low as 2.31 (Ubuntu 20.04). If you still encounter ABI incompatibility issues, you can use a container or try building the third-party libraries from scratch.
     :::

   - Build third-party libraries from scratch (will take considerable time)

     ```bash
     git clone https://github.com/MaaAssistantArknights/MaaDeps
     cd MaaDeps
     # If the system is too old to use our prebuilt llvm 20, please consider using local build enviroment instead of cross compiling.
     # The toolchain config under src/MaaUtils/MaaDeps/cmake needs to be modified.
     python linux-toolchain-download.py
     python build.py
     ```

3. Compile MAA

   ```bash
   cmake --preset linux-x64 -DINSTALL_RESOURCE=ON -DINSTALL_PYTHON=ON
   cmake --build build
   cmake --install build --prefix <target_directory>
   ```

   The first two commands generate `build/bin/Debug/libMaaCore.so` (and `libMaaUtils.so`), and the third command installs (i.e., copies) the build artifacts to the target directory.

   ::: info About the CMake Options
   `-DINSTALL_RESOURCE=ON` copies the `resource` directory to the installation directory. MaaCore requires the `resource` directory to run.

   `-DINSTALL_PYTHON=ON` copies the Python integration (`src/Python`) to the installation directory. If you do not use it or plan to use it directly from the repository source code, you can omit this option.
   :::

   ::: tip
   It is recommended to run MAA by specifying the dynamic library path or `LD_LIBRARY_PATH`. Do not use root privileges to install MAA into `/usr`.
   :::

4. Compile MaaFramework components

   To debug MaaFwAdbController (MaaFwAdb touch mode) features, you need to [compile the Debug version of MaaFramework yourself](https://maafw.com/docs/4.1-BuildGuide) and place `libMaaAdbControlUnit.so` in the installation directory.

5. Run

   Refer to programming language [APIs](../readme.md#api) and [Using Python](../manual/device/linux.md#using-python) on how to use the MAA dynamic library.

::::

## Compiling MaaWpfGui

::: info Note
MaaWpfGui is built for Windows. Wine is required to run MaaWpfGui on Linux. See [Using Wine](../manual/device/linux.md#using-wine) for details.
:::

:::: steps
1. Prepare `MaaCore.dll`

   MaaWpfGui depends on `MaaCore.dll` (and other DLLs it depends on). Currently, there is no way to cross-compile `MaaCore.dll` for Windows on Linux. While it is possible to copy these DLLs from MAA for Windows, mismatching version numbers will cause problems.

   Therefore, the recommended approach is to compile MaaCore first according to the previous section (without installing), then compile [MaaWineBridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev-v2/src/MaaWineBridge), and place the resulting `MaaCore.dll` into the `build/bin/Debug` directory.

2. Install .NET SDK

   ::: code-tabs
   @tab:active Ubuntu

   ```bash
   sudo apt install dotnet-sdk-10.0
   ```

   @tab Arch

   ```bash
   sudo pacman -S --needed dotnet-sdk
   ```

   :::

   See also [Install .NET on Linux](https://learn.microsoft.com/en-us/dotnet/core/install/linux).

3. Compile MaaWpfGui

   Run in the repository root:

   ```bash
   dotnet build src/MaaWpfGui/MaaWpfGui.csproj -p:Platform=x64
   ```

   Dependencies will be downloaded automatically during the first build. This generates `build/bin/Debug/MAA.exe`.

4. Run

   ```bash
   wine build/bin/Debug/MAA.exe
   ```
::::
