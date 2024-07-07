---
order: 2
icon: teenyicons:linux-alt-solid
---
# Linux 編譯教學

**本教程需要讀者有一定的 Linux 環境配置能力及程式設計基礎！**，若您僅希望直接安裝MAA而非自行編譯，請閱讀[使用者手冊 - Linux 模擬器與容器](../manual/device/linux.md)。

::: info 注意
MAA 的構建方法仍在討論中, 本教程的內容可能過時, 請以 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134) 中的指令碼為準。也可參考 [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights)、[nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)。
:::

## 編譯過程

1. 下載編譯所需的依賴

    - Ubuntu/Debian

    ```bash
    sudo apt install gcc-12 g++-12 cmake zlib1g-dev
    ```

2. 構建第三方庫

    - 下載預構建的第三方庫

        > **Note**
        > 包含在相對較新的 Linux 發行版 (Ubuntu 22.04) 中編譯的動態庫，如果您系統中的 libstdc++ 版本較老，可能遇到 ABI 不兼容的問題。

        ```bash
        python maadeps-download.py
        ```

    - 自行構建第三方庫

        ```bash
        git submodule update --init --recursive
        python maadeps-build.py
        ```

3. 編譯 MAA

    ```bash
    mkdir -p build
    CC=gcc-12 CXX=g++-12 cmake -B build \
        -DINSTALL_THIRD_LIBS=ON \
        -DINSTALL_RESOURCE=ON \
        -DINSTALL_PYTHON=ON
    cmake --build build
    ```

    來將 MAA 安裝到目標位置，注意 MAA 推薦通過指定 `LD_LIBRARY_PATH` 來執行，不要使用以系統管理員身分將 MAA 裝入 `/`

    ```bash
    cmake --install build --prefix <target_directory>
    ```

## 集成文件

[~~或許算不上文件~~](3.1-集成文件.md)

### Python

可參考 [Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py) 中 `__main__` 的實現

### C++

可參考 [CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp) 中的實現

### C Sharp

可參考 [MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Main/AsstProxy.cs) 中的實現
