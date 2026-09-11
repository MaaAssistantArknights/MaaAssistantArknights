---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux 编译教程

::: warning
**本教程需要读者有一定的 Linux 环境配置能力及编程基础！**若您仅希望运行 MAA 而非自行编译，请阅读[用户手册 - Linux 模拟器与容器](../manual/device/linux.md)。
:::

::: info 注意
MAA 的构建方法仍在讨论中, 本教程的内容可能过时, 请以 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/.github/workflows/ci.yml#L264#:~:text=ubuntu%3A) 中的脚本为准。  
你也可参考 [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) 或 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)。
:::

::: info
Mac 可以使用 `tools/build_macos_universal.zsh` 脚本进行编译  
建议参考 MaaAssistantArknights/MaaMacGui 项目的 [README.md](https://github.com/MaaAssistantArknights/MaaMacGui/blob/master/README.md)
:::

## 编译 MaaCore

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

   以下方式任选其一：

   - 下载预构建的第三方库 (推荐)

     ```bash
     python tools/maadeps-download.py
     ```

     ::: info
     预构建第三方库基于 [MaaLinuxToolchain](https://github.com/MaaXYZ/MaaLinuxToolchain) 工具链交叉编译，仅需要依赖 glibc 2.31（Ubuntu 20.04）。如果您仍遇到 ABI 不兼容的问题，可以使用容器，或尝试自行构建第三方库。
     :::

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
   cmake --preset linux-x64 -DINSTALL_RESOURCE=ON -DINSTALL_PYTHON=ON
   cmake --build build
   cmake --install build --prefix <target_directory>
   ```

   前 2 行命令生成 `build/bin/Debug/libMaaCore.so`（及 `libMaaUtils.so`），第 3 行命令将编译产物安装（即复制）到目标位置。

   ::: info CMake 选项说明
   `-DINSTALL_RESOURCE=ON` 的作用是将 `resource` 目录复制到安装目录。MaaCore 需要配合 `resource` 目录运行。

   `-DINSTALL_PYTHON=ON` 的作用是将 Python 集成（`src/Python`）复制到安装目录。如果不使用或计划从仓库源代码中使用 Python 集成，则可省略此选项。
   :::

   ::: tip
   推荐通过指定动态库文件路径或 `LD_LIBRARY_PATH` 来运行 MAA，不要使用 root 权限将 MAA 装入 `/usr`。
   :::

4. 编译 MaaFramework 相关组件

   若需调试 MaaFwAdbController（MaaFwAdb 触控模式）相关功能，需要[自行编译 MaaFramework](https://maafw.com/docs/4.1-BuildGuide) 的 Debug 版本，将 `libMaaAdbControlUnit.so` 放到安装目录下。

5. 运行

   参考 [各编程语言 API](../readme.md#api)、[使用 Python](../manual/device/linux.md#使用-python) 等文档说明调用 MAA 动态库。

   ::::

## 编译 MaaWpfGui

::: info 注意
MaaWpfGui 适用于 Windows，编译产物在 Linux 上需要通过 Wine 运行，详见 [使用 Wine](../manual/device/linux.md#使用-wine)。
:::

:::: steps
1. 准备 `MaaCore.dll`

   MaaWpfGui 依赖 `MaaCore.dll`（及其依赖的其他 DLL）。目前无法从 Linux 交叉编译出 Windows 版的 `MaaCore.dll`。虽然可以从 MAA 的 Windows 安装包中复制，但易出现版本不一致问题。

   故推荐的做法是先按上一节的说明编译 MaaCore（无需安装），然后编译 [MaaWineBridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev-v2/src/MaaWineBridge)，将得到的 `MaaCore.dll` 置于 `build/bin/Debug` 目录。

2. 安装 .NET SDK

   ::: code-tabs
   @tab:active Ubuntu/Debian

   ```bash
   sudo apt install dotnet-sdk-10.0
   ```

   @tab Arch

   ```bash
   sudo pacman -S --needed dotnet-sdk
   ```

   :::

   另请参考 [在 Linux 上安装 .NET](https://learn.microsoft.com/en-us/dotnet/core/install/linux)。

3. 编译 MaaWpfGui

   在仓库根目录下运行：

   ```bash
   dotnet build src/MaaWpfGui/MaaWpfGui.csproj -p:Platform=x64
   ```

   首次编译时会自动下载依赖。此步骤会生成 `build/bin/Debug/MAA.exe`。

4. 运行

   ```bash
   wine build/bin/Debug/MAA.exe
   ```
::::
