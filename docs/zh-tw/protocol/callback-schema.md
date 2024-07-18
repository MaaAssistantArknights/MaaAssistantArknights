---
order: 2
icon: material-symbols:u-turn-left
---
# 回呼訊息協議

::: info 注意
回呼訊息隨版本更新快速迭代中，本檔案可能過時。如需獲取最新内容可参考 [簡中檔案](../../zh-cn/protocol/callback-schema.md) 或 [C# 集成源碼](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/MaaWpfGui/Main/AsstProxy.cs)
:::

## 回調函數原型

```cpp
typedef void(ASST_CALL* AsstCallback)(int msg, const char* details, void* custom_arg);
```

## 參數總覽

- `int msg`  
    消息類型

    ```cpp
    enum class AsstMsg
    {
        /* Global Info */
        InternalError     = 0,           // 內部錯誤
        InitFailed        = 1,           // 初始化失敗
        ConnectionInfo    = 2,           // 連接相關資訊
        AllTasksCompleted = 3,           // 全部任務完成
        AsyncCallInfo = 4,               // 外部異步調用資訊

        /* TaskChain Info */
        TaskChainError     = 10000,      // 任務鏈執行/辨識錯誤
        TaskChainStart     = 10001,      // 任務鏈開始
        TaskChainCompleted = 10002,      // 任務鏈完成
        TaskChainExtraInfo = 10003,      // 任務鏈額外資訊
        TaskChainStopped   = 10004,      // 任務鏈手動停止

        /* SubTask Info */
        SubTaskError      = 20000,       // 原子任務執行/辨識錯誤
        SubTaskStart      = 20001,       // 原子任務開始
        SubTaskCompleted  = 20002,       // 原子任務完成
        SubTaskExtraInfo  = 20003,       // 原子任務額外資訊
        SubTaskStopped    = 20004,       // 原子任務手動停止
    };
    ```

- `const char* details`  
    消息詳情，json 字符串，詳見 [欄位解釋](#欄位解釋)
- `void* custom_arg`  
    調用方自定義參數，會原樣傳出 `AsstCreateEx` 接口中的 `custom_arg` 參數，C 系語言可利用該參數傳出 `this` 指針

## 欄位解釋

### InternalError

Todo

### InitFailed

```json
{
    "what": string,     // 錯誤類型
    "why": string,      // 錯誤原因
    "details": object   // 錯誤詳情
}
```

### ConnectionInfo

```json
{
    "what": string,     // 資訊類型
    "why": string,      // 資訊原因
    "uuid": string,     // 設備唯一碼（連接失敗時為空）
    "details": {
        "adb": string,     // AsstConnect 接口 adb_path 參數
        "address": string, // AsstConnect 接口 address 參數
        "config": string   // AsstConnect 接口 config 參數
    }
}
```

### 常見 `What` 欄位

- `ConnectFailed`  
    連接失敗
- `Connected`  
    已連接，注意此時的 `uuid` 欄位值為空（下一步才是獲取）
- `UuidGot`  
    已獲取到設備唯一碼
- `UnsupportedResolution`  
    解析度不被支援
- `ResolutionError`  
    解析度獲取錯誤
- `Reconnecting`  
    連接斷開（adb / 模擬器 炸了），正在重連
- `Reconnected`  
    連接斷開（adb / 模擬器 炸了），重連成功
- `Disconnect`  
    連接斷開（adb / 模擬器 炸了），並重試失敗
- `ScreencapFailed`  
    截圖失敗（adb / 模擬器 炸了），並重試失敗
- `TouchModeNotAvailable`  
    不支援的觸控模式

### AsyncCallInfo

```json
{
    "uuid": string,             // 設備唯一碼
    "what": string,             // 回調類型，"Connect" | "Click" | "Screencap" | ...
    "async_call_id": int,       // 異步請求 id，即調用 AsstAsyncXXX 時的返回值
    "details": {
        "ret": bool,            // 實際調用的返回值
        "cost": int64,          // 耗時，單位毫秒
    }
}
```

### AllTasksCompleted

```json
{
    "taskchain": string,            // 最後的任務鏈
    "uuid": string,                 // 設備唯一碼
    "finished_tasks": [             // 已經執行過的任務 id
        int,
        ...
    ]
}
```

#### 常見 `taskchain` 欄位

- `StartUp`  
    開始喚醒
- `CloseDown`  
    關閉遊戲
- `Fight`  
    刷理智
- `Mall`  
    信用點及購物
- `Recruit`  
    自動公招
- `Infrast`  
    基建換班
- `Award`  
    領取日常獎勵
- `Roguelike`  
    無限刷肉鴿
- `Copilot`  
    自動抄作業
- `SSSCopilot`  
    自動抄保全作業
- `Depot`  
    倉庫辨識
- `OperBox`  
    幹員 box 辨識
- `ReclamationAlgorithm`  
    生息演算
- `Custom`  
    自定義任務
- `SingleStep`  
    單步任務
- `VideoRecognition`  
    影片辨識任務
- `Debug`  
    偵錯

### TaskChain 相關消息

```json
{
    "taskchain": string,            // 當前的任務鏈
    "taskid": int,                  // 當前任務 TaskId
    "uuid": string                  // 設備唯一碼
}
```

### TaskChainExtraInfo

Todo

### SubTask 相關消息

```json
{
    "subtask": string,             // 子任務名
    "class": string,               // 子任務符號名
    "taskchain": string,           // 當前任務鏈
    "taskid": int,                 // 當前任務 TaskId
    "details": object,             // 詳情
    "uuid": string                 // 設備唯一碼
}
```

#### 常見 `subtask` 欄位

- `ProcessTask`  

    ```json
    // 對應的 details 欄位舉例
    {
        "task": "StartButton2",     // 任務名
        "action": 512,
        "exec_times": 1,            // 已執行次數
        "max_times": 999,           // 最大執行次數
        "algorithm": 0
    }
    ```

- Todo 其他

##### 常見 `task` 欄位

- `StartButton2`  
    開始戰鬥
- `MedicineConfirm`  
    使用理智藥
- `ExpiringMedicineConfirm`  
    使用 48 小時內過期的理智藥
- `StoneConfirm`  
    碎石
- `RecruitRefreshConfirm`  
    公招刷新標籤
- `RecruitConfirm`  
    公招確認招募
- `RecruitNowConfirm`  
    公招使用加急許可
- `ReportToPenguinStats`  
    匯報到企鵝數據統計
- `ReportToYituliu`  
    匯報到一圖流大數據
- `InfrastDormDoubleConfirmButton`  
    基建宿舍的二次確認按鈕，僅當幹員衝突時才會有，請提示用戶
- `StartExplore`  
    肉鴿開始探索
- `StageTraderInvestConfirm`  
    肉鴿投資了源石錠
- `StageTraderInvestSystemFull`  
    肉鴿投資達到了遊戲上限
- `ExitThenAbandon`  
    肉鴿放棄了本次探索
- `MissionCompletedFlag`  
    肉鴿戰鬥完成
- `MissionFailedFlag`  
    肉鴿戰鬥失敗
- `StageTraderEnter`  
    肉鴿關卡：詭異行商
- `StageSafeHouseEnter`  
    肉鴿關卡：安全的角落
- `StageEncounterEnter`  
    肉鴿關卡：不期而遇 / 古堡饋贈
- `StageCombatDpsEnter`  
    肉鴿關卡：普通作戰
- `StageEmergencyDps`  
    肉鴿關卡：緊急作戰
- `StageDreadfulFoe`  
    肉鴿關卡：險路惡敵
- `StartGameTask`
    打開用戶端失敗（配置檔案與傳入 client_type 不匹配）
- Todo 其他

### SubTaskExtraInfo

```json
{
    "taskchain": string,           // 當前任務鏈
    "class": string,               // 子任務類型
    "what": string,                // 資訊類型
    "details": object,             // 資訊詳情
    "uuid": string,                // 設備唯一碼
}
```

#### 常見 `what` 及 `details` 欄位

- `StageDrops`  
    關卡材料掉落資訊

    ```json
    // 對應的 details 欄位舉例
    {
        "drops": [              // 本次辨識到的掉落材料
            {
                "itemId": "3301",
                "quantity": 2,
                "itemName": "技巧概要·卷1"
            },
            {
                "itemId": "3302",
                "quantity": 1,
                "itemName": "技巧概要·卷2"
            },
            {
                "itemId": "3303",
                "quantity": 2,
                "itemName": "技巧概要·卷3"
            }
        ],
        "stage": {              // 關卡資訊
            "stageCode": "CA-5",
            "stageId": "wk_fly_5"
        },
        "stars": 3,             // 行動結束星級
        "stats": [              // 本次執行期間總的材料掉落
            {
                "itemId": "3301",
                "itemName": "技巧概要·卷1",
                "quantity": 4,
                "addQuantity": 2     //本次新增的掉落數量
            },
            {
                "itemId": "3302",
                "itemName": "技巧概要·卷2",
                "quantity": 3,
                "addQuantity": 1
            },
            {
                "itemId": "3303",
                "itemName": "技巧概要·卷3",
                "quantity": 4,
                "addQuantity": 2
            }
        ]
    }
    ```

- `RecruitTagsDetected`  
    公招辨識到了 Tags

    ```json
    // 對應的 details 欄位舉例
    {
        "tags": [
            "費用回覆",
            "防護",
            "先鋒幹員",
            "輔助幹員",
            "近戰位"
        ]
    }
    ```

- `RecruitSpecialTag`  
    公招辨識到了特殊 Tag

    ```json
    // 對應的 details 欄位舉例
    {
        "tag": "高級資深幹員"
    }
    ```

- `RecruitResult`  
    公招辨識結果

    ```json
    // 對應的 details 欄位舉例
    {
        "tags": [                   // 所有辨識到的 tags，目前來說一定是 5 個
            "削弱",
            "減速",
            "術師幹員",
            "輔助幹員",
            "近戰位"
        ],
        "level": 4,                 // 總的星級
        "result": [
            {
                "tags": [
                    "削弱"
                ],
                "level": 4,         // 這組 tags 的星級
                "opers": [
                    {
                        "name": "初雪",
                        "level": 5  // 幹員星級
                    },
                    {
                        "name": "隕星",
                        "level": 5
                    },
                    {
                        "name": "槐琥",
                        "level": 5
                    },
                    {
                        "name": "夜煙",
                        "level": 4
                    },
                    {
                        "name": "流星",
                        "level": 4
                    }
                ]
            },
            {
                "tags": [
                    "減速",
                    "術師幹員"
                ],
                "level": 4,
                "opers": [
                    {
                        "name": "夜魔",
                        "level": 5
                    },
                    {
                        "name": "格雷伊",
                        "level": 4
                    }
                ]
            },
            {
                "tags": [
                    "削弱",
                    "術師幹員"
                ],
                "level": 4,
                "opers": [
                    {
                        "name": "夜煙",
                        "level": 4
                    }
                ]
            }
        ]
    }
    ```

- `RecruitTagsRefreshed`  
    公招刷新了 Tags

    ```json
    // 對應的 details 欄位舉例
    {
        "count": 1,               // 當前槽位已刷新次數
        "refresh_limit": 3        // 當前槽位刷新次數上限
    }
    ```

- `RecruitNoPermit`  
    公招無招聘許可

    ```json
    // 對應的 details 欄位舉例
    {
        "continue": true,   // 是否繼續刷新
    }
    ```

- `RecruitTagsSelected`  
    公招選擇了 Tags

    ```json
    // 對應的 details 欄位舉例
    {
        "tags": [
            "減速",
            "術師幹員"
        ]
    }
    ```

- `RecruitSlotCompleted`  
    當前公招槽位任務完成

- `RecruitError`  
    公招辨識錯誤

- `EnterFacility`  
    基建進入了設施

    ```json
    // 對應的 details 欄位舉例
    {
        "facility": "Mfg",  // 設施名
        "index": 0          // 設施序號
    }
    ```

- `NotEnoughStaff`  
    基建可用幹員不足

    ```json
    // 對應的 details 欄位舉例
    {
        "facility": "Mfg",  // 設施名
        "index": 0          // 設施序號
    }
    ```

- `ProductOfFacility`  
    基建產物

    ```json
    // 對應的 details 欄位舉例
    {
        "product": "Money", // 產物名
        "facility": "Mfg",  // 設施名
        "index": 0          // 設施序號
    }
    ```

- `StageInfo`  
    自動作戰關卡資訊

    ```json
    // 對應的 details 欄位舉例
    {
        "name": string  // 關卡名
    }
    ```

- `StageInfoError`  
    自動作戰關卡辨識錯誤

- `PenguinId`  
    企鵝物流 ID

    ```json
    // 對應的 details 欄位舉例
    {
        "id": string
    }
    ```

- `Depot`  
    倉庫辨識結果

    ```json
    // 對應的 details 欄位舉例
    "done": bool,       // 是否已經辨識完了，為 false 表示仍在辨識中（過程中的數據）
    "arkplanner": {     // https://penguin-stats.io/planner
        "object": {
            "items": [
                {
                    "id": "2004",
                    "have": 4,
                    "name": "高級作戰記錄"
                },
                {
                    "id": "mod_unlock_token",
                    "have": 25,
                    "name": "模組數據塊"
                },
                {
                    "id": "2003",
                    "have": 20,
                    "name": "中級作戰記錄"
                }
            ],
            "@type": "@penguin-statistics/depot"
        },
        "data": "{\"@type\":\"@penguin-statistics/depot\",\"items\":[{\"id\":\"2004\",\"have\":4,\"name\":\"高級作戰記錄\"},{\"id\":\"mod_unlock_token\",\"have\":25,\"name\":\"模組數據塊\"},{\"id\":\"2003\",\"have\":20,\"name\":\"中級作戰記錄\"}]}"
    },
    "lolicon": {     // https://arkntools.app/#/material
        "object": {
            "2004" : 4,
            "mod_unlock_token": 25,
            "2003": 20
        },
        "data": "{\"2003\":20,\"2004\": 4,\"mod_unlock_token\": 25}"
    }
    // 目前只支援 ArkPlanner 和 Lolicon (Arkntools) 的格式，以後可能會兼容更多網站
    ```

- `OperBox`  
    幹員辨識結果

    ```json
    // 對應的 details 欄位舉例
    "done": bool,       // 是否已經辨識完了，為 false 表示仍在辨識中（過程中的數據）
    "all_oper": [
        {
            "id": "char_002_amiya",
            "name": "阿米婭",
            "own": true,
            "rarity": 5
        },
        {
            "id": "char_003_kalts",
            "name": "凱爾希",
            "own": true,
            "rarity": 6
        },
        {
            "id": "char_1020_reed2",
            "name": "焰影葦草",
            "own": false,
            "rarity": 6
        },
    ]
    "own_opers": [
        {
            "id": "char_002_amiya", // 幹員id
            "name": "阿米婭", // 幹員名稱
            "own": true, // 是否擁有
            "elite": 2, // 精英度 0,1,2
            "level": 50, // 幹員等級
            "potential": 6, // 幹員潛能 [1, 6]
            "rarity": 5 // 幹員稀有度 [1, 6]
        },
        {
            "id": "char_003_kalts",
            "name": "凱爾希",
            "own": true,
            "elite": 2,
            "level": 50,
            "potential": 1,
            "rarity": 6
        }
    ]
    ```

- `UnsupportedLevel`  
    自動抄作業，不支援的關卡名
