---
icon: ri:windows-fill
---
# Windows 模擬器支援

::: tip
遇到問題請先參考 [常見問題](1.2-常見問題.md)
:::

以下模擬器排序為隨機產生，排名不分先後

<script setup>
import MarkdownIt from 'markdown-it'
import MarkdownItAnchor from 'markdown-it-anchor'

const shuffleArray = (array) => {
    for (let i = array.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [array[i], array[j]] = [array[j], array[i]];
    }
    return array;
}

const fullySupport = shuffleArray([
`
### ✅ [藍疊模擬器 5](https://www.bluestacks.cn/)
完美支援。需要在模擬器 \`設定\` - \`引擎設定\` 中打開 \`允許 ADB 連接\`
`,
`
### ✅ [藍疊模擬器國際版5](https://www.bluestacks.com/tw/index.html)（最穩定👍）

完美支援。需要在模擬器 \`設定\` - \`進階\` 中打開 \`Android 調試橋\`。

- 若網絡環境較差可嘗試下載 [離線安裝程式](https://support.bluestacks.com/hc/zh-tw/articles/4402611273485-BlueStacks-5-%E9%9B%A2%E7%B7%9A%E5%AE%89%E8%A3%9D%E7%A8%8B%E5%BC%8F)；
- 若 adb 通訊埠號不斷的無規律變動，每次啟動都不相同，可能是因為您的電腦開啟了 [Hyper-V](https://support.bluestacks.com/hc/zh-tw/articles/4415238471053-BlueStacks-5-%E6%94%AF%E6%8F%B4-Hyper-V-%E7%9A%84-Windows-10-%E5%92%8C-11-%E4%B8%8A%E7%9A%84%E9%9B%BB%E8%85%A6%E8%A6%8F%E6%A0%BC%E9%9C%80%E6%B1%82)，
    MAA 現在會嘗試自動讀取藍疊模擬器配置檔案內的通訊埠號，若該功能失效 / 你有多開需求 / 安裝了多個模擬器核心，請參考 [常見問題](1.2-%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98.html#藍疊模擬器每次啟動通訊埠號都不一樣-hyper-v) 做出修改。
`,
`
### ✅ [夜神模擬器](https://www.yeshen.com/)

完美支援。
`,
`
### ✅ [夜神模擬器 安卓 9](https://www.yeshen.com/)

完美支援。
`,
`
### ✅ [逍遙模擬器](https://www.xyaz.cn/)

完美支援，但測試較少。
`,
`
### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

完美支援。
`,
`
### ✅ [MuMu 模擬器 12](https://mumu.163.com/)（最流暢👍）

完美支援。

- “完成後退出模擬器” 功能可能偶發異常，如果遇到請向 MuMu 官方反饋；
- 高版本 MuMu 12 引入的 “後台保活” 功能會導致截圖失敗，如您使用大於等於 3.5.4 版本的 MuMu 12，請確認 MuMu 12 設定 - 其他 中將 “後台掛機時保活運行” 功能關閉（詳見[官方公告](https://mumu.163.com/help/20230802/35047_1102450.html)）；
- 多開時需通過 MuMu 12 多開器的 ADB 按鈕，查看對應實例的通訊埠資訊，將 MAA 設定 - 連接設定 的連接地址的通訊埠號修改為對應的通訊埠。
`,
`
### ✅ [雷電模擬器](https://www.ldmnq.com/)

完美支援。

- **雷電 9 推薦使用 9.0.57 及以上版本；雷電 5 推薦使用 5.0.67 及以上版本；**
- 低於上述版本則需要在 MAA 設定 - 連接設定 中，進行 \`強制替換 ADB\`，才能使用 minitouch, maatouch 等高效的觸控模式。
`,
])

