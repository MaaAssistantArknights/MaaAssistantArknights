---
order: 8
icon: mdi:remote-desktop
---

# 遠端控制協定

若要實作對 MAA 的遠端控制，您須提供一個服務，該服務必須是 HTTP(S) 服務，並提供下列兩個可匿名存取的端點（Endpoint）。這兩個端點必須是 HTTP(S) 協定的 Web 端點。

::: warning
若該端點為 HTTP 協定，MAA 會在每次連線時發出不安全警告。**在外網上部署明文傳輸服務是極不推薦且危險的行為，僅供測試使用。**
:::

::: tip
請注意，json 檔案不支援註解。文件中的註解僅供說明參考，請勿直接複製使用。
:::

## 獲取任務端點

MAA 會以 1 秒的間隔持續輪詢此端點，嘗試獲取要執行的任務，並按照獲取到的列表依序執行。

端點路徑不限，但必須是 HTTP(S) 端點。例如：`https://your-control-host.net/maa/getTask`

被控端 MAA 須將該端點填寫至 MAA 設定的 `獲取任務端點` 文字框中。

該端點必須能接受 `Content-Type=application/json` 的 POST 請求，且該請求必須可接受下列 JSON 作為請求內容（Content）：

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",         // 使用者在 MAA 設定中填寫的使用者識別碼。
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec"         // MAA 自動產生的設備識別碼。
    ...     // 若您的端點有其他用途，可自行新增選填參數，但 MAA 只會傳遞 user 與 device。
}
```

該端點必須回傳 json 格式的 Response，且至少須滿足下列格式：

```json
{
    "tasks":                                // 須讓 MAA 執行的 Task 列表。目前支援的類型如範例所示，若不存在 tasks 則視為連線無效。
    [
        // 順序執行任務：下列任務會按照指派順序排隊執行
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // 任務唯一 ID（字串），彙報任務時會使用。
            "type": "CaptureImage"                          // 截圖任務。會擷取一張目前模擬器截圖，並以 Base64 字串形式置於彙報任務的 payload 中。若需下發此類型任務，請務必注意端點可接受的最大請求限制，因截圖可能達數十 MB，會超過一般網關的預設大小限制。
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   // 任務唯一 ID（字串），彙報任務時會使用。
            "type": "LinkStart"                             // 啟動一鍵長草。
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   // 任務唯一 ID（字串），彙報任務時會使用。
            "type": "LinkStart-Recruiting"                  // 立即根據當前設定，單獨執行一鍵長草中的對應子功能，無視主介面上該功能的勾選框。此類 Type 的可選值詳見下述。
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // 任務唯一 ID（字串），彙報任務時會使用。
            "type": "Toolbox-GachaOnce"                     // 工具箱抽卡任務。可選值：Toolbox-GachaOnce, Toolbox-GachaTenTimes。
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // 任務唯一 ID（字串），彙報任務時會使用。
            "type": "Settings-ConnectionAddress",           // 修改設定項任務，等同於執行 ConfigurationHelper.SetValue("ConnectionAddress", params)。為求安全，僅部分設定可修改，詳見下述。
            "params": "value"                               // 要修改的值。
        },
        // 立即執行任務：下列任務可在順序執行任務運作中執行，且 MAA 保證任何一個任務都會儘快回傳結果，通常用於控制遠端控制功能本身。
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // 任務唯一 ID（字串），彙報任務時會使用。
            "type": "CaptureImageNow"                       // 立刻截圖任務。與上方截圖任務基本相同，區別在於此任務會立刻執行，不會等待其他任務。
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // 任務唯一 ID（字串），彙報任務時會使用。
            "type": "StopTask"                              // 「結束目前任務」。將嘗試結束目前執行的任務。若列表內還有其他任務，將繼續執行下一個。該任務不會等待並確認目前任務已停止才回傳，請使用心跳任務確認停止指令是否生效。
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   // 任務唯一 ID（字串），彙報任務時會使用。
            "type": "HeartBeat"                             // 心跳任務。會立即回傳，並將目前「順序執行任務」隊列中正在執行的任務 ID 作為 Payload 回傳；若目前無任務執行，則回傳空字串。
        }
    ],
    ...     // 若您的端點有其他用途，可自行新增回傳值，但 MAA 只會讀取 tasks。
}
```

這些任務會依序執行，意即若先指派一項公招任務，再指派一項截圖任務，則截圖會在公招任務結束後才執行。
該端點應具備可重入性，並可重複回傳須執行的任務。MAA 會自動記錄任務 ID，相同 ID 不會重複執行。

::: note

- `LinkStart-[TaskName]` 型任務的 type 可選值為：LinkStart-Base, LinkStart-WakeUp, LinkStart-Combat, LinkStart-Recruiting, LinkStart-Mall, LinkStart-Mission, LinkStart-AutoRoguelike, LinkStart-Reclamation。
- `Settings-[SettingsName]` 型任務的 type 可選值為：Settings-ConnectionAddress, Settings-Stage1。
- Settings 系列任務仍須依序執行，收到任務時不會立刻執行，而是排在上一項任務之後。
- 多個立即執行任務亦會按部就班執行，但因執行速度極快，通常無需關注其順序。

:::

## 彙報任務端點

每當 MAA 執行完一項任務，就會透過此端點將執行結果彙報給遠端。

端點路徑不限，但必須是 HTTP(S) 端點。例如：`https://your-control-host.net/maa/reportStatus`

