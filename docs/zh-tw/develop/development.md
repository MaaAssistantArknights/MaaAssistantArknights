---
order: 1
icon: iconoir:developer
---

# 開始開發

## Github Pull Request 流程簡述

### 我不懂程式，只是想改一點點 JSON 檔案，要怎麽操作？

歡迎收看 [牛牛也能看懂的 GitHub Pull Request 使用指南](./pr-tutorial.md) （純網頁端操作）

### 我會程式，但沒接觸過 GitHub/C++/……，要怎麽操作？

1. 如果很久以前 fork 過，先在自己倉庫的 `Settings` 裡，翻到最下面，刪除
2. 打開 [MAA 主倉庫](https://github.com/MaaAssistantArknights/MaaAssistantArknights) ，點擊 `Fork`，繼續點擊 `Create fork`
3. clone MAA 主倉庫（到你自己帳號下） dev 分支到本地

    ```bash
    git clone --recurse-submodules <你的倉庫 git 連結> -b dev
    ```

    如果正在使用 Visual Studio 等不附带 `--recurse-submodules` 参数的 Git GUI，則需在克隆後再執行 `git submodule update --init` 以拉取子模塊。

4. 下載預構建的第三方庫

    **需要有 Python 環境，請自行搜尋 Python 安裝教學**  
    _（maadeps-download.py 檔案在項目根目錄）_

    ```cmd
    python maadeps-download.py
    ```

5. 配置程式環境

    - 下載並安裝 `Visual Studio 2022 community`，安裝的時候需要選中 `基於 c++ 的桌面開發` 和 `.NET 桌面開發`

6. 雙擊打開 `MAA.sln` 檔案。Visual Studio 會自動加載整個項目。
7. 設定 Visual Studio

    - Visual Studio 上方配置選擇 `RelWithDebInfo` `x64` （如果編譯 Release 包 或 ARM 平台，請忽略這步）
    - 右鍵 `MaaWpfGui` - 屬性 - 調試 - 啟用本地調試（這樣就能把斷點掛到 C++ Core 那邊了）

8. 到這裡，你就可以愉快地~~瞎ㄐㄅ改~~發電了
9. 開發過程中，每一定數量，記得提交一個 commit，別忘了寫上 message  
   假如你不熟悉 git 的使用，你可能想要新建一個分支進行更改，而不是直接提交在 `dev` 上

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    這樣你的提交就能在新的分支上生長，不會受到 `dev` 更新的打擾

10. 完成開發後，推送你修改過的本地分支（以 `dev` 為例）到遠端（fork 的倉庫）

    ```bash
    git push origin dev
    ```

11. 打開 [MAA 主倉庫](https://github.com/MaaAssistantArknights/MaaAssistantArknights)。提交一個 Pull Request，等待管理員通過。別忘了你是在 dev 分支上修改，別提交到 master 分支去了。

12. 當 MAA 主倉庫出現更改（別人做的），你可能需要把這些更改同步到你的分支
    1. 關聯 MAA 主倉庫

        ```bash
        git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
        ```

    2. 從 MAA 主倉庫拉取更新

        ```bash
        git fetch upstream
        ```

    3. 變基（推薦）或者合併修改

        ```bash
        git rebase upstream/dev # 變基
        ```

        或者

        ```bash
        git merge # 合並
        ```

    4. 重複上述 7, 8, 9, 10 中的操作

::: tip
在打開 VS2022 之後，和 git 有關的操作可以不用命令列工具，直接使用 VS 內建的 “Git 更改” 即可
:::

## 在 Visual Studio 中啟用 clang-format

1. 安裝 clang-format 版本17或更高。

    ```bash
    python -m pip install clang-format
    ```

2. 使用 'Everything' 等工具尋找 clang-format.exe 的安裝位置。作為參考，若您使用了 Anaconda，clang-format.exe 將安裝在 YourAnacondaPath/Scripts/clang-format.exe。
3. 在 Visual Studio 的 工具-選項 中搜索 clang-format。
4. 點擊 `啟用 ClangFormat 支援`，然後選擇下面的 `使用自定義 clang-format.exe 檔案`，選擇第 2 步得到的 `clang-format.exe`

![Visual Studio 設定 clang-format](https://github.com/MaaAssistantArknights/MaaAssistantArknights/assets/18511905/23ab94dd-09da-4b88-8c62-6b5f9dfad1a2)

然後你的 Visual Studio 就能愉快的使用支援 c++20 語法的 clang-format 啦！

你也可以使用 `tools\ClangFormatter\clang-formatter.py` 來直接調用你的 clang-format 來進行格式化，只需要在項目根目錄下執行：

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`

## 使用 GitHub codespace 進行在線開發

建立 GitHub codespace 自動配置 C++ 開發環境

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg?color=green)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights)

隨後根據 vscode 的提示或 [Linux 教程](./linux-tutorial.md) 配置 GCC 12 和 CMake 工程
