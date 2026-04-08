---
order: 2
icon: material-symbols:download-2-rounded
---

<!-- markdownlint-disable MD024 -->

# 下載與安裝

::: tip
您正在查閱 MAA GUI 的下載與安裝說明文件。若您需要查閱 maa-cli 的下載與安裝說明，請前往 maa-cli 的 [安裝及編譯](./cli/install.md) 文件。目前 MAA Android 版已開放測試，請前往 [MAA-Meow](https://github.com/Aliothmoon/MAA-Meow) 瞭解更多資訊。
:::

## 下載 MAA

MAA 提供多種下載方式，包括官網下載、透過套件管理員安裝、群組檔案下載等方式。請選擇適合您的方式進行下載。

### 透過 [官網](https://maa.plus) 下載最新的 MAA 安裝檔

官網一般會自動選擇正確的版本架構，對於大多數閱讀本文件的 Windows 使用者來說，應為 Windows x64。對於閱讀本文件的 macOS 使用者來說，應下載 macOS 通用版本。

### 透過 [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install) 下載最新的 MAA 安裝檔

請確認系統架構並下載對應的安裝檔案。對於大多數閱讀本文件的 Windows 使用者來說，應為 Windows x64。對於閱讀本文件的 Mac 使用者，Mirror酱 不提供通用安裝檔，請確認您的晶片架構（arm/x86）後下載對應版本。

::: tip
[Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install) 是獨立的第三方下載加速服務，需要付費使用，而非由 MAA 收費。其營運成本由訂閱收入支撐，部分收益將回饋專案開發者。歡迎訂閱 CDK 享受快速下載，同時支援專案持續開發。
:::

### 使用 Windows 套件管理員（Winget）安裝

::: tip
本方法僅適用於使用 Windows 的使用者
:::

請在終端機中執行以下指令：

```bash
winget install maa
```

透過此方式安裝的預設安裝路徑為 `C:\Users\使用者名稱\AppData\Local\Microsoft\WinGet\Packages`。

### 透過 QQ 群組檔案下載最新的 MAA 安裝檔

1. 加入 [MAA 官方 QQ 群](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)
2. 在群組檔案中找到最新的 MAA 壓縮檔進行下載。

### 透過 [GitHub Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 下載最新的 MAA 安裝檔

請確認系統架構並下載對應的安裝檔。對於大多數閱讀本文件的 Windows 使用者來說，檔案名稱應為 `MAA-<版本號>-win-x64.zip`。對於閱讀本文件的 macOS 使用者來說，應選擇 `MAA-<版本號>-macos-universal.dmg`。

## Linux 和其他作業系統

MAA GUI **暫不支援** Linux 和其他作業系統。您可以使用 **maa-cli** 在這些系統上使用 MAA 的功能。請前往 maa-cli 的 [安裝及編譯](./cli/install.md) 文件瞭解更多資訊。

## 安裝 MAA

### Windows

下載完成後，您會得到一個 `.zip` 檔案。使用解壓縮軟體將其完整解壓縮後，會得到一個包含 MAA 所有檔案的資料夾。

::: warning
1. 請不要將 MAA 解壓縮到如 `C:\` 或 `C:\Program Files\` 等受系統保護的路徑，以免因「管理員權限限制」導致程式執行失敗。
2. MAA 已內建 .NET 執行環境（獨立發行版），但仍需要 Visual C++ Redistributable x64（VCRedist x64）。請在解壓縮後的 MAA 目錄中以系統管理員身分執行 `DependencySetup_依赖库安装.bat` 來安裝該相依元件，安裝完成後再執行 `MAA.exe`。

更多資訊請參考 [常見問題](./faq.md) 置頂內容。
:::

連按兩下 `MAA.exe` 即可啟動 MAA。

::: tip
透過 Windows 套件管理員（Winget）安裝的使用者，可直接在命令列中輸入 `maa` 來啟動 MAA，而無需額外執行解壓縮、安裝執行環境等操作。若環境變數 PATH 中已有 `maa-cli`，可能需要額外步驟以區分兩者。
:::

### macOS

下載完成後，您會得到一個 `.dmg` 檔案。連按兩下開啟該 `.dmg`，將 `MAA.app` 拖曳至 `/Applications` 以完成安裝。

## 後續步驟

安裝完成後，請返回 [新手上路](./newbie.md) 繼續設定，或前往 [功能介紹](./introduction/) 查看 MAA 支援的各項功能吧！若您在安裝中遇到問題，可以嘗試查閱 [常見問題](./faq.md) 來解決問題。