---
order: 5
icon: ri:earth-fill
---
# 外服適配教學

## 準備工作

在開始這個教學之前，請確保你已經：

1. 安裝並正確配置了所需軟體。在陸服或相應用戶端的 `readme.md` 中應該會有相關資訊，確保所支援的功能可以正常執行。
2. 閱讀了 `3.4-任務流程協議.md`，對各個欄位的含義和用法有基本瞭解，並能理解 `@`、`#` 類型任務的含義和用法。
3. 瞭解外服的 `task.json` 和範本圖片中未提及的和缺少的內容會使用陸服的 `task.json` 和範本圖片等內容作為備選。外服的 `task.json` 中的內容會覆蓋並重寫陸服對應任務的相應欄位。
4. 具備一定的英語能力，能夠閱讀英文日誌，並能通過日誌找出缺失的圖片等資訊。
5. 建議按照任務鏈進行修改。例如，對於 `Award` 任務，根據陸服 `task.json` 中 `Award` 任務的 `next` 順序逐步替換 `範本圖片` / `文字` / `修改 roi`，以確保修改後的每一步都能正常執行，或者能夠迅速發現錯誤。這樣可以避免因為一次修改修改了太多內容而不清楚程式因為哪一步卡住而不能執行。

### 修改前準備

在進行修改之前，有幾個準備工作需要注意：

1. 參考陸服 task.json，確保你已經準備好了與陸服不同的用於外服的範本圖片和文字內容。
2. 確保你能夠隨時獲取這些圖片和文字內容。

## 獲取截圖

為了獲取高品質的截圖，請依照以下說明：

1. 使用模擬器內建的截圖工具進行截圖並保存。
2. 確保截圖的尺寸大於 `1280*720`，長寬比為 `16:9`。
3. 確保截圖中不包含任何無關內容，例如任務欄、狀態欄、通知欄等。
4. 確保截圖中包含所有需要辨識的內容。

為了裁剪圖片並獲取文字 / 圖片 `roi`，你需要安裝 `python` 和 `opencv`，並下載 `MaaAssistantArknights/tools/CropRoi/main.py` 檔案。

然後，按照以下步驟操作：

1. 在 `main.py` 同目錄下新建 `src` 和 `dst` 資料夾。
2. 將需要修改大小或獲取文字 / 圖片 `roi` 的 **完整截圖** 放入 `src` 資料夾。
3. 執行 `main.py`。
4. 使用滑鼠拖動選擇目標範圍，儘量不要包含無關內容。
5. 確定範圍後，按 `S` 鍵保存，按 `Q` 鍵退出。裁剪後的圖片將被保存在 `dst` 資料夾。

例如完成一次裁剪後的輸出內容為：

``` log
src: Screenshot_xxx.png
dst: Screenshot_xxx.png_426,272,177,201.png
original roi: 476, 322, 77, 101,
amplified roi: 426, 272, 177, 201
```

其中，

`Screenshot_xxx.png` 為放入 `src` 資料夾的完整截圖的名稱。`Screenshot_xxx.png_426,272,177,201.png` 為截取後的圖片。

`original roi` 為滑鼠選取的區域。`amplified roi` 為擴大後的區域，你需要的是擴大後的範圍，因此在 `task.json` 中的 `roi` 欄位填入的就是這個值。

## 修改範本圖片

在修改範本圖片之前，需要打開對應用戶端的範本圖片資料夾和陸服的範本圖片資料夾。

例如：

- 美服的範本圖片資料夾位置為 `MaaAssistantArknights\resource\global\YoStarEN\resource\template`。
- 陸服的範本圖片資料夾位置為 `MaaAssistantArknights\resource\template`。

參考 `task.json` 中提到的範本圖片，對比陸服和外服的範本圖片，找出外服中缺少的範本。

通常情況下，除了標誌等圖片，包含文字的範本都需要通過截圖來替換。如果圖片尺寸明顯大於陸服的對應範本圖片，則需要修改 `roi` 的大小。

