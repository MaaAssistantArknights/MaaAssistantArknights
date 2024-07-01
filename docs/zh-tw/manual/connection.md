---
order: 3
icon: mdi:plug
---

# 連接設置

:::note
實體機相關問題請同時參閱 [Android 實體設備](./device/android.md)。
:::

## ADB 路徑

:::info 技術細節
自動偵測使用的是模擬器的 ADB，但有時自動偵測可能會出現問題，此時需要手動設置。
`強制替換 ADB` 是下載 Google 提供的 ADB 後進行替換，即可一勞永逸地設置 Google 的 ADB。
:::

### 使用模擬器提供的 ADB

前往模擬器安裝路徑，Windows 可在模擬器運行時在任務管理器中右鍵進程點擊 `打開文件所在的位置`。

顶層或下層目錄中應該會有一個名字中帶有 `adb` 的 exe 文件，可以使用搜索功能，然後選擇。

:::details 一些例子
`adb.exe` `HD-adb.exe` `adb_server.exe` `nox_adb.exe`
:::

### 使用谷歌提供的 ADB

[點擊下載](https://dl.google.com/android/repository/platform-tools-latest-windows.zip)後解壓，然後選擇其中的 `adb.exe`。

推薦直接解壓到 MAA 文件夾下，這樣可以直接在 ADB 路徑中填寫 `.\platform-tools\adb.exe`，也可以隨著 MAA 文件夾一起移動。

## 連接地址

::: tip
運行在本機的模擬器連接地址應該是 `127.0.0.1:<端口號>` 或 `emulator-<四位數字>`。
:::

### 獲取端口號

#### 模擬器相關文檔及參考端口

- [Bluestacks 5](https://support.bluestacks.com/hc/zh-tw/articles/360061342631-%E5%A6%82%E4%BD%95%E5%B0%87%E6%82%A8%E7%9A%84%E6%87%89%E7%94%A8%E5%BE%9EBlueStacks-4%E8%BD%89%E7%A7%BB%E5%88%B0BlueStacks-5#%E2%80%9C2%E2%80%9D) `5555`
- [MuMu Pro](https://mumu.163.com/mac/function/20240126/40028_1134600.html) `16384`
- [MuMu 12](https://mumu.163.com/help/20230214/35047_1073151.html) `16384`
- [MuMu 6](https://mumu.163.com/help/20210531/35047_951108.html) `7555`
- [逍遙](https://bbs.xyaz.cn/forum.php?mod=viewthread&tid=365537) `21503`
- [夜神](https://support.yeshen.com/zh-CN/qt/ml) `62001`

其他模擬器可參考 [趙青青的博客](https://www.cnblogs.com/zhaoqingqing/p/15238464.html)。

#### 關於多開

- MuMu 12 多開器右上角可查看正在運行的多開端口。
- Bluestacks 5 模擬器設置內可查看當前的多開端口。
- *待補充*

#### 備選方案

- 方案 1：使用 ADB 命令查看模擬器端口

  1. 啟動**一個**模擬器，並確保沒有其他安卓設備連接在此電腦上。
  2. 在存放有 ADB 可執行文件的文件夾中啟動終端。
  3. 執行以下命令。

  ```sh
  # Windows 命令提示符
  adb devices
  # Windows PowerShell
  .\adb devices
  ```

  以下是輸出內容的例子：

  ```text
  List of devices attached
  127.0.0.1:<端口號>   device
  emulator-<四位數字>  device
  ```

  使用 `127.0.0.1:<端口>` 或 `emulator-<四位數字>` 作為連接地址。

- 方案 2 : 查找已建立的 adb 連接

  1. 執行方案 1。
  2. 按 `徽標鍵+S` 打開搜索欄，輸入 `資源監視器` 並打開。
  3. 切換到 `網路` 選項卡，在 `監聽端口` 的名稱列中查找模擬器進程名，如 `HD-Player.exe`。
  4. 記錄模擬器進程的所有監聽端口。
  5. 在 `TCP 連接` 的名稱列中查找 `adb.exe`，在遠程端口列中與模擬器監聽端口一致的端口即為模擬器調試端口。

### 自動啟動多開模擬器

若需要多開模擬器同時操作，可將 MAA 文件夾複製多份，使用 **不同的 MAA**、**同一個 adb.exe**、**不同的連接地址** 來進行連接。
**以[藍叠國際版](./device/windows.md)為例**，介紹兩種啟動多開模擬器的方式。

#### 通過為模擬器 exe 附加命令來進行多開操作

1. 啟動**單一**模擬器多開。
2. 打開任務管理器，找到對應模擬器進程，轉到詳細信息選項卡，右鍵列首，點擊 `選擇列`，勾選 `命令行`。
3. 在多出來的 `命令行` 列中找到 `...\\Bluestacks_nxt\\HD-Player.exe"` 後的內容。
4. 將找到的類似於 `--instance Nougat32` 的內容填寫到 `啟動設置` - `附加命令` 中。

::: tip
操作結束後建議重新隱藏 `步驟 2` 中打開的 `命令行` 列以防止卡頓
:::

::: details 示例

```text
多開1:
模擬器路徑: C:\Program Files\BlueStacks_nxt\HD-Player.exe
附加命令: --instance Nougat32 --cmd launchApp --package "com.hypergryph.arknights"
多開2:
模擬器路徑: C:\Program Files\BlueStacks_nxt\HD-Player.exe
附加命令: --instance Nougat32_1 --cmd launchApp --package "com.hypergryph.arknights.bilibili"
```

其中 `--cmd launchApp --package` 部分為啟動後自動運行指定包名應用，可自行更改。
:::

#### 通過使用模擬器或應用的快捷方式來進行多開操作

1. 打開多開管理器，新增對應模擬器的快捷方式。
2. 將模擬器快捷方式的路徑填入 `啟動設置` - `模擬器路徑` 中

::: tip
部分模擬器支持創建應用快捷方式，可直接使用應用的快捷方式直接啟動模擬器並打開明日方舟
:::

::: details 示例

```text
多開1:
模擬器路徑: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\多開1.lnk
多開2:
模擬器路徑: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\多開2-明日方舟.lnk
```

:::

若使用 `模擬器路徑` 進行多開操作，建議將 `啟動設置` - `附加命令` 置空。

### 藍叠模擬器 Hyper-V 每次啟動端口號都不一樣

在 `連接設置` 中設置 `連接配置` 為 `藍叠模擬器` ，隨後勾選 `自動檢測連接` 和 `每次重新檢測`。

通常情況下這樣就可以連接。如果無法連接，可能是存在多個模擬器核心或出現了問題，請閱讀下文進行額外設置。

#### 指定 `Bluestacks.Config.Keyword`

::: info 注意
如果啟用了多開功能或安裝了多個模擬器核心，則需要進行額外設置來指定使用的模擬器編號
:::

在 `.\config\gui.json` 中搜索 `Bluestacks.Config.Keyword` 字段，内容為 `"bst.instance.<模擬器編號>.status.adb_port"`，模擬器編號可在模擬器路徑的 `BlueStacks_nxt\Engine` 中看到

::: details 示例
Nougat64 核心：

```json
"Bluestacks.Config.Keyword":"bst.instance.Nougat64.status.adb_port",
```

Pie64_2 核心：（核心名稱後的數字代表這是一個多開核心）

```json
"Bluestacks.Config.Keyword": "bst.instance.Pie64_2.status.adb_port",
```

:::

#### 指定 `Bluestacks.Config.Path`

::: info 注意
MAA 現在會嘗試從註冊表中讀取 `bluestacks.conf` 的存儲位置，當該功能無法工作時，則需要手動指定配置文件路徑
:::

1. 在藍叠模擬器的數據目錄下找到 `bluestacks.conf` 這個文件

    - 國際版默認路徑為 `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`
    - 中國內地版默認路徑為 `C:\ProgramData\BlueStacks_nxt_cn\bluestacks.conf`

2. 如果是第一次使用，請運行一次 MAA，使 MAA 自動生成配置文件。

3. **先關閉** MAA，**然後**打開 `gui.json`，找到 `Configurations` 下的當前配置名字段（可在 設置-切換配置 中查看，默認為 `Default`），在其中搜索字段 `Bluestacks.Config.Path`，填入 `bluestacks.conf` 的完整路徑。（注意斜槓要用轉義 `\\`）

::: details 示例
以 `C:\ProgramData\BlueStacks_nxt\bluestacks.conf` 為例

```json
{
    "Configurations": {
        "Default": {
            "Bluestacks.Config.Path": "C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf"
            // 其餘配置字段，不要手動輸入修改
        }
    }
}
```

:::

## 連接配置

需選擇對應模擬器的配置，若列表中沒有則選擇通用配置。若通用配置不可用請嘗試並選擇其他任一可用的配置。

具體區別可以閱讀[源碼](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/resource/config.json#L68)。

### MuMu  截圖增強模式

需使用官版 MuMu 12 V3.8.13 及更新版本，並關閉後台保活。方舟專版、國際版等暫不支持。

1. `設置` - `連接設置`，勾選 `啟用 MuMu 截圖增強模式`。

2. `MuMu 模擬器路徑` 填寫 `MuMuPlayer-12.0` 文件夾的路徑，如 `C:\Program Files\Netease\MuMuPlayer-12.0`。

3. `實例編號` 填寫 MuMu 多開器內對應模擬器的序號，如主多開為 `0`。

4. `實例屏幕` 填 `0`。

#### 關於後台保活

推薦關閉，此時實例屏幕始終為 `0`。

開啟時，MuMu 模擬器標籤頁的順序應為實例屏幕的序號，如 `0` 為模擬器桌面，`1` 為明日方舟客戶端。

針對後台保活的適配非常不完善，總會出現各種各樣莫名其妙的問題，非常不建議使用。

## 觸控模式

1. [Minitouch](https://github.com/DeviceFarmer/minitouch)：使用 C 編寫的 Android 觸控事件器，操作 `evdev` 設備，提供 Socket 接口供外部程序觸發觸控事件和手勢。從 Android 10 開始，Minitouch 在 SELinux 為 `Enforcing` 模式時不再可用。<sup>[來源](https://github.com/DeviceFarmer/minitouch?tab=readme-ov-file#for-android-10-and-up)</sup>
2. [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)：由 MAA 基於 Java 對 Minitouch 的重新實現，使用安卓的 `InputDevice`，並添加了額外特性。高版本 Android 可用性尚待測試。~~幫我們做做測試~~
3. Adb Input：直接調用 ADB 使用安卓的 `input` 命令進行觸控操作，兼容性最強，速度最慢。

## ADB Lite

由 MAA 獨立實現的 ADB Client，相較原版 ADB 可以避免不停開啟多個 ADB 進程，減少性能損耗，但部分截圖方式不可用。

推薦啟用，但具體優缺點尚待反饋。~~幫我們做做測試 x2~~
