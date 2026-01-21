---
order: 3
icon: mdi:plug
---

# 連線設定

## 自動偵測

MAA 可以透過目前**正在執行中的單一模擬器**，自動偵測並填充 ADB 路徑、連線位址與連線配置。

截至 MAA v5.22.3 為止，支援自動偵測的模擬器及其連線位址如下：

- BlueStacks 藍疊模擬器 5：`127.0.0.1:5555/5556/5565/5575/5585/5595/5554`
- MuMu 模擬器：`127.0.0.1:16384/16416/16448/16480/16512/16544/16576`
- 雷電模擬器 9：`emulator-5554/5556/5558/5560`、`127.0.0.1:5555/5557/5559/5561`
- 夜神模擬器：`127.0.0.1:62001/59865`
- 逍遙模擬器：`127.0.0.1:21503`

若偵測失敗，請嘗試以系統管理員權限啟動 MAA 並再次偵測。若仍失敗，請參考下文進行手動設定，並參考上述清單向我們回報。

## ADB 路徑

:::info 技術細節
自動偵測預設會使用模擬器內建的 ADB，當自動偵測出現問題時才需要手動設定。  
點選 `強制替換 ADB` 會自動下載 Google 提供的 ADB 並自動填入路徑。
若系統環境變數中已有可用的 ADB，可直接填寫 `adb`。
:::

### 使用模擬器提供的 ADB

前往模擬器的安裝路徑。在 Windows 系統中，您可以在模擬器執行時，於工作管理員對該程序點擊右鍵，選擇 `開啟檔案所在位置`。

在該層或下層目錄中，應該能找到一個檔名包含 `adb` 的 exe 檔案（可使用搜尋功能），將其選取即可。

:::details 範例檔名
`adb.exe` `HD-adb.exe` `adb_server.exe` `nox_adb.exe`
:::

### 使用 Google 提供的 ADB

[點此下載](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) 後解壓縮，並選取其中的 `adb.exe`。

建議直接解壓縮到 MAA 資料夾下，這樣可以直接在 ADB 路徑中填寫 `.\platform-tools\adb.exe`，方便隨著 MAA 資料夾移動。

## 連線位址

::: tip
執行於本機電腦的模擬器，連線位址通常為 `127.0.0.1:<連接埠>` 或 `emulator-<四位數字>`。
:::

### 模擬器連接埠相關文件