將截取並重新命名完成的範本圖片放入對應用戶端的範本圖片資料夾。

## 修改文字內容

在修改文字內容之前，需要打開對應伺服器的 `task.json` 和陸服的 `task.json`。

例如：

- 美服的 `task.json` 位置為 `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`。
- 陸服的 `task.json` 位置為 `MaaAssistantArknights\resource\tasks.json`。

找到對應任務，將 `text` 欄位修改為對應伺服器內顯示的內容。注意，辨識的內容可以是遊戲內完整內容的字串。

通常情況下，除非是純 ASCII 字元辨識，否則包含文字的 `text` 都需要替換。

如果文字長度明顯大於陸服，則需要修改 `roi` 的大小，如 `"任務"` 和 `"Mission"` 長度差距過大，則需要修改外服該任務 `roi` 的大小。

如果對應外服的 `task.json` 中沒有該任務，則需要添加任務，只需要填寫 `text` 欄位即可。

## 修改 ROI 範圍

1. 打開對應伺服器的 `task.json`，如美服的位置為 `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`
2. 找到對應需要修改的 `roi` 範圍的任務，使用您準備好的外服遊戲介面截圖，根據 `amplified roi`，調整 `roi` 範圍的大小。
3. 通常情況下， `roi` 不需要修改，只有和陸服的辨識內容大小差距過大時才需要修改。
4. 如對應外服的 `task.json` 中任務不存在，則新增任務，寫上 `roi` 欄位。

## 保存設定並重新啟動軟體

在修改完成後，重新啟動軟體，重新載入檔案，使修改生效。

或在軟體目錄下新建一個 `DEBUG.txt`，這樣每次點擊開始都會重新載入範本和文件，不需要重開。

檢查是否成功：

1. 檢查軟體的執行情況，確保軟體能夠在外服中正常使用。
2. 如不能正常執行，需要檢查修改是否正確，或查看日誌輸出，進而找到出錯的地方。

## 解讀日誌

有些時候，我們修改完了 `task.json` 之後發現程式仍然不能正確執行，這時候我們考慮考慮查看日誌找到出錯的地方，進而修改對應任務。

日誌檔的位置在軟體的根目錄下，檔案名為 `asst.log`。

如果你是自己編譯的 MAA ，則在 `\x64\Release` 或 `x64\RelWithDebInfo` ，具體在哪個資料夾取決你編譯時選擇的編譯模式。

下面是一段日誌範例：

``` log
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

在這段日誌中，你可以看到：

- `taskchain` 代表當前進行中的任務。
- `details` 是任務的內容，包括需要辨識的欄位（`to_be_recognized`）和當前重試次數（`cur_retry`）和總重試次數（`retry_times`）。
- `first` 代表任務的開始。
- `taskid` 是任務的編號。
- `class` 和 `subtask` 分別代表任務的類別和子任務。
- `pre_task` 代表前一個任務。
此外，日誌中還會記錄命令的執行情況（如 `Call`）和 `OCR` 的資訊（如 `OcrPack::recognize`）。

在這段日誌中 `"to_be_recognized"`,`"cur_retry":3,"retry_times":20` 表示已經重複辨識了 10 次，最大辨識次數為 20 次，到了最大辨識次數後會跳過該任務並報錯，繼續執行下一個任務。

如果前面的任務沒有問題，我們基本可以確定是這裡的辨識出了問題，需要查看日誌中的提到的任務，尋找是否有對應的 `範本圖片`，對應任務的 `text` 是否錯誤，任務辨識 `roi` 範圍是否正確，進而找出問題所在並修改。

通過查看對應範本圖片，發現美服範本圖片資料夾中有這張圖的範本，但是大小明顯大於陸服圖片，導致陸服的 `roi` 用在美服上辨識不出來，所以需要修改美服的 `task.json` 中的對應任務 `roi`，使其與美服圖片大小對應。

## 提交你的修改

請參考 [Github Pull Request 說明](../develop/pr-tutorial.md)。
