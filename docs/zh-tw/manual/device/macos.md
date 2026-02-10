---
order: 2
icon: basil:apple-solid
---

# Mac 模擬器

## Apple Silicon 晶片

### ✅ [PlayCover](https://playcover.io) 原生執行最流暢 🚀

實驗性支援，遇到問題請多加提交 Issue，並在標題中提及 `macOS`。

請注意：由於 `macOS` 本身機制的限制，將遊戲視窗最小化、幕前排程 (Stage Manager) 狀態下切換到其他視窗、或將視窗移動到其他桌面 / 螢幕後，截圖會出現問題，導致無法正確執行。相關 Issue [#4371](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/4371#issuecomment-1527977512)

0. 要求：MAA 版本 v4.13.0-rc.1 以上

1. 下載 [fork 版本的 PlayCover](https://github.com/hguandl/PlayCover/releases) 並安裝。

2. 下載 [脫殼的《明日方舟》客戶端安裝包](https://decrypt.day/app/id1454663939)，並在 PlayCover 中安裝。

3. 在 PlayCover 中右鍵點選《明日方舟》，選擇 `設定` - `繞過`，勾選 `啟用 PlayChain`、`啟用繞過越獄偵測`、`插入內省庫`、`MaaTools`，然後點選 `好`。

4. 此時再啟動《明日方舟》，即可正常執行。標題列結尾會有 `[localhost:連接埠號]`，說明已經成功啟用。

5. 在 MAA 中，點選 `設定 - 連線設定`，`觸控模式` 選擇 `MacPlayTools`。`連線位址` 填入上面標題列 `[]` 內容。

6. 設定完成，MAA 可以連線了。如果遇到圖像識別出錯，可以嘗試在 PlayCover 內將解析度設定為 1080P。

7. 3-5 步驟只需要做一次，之後只需要啟動《明日方舟》即可。在《明日方舟》每次更新客戶端之後，需要重新執行第 2 步。

### ✅ [MuMu 模擬器 Pro](https://mumu.163.com/mac/)

支援，但測試較少，需使用除 `MacPlayTools` 以外的觸控模式。相關 Issue [#8098](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8098)

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

支援，但從 Android 10 開始，Minitouch 在 SELinux 為 `Enforcing` 模式時不再可用。請切換至其他觸控模式，或將 SELinux **臨時**切換為 `Permissive` 模式。

### ✅ [BlueStacks 藍疊模擬器 Air 版](https://www.bluestacks.com/mac) （免費，針對 Apple M 系列晶片進行優化的版本）

支援，經測試，可用 maatouch 透過 `127.0.0.1:5555` 連線。

需要在模擬器 `設定` - `進階` 中打開 `Android 調試 (ADB)`。

## Intel 晶片

::: tip
由於 Mac 版開發人手較少，更新速度相對較慢，更推薦使用 Mac 內建的 Boot Camp 安裝 Windows，並使用 Windows 版 MAA。
:::

### ✅ [BlueStacks 藍疊模擬器](https://www.bluestacks.cn/)

完美支援。需要在模擬器 `設定` - `引擎設定` 中打開 `允許 ADB 連線`。

### ✅ [BlueStacks 藍疊模擬器國際版](https://www.bluestacks.com/tw/index.html)

完美支援。需要在模擬器 `設定` - `進階` 中打開 `Android 調試橋 (ADB)`。

### ✅ [夜神模擬器 (NoxPlayer)](https://www.yeshen.com/)

完美支援。

補充：mac 下夜神模擬器的 ADB 執行檔路徑為 `/Applications/NoxAppPlayer.app/Contents/MacOS/adb`，在父目錄 `MacOS` 下可使用 `adb devices` 指令查看 ADB 連接埠。

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

支援，但從 Android 10 開始，Minitouch 在 SELinux 為 `Enforcing` 模式時不再可用。請切換至其他觸控模式，或將 SELinux **臨時**切換為 `Permissive` 模式。
