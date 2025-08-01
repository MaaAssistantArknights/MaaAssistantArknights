---
order: 1
icon: bxs:book
---
# 集成文件

## 接口介紹

### `AsstAppendTask`

#### 接口原型

```cpp
AsstTaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### 接口說明

添加任務

#### 返回值

- `AsstTaskId`  
    若添加成功，返回該任務 ID，可用於後續設定任務參數；  
    若添加失敗，返回 0

#### 參數說明

- `AsstHandle handle`  
    實例句柄
- `const char* type`  
    任務類型
- `const char* params`  
    任務參數，json string

##### 任務類型一覽

- `StartUp`  
    開始喚醒  

```json5
// 對應的任務參數
{
    "enable": bool,              // 是否啟用本任務，可選，預設為 true
    "client_type": string,       // 用戶端版本，必選
                                 // 選項："Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "start_game_enabled": bool,  // 是否自動啟動用戶端，可選，預設不啟動
    "account_name": string       // 切換賬號，可選，預設不切換
                                 // 僅支持切換至已登錄的賬號，使用登錄名進行查找，保證輸入內容在所有已登錄賬號唯一即可
                                 // 官服：123****4567，可輸入 123****4567、4567、123、3****4567
                                 // B服：張三，可輸入 張三、張、三
}
```

- `CloseDown`  
    關閉遊戲  

```json5
// 對應的任務參數
{
    "enable": bool,              // 是否啟用本任務，可選，預設為 true
    "client_type": string,       // 用戶端版本，必選，填空則不執行
                                 // 選項："Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
}
```

- `Fight`  
    刷理智

```json5
// 對應的任務參數
{
    "enable": bool,             // 是否啟用本任務，可選，預設為 true
    "stage": string,            // 關卡名，可選，預設為空，辨識當前/上次的關卡。不支援執行中設定
                                // 支援全部主線關卡，如 "1-7"、"S3-2" 等
                                // 可在關卡結尾輸入 Normal/Hard 表示需要切換標準與磨難難度
                                // 剿滅作戰，必須輸入 "Annihilation"
                                // 當期 SS 活動 後三關，必須輸入完整關卡編號
    "medicine": int,            // 最大使用理智藥數量，可選，預設 0
    "expiring_medicine": int,   // 最大使用 48 小時內過期理智藥數量，可選，預設 0
    "stone": int,               // 最大吃石頭數量，可選，預設 0
    "times": int,               // 戰鬥次數，可選，預設int32.max
    "series": int,              // 連戰次數, 可選, -1~6
                                // -1  為禁用切換
                                // 0   為自動切換為目前可用的最大次數, 如當前理智不夠6次, 則選擇最低可用次數
                                // 1~6 為指定連戰次數
    "drops": {                  // 指定掉落數量，可選，預設不指定
        "30011": int,           // key - item_id, value 數量. key 可參考 resource/item_index.json 檔案
        "30062": int            // 是或的關系
    },
    /* 以上全部是或的關系，即任一達到即停止任務 */

    "report_to_penguin": bool,  // 是否匯報企鵝數據，可選，預設 false
    "penguin_id": string,       // 企鵝數據匯報 id，可選，預設為空。僅在 report_to_penguin 為 true 時有效
    "server": string,           // 伺服器，可選，預設 "CN", 會影響掉落辨識及上傳
                                // 選項："CN" | "US" | "JP" | "KR"
    "client_type": string,      // 用戶端版本，可選，預設為空。用於遊戲崩潰時重開並連回去繼續刷，若為空則不啟用該功能
                                // 選項："Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "DrGrandet": bool,          // 節省理智碎石模式，可選，預設 false，僅在可能產生碎石效果時生效。
                                // 在碎石確認界面等待，直到當前的 1 點理智恢復完成後再立刻碎石
}
```

另支援少部分資源關卡名請參考[集成範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/tools/AutoLocalization/example/zh-tw.xaml#L219)

- `Recruit`  
    公開招募

```json5
// 對應的任務參數
{
    "enable": bool,             // 是否啟用本任務，可選，預設為 true
    "refresh": bool,            // 是否刷新三星 Tags, 可選，預設 false
    "select": [                 // 會去點擊標籤的 Tag 等級，必選
        int,
        ...
    ],
    "confirm": [                // 會去點擊確認的 Tag 等級，必選。若僅公招計算，可設定為空陣列
        int,
        ...
    ],
    "first_tags": [             // 首選 Tags，僅在 Tag 等級為 3 時有效。可選，預設為空
        string,                 // 當 Tag 等級為 3 時，會盡可能多地選擇這裡的 Tags（如果有）
        ...                     // 且是強制選擇，也就是會忽略所有“讓 3 星 Tags 不被選擇”的設定
    ],
    "extra_tags_mode": int,     // 選擇更多的 Tags, 可選, 預設為 0
                                // 0 - 預設行為
                                // 1 - 選 3 個 Tags, 即使可能衝突
                                // 2 - 如果可能, 同時選擇更多的高星 Tag 組合, 即使可能衝突
    "times": int,               // 招募多少次，可選，預設 0。若僅公招計算，可設定為 0
    "set_time": bool,           // 是否設定招募時限。僅在 times 為 0 時生效，可選，預設 true
    "expedite": bool,           // 是否使用加急許可，可選，預設 false
    "expedite_times": int,      // 加急次數，僅在 expedite 為 true 時有效。
                                // 可選，預設無限使用（直到 times 達到上限）
    "skip_robot": bool,         // 是否在辨識到小車詞條時跳過，可選，預設跳過
    "recruitment_time": {       // Tag 等級（大於等於 3）和對應的希望招募時限，單位為分鐘，預設值都為 540（即 09:00:00）
        "3": int,
        "4": int,
        ...
    },
    "report_to_penguin": bool,  // 是否匯報企鵝數據，可選，預設 false
    "penguin_id": string,       // 企鵝數據匯報 id，可選，預設為空。僅在 report_to_penguin 為 true 時有效
    "report_to_yituliu": bool,  // 是否匯報一圖流數據，可選，預設 false
    "yituliu_id": string,       // 一圖流匯報 id，可選，預設為空。僅在 report_to_yituliu 為 true 時有效
    "server": string,           // 伺服器，可選，預設 "CN"，會影響上傳
                                // 選項："CN" | "US" | "JP" | "KR"
}
```

- `Infrast`  
    基建換班

```json5
{
    "enable": bool,         // 是否啟用本任務，可選，預設為 true
    "mode": int,            // 換班工作模式，可選，預設 0
                            // 0     - Default: 預設換班模式，單一設施最適解
                            // 10000 - Custom: 自訂換班模式，讀取使用者配置，可參考 protocol/base-scheduling-schema.md
                            // 20000 - Rotation: 一鍵輪換模式，會跳過控制中樞、發電站、宿舍以及辦公室，其餘設施不進行換班但保留基本操作（如使用無人機、會客室邏輯）

    "facility": [           // 要換班的設施（有序），必選。不支援執行中設定
        string,             // 設施名，"Mfg" | "Trade" | "Power" | "Control" | "Reception" | "Office" | "Dorm"
        ...
    ],
    "drones": string,       // 無人機用途，可選項，預設 _NotUse
                            // mode = 10000 時該欄位無效（會被忽略）
                            // "_NotUse"、"Money"、"SyntheticJade"、"CombatRecord"、"PureGold"、"OriginStone"、"Chip"
    "threshold": float,     // 工作心情閾值，可選，取值範圍 [0, 1.0]，預設 0.3
                            // mode = 10000 時該欄位僅針對 "autofill" 有效
                            // mode = 20000 時該欄位無效（會被忽略）
    "replenish": bool,      // 貿易站 “源石碎片” 是否自動補貨，可選，預設 false

    "dorm_notstationed_enabled": bool, // 是否啟用宿舍 “未進駐” 選項，可選，預設 false
    "dorm_trust_enabled": bool, // 是否將宿舍剩餘位置填入信賴未滿幹員，可選，預設 false

    /* 以下參數僅在 mode = 10000 時生效，否則會被忽略 */
    "filename": string,     // 自定義配置路徑，必選。不支援執行中設定
    "plan_index": int,      // 使用配置中的方案序號，必選。不支援執行中設定
}
```

- `Mall`  
    領取信用及商店購物。  
    會先有序的按 `buy_first` 購買一遍，再從左到右並避開 `blacklist` 購買第二遍，在信用溢出時則會無視黑名單，從左到右購買第三遍直到不再溢出

```json5
// 對應的任務參數
{
    "enable": bool,         // 是否啟用本任務，可選，預設為 true
    "shopping": bool,       // 是否購物，可選，預設 false。不支援執行中設定
    "buy_first": [          // 優先購買列表，可選。不支援執行中設定
        string,             // 商品名，如 "招聘許可"、"龍門幣" 等
        ...
    ],
    "blacklist": [          // 黑名單列表，可選。不支援執行中設定
        string,             // 商品名，如 "加急許可"、"家具零件" 等
        ...
    ],
   "force_shopping_if_credit_full": bool // 是否在信用溢出時無視黑名單，預設為 true
    "only_buy_discount": bool // 是否只購買折扣物品，只作用於第二輪購買，預設為 false
    "reserve_max_credit": boll // 是否在信用點低於300時停止購買，只作用於第二輪購買，預設為 false
}
```

- `Award`  
    領取日常獎勵

```json5
// 對應的任務參數
{
    "enable": bool          // 是否啟用本任務，可選，預設為 true
}
```

- `Roguelike`  
    無限刷肉鴿

```json5
// 對應的任務參數
{
    "enable": bool,  // 是否啟用本任務，可選，預設值 true
    "theme": string, // 主題，可選，預設值 "Phantom"
                     //   Phantom   - 傀影與猩紅血鑽
                     //   Mizuki    - 水月與深藍之樹
                     //   Sami      - 探索者的銀霜止境
                     //   Sarkaz    - 薩卡茲的無終奇語
                     //   JieGarden - 界园
    "mode": int,     // 模式，可選，預設值 0
                     //   0 - 刷分/獎勵點數，盡可能穩定地打更多層數
                     //   1 - 刷源石錠，第一層投資完就退出
                     //   2 - 【已棄用】兼顧模式 0 與 1，投資過後再退出，沒有投資就繼續往後打
                     //   3 - 開發中...
                     //   4 - 凹開局，先在 0 難度下達到第三層後重開，再到指定難度下凹開局獎勵，若不為熱水壺或希望則回到 0 難度下重新來過；
                     //       若在 Phantom 主題下則不切換難度，僅在當前難度下嘗試達到第三層、重開、凹開局
                     //   5 - 刷坍縮範式；僅適用於 Sami 主題；通過戰鬥漏怪等方式加快坍縮值積累，
                     //       若遇到的第一個坍縮範式在 expected_collapsal_paradigms 列表中則停止任務，否則重開
    "squad": string,                // 開局分隊名，可選，預設值 "指揮分隊"；
    "roles": string,                // 開局職業組，可選，預設值 "取長補短"；
    "core_char": string,            // 開局幹員名，可選；僅支持單個幹員**中文名**，無論伺服器；若留空或設定為空字串 "" 則根據練度自動選擇
    "use_support": bool,            // 開局幹員是否為助戰幹員，可選，預設值 false
    "use_nonfriend_support": bool,  // 是否可以是非好友助戰幹員，可選，預設值 false；僅在 use_support 為 true 時有效
    "starts_count": int,                // 開始探索的次數，可選，預設值 INT_MAX；達到後自動停止任務
    "difficulty": int,                  // 指定難度等級，可選，預設值 0；僅適用於**除 Phantom 以外**的主題；
                                        // 若未解鎖難度，則會選擇當前已解鎖的最高難度
    "stop_at_final_boss": bool,         // 是否在第 5 層險路惡敵節點前停止任務，可選，預設值 false；僅適用於**除 Phantom 以外**的主題
    "stop_at_max_level": bool,          // 是否在肉鴿等級刷滿後停止任務，可選，預設值 false
    "investment_enabled": bool,         // 是否投資源石錠，可選，預設值 true
    "investments_count": int,           // 投資源石錠的次數，可選，預設值 INT_MAX；達到後自動停止任務
    "stop_when_investment_full": bool,  // 是否在投資到達上限後自動停止任務，可選，預設值 false
    "investment_with_more_score": bool, // 是否在投資後嘗試購物，可選，預設值 false；僅適用於模式 1
    "start_with_elite_two": bool,       // 是否在凹開局的同時凹幹員精二直升，可選，預設值 false；僅適用於模式 4
    "only_start_with_elite_two": bool,  // 是否只凹開局幹員精二直升而忽視其他開局條件，可選，預設值 false；
                                        // 僅在模式為 4 且 start_with_elite_two 為 true 時有效
    "refresh_trader_with_dice": bool,   // 是否用骰子刷新商店購買特殊商品，可選，預設值 false；僅適用於 Mizuki 主題，用於刷指路鱗
    "first_floor_foldartal": string,    // 希望在第一層遠見階段得到的密文版，可選；僅適用於 Sami 主題，不限模式；若成功凹到則停止任務
    "start_foldartal_list": [           // 凹開局時希望在開局獎勵階段得到的密文板，可選，預設值 []；僅主題為 Sami 且模式為 4 時有效；
        string,                         // 僅當開局擁有列表中所有的密文板時才算凹開局成功；
        ...                             // 注意，此參數須與 “生活至上分隊” 同時使用，其他分隊在開局獎勵階段不會獲得密文板；
    ],
    "collectible_mode_start_list": {    // 開局期望的獎勵，可選，預設為全 false；僅在模式為 4 時有效
        "hot_water": bool,              // 熱水壺獎勵，可觸發燒水機制（通用）
        "shield": bool,                 // 護盾獎勵，相當於額外生命值（通用）
        "ingot": bool,                  // 源石錠獎勵（通用）
        "hope": bool,                   // 希望獎勵（通用，注意：JieGarden 主題中無 hope 獎勵）
        "random": bool,                 // 隨機獎勵選項：指的是「消耗所有源石錠以換取一個隨機收藏品」（通用）
        "key": bool,                    // 鑰匙獎勵，僅在 Mizuki 主題中有效
        "dice": bool,                   // 骰子獎勵，僅在 Mizuki 主題中有效
        "ideas": bool,                  // 2 构想獎勵，僅在 Sarkaz 主題中有效
    },
    "use_foldartal": bool,                    // 是否使用密文板，模式 5 下預設值 false，其他模式下預設值 true；僅適用於 Sami 主題，
    "check_collapsal_paradigms": bool,        // 是否檢測獲取的坍縮範式，模式 5 下預設值 true，其他模式下預設值 false
    "double_check_collapsal_paradigms": bool, // 是否執行坍縮範式檢測防漏措施，模式 5 下預設值 true，其他模式下預設值 false；
                                              // 僅在主題為 Sami 且 check_collapsal_paradigms 為 true 時有效
    "expected_collapsal_paradigms": [         // 希望觸發的坍縮範式，預設值 ["目空一些", "睜眼瞎", "圖像損壞", "一抹黑"]；
        string,                               // 僅在主題為 Sami 且模式為 5 時有效
        ...
    ]
}
```

- `Copilot`  
    自動抄作業

```json5
{
    "enable": bool,             // 是否啟用本任務，可選，預設為 true
    "filename": string,         // 作業 JSON 的檔案路徑，絕對、相對路徑均可。不支援執行中設定
    "formation": bool           // 是否進行 “快捷編隊”，可選，預設否。不支援執行中設定
}
```

作業 JSON 請參考 [3.3-戰鬥流程協議](./copilot-schema.md)

- `SSSCopilot`  
    自動抄保全作業

```json5
{
    "enable": bool,             // 是否啟用本任務，可選，預設為 true
    "filename": string,         // 作業 JSON 的檔案路徑，絕對、相對路徑均可。不支援執行中設定
    "loop_times": int           // 循環執行次數
}
```

保全作業 JSON 請參考 [3.7-保全派駐協議](./sss-schema.md)

- `Depot`  
    倉庫辨識

```json5
// 對應的任務參數
{
    "enable": bool          // 是否啟用本任務，可選，預設為 true
}
```

- `OperBox`  
    幹員 box 辨識

```json5
// 對應的任務參數
{
    "enable": bool          // 是否啟用本任務，可選，預設為 true
}
```

- `Reclamation`  
    生息演算

```json5
{
    "enable": bool,
    "theme": string,            // 主題，可選項。預設 1
                                // Fire  - *沙中之火*
                                // Tales - *沙洲遺聞*
    "mode": int,                // 模式，可選項。默認為 0
                                // 0 - 刷分與建造點，進入戰鬥直接退出
                                // 1 - 沙中之火：刷赤金，聯絡員買水後基地鍛造；
                                //     沙洲遺聞：自動製造物品並讀檔刷貨幣
    "tools_to_craft": [
        string,                 // 自動製造的物品，可選項，默認為荧光棒
        ...
    ] 
                                // 建議填寫子串
    "increment_mode": int,      // 點擊類型，可選項。默認為0
                                // 0 - 連點
                                // 1 - 長按
    "num_craft_batches": int    // 單次最大製造輪數，可選。默認為 16
}
```

- `Custom`  

  自定義任務

```json5
{
    "enable": bool,
    "task_names": [     // 執行陣列中第一個匹配上的任務（及後續 next 等）
                        // 若想執行多個任務，可多次 append Custom task
        string,
        ...
    ]
}
```

- `SingleStep`  

  單步任務（目前僅支援戰鬥）

```json5
{
    "enable": bool,
    "type": string,     // 目前僅支援 "copilot"
    "subtask": string,  // "stage" | "start" | "action"
                        // "stage" 設定關卡名，需要 "details": { "stage": "xxxx" }
                        // "start" 開始作戰，無 details
                        // "action": 單步作戰操作，details 需為作戰協議中的單個 action，
                        //           例如："details": { "name": "史爾特爾", "location": [ 4, 5 ], "direction": "左" }，詳情參考 3.3-戰鬥流程協議.md
    "details": {
        ...
    }
}
```

- `VideoRecognition`  

  影片辨識，目前僅支援作業（作戰）影片

```json5
{
    "enable": bool,
    "filename": string, // 影片的檔案路徑，絕對、相對路徑均可。不支援執行中設定
}
```

### `AsstSetTaskParams`

#### 接口原型

```cpp
bool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);
```

#### 接口說明

設定任務參數

#### 返回值

- `bool`  
    返回是否設定成功

#### 參數說明

- `AsstHandle handle`  
    實例句柄
- `AsstTaskId task`  
    任務 ID, `AsstAppendTask` 接口的返回值
- `const char* params`  
    任務參數，json string，與 `AsstAppendTask` 接口相同。  
    未標注 “不支援執行中設定” 的欄位都支援實時修改；否則若當前任務正在執行，會忽略對應的欄位

### `AsstSetStaticOption`

#### 接口原型

```cpp
bool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### 接口說明