const particallySupport = shuffleArray([
`
### ⚠️ [MuMu 模擬器 6](https://mumu.163.com/)

支援。但：

- 需要在 MAA 設定 - 連接設定 中，進行 \`強制替換 ADB\`，才能使用 minitouch, maatouch 等高效的觸控模式；
- 需要使用系統管理員身分執行 MAA 才能自動獲取 adb 路徑和地址（因為 MuMu 6 本身是以系統管理員身分啟動的）；
- 需要使用系統管理員身分執行 MAA 才能支援 “完成後退出模擬器” 相關功能；
- 不推薦使用 MuMu 6 預設的幾個奇葩解析度，最好改成主流的 \`1280x720\`, \`1920x1080\`, \`2560x1440\` 等；
- MuMu 6 多開使用的是同一個 adb 通訊埠，所以無法支援多開的 MuMu 6。
`,
`
### ⚠️ [Win11 WSA](https://docs.microsoft.com/zh-cn/windows/android/wsa/)

勉強支援。

- 需要使用 [自定義連接](1.1-詳細介紹.html#自定義連接) 的方式來連接；
- WSA 2204 或更高版本（版本號在子系統設定的 \`關於\` 頁面中），連接配置選擇 \`通用配置\`；
- WSA 2203 或更老版本（版本號在子系統設定頁面的上方），連接配置選擇 \`WSA 舊版本\`；
- 由於本軟體僅對 720p 以上 \`16:9\` 解析度支援較好，所以請手動調整視窗大小，盡量貼近 16:9 比例。（如果你的螢幕是 16:9 的，可以直接按 \`F11\` 全螢幕）；
- 任務執行過程中，請盡量保證明日方舟在前台，且無其他安卓應用同時在前台執行，否則可能導致遊戲暫停執行或任務辨識錯誤；
- WSA 的截圖經常莫名其妙截出來一個白畫面，導致辨識異常，還是不推薦使用。
`,
])

const notSupport = shuffleArray([
`
### 🚫 MuMu 手遊助手（星雲引擎）

不支援，未開放 adb 通訊埠。
`,
`
### 🚫 騰訊手遊助手

不支援，未開放 adb 通訊埠。
`,
`
### 🚫 [Google Play 遊戲 Beta](https://play.google.com/googleplaygames)

不支援，[消費者用戶端](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)未開放 adb 通訊埠。
`,
])

const md = (new MarkdownIt()).use(MarkdownItAnchor, { permalink: MarkdownItAnchor.permalink.linkInsideHeader()})

const fullySupportHtml = md.render(fullySupport.join(''))
const partiallySupportHtml = md.render(particallySupport.join(''))
const notSupportHtml = md.render(notSupport.join(''))

</script>

## ✅ 完美支援

<ClientOnly><div v-html="fullySupportHtml"></div></ClientOnly>

## ⚠️ 部分支援

<ClientOnly><div v-html="partiallySupportHtml"></div></ClientOnly>

## 🚫 不支援

<ClientOnly><div v-html="notSupportHtml"></div></ClientOnly>

## ⚙️ 手機、平板等實體**安卓**設備

::: info 注意
本段內容雖已盡可能寫的簡潔易懂，但也許仍對部分萌新用戶不甚友好。若嫌麻煩、看不懂或操作不清還請繼續使用模擬器
:::

以 Windows 用戶為主，其他系統請依樣畫葫蘆。

- 由於 MAA 僅對 `16:9` 比例的 `720p` 及更高解析度的支援較為完善，所以非 `16:9` 或 `9:16` 螢幕比例的設備需要強制修改解析度，這包含大多數現代手機。
- 若被連接設備螢幕解析度比例原生為 `16:9` 或 `9:16`，則可跳過 `更改解析度` 部分。

::: tip
典型的 `16:9` 比例的解析度有 `3840*2160`、`2560*1440`、`1920*1080`、`1280*720` 等
:::

### 下載、執行 adb 偵錯工具並連接設備

1. 下載 [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) 並解壓縮。
2. 打開解壓縮後的資料夾，清空地址欄並輸入 `cmd` 後 Enter。
3. 在彈出的命令提示視窗中輸入 `adb` ，若給出大量英文幫助文字則執行成功。
4. 手機開啟 `USB 偵錯`，具體步驟可使用搜尋引擎搜尋 `<機型> + 開啟 USB 偵錯`。
5. 將手機通過數據線連接至電腦，在剛剛的命令提示視窗中輸入以下命令。

    ```bash
    adb devices
    ```

- 成功執行後會給出已連接 `USB 偵錯` 設備的資訊。

  - 連接成功的例子：

      ```bash
      List of devices attached
      VFNDU1682100xxxx        device
      ```

  - **`device` 前的英文數字組合為設備序列號，同時也作為 MAA 的 `連接地址`。**

- 現代安卓設備進行 `USB 偵錯` 需在被偵錯設備上點擊彈窗授權，若未授權則例子如下：

    ```bash
    List of devices attached
    VFNDU1682100xxxx        unauthorized
    ```

- 若無論如何都提示未授權或設備序列號後顯示 `offline`，則需重開設備及電腦後重試。如仍未解決問題，可刪除目前用戶個人資料夾下的 `.android` 資料夾並再次重開後重試，具體位置請自行搜尋。

### 更改解析度

::: tip
手機螢幕解析度為 `短邊 * 長邊`，而非電腦顯示器的 `長邊 * 短邊`。具體數值請根據目標設備自行確定。
:::

