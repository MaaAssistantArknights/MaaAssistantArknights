---
order: 2
icon: 'teenyicons:linux-alt-solid'
---
# Linux Compilation Tutorial

**This tutorial requires readers to have some Linux environment configuration skills and programming foundation!** If you only want to install MAA directly instead of compiling it yourself, please read the [User Manual - Linux Emulator and Container](../manual/device/linux.md).

::: info Note
The build method for MAA is still under discussion. The content of this tutorial may be outdated. Please refer to the scripts in the [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134) for the most up-to-date information.  
You can also refer to the [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) or [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix).
:::

::: info
Mac users can use the `tools/build_macos_universal.zsh` script for compilation.  
It is recommended to refer to the [README.md](https://github.com/MaaAssistantArknights/MaaMacGui/blob/master/README.md) of the MaaAssistantArknights/MaaMacGui project.
:::

## Compilation Process

:::: steps

1. Download the dependencies required for compilation.
   ::: code-tabs
   @tab:active Ubuntu/Debian

   ```bash :no-line-numbers
   sudo apt install cmake
   ```

   @tab Arch

   ```bash :no-line-numbers
   sudo pacman -S --needed cmake
   ```

   :::

2. Build third-party libraries.

   You can choose to download pre-built dependency libraries or compile them from scratch.
   - Download pre-built third-party libraries (Recommended)

     > [!Note]
     > ~~Contains dynamic libraries compiled in relatively new Linux distributions (Ubuntu 22.04). If the libstdc++ version on your system is older, you may encounter ABI compatibility issues.~~  
     > Currently, the runtime environment requirements have been lowered through cross-compilation, only requiring glibc 2.31 (ubuntu 20.04).

     ```bash
     python tools/maadeps-download.py
     ```

   If you find that the libraries downloaded via the above method cannot run on your system due to ABI version or other reasons, and you do not wish to use solutions like containers, you can try compiling from scratch.
   - Build third-party libraries from scratch (Will take a long time)

     ```bash
     git clone https://github.com/MaaAssistantArknights/MaaDeps
     cd MaaDeps
     # If your system environment is too low to use our pre-built llvm 20, consider not using cross-compilation and directly using the local compilation environment.
     # You need to adjust the toolchain configuration in src/MaaUtils/MaaDeps/cmake.
     python linux-toolchain-download.py
     python build.py
     ```

3. Compile MAA.

   ```bash
   cmake -B build \
       -DINSTALL_RESOURCE=ON \
       -DINSTALL_PYTHON=ON \
       -DCMAKE_TOOLCHAIN_FILE=src/MaaUtils/MaaDeps/cmake/maa-x64-linux-toolchain.cmake
   cmake --build build
   ```

   To install MAA to the target location, note that MAA recommends running by specifying `LD_LIBRARY_PATH`. Do not use administrator privileges to install MAA into `/usr`.

   > It should no longer be necessary to specify `LD_LIBRARY_PATH` to run.

   ```bash
   cmake --install build --prefix <target_directory>
   ```

4. Done. You should see the build files in the directory.

::::

## Integration Documentation

[~~Perhaps not quite documentation~~](../protocol/integration.md)

### Python

You can refer to the implementation in the `__main__` function of the [Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py).

### C++

You can refer to the implementation in [CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp).

### C Sharp

<!-- Do not use C#, MD003/heading-style: Heading style [Expected: atx; Actual: atx_closed] -->

You can refer to the implementation in [MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Main/AsstProxy.cs).
