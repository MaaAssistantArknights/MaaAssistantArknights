---
order: 1
icon: iconoir:developer
---

# 開發指南

::: tip
本頁面主要描述了 PR 流程以及 MAA 的檔案格式化要求，如果您想要具體了解如何對 MAA 的運行邏輯進行更改，請參閱 [協議文件](../protocol/)
:::

::: tip
您可以 [向 DeepWiki 詢問](https://deepwiki.com/MaaAssistantArknights/MaaAssistantArknights)，以初步了解 MAA 專案的整體架構。
:::

## 我不懂程式設計，只想改一點點 JSON 檔案 / 文件等，要怎麼操作？

歡迎收看 [牛牛也能看懂的 GitHub Pull Request 使用指南](./pr-tutorial.md) （純網頁端操作 github.com）

## 我只想簡單修改幾行程式碼，但配置環境太麻煩，純網頁編輯又很難用，怎麼辦？

請使用 [GitHub Codespaces](https://github.com/codespaces) 線上開發環境，盡情嘗試！

我們預置了多種不同的開發環境以供選擇：

- 基礎環境，純 Linux 容器（預設）

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2Fdevcontainer.json)

- 精簡環境，適合文件站前端開發

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F0%2Fdevcontainer.json)

- 完整環境，適合 MAA Core 相關開發（不推薦使用，建議本機開發，完整配置相關環境。詳見下一章節）

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F1%2Fdevcontainer.json)

## 完整環境配置流程（Windows）

