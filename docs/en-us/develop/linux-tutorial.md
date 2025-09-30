---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux Compilation Tutorial

**This tutorial requires readers to have some Linux environment configuration ability and programming foundation!** If you only want to directly install MAA instead of compiling it yourself, please read [User Manual - Linux Emulators and Containers](../manual/device/linux.md).

::: info Note
MAA's build method is still under discussion. The content of this tutorial may be outdated. Please refer to the scripts in [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134). You can also refer to [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights), [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix).
:::

::: info
Mac can use the `tools/build_macos_universal.zsh` script for compilation. It's recommended to refer to the README.md of the [MaaAssistantArknights/MaaMacGui](https://github.com/MaaAssistantArknights/MaaMacGui) project.
:::

## Compilation Process

1. Download compilation dependencies
   - Ubuntu/Debian

   ```bash
   sudo apt install cmake
   ```

   - Arch Linux

   ```bash
   sudo pacman -S --needed cmake
   ```

2. Build third-party libraries

   You can choose to download pre-built dependency libraries or compile from scratch
   - Download pre-built third-party libraries (recommended)

     > **Note**
     > ~~Contains dynamic libraries compiled on relatively new Linux distributions (Ubuntu 22.04). If your system's libstdc++ version is older, you may encounter ABI incompatibility issues.~~
     > After introducing cross compiling to lower the runtime requirement, only glibc 2.31 (aka. ubuntu 20.04) is required now.

     ```bash
     python tools/maadeps-download.py
     ```

   If you find the libraries downloaded above cannot run on your system due to ABI version issues and you don't want to use container solutions, you can also try compiling from scratch
   - Build third-party libraries from scratch (will take considerable time)

     ```bash
     git clone https://github.com/MaaAssistantArknights/MaaDeps
     cd MaaDeps
     # If the system is too old to use our prebuilt llvm 20, please consider using local build enviroment instead of cross compiling.
     # The toolchain config under MaaDeps/cmake needs to be modified.
     python linux-toolchain-download.py
     python build.py
     ```

3. Compile MAA

   ```bash
   cmake -B build \
       -DINSTALL_RESOURCE=ON \
       -DINSTALL_PYTHON=ON \
       -DCMAKE_TOOLCHAIN_FILE=MaaDeps/cmake/maa-x64-linux-toolchain.cmake
   cmake --build build
   ```

   To install MAA to target location, note that MAA is recommended to run by specifying `LD_LIBRARY_PATH`, don't use administrator privileges to install MAA into `/usr`

   > Now it shall be able to run without specifying `LD_LIBRARY_PATH`

   ```bash
   cmake --install build --prefix <target_directory>
   ```

## Integration Documentation

[~~Perhaps not really documentation~~](../protocol/integration.md)

### Python

You can refer to the implementation of `__main__` in [Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py)

### C++

You can refer to the implementation in [CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp)

### C Sharp

<!-- Do not use C#, MD003/heading-style: Heading style [Expected: atx; Actual: atx_closed] -->

You can refer to the implementation in [MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Main/AsstProxy.cs)
