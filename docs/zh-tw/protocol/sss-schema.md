---
order: 7
icon: game-icons:prisoner
---
# 保全派駐協議

::: tip
請注意 JSON 檔是不支援註解的，下方的註解僅用於說明，請勿直接複製使用
:::

```json
{
    "type": "SSS",                         // 協議類型，SSS 表示保全派駐，必選，不可修改
    "stage_name": "多索雷斯在建地塊",       // 保全派駐地圖名，必選
    "minimum_required": "v4.9.0",          // 最低要求 maa 版本號，必選
    "doc": {                               // 描述，可選
        "title": "低練度高成功率作業",
        "title_color": "dark",
        "details": "對練度要求很低 balabala……", // 建議在這裡寫上你的名字！（作者名）、參考的影片攻略連結等
        "details_color": "dark"
    },
    "buff": "自適應補給元件",               // 開局導能元件選擇，可選
    "equipment": [                         // 開局裝備選擇，橫著數，可選
                                           // 目前版本暫未實現，只會在介面上顯示一下
        "A", "A", "A", "A",
        "B", "B", "B", "B"
    ],
    "strategy": "優選策略",                 // 或者 自由策略，可選
                                           // 目前版本暫未實現，只會在介面上顯示一下
    "opers": [                             // 指定幹員，可選
        {
            "name": "棘刺",
            "skill": 3,
            "skill_usage": 1
        }
    ],
    "tool_men": {                          // 剩餘所需各職業人數，按費用排序隨便拿，可選
                                           // 目前版本暫未實現，只會在介面上顯示一下
        "Pioneer": 13,
        "近衛": 2,       // 中英文均可
        "Medic": 2
    },
    "drops": [              // 戰鬥開始時和戰鬥中途，招募幹員、獲取裝備優先級
        "空弦",
        "能天使",           // 支援幹員名、職業名
        "先鋒",             // 職業名中英文均可
        "Support",
        "無需增調幹員",      // 不招人
        "重整導能組件",      // 支援裝備名，全寫一起.jpg
        "反制導能組件",
        "戰備啟動閥",        // 關卡中途的可選裝備，也放這裡
        "改派發訊器",
    ],
    "blacklist": [ // 黑名單，可選。在 drops 裡不會選這些人。
                   // 後續版本支援編隊後，編隊工具人也不會選這些人
        "夜半",
        "梅爾"
    ],
    "stages": [
        {
            "stage_name": "蜂擁而上",       // 單層關卡名，必選
                                           // 支援 name, stageId, levelId，推薦 stageId 或 levelId
                                           // 請勿使用 code（例如 LT-1），因為會和其他保全關卡衝突

            "strategies": [                // 必選
                                           // 會按部署各 object 裡的 tool_men，如果目前手牌都沒有，才會部署下一個 object 裡的
                {
                    "core": "棘刺",
                    "tool_men": {
                        "Pioneer": 1,     // 中英文均可
                        "Warrior": 1,
                        "Medic": 1
                    },
                    "location": [
                        10,
                        1
                    ],
                    "direction": "Left"
                },
                {
                    "core": "泥岩",
                    "tool_men": {
                        "Pioneer": 1,
                        "Warrior": 1,
                        "Medic": 1
                    },
                    "location": [
                        2,
                        8
                    ],
                    "direction": "Left"
                },
                {
                    // 不填寫 core，可以用於部署輔助過牌的之類的
                    "tool_men": {
                        "Support": 100
                    },
                    "location": [
                        2,
                        8
                    ],
                    "direction": "Left"
                }
            ],
            "draw_as_possible": true,   // “調配幹員” 按鈕，是否好了就用，可選，預設 true
            "actions": [                // 可選
                                        // 基本複用抄作業的邏輯，可參考 3.3-戰鬥流程協議.md
                                        // 符合 action 的條件就執行 action，否則執行上面的 strategies 的邏輯
                {
                    "type": "調配幹員"  // 新 type，“調配幹員” 按鈕，點一下，在 "draw_as_possible" 為 true 時無效
                },
                {
                    "type": "CheckIfStartOver", // 新 type，檢查幹員在不在，不在就退出重開
                    "name": "棘刺"
                },
                {
                    "name": "桃金娘",
                    "location": [
                        4,
                        5
                    ],
                    "direction": "左"
                },
                {
                    "kills": 10,
                    "type": "撤退",
                    "name": "桃金娘"
                }
            ],
            "retry_times": 3 // 戰鬥失敗重試次數，超過了直接放棄整局
        },
        {
            "stage_name": "見者有份"
            // ...
        }
        // 寫幾關打幾關，比如只寫到了 4，則打完 4 自動重開
    ]
}
```

## 範例檔案

[SSS_阿卡胡拉叢林.json](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/SSS_阿卡胡拉丛林.json)

[SSS_多索雷斯在建地塊.json](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/SSS_多索雷斯在建地块.json)
