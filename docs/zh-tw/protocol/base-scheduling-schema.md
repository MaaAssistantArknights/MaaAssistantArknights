---
order: 6
icon: material-symbols:view-quilt-rounded
---

# 基建排班協議

`resource/custom_infrast/*.json` 的使用方法及各欄位說明

::: tip
請注意 JSON 檔是不支援註解的，下方的註解僅用於說明，請勿直接複製使用
:::

[一圖流排班產生器](https://ark.yituliu.cn/tools/schedule)

## 完整欄位一覽

```json
{
    "title": "小號的換班方案",       // 作業名，可選
    "description": "哈哈哈哈",       // 作業描述，可選
    "plans": [
        {
            "name": "早班",         // 計劃名，可選
            "description": "lol",   // 計劃描述，可選
            "description_post": "", // 計劃執行完時顯示的描述，可選
            "period": [             // 換班時間段，可選
                                    // 若當前時間在該區間內，則自動選擇該計劃（整個 json 檔案中可能包含多個計劃）
                                    // 如果該欄位不存在，則每次換班結束後，自動切換為下一個計劃
                                    // core 不處理該欄位，若您使用接口集成 maa，請自行實現該邏輯
                [
                    "22:00",        // 要求格式 hh:mm，目前只是簡單的比較數字大小，如果要跨天請仿照該範例中寫法
                    "23:59"
                ],
                [
                    "00:00",
                    "06:00"
                ]
            ],
            "duration": 360,        // 工作持續時長（分鐘），保留欄位，目前無作用。以後可能到時間了彈窗提醒該換班了，或者直接自動換了
            "Fiammetta": {          // “菲亞梅塔” 為哪位幹員使用，可選，不填寫則不使用
                "enable": true,     // 是否使用“菲亞梅塔”，可選，預設 true
                "target": "巫戀",   // 目標幹員，使用 OCR 進行，需要傳入對應用戶端語言的幹員名
                "order": "pre",     // 在整個換班前使用，還是換完班才用，可選，取值範圍 "pre" / "post"，預設 "pre"
            },
            "drones": {             // 無人機使用，可選，不填寫則不使用無人機
                "enable": true,     // 是否使用無人機，可選，預設 true
                "room": "trading",  // 為哪個類型房間使用，取值範圍 "trading" / "manufacture"
                "index": 1,         // 為第幾個該類型房間使用，對應左邊 tab 欄序號，取值範圍 [1, 5]
                "rule": "all",      // 使用規則，保留欄位，目前無作用。以後可能拿來支援插拔等操作
                "order": "pre"      // 在換幹員前使用還是在換完才用，可選，取值範圍 "pre" / "post"，預設 "pre"
            },
            "groups":[              // 對於 "control" / "manufacture" / "trading"，可以設置幹員編組
                {
                    "name":"古+银",
                    "operators":[
                        "古米",
                        "银灰",
                        "梅"
                    ]
                },
                {
                    "name":"清流",
                    "operators":[
                        "清流",
                        "森蚺",
                        "温蒂"
                    ]
                }
            ],
            "rooms": {              // 房間資訊，必選
                                    // 取值範圍 "control" / "manufacture" / "trading" / "power" / "meeting" / "hire" / "dormitory" / "processing"
                                    // 缺少某個則該設施使用預設算法進行換班。
                                    // 若想不對某個房間換班請使用 skip 欄位，或直接在軟體 任務設定 - 基建換班 - 常規設定 中取消改設施的勾選
                "control": [
                    {
                        "operators": [
                            "夕",   // 使用 OCR 進行，需要傳入對應用戶端語言的幹員名
                            "令",
                            "凱爾希",
                            "阿米婭",
                            "瑪恩納"
                        ]
                    }
                ],
                "manufacture": [
                    {
                        "operators": [
                            "芬",
                            "稀音",
                            "克洛絲"
                        ],
                        "sort": false,  // 是否排序（按照上面 operators 的順序），可選，預設 false
                                        // 例子：當使用稀音、帕拉斯、巫戀、等幹員且 "sort": false，幹員順序可能會被打亂，導致暖機效果丟失。
                                        //     使用 "sort": true，可以避免這個問題
                    },
                    {
                        "skip": true    // 是否跳過當前房間（陣列序號對應），可選，預設 false
                                        // 若為 true，其他欄位均可為空。僅跳過換幹員操作，其他如使用無人機、線索交流等仍會正常進行
                    },
                    {
                        "operators": [
                            "Castle-3"
                        ],
                        "autofill": true,   // 使用原先的算法，自動填充剩下的位置，可選，預設 false
                                        // 若 operators 為空，則該房間完整的使用原先算法進行排班
                                        // 若 operators 不為空，將僅考慮單幹員效率，而不考慮整個組合效率
                                        // 注意可能和後面自定義的幹員產生衝突，比如把後面需要的幹員拿到這裡用了，請謹慎使用，或將 autofill 的房間順序放到最後
                        "product": "Battle Record"  // 當前制造產物，可選。
                                                    // 若識別到當前設施與作業中設定的產物不符合，介面會彈個紅色字樣提示，以後可能有更多作用
                                                    // 取值範圍： "Battle Record" | "Pure Gold" |  "Dualchip" | "Originium Shard" | "LMD" | "Orundum"
                    },
                    {
                        "operators": [
                            "多蘿西"
                        ],
                        "candidates": [ // 備選幹員，可選。這裡面的有誰用誰，選滿為止
                                        // 與 autofill=true 不相容，即該陣列不為空時，autofill 需要為 false
                            "星源",
                            "白面鴞",
                            "赫默"
                        ]
                    },
                    {
                        "use_operator_groups":true,  // 設置為 true 以使用 groups 中的干員編組，默認為 false
                        "operators":[                // 啟用後, operators 中的名字將被解釋為編組名
                            "古+银",                 // 將按照心情閾值以及設置順序選擇編組
                            "清流"                   // 如 古+银 組中有乾員心情低於閾值，將使用 清流 組
                        ]
                    }
                ],
                "meeting": [
                    {
                        "autofill": true // 這個房間內整個 autofill
                    }
                ]
            }
        },
        {
            "name": "晚班"
            // ...
        }
    ]
}
```

## 範例檔案

[243 極限效率，一天三換](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/243_layout_3_times_a_day.json)

[153 極限效率，一天三換](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/153_layout_3_times_a_day.json)
