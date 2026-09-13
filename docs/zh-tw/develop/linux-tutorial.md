---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux 編譯教學

::: warning
**本教學需要讀者具備一定的 Linux 環境配置能力與程式設計基礎！**若您僅希望執行 MAA 而非自行編譯，請參閱[使用者手冊 - Linux 模擬器與容器](../manual/device/linux.md)。
:::

::: info 注意
MAA 的建置方法仍在討論中，本教學內容可能過時，請以 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/.github/workflows/ci.yml#L264#:~:text=ubuntu%3A) 中的腳本為準。  
您也可以參考 [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) 或 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)。
:::

::: info
Mac 使用者可以使用 `tools/build_macos_universal.zsh` 腳本進行編譯。  
建議參考 MaaAssistantArknights/MaaMacGui 專案的 [README.md](https://github.com/MaaAssistantArknights/MaaMacGui/blob/master/README.md)。
:::

## 編譯 MaaCore

:::: steps

1. 下載編譯所需的依賴項目
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

2. 建置第三方函式庫

   以下方式任選其一：

   - 下載預編譯的第三方函式庫（推薦）

     ```bash
     python tools/maadeps-download.py
     ```

     ::: info
     預編譯第三方函式庫基於 [MaaLinuxToolchain](https://github.com/MaaXYZ/MaaLinuxToolchain) 工具鏈交叉編譯，僅需依賴 glibc 2.31（Ubuntu 20.04）。如果您仍遇到 ABI 不相容的問題，可以使用容器，或嘗試自行建置第三方函式庫。
     :::

   - 自行建置第三方函式庫（耗時較長）

     ```bash
     git clone https://github.com/MaaAssistantArknights/MaaDeps
     cd MaaDeps
     # 若系統環境版本過低無法使用我們預設提供的 LLVM 20，請考慮不使用交叉編譯，直接使用在地編譯環境。
     # 需調整 src/MaaUtils/MaaDeps/cmake 中的 toolchain 設定。
     python linux-toolchain-download.py
     python build.py
     ```

3. 編譯 MAA

   ```bash
   cmake --preset linux-x64 -DINSTALL_RESOURCE=ON -DINSTALL_PYTHON=ON
   cmake --build build
   cmake --install build --prefix <target_directory>
   ```

   前 2 行指令生成 `build/bin/Debug/libMaaCore.so`（及 `libMaaUtils.so`），第 3 行指令將編譯成品安裝（即複製）到目標位置。

   ::: info CMake 選項說明
   `-DINSTALL_RESOURCE=ON` 的作用是將 `resource` 目錄複製到安裝目錄。MaaCore 需要配合 `resource` 目錄執行。

   `-DINSTALL_PYTHON=ON` 的作用是將 Python 整合（`src/Python`）複製到安裝目錄。若不使用或計劃從儲存庫原始碼中使用 Python 整合，則可省略此選項。
   :::

   ::: tip
   推薦透過指定動態函式庫檔案路徑或 `LD_LIBRARY_PATH` 來執行 MAA，請勿使用 root 權限將 MAA 安裝入 `/usr`。
   :::

4. 編譯 MaaFramework 相關元件

   若需針對 MaaFwAdbController（MaaFwAdb 觸控模式）相關功能進行除錯，需要[自行編譯 MaaFramework](https://maafw.com/docs/4.1-BuildGuide) 的 Debug 版本，將 `libMaaAdbControlUnit.so` 放到安裝目錄下。

5. 執行

   參考 [各程式語言 API](../readme.md#api)、[使用 Python](../manual/device/linux.md#使用-python) 等文件說明呼叫 MAA 動態函式庫。

::::

## 編譯 MaaWpfGui

::: info 注意
MaaWpfGui 適用於 Windows，編譯成品在 Linux 上需要透過 Wine 執行，詳見 [使用 Wine](../manual/device/linux.md#使用-wine)。
:::

:::: steps
1. 準備 `MaaCore.dll`

   MaaWpfGui 依賴 `MaaCore.dll`（及其依賴的其他 DLL）。目前無法從 Linux 交叉編譯出 Windows 版的 `MaaCore.dll`。雖然可以從 MAA 的 Windows 安裝包中複製，但容易出現版本不一致問題。

   故推薦的做法是先按上一節的說明編譯 MaaCore（無需安裝），然後編譯 [MaaWineBridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev-v2/src/MaaWineBridge)，將得到的 `MaaCore.dll` 置於 `build/bin/Debug` 目錄。

2. 安裝 .NET SDK

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

   另請參考 [在 Linux 上安裝 .NET](https://learn.microsoft.com/zh-tw/dotnet/core/install/linux)。

3. 編譯 MaaWpfGui

   在儲存庫根目錄下執行：

   ```bash
   dotnet build src/MaaWpfGui/MaaWpfGui.csproj -p:Platform=x64
   ```

   首次編譯時會自動下載依賴。此步驟會產生 `build/bin/Debug/MAA.exe`。

4. 執行

   ```bash
   wine build/bin/Debug/MAA.exe
   ```
::::
