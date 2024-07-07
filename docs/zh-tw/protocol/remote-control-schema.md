---
order: 8
icon: mdi:remote-desktop
---

# 遠程控制協議

要實現對 MAA 的遠程控制，你需要提供一個服務，該服務必須是 http(s) 服務，並且提供下面兩個可匿名訪問的端點（Endpoint）。這兩個端點必須是 http(s) 協議的 web 端點。

::: warning
如果該端點為 http 協議，MAA 會在每次連接時發出不安全警告。**在公網部署明文傳輸服務是一種非常不推薦且危險的行為，僅供測試使用。**
:::

::: tip
請注意 JSON 文件是不支持註釋的，文本中的註釋僅用於演示，請勿直接複製使用
:::

## 獲取任務端點

MAA 會以 1 秒的間隔持續輪詢這個端點，嘗試獲取它要執行的任務，並按照獲取到的列表按順序執行。

端點路徑隨意，但是必須是 http(s) 端點。比如：`https://your-control-host.net/maa/getTask`

被控 MAA 需要將該端點填寫到 MAA 配置的`獲取任務端點`文本框中。

該端點必需能夠接受一個 `Content-Type=application/json` 的 POST 請求，並該請求必須可以接受下面這個 Json 作為請求的 content：

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",          // 用戶在MAA設置中填寫的用戶標識符。
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec"         // MAA自動生成的設備標識符。
    ...     // 如果你的這個端點還有其他用途，你可以自行添加可選的參數，但是MAA只會傳遞user和device
}
```

該端點必須返回一個 Json 格式的 Response，並且至少要滿足下列格式：

```json
{
    "tasks":                            // 需要讓MAA執行的Task的列表，目前可以支持的類型如示例中所示，如果不存在tasks則視為連接無效。
    [
        // 順序執行的任務：下面這些任務會按照下發的順序排隊執行
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任務的唯一id，字符串類型，在匯報任務時會使用
            "type": "CaptureImage",                         //截圖任務，會截取一張當前模擬器的截圖，並以Base64字符串的形式放在匯報任務的payload裡。如果你需要下發這種類型的任務，請務必注意你的端點可接受的最大請求大小，因為截圖會有數十MB，會超過一般網關的默認大小限制。
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   //任務的唯一id，字符串類型，在匯報任務時會使用
            "type": "LinkStart",                            //啟動一鍵長草
        },
        {
            "id": "15be4725-5bd3-443d-8ae3-0a5ae789254c",   //任務的唯一id，字符串類型，在匯報任務時會使用
            "type": "LinkStart-Recruiting",                 //立即根據當前配置，單獨執行一鍵長草中的對應子功能，無視主界面上該功能的勾選框。這一類Type的可選值詳見下述
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任務的唯一id，字符串類型，在匯報任務時會使用
            "type": "Toolbox-GachaOnce",                    //工具箱中的牛牛抽卡任務，該類Type的可選取值為：Toolbox-GachaOnce, Toolbox-GachaTenTimes
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任務的唯一id，字符串類型，在匯報任務時會使用
            "type": "Settings-ConnectionAddress",           //修改配置項的任務，等同於執行ConfigurationHelper.SetValue("ConnectionAddress", params); 為了安全起見，不是每個配置都可以修改，能修改的配置詳見下述。
            "params": "value"                               //要修改的值
        },
        // 立即執行任務：下面這些任務可以在順序執行任務運行中執行，並且MAA保證下面的任何一個任務都會盡快返回結果，通常用於對遠程控制功能本身的控制。
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任務的唯一id，字符串類型，在匯報任務時會使用
            "type": "CaptureImageNow",                      //立刻截圖任務，和上面的截圖任務是基本一樣的，唯一的區別是這個任務會立刻被運行，而不會等待其他任務。
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任務的唯一id，字符串類型，在匯報任務時會使用
            "type": "StopTask",                             //"結束當前任務"任務，將會嘗試結束當前運行的任務。如果任務列表還有其他任務會繼續開始執行下一個。該任務不會等待並確認當前任務已停止才會返回，因此請使用心跳任務來確認停止命令是否已生效。
        },
        {
            "id": "b353c469-b902-4357-bd8f-d133199eea31",   //任務的唯一id，字符串類型，在匯報任務時會使用
            "type": "HeartBeat",                            //心跳任務，該任務會立即返回，並且將當前“順序執行的任務”隊列中正在執行的任務的Id作為Payload返回，如果當前沒有任務執行，返回空字符串。
        },
    ],
    ...     // 如果你的這個端點還有其他用途，你可以自行添加可選的返回值，但是MAA只會讀取tasks
}
```

這些任務會被按順序執行，也就是說如果你先發下一個公招任務，再發下一個截圖任務，則截圖會在公招任務結束後執行。
該端點應當可以重入並且重複返回需要執行的任務，MAA 會自動記錄任務 id，對於相同的 id，不會重複執行。

::: note

- LinkStart-[TaskName] 型的任務 type 的可選值為 LinkStart-Base，LinkStart-WakeUp，LinkStart-Combat，LinkStart-Recruiting，LinkStart-Mall，LinkStart-Mission，LinkStart-AutoRoguelike，LinkStart-ReclamationAlgorithm
- Settings-[SettingsName] 型的任務的 type 的可選值為 Settings-ConnectionAddress, Settings-Stage1
- Settings 系列任務仍然是要按順序執行的，並不會在收到任務的時候立刻執行，而是排在上一個任務的後面
- 多個立即執行的任務也會按下發順序執行，只不過這些任務的執行速度都很快，通常來說，並不需要關注他們的順序。
:::

## 匯報任務端點

每當 MAA 執行完一個任務，它就會通過該端點將任務的執行結果匯報給遠端。

端點路徑隨意，但是必須是 http(s) 端點。比如：`https://your-control-host.net/maa/reportStatus`

