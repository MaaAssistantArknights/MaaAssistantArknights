---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux 編譯教學

**本教學需要讀者具備一定的 Linux 環境配置能力與程式設計基礎！** 若您僅希望直接安裝 MAA 而非自行編譯，請參閱[用戶手冊 - Linux 模擬器與容器](../manual/device/linux.md)。

::: info 注意
MAA 的建置方法仍在討論中，本教學內容可能過時，請以 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134) 中的指令碼為準。  
您也可以參考 [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) 或 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)。
:::

::: info
Mac 使用者可以使用 `tools/build_macos_universal.zsh` 指令碼進行編譯。  
建議參考 MaaAssistantArknights/MaaMacGui 專案的 [README.md](https://github.com/MaaAssistantArknights/MaaMacGui/blob/master/README.md)。
:::

## 編譯過程

:::: steps

1. 下載編譯所需的依賴項目
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

2. 建置第三方函式庫

   可以選擇下載預編譯的依賴庫，或從頭開始編譯。
   - 下載預編譯的第三方庫（推薦）

     > [!Note]
     > ~~先前提供的動態庫是在較新的 Linux 發行版 (Ubuntu 22.04) 中編譯的，若您的系統 libstdc++ 版本較舊，可能會遇到 ABI 不相容的問題。~~  
     > 目前已透過交叉編譯降低了執行環境要求，僅需依賴 glibc 2.31 (Ubuntu 20.04)。

     ```bash
     python tools/maadeps-download.py
     ```

   如果您發現上述預編譯庫因 ABI 版本等原因無法在您的系統執行，且不打算使用容器等方案，也可以嘗試自行編譯。
   - 自行建置第三方庫（耗時較長）

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
   cmake -B build \
       -DINSTALL_RESOURCE=ON \
       -DINSTALL_PYTHON=ON \
       -DCMAKE_TOOLCHAIN_FILE=src/MaaUtils/MaaDeps/cmake/maa-x64-linux-toolchain.cmake
   cmake --build build
   ```

   將 MAA 安裝到目標路徑。請注意，MAA 建議透過指定 `LD_LIBRARY_PATH` 來執行，請勿使用管理員權限將 MAA 安裝至 `/usr` 目錄。

   > 目前應該不需指定 `LD_LIBRARY_PATH` 即可正常執行。

   ```bash
   cmake --install build --prefix <target_directory>
   ```

4. 結束，您現在應該能在目錄下看到建置完成的檔案了。

::::

## 整合文件

[~~或許算不上文件~~](../protocol/integration.md)

### Python

可參考 [Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py) 中 `__main__` 中的實作方式。

### C++

可參考 [CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp) 中的實作方式。

### C Sharp

<!-- Do not use C#, MD003/heading-style: Heading style [Expected: atx; Actual: atx_closed] -->

可參考 [MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Main/AsstProxy.cs) 中的實作方式。
