---
order: 5
icon: ri:earth-fill
---

# 外服適配教學

## 準備工作

在開始本教學之前，請確保您已經：

1. 安裝並正確配置所需軟體：請參考官服或對應客戶端的 `README.md`，確保目前支援的功能皆可正常執行。
2. 閱讀 [任務流程協議](../protocol/task-schema.md)：對各欄位的含義與用法有基本了解，並理解 `@`、`#` 類型任務的定義。
3. 理解資源覆蓋邏輯：外服的 `task.json` 與範本圖片中未提及或缺失的內容，會自動以官服內容作為備選；若外服的 `task.json` 中已定義相關欄位，則會覆蓋並重寫官服的設定。
4. 具備基本英文能力：需能閱讀英文日誌（Log），並能透過日誌分析缺失的圖片或資訊。
5. 建議按任務鏈進行修改：例如針對 `Award` 任務，建議根據官服 `task.json` 的 `next` 順序，逐步替換「範本圖片」、「文字」或「修改 ROI」，確保每一步修改都能正常執行。這有助於快速定位錯誤，避免因一次改動過多而難以偵錯。

### 修改前準備

1. 參考官服 `task.json`，準備好外服專用的範本圖片與文字內容。
2. 確保在修改過程中能隨時獲取這些素材。

## 獲取螢幕截圖

為了獲取高品質的截圖，請遵循以下指南：

1. 使用模擬器內建的截圖工具進行擷取並儲存。
2. 截圖尺寸需大於 `1280*720`，長寬比為 `16:9`。
3. 截圖中不應包含無關內容（如：系統任務欄、狀態列、通知中心等）。
4. 確保截圖涵蓋所有需要辨識的內容。

為了裁剪圖片並獲取文字或圖片的 `ROI`，建議使用 `MaaAssistantArknights/tools/ImageCropper` 工具。

**ImageCropper** 是一個功能強大的工具，支援對預先準備好的截圖或透過 ADB 連線設備進行 ROI 區域擷取、儲存及取色操作。

### 環境配置

需要 `Python` 環境，推薦版本為 `3.11`，最低需求為 `3.9` 以上。

### 安裝依賴項目

Windows 用戶推薦直接執行 `install.bat`，或手動安裝：

```shell
python -m pip install -r requirements.txt
```

### 使用步驟

1. 若有預先準備好的截圖，請存放到 `./src/` 路徑下。
2. 執行 `start.bat` 或 `python main.py [device serial]`（設備位址為選填）。
   - 工具會自動搜尋已連線的 ADB 設備，請根據提示選擇（按 ENTER 跳過選擇）。
   - 亦可直接使用 `python main.py [device serial]` 連線指定設備。
3. 在視窗中左鍵選擇目標區域，滾輪縮放圖片，右鍵移動圖片。
4. 可使用快捷鍵操作：
   - `S` 或 `ENTER`：儲存目標區域。
   - `F`：儲存全螢幕標準化截圖。
   - `R`：不儲存，僅在控制台輸出 ROI 範圍。
   - `C`：不儲存，輸出 ROI 範圍與 ColorMatch 所需欄位。
   - `Z`、`DELETE` 或 `BACKSPACE`：復原。
   - `0` ~ `9`：縮放視窗。
   - `Q` 或 `ESC`：退出。
   - 任意鍵：跳過或重新整理目前截圖。
5. 裁剪後的圖片會儲存在 `./dst/` 路徑下。

範例輸出內容：

```log
src: Screenshot_xxx.png
dst: Screenshot_xxx.png_426,272,177,201.png
original roi: 476, 322, 77, 101,
amplified roi: 426, 272, 177, 201
```

其中：

`Screenshot_xxx.png` 為放入 `src` 資料夾的完整截圖檔案名稱。`Screenshot_xxx.png_426,272,177,201.png` 為截取後的圖片。

`original roi` 為滑鼠選取的原始區域。`amplified roi` 為擴大後的區域，您在 `task.json` 中的 `roi` 欄位中應填入此數值。

## 修改範本圖片

在修改範本圖片之前，需要先開啟對應客戶端的範本圖片資料夾，以及官服的範本圖片資料夾。

例如：

- 美服的範本圖片資料夾位置為 `MaaAssistantArknights\resource\global\YoStarEN\resource\template`。
- 官服的範本圖片資料夾位置為 `MaaAssistantArknights\resource\template`。

請參考 `task.json` 中提到的範本圖片，對比官服和外服的範本圖片，找出外服中缺少的範本。

通常情況下，除了標誌 (Logo) 等圖片外，包含文字的範本都需要透過螢幕截圖來替換。如果圖片尺寸明顯大於官服對應的範本圖片，則需要修改 `roi` 的大小。

將截取並重新命名完成的範本圖片，放入對應客戶端的範本圖片資料夾中。

## 修改文字內容

在修改文字內容之前，需要開啟對應客戶端的 `task.json` 與官服的 `task.json`。

例如：

- 美服的 `task.json` 位置為 `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`。
- 官服的 `task.json` 位置為 `MaaAssistantArknights\resource\tasks.json`。

找到對應任務後，將 `text` 欄位修改為該客戶端內顯示的內容。請注意，辨識內容可以是遊戲中完整內容的部分字串。