被控 MAA 需要將該端點填寫到 MAA 配置的 `匯報任務端點` 文本框中。

該端點必需能夠接受一個 `Content-Type=application/json` 的 POST 請求，並該請求必須可以接受下面這個 Json 作為請求的 content：

```json
{
    "user":"ea6c39eb-a45f-4d82-9ecc-33a7bf2ae4dc",          // 用戶在MAA設置中填寫的用戶標識符。
    "device":"f7cd9682-3de9-4eef-9137-ec124ea9e9ec",        // MAA自動生成的設備標識符。
    "task":"15be4725-5bd3-443d-8ae3-0a5ae789254c",          // 要匯報的任務的Id，和獲取任務時的Id對應。
    "status":"SUCCESS",                                     // 任務執行結果，SUCCESS或者FAILED。一般不論任務執行成功與否只會返回SUCCESS，只有特殊情況才會返回FAILED，會返回FAILED的情況，會在上面的任務介紹時明確說明。
    "payload":"",                                           //匯報時攜帶的數據，字符串類型。具體取決於任務類型，比如截圖任務匯報時，這裡就會攜帶截圖的Base64字符串。
    ...     // 如果你的這個端點還有其他用途，你可以自行添加可選的參數，但是MAA只會傳遞user和device
}
```

該端點的返回內容任意，但是如果你不返回 200OK，會在 MAA 端彈出一個 Notification，顯示 `上傳失敗`

## 範例工作流-用 QQBot 控制 MAA

A 開發者想要用自己的 QQBot 控制 MAA，於是他開發了一個後端，暴露在公網上，提供兩個端點：

```text
https://myqqbot.com/maa/getTask
https://myqqbot.com/maa/reportStatus
```

為了讓用戶用的更方便，他的 getTask 接口不管接收什麼參數都默認返回 200OK 和一個空的 tasks 列表。
每次他接收到一個請求，他就去數據庫裡看一下有沒有重複的 device，如果沒有，他就將該 device 和 user 記錄在數據庫。
也就是說，在這個工作流下，這個接口同時還承擔了用戶註冊的功能。

他在 QQBot 上提供了一條指令，供用戶提交自己的 deviceId。

在它的 QQBot 的使用說明上，他告訴用戶，在 MAA 的用戶標識符中填寫自己的 QQ 號，然後將設備標識符通過 QQ 聊天發送給 Bot。

QQBot 在收到標識符後，再根據消息中的用戶 QQ 號，尋找數據庫中是否有對應的數據，如果沒有，則叫用戶先去配置 MAA。

因為 MAA 在配置好後就會持續的發送請求，因此如果用戶配置好了 MAA，在他通過 QQ 提交時，數據庫內應該有匹配的記錄。

這時 Bot 將數據庫內的該記錄設置一個已驗證標記，未來 getTask 再使用這套 device 和 user 請求時，就會返回真正的任務列表。

當用戶通過 QQBot 提交指令後，Bot 將一條任務寫入數據庫，這樣稍後，getTask 就會返回這條任務。並且，該 QQbot 還很貼心的，在每次用戶提交指令後，都默認再附加一個截圖任務。

MAA 在任務執行完後，會調用 reportStatus 匯報結果，Bot 在收到結果後，在 QQ 端發送消息通知用戶以及展示截圖。

## 範例工作流-用網站控制 MAA

B 開發者寫了一個網站，設想通過網站批量管理 MAA，因此，他擁有一套自己的用戶管理系統。但是它的後端在公網上，提供兩個可匿名訪問的端點：

```text
https://mywebsite.com/maa/getTask
https://mywebsite.com/maa/reportStatus
```

在網站上，有個連接 MAA 實例的界面，會展示一個 B 開發者稱之為 用戶密鑰 的隨機字符串，並有一個填入設備 id 的文本框。

網站要求用戶在 MAA 的 用戶標識符 中填寫自己的用戶密鑰，然後將 設備標識符 填入網站。

只有在網站上成功創建了 MAA 連接，getTask 才會返回 200OK，其他時候都返回 401Unauthorized。

因此如果用戶在 MAA 上填錯了，按下測試連接按鈕，會得到測試失敗的提示。

用戶可以在網站上下發任務，為任務排隊，查看截圖等等，這些功能的實現和上面 QQBot 例子類似，都是通過 getTask 和 reportStatus 組合完成。