1. 如果很久以前 Fork 過，先在自己倉庫的 `Settings` 裡，翻到最下面，將其刪除。
2. 打開 [MAA 主倉庫](https://github.com/MaaAssistantArknights/MaaAssistantArknights)，點擊 `Fork`，繼續點擊 `Create fork`。
3. 複製（Clone）您自己倉庫下的 dev 分支到在地，並拉取子模組（Submodules）。

   ```bash
   git clone --recurse-submodules <您的倉庫 git 連結> -b dev
   ```

   ::: warning
   如果正在使用 Visual Studio 等不附帶 `--recurse-submodules` 參數的 Git GUI，則需在複製後再執行 `git submodule update --init` 以拉取子模組。
   :::

4. 下載預編譯的第三方函式庫

   **需要有 Python 環境，請自行搜尋 Python 安裝教學。**

   ```cmd
   python tools/maadeps-download.py
   ```

5. 配置開發環境
   - 下載並安裝 `CMake`
   - 下載並安裝 `Visual Studio 2026 Community`，安裝時需要選中 `使用 C++ 的桌面開發` 和 `.NET 桌面開發`。

6. 執行 CMake 專案配置

   ```cmd
   cmake --preset windows-x64
   ```

7. 雙擊開啟 `build/MAA.slnx` 檔案，Visual Studio 會自動載入整個專案。
8. 設定 Visual Studio
   - Visual Studio 上方配置選擇 `Debug` `x64`
   - 右鍵點擊 `MaaWpfGui` - 設為啟動專案。
   - 按 F5 執行。

   ::: tip
   若需針對 Win32Controller（Windows 視窗控制）相關功能進行除錯，需要自行從 [MaaFramework Releases](https://github.com/MaaModular/MaaFramework/releases) 下載對應平台的壓縮檔案，將 `bin` 目錄中的 `MaaWin32ControlUnit.dll` 放到 MAA 的 DLL 同目錄下（例如 `build/bin/Debug`）。歡迎 PR 一個自動下載腳本！
   :::

9. 到這裡，你就可以愉快地 ~~瞎雞巴改~~ 發電了
10. 開發過程中，每一定數量，記得提交一個 Commit, 別忘了寫上 Message  
    假如你不熟悉 git 的使用，你可能想要新建一個分支進行更改，而不是直接提交在 `dev` 上

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    這樣你的提交就能在新的分支上生長，不會受到 `dev` 更新的打擾

11. 完成開發後，推送你修改過的本地分支（以 `dev` 為例）到遠端（Fork 的倉庫）

    ```bash
    git push origin dev
    ```

12. 打開 [MAA 主倉庫](https://github.com/MaaAssistantArknights/MaaAssistantArknights)。提交一個 Pull Request，等待管理員通過。別忘了您是在 dev 分支上修改，別提交到 master 分支去了。

13. 當 MAA 原倉庫出現更改（他人貢獻）時，您可能需要把這些更改同步到您的分支：
    1. 關聯 MAA 原倉庫

       ```bash
       git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
       ```

    2. 從 MAA 原倉庫拉取更新

       ```bash
       git fetch upstream
       ```

    3. 變基（推薦）或者合併修改

       ```bash
       git rebase upstream/dev # 變基
       ```

       或者

       ```bash
       git merge # 合併
       ```

    4. 重複上述 8, 9, 10, 11 中的操作

::: tip
開啟 Visual Studio 之後，和 Git 有關的操作可以不用命令列工具，直接使用 Visual Studio 內建的「Git 變更」功能即可。
:::

## 使用 VS Code 進行開發（可選）

::: warning
**建議優先使用 Visual Studio 進行開發。** MAA 專案主要基於 Visual Studio 建置，上述完整環境配置流程已涵蓋所有開發需求，開箱即用體驗最佳。VSCode 方案僅作為備選，適合已經熟悉 VS Code + CMake + clangd 工作流的開發者，配置門檻相對較高。
:::

如果您偏好使用 VSCode，可以搭配 CMake、clangd 等擴充套件獲得程式碼補全、跳轉與除錯能力。完成前述 1–6 步（複製、相依、CMake 設定）後，可依下列步驟設定：

### 推薦擴充套件

於 VS Code 擴充套件市集安裝：

| 擴充套件                                                                                            | 作用                                                |
| --------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)            | CMake 設定、建置、除錯整合                          |
| [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) | C++ 智慧提示、程式碼跳轉、診斷（基於 LSP）          |
| [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)                     | 除錯 C++ 程式（與 CMake Tools 或 launch.json 搭配） |

::: tip
使用 clangd 時，建議停用 C/C++ 擴充套件的 IntelliSense（將 `C_Cpp.intelliSenseEngine` 設為 `disabled`），以免與 clangd 衝突。
:::

### 設定步驟

1. 使用 VS Code 開啟專案根目錄
2. **CMake Tools**：
   - 於狀態列選擇 Configure Preset（如 `windows-x64`、`linux-x64` 等）
   - 選擇 Build Preset，執行設定與建置
3. **clangd**：Linux/macOS 下預設已開啟 `CMAKE_EXPORT_COMPILE_COMMANDS`，clangd 會自動使用 `build/compile_commands.json`。在 Windows 上如需 clangd 的補全與跳轉，需先生成 `compile_commands.json`：

   ::: warning Windows 下 clangd 設定說明
   - 在 VS Installer 中勾選安裝 **用於 Windows 的 C++ Clang 編譯器**（clang-cl）
   - 需切換為 `windows-x64-clang` 執行一次 Configure 以在 `build/` 下生成 `compile_commands.json`，此後 clangd 即可使用
   - **該 preset 使用 clang-cl 而非 MSVC，無法直接編譯出可用產物**，實際建置時必須切回 `windows-x64`
   - clangd 基於 clang-cl 的編譯資訊進行分析，部分程式碼（如 MSVC 特有擴充）可能仍會顯示報錯，可忽略，不影響實際 MSVC 編譯

   **命令列切換 preset 範例**（在專案根目錄執行）：

   ```cmd
   rem 生成 compile_commands.json（僅 Configure，不建置）
   cmake --preset windows-x64-clang

   rem 切回 MSVC 進行實際建置
   cmake --preset windows-x64
   cmake --build --preset windows-x64-RelWithDebInfo
   ```

   :::

4. **除錯**：專案已包含 `.vscode/launch.json`，可直接啟動 MaaWpfGui 或 Debug Demo 進行除錯

### 快速建置與除錯

- **建置**：`Ctrl+Shift+B` 或透過 CMake Tools 狀態列
- **除錯**：F5 或於執行與除錯面板選擇對應設定

## MAA 的檔案格式化要求

MAA 使用一系列的格式化工具來確保倉庫中的程式碼和資源檔案美觀統一，以便於維護和閱讀。

請確保在提交之前已經完成格式化，或是[啟用 Pre-commit Hooks 來進行自動格式化](#利用-pre-commit-hooks-自動進行程式碼格式化)

目前啟用的格式化工具如下：

| 檔案類型  | 格式化工具                                                      |
| --------- | --------------------------------------------------------------- |
| C++       | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)    |
| JSON/YAML | [Prettier](https://prettier.io/)                                |
| Markdown  | [markdownlint](https://github.com/DavidAnson/markdownlint-cli2) |

### 利用 Pre-commit Hooks 自動進行程式碼格式化

1. 確保您的電腦上有 Python 與 Node 環境

2. 在專案根目錄下執行以下命令：

   ```bash
   pip install pre-commit
   pre-commit install
   ```

如果 pip 安裝後依然無法執行 pre-commit，請確認 pip 安裝路徑已被添加到 PATH。

接下來，每次提交時都將會自動執行格式化工具，以確保您的程式碼格式符合規範。

### 在 Visual Studio 中啟用 clang-format

1. 安裝 clang-format 20.1.0 或更高版本

   ```bash
   python -m pip install clang-format
   ```

2. 使用 Everything 等工具找到 `clang-format.exe` 的安裝位置。作為參考，若您使用了 Anaconda，`clang-format.exe` 將安裝在 `YourAnacondaPath/Scripts/clang-format.exe`

3. 在 Visual Studio `工具-選項` 中搜尋 `clang-format`

4. 點擊 `啟用 ClangFormat 支援`，然後選擇下方的 `使用自定義 clang-format.exe 檔案`，選擇第 2 步找到的 `clang-format.exe`

![Visual Studio 設定 clang-format](/images/zh-cn/development-enable-vs-clang-format.png)

然後您的 Visual Studio 就能愉快地使用支援 C++20 語法的 clang-format 啦！

您也可以使用 `tools\ClangFormatter\clang-formatter.py` 來直接呼叫 clang-format 進行格式化，只需在專案根目錄下執行：

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`
