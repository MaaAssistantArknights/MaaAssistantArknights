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
    sudo apt install gcc-12 g++-12 cmake zlib1g-dev
    ```

    - Arch Linux

        ```bash
        sudo pacman -S --needed gcc cmake zlib
        ```

2. Build third-party libraries

    You can choose to download pre-built dependency libraries or compile from scratch

    - Download pre-built third-party libraries (recommended)

        > **Note**
        > Contains dynamic libraries compiled on relatively new Linux distributions (Ubuntu 22.04). If your system's libstdc++ version is older, you may encounter ABI incompatibility issues.

        ```bash
        python maadeps-download.py
        ```

    If you find the libraries downloaded above cannot run on your system due to ABI version issues and you don't want to use container solutions, you can also try compiling from scratch

    - Build third-party libraries from scratch (will take considerable time)

        ```bash
        git submodule update --init --recursive
        cd MaaDeps
        python build.py
        ```

3. Compile MAA

    ```bash
    CC=gcc-12 CXX=g++-12 cmake -B build \
        -DINSTALL_THIRD_LIBS=ON \
        -DINSTALL_RESOURCE=ON \
        -DINSTALL_PYTHON=ON
    cmake --build build
    ```

    To install MAA to target location, note that MAA is recommended to run by specifying `LD_LIBRARY_PATH`, don't use administrator privileges to install MAA into `/usr`

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
