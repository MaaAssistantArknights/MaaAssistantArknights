---
order: 3
icon: teenyicons:linux-alt-solid
---
# Linux 模擬器與容器

## 準備工作

以下安裝方式任選其一即可：

### 使用 maa-cli

[maa-cli](https://github.com/MaaAssistantArknights/maa-cli) 是一個使用 Rust 編寫的簡單 MAA 命令列工具。相關安裝與使用教程請閱讀[CLI 使用指南](./1.6-CLI使用說明)。

### 使用 Python

#### 1. 安裝 MAA 動態庫

1. 在 [MAA 官網](https://maa.plus/) 下載 Linux 動態庫並解壓，或從軟體源安裝：

   - AUR：[maa-assistant-arknights](https://aur.archlinux.org/packages/maa-assistant-arknights)，按照安裝後的提示編輯檔案
   - Nixpkgs: [maa-assistant-arknights](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)

2. 進入 `./MAA-v{版本號}-linux-{架構}/Python/` 目錄下打開 `sample.py` 文件

::: tip
預編譯的版本包含在相對較新的 Linux 發行版 (Ubuntu 22.04) 中編譯的動態庫，如果您系統中的 libstdc++ 版本較老，可能遇到 ABI 不兼容的問題
可以參考 [2.1-Linux編譯教學](./2.1-Linux編譯教學.md) 重新編譯或使用容器執行
:::

#### 2. `adb` 配置

1. 找到 [`if asst.connect('adb.exe', '127.0.0.1:5554'):`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L48) 一欄

2. `adb` 工具調用

   - 如果模擬器使用 `Android Studio` 的 `avd` ，其內建 `adb` 。可以直接在 `adb.exe` 一欄填寫 `adb` 路徑，一般在 `$HOME/Android/Sdk/platform-tools/` 裡面可以找到，例如：

    ```python
    if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "模擬器的 adb 地址"):
    ```

   - 如果使用其他模擬器須先下載 `adb` ： `$ sudo apt install adb` 後填寫路徑或利用 `PATH` 環境變量直接填寫 `adb` 即可

3. 模擬器 `adb` 路徑獲取

   - 可以直接使用 adb 工具： `$ adb路徑 devices` ，例如：

    ```shell
    $ /home/foo/Android/Sdk/platform-tools/adb devices
    List of devices attached
    emulator-5554 device
    ```

   - 返回的 `emulator-5554` 就是模擬器的 adb 地址，覆蓋掉 `127.0.0.1:5555` ，例如：

    ```python
    if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "emulator-5554"):
    ```

4. 這時候可以測試下： `$ python3 sample.py` ，如果返回 `連接成功` 則基本成功了

#### 3. 任務配置

自定義任務： 根據需要參考 [3.x 集成文件](https://maa.plus/docs/zh-tw/3.1-%E9%9B%86%E6%88%90%E6%96%87%E4%BB%B6.html) 對 `sample.py` 的 [`# 任務及參數請參考 docs/集成文件.md`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L54) 一欄進行修改

## 模擬器支援

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

必選配置： 16:9 的螢幕解析度，且解析度需大於 720p

推薦配置： x86\_64 的框架 (R - 30 - x86\_64 - Android 11.0) 配合 MAA 的 Linux x64 動態庫

### ⚠️ [Genymotion](https://www.genymotion.com/)

高版本安卓內建 x86\_64 框架，輕量但是執行明日方舟時易閃退

暫未嚴格測試， adb 功能和路徑獲取沒有問題

## 容器化安卓的支援

::: tip
以下方案通常對核心模組有一定要求，請根據具體方案和發行版安裝合適的核心模組
:::

### ✅ [Waydroid](https://waydro.id/)

安裝後需要重新設定解析度（或者大於 720P 且為 16:9 的解析度，然後重新啟動）：

```shell
waydroid prop set persist.waydroid.width 1280
waydroid prop set persist.waydroid.height 720
```

設定 adb 的 IP 地址：打開 `設定` - `關於` - `IP 地址` ，記錄第一個 `IP` ，將 `${記錄的 IP}:5555` 填入`sample.py` 的 adb IP 一欄。

### ✅ [redroid](https://github.com/remote-android/redroid-doc)

安卓 11 版本的鏡像可正常執行遊戲，需要暴露 5555 adb 通訊埠.