被控端 MAA 須將該端點填寫至 MAA 設定的 `彙報任務端點` 文字框中

該端點必須能接受 `Content-Type=application/json` 的 POST 請求，且請求內容（Content）須符合下列 json 格式：

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",         // 使用者在 MAA 設定中填寫的使用者識別碼。
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec",        // MAA 自動產生的設備識別碼。
    "task":"15be4725-5bd3-443d-8ae3-0a5ae789254c",          // 要彙報的任務 ID，與獲取任務時的 ID 對應。
    "status":"SUCCESS",                                     // 任務執行結果：SUCCESS 或 FAILED。通常不論成敗皆彙報 SUCCESS，僅特殊情形才會彙報 FAILED（相關情形於上方任務介紹中說明）。
    "payload":"",                                           // 彙報時攜帶的資料（字串）。視任務類型而定，如截圖任務彙報時，會攜帶截圖的 Base64 字串。
    ...     // 若您的端點有其他用途，可自行新增選填參數，但 MAA 只會傳遞 user 與 device。
}
```

該端點的回傳內容不限，但若未回傳 `200 OK`，MAA 端會彈出通知顯示 `上傳失敗`。

## 範例工作流——使用 QQBot 控制 MAA

開發者 A 想要用自己的 QQBot 控制 MAA，因此開發了一個後端服務並部署於外網上，提供兩個端點：

```text
https://myqqbot.com/maa/getTask
https://myqqbot.com/maa/reportStatus
```
為了提升使用者體驗，他的 getTask 介面不論接收何種參數，均預設回傳 200 OK 與空的 tasks 列表。   
每當接收到請求，他會檢查資料庫是否有重複的 device；若無，則將該 device 與 user 記錄在資料庫中。  
意即在此工作流下，該介面同時承擔了使用者註冊的功能。

他在 QQBot 上提供一條指令，供使用者提交自己的 deviceId。

在 QQBot 的使用說明中，他引導使用者在 MAA 的 `使用者識別碼` 填寫自己的 QQ 號，再將 `設備識別碼` 透過 QQ 傳送給 Bot。

QQBot 收到識別碼後，根據訊息中的使用者 QQ 號尋找資料庫是否有對應資料；若無，則要求使用者先設定 MAA。

由於 MAA 設定完成後會持續發送請求，因此當使用者透過 QQ 提交時，資料庫內理應已有匹配記錄。

此時 Bot 將資料庫內的該筆記錄標記為「已驗證」。日後 getTask 收到相同的 device 與 user 請求時，便會回傳真正的任務列表。

當使用者透過 QQBot 提交指令後，Bot 將一項任務寫入資料庫，稍後 getTask 便會回傳該任務。此外，該 QQBot 甚至在每次使用者提交指令後，均預設額外指派一個截圖任務。

MAA 執行完畢後會調用 reportStatus 彙報結果，Bot 收到結果後便於 QQ 端發送通知並展示截圖。

## 範例工作流——用網站控制 MAA

開發者 B 寫了一個網站，設想透過網站批次管理 MAA，因此擁有一套自有的使用者管理系統。他的後端部署於外網上，提供兩個可匿名存取的端點：

```text
https://mywebsite.com/maa/getTask
https://mywebsite.com/maa/reportStatus
```

網站上的連線介面會展示一個稱為 `使用者金鑰` 的隨機字串，並提供一個填入設備 ID 的文字框。

網站要求使用者在 MAA 的 `使用者識別碼` 填寫使用者金鑰，並將 `設備識別碼` 填入網站。

唯有在網站上成功建立 MAA 連線，getTask 才會回傳 200 OK，其餘情況均回傳 401 Unauthorized。

因此若使用者在 MAA 填錯資訊，按下測試連線按鈕時會提示測試失敗。

使用者可於網站上指派任務、排隊、查看截圖等，這些功能的實作方式與上述 QQBot 範例類似，皆透過 getTask 與 reportStatus 組合達成。