- [BlueStacks 藍疊模擬器 5](https://support.bluestacks.com/hc/zh-tw/articles/360061342631)：模擬器設定 → 進階中可查看目前的連線位址。
- [MuMu 模擬器](https://mumu.163.com/help/20240807/40912_1073151.html?maa)：主視窗（多開器）右上角選單按鈕 → 設定中心 → 底部 `ADB 端口` 可查看正在執行的多開連接埠。
- [MuMu 模擬器 Pro](https://mumu.163.com/mac/function/20240126/40028_1134600.html)
- [雷電模擬器 9](https://help.ldmnq.com/docs/LD9adbserver#edc3863750608062bcb3feea256413dc)
- [夜神模擬器](https://support.yeshen.com/zh-CN/qt/ml)
- [逍遙模擬器](https://bbs.xyaz.cn/forum.php?mod=viewthread&tid=365537)

其他模擬器可參閱 [趙青青的博客](https://www.cnblogs.com/zhaoqingqing/p/15238464.html)。

::: details 備選方案

- **方案 1：使用 ADB 指令查看模擬器連接埠**
  1. 啟動**一個**模擬器，並確保沒有其他 Android 裝置連接到此電腦。
  2. 在存放 ADB 執行檔的資料夾中啟動終端機（Terminal）。
  3. 執行以下指令：

  ```sh
  # Windows 命令提示字元 (cmd)
  adb devices
  # Windows PowerShell
  .\adb devices
  ```

  輸出範例如下：

  ```text
  List of devices attached
  127.0.0.1:<連接埠>   device
  emulator-<四位數字>  device
  ```

  使用 `127.0.0.1:<連接埠>` 或 `emulator-<四位數字>` 作為連線位址即可。

- 方案 2 ：尋找已建立的 ADB 連線
  1. 執行方案 1。
  2. 按下 `Win+S` 開啟搜尋列，輸入 `資源監控制器` 並開啟。
  3. 切換到 `網路` 索引標籤，在 `接聽連接埠` 的名稱欄位中尋找模擬器程序名稱（例如 `HD-Player.exe`）。
  4. 記錄模擬器程序所有的接聽連接埠。
  5. 在 `TCP 連線` 的名稱欄位尋找 `adb.exe`，遠端連接埠與模擬器接聽連接埠一致者，即為偵錯連接埠。

:::

### BlueStacks 藍疊模擬器 Hyper-V 每次啟動連接埠都不一樣

在 `連線設定` 中將 `連線配置` 設為 `藍疊模擬器`，隨後勾選 `自動偵測連線` 和 `每次重新偵測`。

通常這樣即可連線。若無法連線，可能是因為存在多個核心或設定異常，請參考下文進行進階設定。

:::: steps

1. 指定 `Bluestacks.Config.Keyword`

   ::: info 注意
   若啟用了多開功能或安裝了多個模擬器核心，則需要指定使用的模擬器編號。
   :::

   在 `.\config\gui.json` 中搜尋 `Bluestacks.Config.Keyword` 欄位，內容格式為 `"bst.instance.<模擬器編號>.status.adb_port"`。編號可於模擬器路徑的 `BlueStacks_nxt\Engine` 中確認。

   ::: details 範例
   Nougat64 核心：

   ```json
   "Bluestacks.Config.Keyword":"bst.instance.Nougat64.status.adb_port",
   ```

   Pie64_2 核心（核心名稱後的數字代表這是一個多開核心）：

   ```json
   "Bluestacks.Config.Keyword": "bst.instance.Pie64_2.status.adb_port",
   ```

   :::

2. 指定 `Bluestacks.Config.Path`

   ::: info 注意
   MAA 現在會嘗試從登錄檔（Registry）讀取 `bluestacks.conf` 的位置，若偵測失敗才需要手動指定路徑。
   :::
   1. 在 BlueStacks 藍疊模擬器的數據目錄下找到 `bluestacks.conf` 檔案：
      - 國際版預設路徑： `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`
      - 中國版預設路徑： `C:\ProgramData\BlueStacks_nxt_cn\bluestacks.conf`

      備註：`C:\ProgramData` 為隱藏目錄，請直接在檔案總管地址欄貼上路徑進入。

   2. 初次使用請先執行一次 MAA 以產生設定檔。
   3. **先關閉** MAA，**再**開啟 `gui.json`。找到 `Configurations` 下目前的設定名稱（可在 `設定-切換配置` 中查看，預設為 `Default`），搜尋 `Bluestacks.Config.Path` 並填入 `bluestacks.conf` 的完整路徑（斜線請使用轉義符 `\\`）。

   ::: details 範例
   以 `C:\ProgramData\BlueStacks_nxt\bluestacks.conf` 為例：

   ```json
   {
     "Configurations": {
       "Default": {
         "Bluestacks.Config.Path": "C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf"
         // 其餘配置欄位，不要手動輸入修改
       }
     }
   }
   ```

   :::

3. 返回 MAA 並測試

   您可以使用 `設定 - 執行設定` 中的 `截圖測試` 功能，檢查連線的是否為預期的核心。

::::

## 連線配置

請選擇對應模擬器的配置，若清單中沒有則選擇「通用配置」。若通用配置不可用，請嘗試並選擇其他任一可用的配置。

具體區別可以參閱 [原始碼](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/resource/config.json#L57)。

### MuMu 截圖增強模式

需使用官版或方舟專版 MuMu V4.1.26 及更新版本。<!-- 官版 V3.8.13 支援初版截圖增強 -->

1. 在 `設定 - 連線設定` 勾選 `啟用 MuMu 截圖增強模式`，MAA 會嘗試自動獲取路徑。

2. `MuMu 安裝路徑` 填寫 `MuMu Player` 或 `MuMuPlayerGlobal-12.0` 或 `YXArkNights-12.0` 資料夾路徑（例如 `C:\Program Files\Netease\MuMuPlayerGlobal-12.0`）。

3. 若正在使用 MuMu 網路橋接，需勾選 `MuMu 網路橋接模式` 並手動填寫多開器內對應的模擬器序號，主多開為 `0`。

### 雷電截圖增強模式

需使用官版或國際版雷電模擬器 9 V9.1.32 及更新版本。<!-- 官版 V9.0.78 支援初版截圖增強 但存在高解析度失效問題 V9.1.29 修復-->

1. 在 `設定 - 連線設定` 勾選 `啟用 LD 截圖增強模式`，MAA 會嘗試自動獲取路徑。

2. `LD 模擬器路徑` 填寫 `LDPlayer9` 資料夾路徑（例如 `C:\leidian\LDPlayer9\`）。

3. `執行個體編號` 填寫雷電多開器內對應的模擬器序號（ID），主多開為 `0`。

## 觸控模式

1. [Minitouch](https://github.com/DeviceFarmer/minitouch)：基於 C 語言編寫的 Android 觸控事件驅動程式，透過操作 `evdev` 裝置，提供 Socket 介面供外部程式觸發觸控事件與手勢。自 Android 10 開始，若系統的 SELinux 為 `Enforcing`（強制）模式，Minitouch 將無法運作。<sup>[來源](https://github.com/DeviceFarmer/minitouch?tab=readme-ov-file#for-android-10-and-up)</sup>
2. [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)：由 MAA 基於 Java 對 Minitouch 的重新實作，改用 Android 原生的 `InputDevice`介面，並加入額外特性。目前在高版本 Android 上的相容性仍待測試，~~幫我們做做測試~~。
3. Adb Input：直接呼叫 ADB 執行 Android 內建的 `input` 指令來進行觸控操作。此方式的相容性最強，但執行速度最慢。

## ADB Lite

由 MAA 獨立實現的 ADB 用戶端，使用 TCP 直接與 ADB Server 通信。相較原版 ADB 可避免頻繁開啟 ADB 程序並減少效能開銷，但部分截圖方式不支援。<sup>[PR](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/3315)</sup>

推薦開啟，但具體表現尚待回饋。~~幫我們做做測試 x2~~

## MAA 與模擬器多開

::: info 實作方式
若需多開模擬器同時執行，可將 MAA 資料夾複製多份，透過 **不同的 MAA 程式**、**同一個 adb.exe**、**不同的連線地址** 來進行連線。
:::

### 自動啟動多開模擬器

以 [BlueStacks 藍疊模擬器國際版](./device/windows.md) 為例，介紹兩種自動啟動多開模擬器的方式。

#### 透過附加指令啟動

1. 啟動**單一**模擬器多開。
2. 開啟工作管理員，找到對應的模擬器程序，切換到「詳細資料」索引標籤，右鍵點擊標題列，點選 `選擇行`（或選擇列），勾選 `命令列`。
3. 在多出來的 `命令列` 欄位中，找到 `...\Bluestacks_nxt\HD-Player.exe"` 之後的內容。
4. 將找到的內容（例如：`--instance Nougat32`）填寫到 MAA 的 `啟動設定 - 附加指令` 中。

::: note
操作結束後，建議重新隱藏 `步驟 2` 中開啟的 `命令列` 欄位以避免系統卡頓。
:::

::: details 範例

```text
多開 1:
模擬器路徑: C:\Program Files\BlueStacks_nxt\HD-Player.exe
附加指令: --instance Nougat32 --cmd launchApp --package "com.hypergryph.arknights"
多開 2:
模擬器路徑: C:\Program Files\BlueStacks_nxt\HD-Player.exe
附加指令: --instance Nougat32_1 --cmd launchApp --package "com.hypergryph.arknights.bilibili"
```

其中 `--cmd launchApp --package` 部分是啟動後自動執行指定包名的應用程式，可根據需求自行更改。
:::

#### 透過模擬器的捷徑啟動

部分模擬器支援建立應用程式捷徑，可直接使用捷徑啟動模擬器並開啟《明日方舟》。

1. 開啟多開管理器，新增對應模擬器的捷徑到桌面。
2. 將該模擬器捷徑的路徑填入 MAA 的 `啟動設定 - 模擬器路徑` 中。

::: details 範例

```text
多開 1:
模擬器路徑: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\多開1.lnk
多開 2:
模擬器路徑: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\多開2-明日方舟.lnk
```

:::

若使用 `模擬器路徑` （捷徑方式）進行多開操作，建議將 `啟動設定 - 附加指令` 欄位清空。