通常情況下，除非是純 ASCII 字元辨識，否則包含文字的 `text` 都建議替換。若文字長度明顯大於官服（例如 `"任務"` 與 `"Mission"` 長度差距過大），則需要調整外服該任務的 `roi` 大小。

若對應客戶端的 `task.json` 中沒有該任務，則需手動新增任務，並填寫 `text` 欄位即可。

## 修改 ROI 範圍

1. 開啟對應客戶端的 `task.json`（例如美服路徑： `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`）。
2. 找到對應需要修改 `roi` 範圍的任務，使用您準備好的外服截圖，根據 `amplified roi` 數值來調整 `roi` 的範圍大小。
3. 通常情況下 `roi` 不需要修改，只有當辨識內容與官服的大小差距過大時才需要調整。
4. 若外服客戶端的 `task.json` 中該任務不存在，則手動新增並填上 `roi` 欄位。

## 儲存設定並重新啟動軟體

修改完成後，重新啟動軟體以載入文件並使修改生效。或者，您也可以在軟體目錄下新建一個 `DEBUG.txt` 再開啟軟體，如此一來每次點擊「Link Start」時程式都會重新載入範本與檔案，無需反覆重啟。

檢查是否成功：

1. 檢查軟體執行狀況，確保軟體能在外服環境中正常使用。
2. 若無法正常運作，請檢查修改內容是否正確，或查看日誌輸出來定位出錯位置。

## 解讀日誌 (Log)

有時候修改完 `task.json` 後發現程式仍無法正確執行，此時可以透過查看日誌來找出錯誤點，進而修改對應任務。

日誌檔案位於軟體根目錄下，檔名為 `asst.log`。如果您是自行編譯 MAA，則日誌會在 `\x64\Release` 或 `x64\RelWithDebInfo` 資料夾中（具體路徑視編譯模式而定）。

以下是一段日誌範例：

```log
[2022-12-18 17:43:17.535][INF][Px7ec][Tx15c8] {"taskchain":"Award","details":{"to_be_recognized":["Award@ReturnTo","Award","ReceiveAward","DailyTask","WeeklyTask","Award@CloseAnno","Award@CloseAnnoTexas","Award@TodaysSupplies","Award@FromStageSN"],"cur_retry":10,"retry_times":20},"first":["AwardBegin"],"taskid":2,"class":"asst::ProcessTask","subtask":"ProcessTask","pre_task":"AwardBegin"}
[2022-12-18 17:43:18.398][INF][Px7ec][Tx15c8] Call ` C:\Program Files\BlueStacks_nxt\.\HD-Adb.exe -s 127.0.0.1:5555 exec-out "screencap | gzip -1" ` ret 0 , cost 862 ms , stdout size: 2074904 , socket size: 0
[2022-12-18 17:43:18.541][TRC][Px7ec][Tx15c8] OcrPack::recognize | roi: [ 500, 50, 300, 150 ]
[2022-12-18 17:43:18.541][TRC][Px7ec][Tx15c8] Ocr Pipeline with asst::WordOcr | enter
[2022-12-18 17:43:18.634][TRC][Px7ec][Tx15c8] Ocr Pipeline with asst::WordOcr | leave, 93 ms
[2022-12-18 17:43:18.634][TRC][Px7ec][Tx15c8] OcrPack::recognize | raw: [{ : [ 0, 0, 300, 150 ], score: 0.000000 }]
[2022-12-18 17:43:18.634][TRC][Px7ec][Tx15c8] OcrPack::recognize | proc: []
[2022-12-18 17:43:18.637][TRC][Px7ec][Tx15c8] asst::ProcessTask::_run | leave, 1101 ms
[2022-12-18 17:43:18.638][TRC][Px7ec][Tx15c8] ready to sleep 500
[2022-12-18 17:43:19.144][TRC][Px7ec][Tx15c8] end of sleep 500
[2022-12-18 17:43:19.144][TRC][Px7ec][Tx15c8] asst::ProcessTask::_run | enter
```

在範例日誌中，您可以看到：

- `taskchain`：代表目前執行中的任務鏈。
- `details`：任務的具體內容，包含待辨識欄位 (`to_be_recognized`)、目前重試次數 (`cur_retry`) 以及總重試次數 (`retry_times`)。
- `first`：代表任務的起始。
- `taskid`：任務編號。
- `class` 和 `subtask`：分別代表任務類別與子任務。
- `pre_task`：代表前一個執行的任務。
  此外，日誌還會記錄指令的執行狀況（如 `Call`）和 `OCR` 與 OCR 辨識資訊（如 `OcrPack::recognize`）。

在範例日誌中，`"to_be_recognized"`,`"cur_retry":3,"retry_times":20` 表示已重複辨識 3 次（上限為 20 次），一旦達到上限就會跳過該任務並報錯。若先前的任務沒問題，基本可以確定是此處辨識出錯。此時應檢查日誌提到的任務，確認是否有對應的範本圖片檔案、`text` 文字是否正確、以及 `roi` 範圍是否精準？

透過檢查對應範本圖片，若發現外服範本資料夾中有該圖片，但尺寸明顯大於官服圖片，導致官服的 `roi` 設定無法正常辨識，此時就必須修改外服客戶端的 `task.json` 中的 `roi`，使其與外服圖片大小相符。

## 提交您的修改

請參考 [GitHub Pull Request 指南](./pr-tutorial.md)