設定進程級參數

#### 返回值

- `bool`  
    返回是否設定成功

#### 參數說明

- `AsstStaticOptionKey key`  
    鍵
- `const char* value`  
    值

##### 鍵值一覽

暫無

### `AsstSetInstanceOption`

#### 接口原型

```cpp
bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### 接口說明

設定實例級參數

#### 返回值

- `bool`  
    返回是否設定成功

#### 參數說明

- `AsstHandle handle`  
    實例句柄
- `AsstInstanceOptionKey key`  
    鍵
- `const char* value`  
    值

##### 鍵值一覽

```cpp
    enum InstanceOptionKey
    {
        Invalid = 0,
        // 已棄用 // MinitouchEnabled = 1,   // 是否啟用 minitouch
                                // 開了也不代表就一定能用，有可能設備不支援等
                                // "1" 開，"0" 關
        TouchMode = 2,          // 觸控模式設定，預設 minitouch
                                // minitouch | maatouch | adb
        DeploymentWithPause = 3,    // 是否暫停下幹員，同時影響抄作業、肉鴿、保全
                                    // "1" | "0"
        AdbLiteEnabled = 4,     // 是否使用 AdbLite，"0" | "1"
        KillAdbOnExit = 5,       // 退出時是否殺掉 Adb，"0" | "1"
    };
```
