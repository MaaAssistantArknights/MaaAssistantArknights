---
order: 2
icon: basil:apple-solid
---
# Mac 模擬器

::: tip
遇到問題請先參考 [常見問題](../faq.md)
:::

## Apple Silicon 晶片

### ✅ [PlayCover](https://playcover.io)（原生執行最流暢🚀）

試驗性支援，遇到問題請多多提 issue，並在標題中提及 iOS。

注意：由於 `macOS` 本身機制的問題，將遊戲視窗最小化、台前調度狀態下切換到別的視窗、將視窗移動到別的桌面 / 螢幕之後，截圖會出現問題，導致無法正確執行。請參考👉🏻️[issue](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/4371#issuecomment-1527977512)

0. 要求：MAA 版本 v4.13.0-rc.1 以上

1. 下載 [fork 版本的 PlayCover](https://github.com/hguandl/PlayCover/releases) 並安裝。

2. 下載 [脫殼的明日方舟用戶端安裝包](https://decrypt.day/app/id1454663939)，並在 PlayCover 中安裝。

3. 在 PlayCover 中右鍵明日方舟，選擇 `設定` - `繞過`，勾選 `啟用 PlayChain`、`啟用繞過越獄檢測`、`插入內省庫`、`MaaTools`，然後點擊 `好`。

4. 此時再啟動明日方舟，即可正常執行。標題欄結尾會有 `[localhost:通訊埠號]`，說明已經成功啟用。

5. 在 MAA 中，點擊 `設定` - `連接設定`，`觸控模式` 選擇 `MacPlayTools`。`連接地址` 填入上面標題欄 `[]` 裡的內容。

6. 設定完成，MAA 可以連接了。如果遇到圖像辨識出錯，可以嘗試在 PlayCover 內將解析度設定為 1080P。

7. 3-5 步驟只需要做一次，之後只需要啟動明日方舟即可。在明日方舟每次更新用戶端之後，需要重新做第 2 步。

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

支援。

### ✅ [藍疊模擬器Air版](https://www.bluestacks.com/mac) （免費，針對 Apple M 系列晶片進行優化的版本）

支援，經測試，可用 maatouch 經`127.0.0.1:5555`連接。

需要在模擬器 **`設定`** - **`進階`** 中開啟 **`Android除錯（ADB）`**。

## Intel 晶片

::: tip
由於 Mac 版開發人手不足，更新速度相對較慢，更推薦使用 Mac 自帶的多系統安裝 Windows，並使用 Windows 版 MAA。
:::

### ✅ [藍疊模擬器](https://www.bluestacks.cn/)

完美支援。需要在模擬器 `設定` - `引擎設定` 中打開 `允許 ADB 連接`。

### ✅ [藍疊模擬器國際版](https://www.bluestacks.com/tw/index.html)

完美支援。需要在模擬器 `設定` - `進階` 中打開 `Android 調試橋`。

### ✅ [夜神模擬器](https://www.yeshen.com/)

完美支援。目前 mac 上 MAAX 版本還不支援模擬器自動適配，需要在 MAA `設定` - `連接設定` 中使用 `adb` 連接 `127.0.0.1:62001` ，注意通訊埠不是預設的 `5555` ，關於模擬器通訊埠的詳細說明，請參考 [常見模擬器 adb 通訊埠](../faq.md#常見安卓模擬器adb通訊埠)。

補充：mac 下夜神模擬器的 adb 二進制檔案的位置為 `/Applications/NoxAppPlayer.app/Contents/MacOS/adb`，在父目錄 `MacOS` 下可使用 `adb devices` 命令查看 adb 通訊埠。

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

支援。
