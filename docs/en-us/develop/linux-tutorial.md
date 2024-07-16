---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux Compiling Tutorial

**The tutorial requires some basic knowledge about Linux OS!**
If you just want to install MAA directly instead of compiling it yourself, please read [Emulator Support for Linux](../manual/device/linux.md).

::: info Note
Linux build of MAA is still under discussion, some of the content might be outdated, please follow the script in [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134), also reference [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights), [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)
:::

## Compiling MAA

1. Download build depends

    - Ubuntu/Debian

        ```bash
        sudo apt install gcc-12 g++-12 cmake zlib1g-dev
        ```

    - Arch Linux

        ```bash
        sudo pacman -S --needed gcc cmake zlib
        ```

2. Build or download third-party libraries

   You may either download pre-built dependencies or build them from scratch

    - Download pre-built libraries (recommended)

        > **Note**
        > It contains shared objects built on a relatively new linux distro (Ubuntu 22.04), which may cause ABI incompatibility if you are working on a system with older version of libstdc++.

        ```bash
        python maadeps-download.py
        ```

    If this does not work for you due to ABI issue, and containerization is not an option, you may build them from scratch

    - Build from source

        ```bash
        git submodule update --init --recursive
        cd MaaDeps
       python build.py
        ```

3. Build MAA

    ```bash
    CC=gcc-12 CXX=g++-12 cmake -B build \
        -DINSTALL_THIRD_LIBS=ON \
        -DINSTALL_RESOURCE=ON \
        -DINSTALL_PYTHON=ON
    cmake --build build
    ```

    To install MAA to the target location, note that MAA is recommended to run by specifying `LD_LIBRARY_PATH`, do not use administrator privileges to load MAA into `/`

    ```bash
    cmake --install build --prefix <target_directory>
    ```

## Integration

[~~Maybe not a doc~~](../protocol/integration.md)

### Python

Refer to the implementation of `__main__` in [Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py)

### C

Refer to the implementation of [CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp)

### C Sharp

Refer to the implementation of [MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Main/AsstProxy.cs)