- 如果上文設備列表內僅有一台設備，則可直接執行以下命令更改 / 還原解析度。

    ```bash
    adb shell wm size               # 查看目前解析度
    adb shell wm size reset         # 還原預設解析度
    
    adb shell wm size 720x1280      # 更改解析度為 720p
    adb shell wm size 1080x1920     # 更改解析度為 1080p
    ```

- 若存在多台設備，則需在 `adb` 和 `shell` 中間添加參數 `-s <目標設備序列號>`，例子如下。

    ```bash
    adb -s VFNDU1682100xxxx shell wm size               # 查看目前解析度
    adb -s VFNDU1682100xxxx shell wm size reset         # 還原預設解析度
    
    adb -s VFNDU1682100xxxx shell wm size 720x1280      # 更改解析度為 720p
    adb -s VFNDU1682100xxxx shell wm size 1080x1920     # 更改解析度為 1080p
    ```

- 部分設計不規則的應用可能在還原解析度後，內容佈局仍然錯亂，一般重開對應應用或設備即可解決。

::: danger 注意
務必於**重開設備前**還原解析度 ，否則因設備而定可能會導致不可預料的後果 ~~，包括但不限於佈局錯亂，應用閃退，無法開機等~~ 
:::

### 自動化更改解析度

1. 在 MAA 目錄下新增兩個文字檔案，分別在其中填入以下內容。

    ```bash
    ::調整解析度為 1080p
    adb -s <目標設備序列號> shell wm size 1080x1920
    ::降低螢幕亮度（可選）
    adb -s <目標設備序列號> shell settings put system screen_brightness 1 
    ```

    ```bash
    ::還原解析度
    adb -s <目標設備序列號> shell wm size reset
    ::提高螢幕亮度（可選）
    adb -s <目標設備序列號> shell settings put system screen_brightness 20
    ::返回桌面（可選）
    adb -s <目標設備序列號> shell input keyevent 3
    ::鎖屏（可選）
    adb -s <目標設備序列號> shell input keyevent 26
    ```

2. 將第一個檔案重新命名為 `startup.bat`，第二個檔案重新命名為 `finish.bat`。

    - 如果重新命名後沒有彈出修改擴展名的二次確認對話框，且檔案圖示沒有變化，請自行搜尋 “Windows 如何顯示副檔名”。

3. 在 MAA 的 `設定` - `連接設定` - `開始前腳本` 和 `結束後腳本` 中分別填入 `startup.bat` 和 `finish.bat`。

### 連接 MAA

1. 將上文解壓縮資料夾內的 `adb.exe` 路徑填入 MAA `設定` - `連接設定` - `adb 路徑` 中，可右鍵屬性查看路徑。
2. 因不同安卓版本輸出差異較大，請在 MAA `設定` - `連接設定` 中嘗試將 `連接配置` 修改為 `通用模式`、`兼容模式`、`第二解析度`、`通用模式（阻擋異常輸出）` 其中之一，直到某個模式可以連上並正常使用。
3. 將遊戲內設定中的 `異形螢幕 UI 適配` 一項調整為 0 以防止操作錯位。

#### 有線連接

1. 將上文獲取到的目標設備序列號填入 MAA `設定` - `連接設定` - `連接地址` 中。
2. Link Start!
3. 任務結束後還原設備解析度。

#### 無線連接

- 請確保設備與電腦處在同一區域網路下且能互相通訊。諸如 `AP 隔離`、`訪客網絡` 等設定會阻止設備間通訊，具體請查閱對應路由器文件。
- MAA 不支援 `adb pair` 無線配對方式連接，即通過安卓 11 及更新版本中開發者選項內的 `無線偵錯` 進行連接。
- 無線偵錯在設備重開後失效，需要重新設定。

1. 在剛剛的命令提示視窗中輸入以下命令以開啟無線偵錯。

    ```bash
    adb tcpip 5555     # 如存在多台設備可參照上文內容在 adb 和 tcpip 中間添加參數
    ```

2. 查看設備 IP 地址。

    - 進入手機 `設定` - `WLAN`，點擊目前已連接的無線網絡查看 IP 地址。
    - 各類品牌設備設定位置不同，請自行尋找。

3. 將 `<IP>:5555` 填入 MAA `設定` - `連接設定` - `連接地址` 中，如 `192.168.1.2:5555`。
4. Link Start!
5. 任務結束後還原設備解析度。

::: note
如連接失敗並提示 “發生未知錯誤”，有可能是觸控模式 `Minitouch` 的問題，可切換到 `MaaTouch` 再次嘗試。由於 `Adb Input` 操作過於緩慢，請僅將其作為萬不得已的模式。
::: 
