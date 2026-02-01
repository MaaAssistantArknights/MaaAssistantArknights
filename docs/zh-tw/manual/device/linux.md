---
order: 3
icon: teenyicons:linux-alt-solid
---

# Linux 模擬器與容器

## 準備工作

以下安裝方式任選其一即可：

### 使用 maa-cli

[maa-cli](https://github.com/MaaAssistantArknights/maa-cli) 是一個使用 Rust 編寫的簡單 MAA 命令列工具。相關安裝與使用教學請閱讀 [CLI 使用指南](../cli/)。

### 使用 Wine

MAA WPF GUI 當前可以透過 Wine 執行。

#### 安裝步驟

1. 前往 [.NET 發佈頁](https://dotnet.microsoft.com/en-us/download/dotnet/10.0) 下載並安裝 Windows 版 .NET **桌面**執行環境 (Runtime)。

2. 下載 Windows 版 MAA，解壓縮後執行 `wine MAA.exe`。

::: info 注意
需要在連線設定中將 ADB 路徑設定為 [Windows 版 `adb.exe`](https://dl.google.com/android/repository/platform-tools-latest-windows.zip)。

如果您需要透過 ADB 連線 USB 設備，請先在 Wine 外執行 `adb start-server`，即透過 Wine 連線原生 ADB server。
:::

#### 使用 Linux 原生 MaaCore（實驗性功能）

下載 [MAA Wine Bridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/MaaWineBridge) 原始碼並建置，用產生的 `MaaCore.dll`（ELF 檔案）替換 Windows 版本，並將 Linux 原生動態函式庫（`libMaaCore.so` 以及依賴項）放在同一目錄下。

此時透過 Wine 執行 `MAA.exe`，將會載入 Linux 原生動態函式庫。

::: info 注意
使用 Linux 原生 MaaCore 時，需要在連線設定中將 ADB 路徑設定為 Linux 原生 ADB。
:::

#### Linux 桌面整合（實驗性功能）

桌面整合提供原生桌面通知支援，以及將 fontconfig 字型配置對應到 WPF 的功能。

將 MAA Wine Bridge 產生的 `MaaDesktopIntegration.so` 放到 `MAA.exe` 同目錄下即可啟用。

#### 已知問題

- Wine DirectWrite 強制啟用 hinting，且不將 DPI 傳遞給 FreeType，導致字型顯示效果不佳。
- 不使用原生桌面通知時，彈出通知會搶佔全系統滑鼠焦點，導致無法操作其他視窗。可以透過 `winecfg` 啟用虛擬桌面模式緩解，或停用桌面通知。
- Wine-staging 使用者需要關閉 `winecfg` 中的 `隱藏 Wine 版本` 選項，以便 MAA 正確偵測 Wine 環境。
- Wine 的 Light 主題會導致 WPF 中部分文字顏色異常，建議在 `winecfg` 中切換到無主題（Windows 經典主題）。
- Wine 使用舊式 XEmbed 系統匣圖示，在 GNOME 下可能無法正常工作。
- 使用 Linux 原生 MaaCore 時暫不支援自動更新（~~更新程式：我想我應該下載個 Windows 版~~）

### 使用 Python

:::: steps

1. 安裝 MAA 動態函式庫
   1. 在 [MAA 官網](https://maa.plus/) 下載 Linux 動態函式庫並解壓縮，或從軟體源安裝：
      - AUR：[maa-assistant-arknights](https://aur.archlinux.org/packages/maa-assistant-arknights)，按照安裝後的提示編輯檔案
      - Nixpkgs: [maa-assistant-arknights](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)
   2. 進入 `./MAA-v{版本號}-linux-{架構}/Python/` 目錄下開啟 `sample.py` 檔案

   ::: tip
   預編譯的版本包含在相對較新的 Linux 發行版 (Ubuntu 22.04) 中編譯的動態函式庫，如果您系統中的 libstdc++ 版本較舊，可能遇到 ABI 不相容的問題。
   可以參閱 [Linux 編譯教學](../../develop/linux-tutorial.md) 重新編譯或使用容器執行。
   :::

2. ADB 配置
   1. 找到 [`if asst.connect('adb.exe', '127.0.0.1:5554'):`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/b4fc3528decd6777441a8aca684c22d35d2b2574/src/Python/sample.py#L62) 一欄
   2. ADB 工具呼叫
      - 如果模擬器使用 `Android Studio` 的 `avd`，其內建 ADB。可以直接在 `adb.exe` 一欄填寫 ADB 路徑，通常在 `$HOME/Android/Sdk/platform-tools/` 裡面可以找到，例如：

      ```python
      if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "模擬器的 ADB 位址"):
      ```

      - 如果使用其他模擬器須先下載 ADB： `$ sudo apt install adb` 後填寫路徑或利用 `PATH` 環境變數直接填寫 `adb` 即可。

   3. 模擬器 ADB 路徑獲取
      - 可以直接使用 ADB 工具： `$ adb路徑 devices`，例如：

      ```shell
      $ /home/foo/Android/Sdk/platform-tools/adb devices
      List of devices attached
      emulator-5554 device
      ```

      - 回傳的 `emulator-5554` 就是模擬器的 ADB 位址，覆蓋掉 `127.0.0.1:5555`，例如：

      ```python
      if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "emulator-5554"):
      ```

   4. 這時候可以測試一下： `$ python3 sample.py`，如果回傳 `連接成功` 則基本成功了。

3. 任務配置

自定義任務：根據需要參閱 [整合文件](../../protocol/integration.md) 對 `sample.py` 的 [`# 任務及參數請參考 docs/integration.md`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L54) 一欄進行修改。

::::

## 模擬器支援

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

必選配置：16:9 的螢幕解析度，且解析度須大於 720p

推薦配置：x86_64 的架構 (R - 30 - x86_64 - Android 11.0) 配合 MAA 的 Linux x64 動態函式庫

請注意：從 Android 10 開始，Minitouch 在 SELinux 為 `Enforcing` 模式時不再可用，請切換至其他觸控模式，或將 SELinux **臨時**切換為 `Permissive` 模式。

### ⚠️ [Genymotion](https://www.genymotion.com/)

高版本 Android 內建 x86_64 架構，輕量但在執行《明日方舟》時易閃退。

暫未嚴格測試，ADB 功能與路徑獲取沒有問題。

## 容器化 Android 的支援

::: tip
以下方案通常對核心模組 (kernel module) 有一定要求，請根據具體方案和發行版本安裝合適的核心模組。
:::

### ✅ [Waydroid](https://waydro.id/)

安裝後需要重新設定解析度（或者大於 720P 且為 16:9 的解析度，然後重新啟動）：

```shell
waydroid prop set persist.waydroid.width 1280
waydroid prop set persist.waydroid.height 720
```

設定 ADB 的 IP 位址：開啟 `設定` - `關於` - `IP地址` ，記錄第一個 `IP` ，將 `${記錄的IP}:5555` 填入 `sample.py` 的 adb IP 一欄。

如果使用 amdgpu，`screencap` 指令可能向 stderr 輸出資訊導致圖片解碼失敗。
可以執行 `adb exec-out screencap | xxd | head` 並檢查輸出中是否有類似 `/vendor/etc/hwdata/amdgpu.ids: No such file...` 的文字來確認這一點。
嘗試將 `resource/config.json` 中的截圖指令由 `adb exec-out screencap` 改為 `adb exec-out 'screencap 2>/dev/null'`。

### ✅ [redroid](https://github.com/remote-android/redroid-doc)

Android 11 版本的映像檔可正常執行遊戲，需要開放 5555 ADB 連接埠。
