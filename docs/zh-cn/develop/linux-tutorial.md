---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux 编译教程

**本教程需要读者有一定的 Linux 环境配置能力及编程基础！**，若您仅希望直接安装MAA而非自行编译，请阅读[用户手册 - Linux 模拟器与容器](../manual/device/linux.md)。

::: info 注意
MAA 的构建方法仍在讨论中, 本教程的内容可能过时, 请以 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134) 中的脚本为准  
你也可参考 [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) 或 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)
:::

::: info
Mac 可以使用 `tools/build_macos_universal.zsh` 脚本进行编译  
建议参考 MaaAssistantArknights/MaaMacGui 项目的 [README.md](https://github.com/MaaAssistantArknights/MaaMacGui/blob/master/README.md)
:::

## 编译过程

:::: steps

1. 下载编译所需的依赖
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

2. 构建第三方库

   可以选择下载预构建的依赖库或从头进行编译
   - 下载预构建的第三方库 (推荐)

     > [!Note]
     > ~~包含在相对较新的 Linux 发行版 (Ubuntu 22.04) 中编译的动态库, 如果您系统中的 libstdc++ 版本较老, 可能遇到 ABI 不兼容的问题~~  
     > 目前已经基于交叉编译降低了运行环境, 仅需要依赖 glibc 2.31 (ubuntu 20.04).

     ```bash
     python tools/maadeps-download.py
     ```

   如果您发现上面的方法下载的库由于 ABI 版本等原因无法在您的系统上运行且不希望使用容器等方案, 也可以尝试从头编译
   - 自行构建第三方库 (将花费较长时间)

     ```bash
     git clone https://github.com/MaaAssistantArknights/MaaDeps
     cd MaaDeps
     # 如果系统环境过低无法使用我们预构建的 llvm 20, 请考虑不使用交叉编译, 直接使用本地编译环境.
     # 需要调整 src/MaaUtils/MaaDeps/cmake 中的 toolchain 配置.
     python linux-toolchain-download.py
     python build.py
     ```

3. 编译 MAA

   ```bash
   cmake -B build \
       -DINSTALL_RESOURCE=ON \
       -DINSTALL_PYTHON=ON \
       -DCMAKE_TOOLCHAIN_FILE=src/MaaUtils/MaaDeps/cmake/maa-x64-linux-toolchain.cmake
   cmake --build build
   ```

   来将 MAA 安装到目标位置, 注意 MAA 推荐通过指定 `LD_LIBRARY_PATH` 来运行, 不要使用管理员权限将 MAA 装入 `/usr`

   > 现在应该不需要指定 `LD_LIBRARY_PATH` 即可运行

   ```bash
   cmake --install build --prefix <target_directory>
   ```

4. 结束，你应该能在目录下看到构建文件了

::::

## 集成文档

[~~或许算不上文档~~](../protocol/integration.md)

### Python

可参考 [Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py) 中 `__main__` 的实现

### C++

可参考 [CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp) 中的实现

### C Sharp

<!-- Do not use C#, MD003/heading-style: Heading style [Expected: atx; Actual: atx_closed] -->

可参考 [MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Main/AsstProxy.cs) 中的实现
