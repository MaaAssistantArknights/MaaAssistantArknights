---
order: 6
icon: material-symbols:view-quilt-rounded
---

# 基建排班協定

`resource/custom_infrast/*.json` 的使用方法及各欄位說明

::: tip  
請注意，json 檔案不支援註解。文件中的註解僅供說明參考，請勿直接複製使用。  
:::

[視覺化排班表產生工具](https://ark.yituliu.cn/tools/scheduleV2)

[視覺化排班表自動產生工具](https://ark.yituliu.cn/tools/scheduleV3)

[自動產生基建排班表工具](https://riic.autos/)

## 完整欄位一覽

```json
{
    "title": "小帳的換班方案", // 作業名稱，可選
    "description": "內容描述", // 作業描述，可選
    "plans": [
        {
            "name": "早班", // 計畫名稱，可選
            "description": "lol", // 計畫描述，可選
            "description_post": "", // 計畫執行完畢時顯示的描述，可選
            "period": [
                // 換班時段，可選
                // 若目前時間在該區間內，則自動選擇該計畫（整個 json 檔案中可能包含多個計畫）
                // 如果該欄位不存在，則每次換班結束後，自動切換為下一個計畫
                // core 不處理該欄位，若您使用介面整合 MAA，請自行實現該邏輯
                [
                    "22:00", // 要求格式 hh:mm，目前只是簡單的比較數字大小，如果要跨天請仿照該範例中寫法
                    "23:59"
                ],
                [
                    "00:00",
                    "06:00"
                ]
            ],
            "duration": 360, // 工作持續時長（分鐘），保留欄位，目前無作用。以後可能到時間了彈窗提醒該換班了，或者直接自動換了
            "Fiammetta": {
                // ｢菲亞梅塔｣ 為哪位幹員使用，可選，不填寫則不使用
                "enable": true, // 是否使用 ｢菲亞梅塔｣ ，可選，預設 true
                "target": "巫戀", // 目標幹員，使用 OCR 進行，需要傳入對應用戶端語言的幹員名稱
                "order": "pre" // 在整個換班前使用，還是換完班才用，可選，取值範圍 "pre" / "post"，預設 "pre"
            },
            "drones": {
                // 無人機使用，可選，不填寫則不使用無人機
                "enable": true, // 是否使用無人機，可選，預設 true
                "room": "trading", // 為哪個類型房間使用，取值範圍 "trading" / "manufacture"
                "index": 1, // 為第幾個該類型房間使用，對應左側分頁標籤欄序號，取值範圍 [1, 5]
                "rule": "all", // 使用規則，保留欄位，目前無作用。以後可能拿來支援插拔等操作
                "order": "pre" // 在換幹員前使用還是在換完才用，可選，取值範圍 "pre" / "post"，預設 "pre"
            },
            "groups": [
                // 對於 "control" / "manufacture" / "trading" / "recycle"，可以設定幹員編組
                {
                    "name": "古+銀",
                    "operators": ["古米", "銀灰", "梅"]
                },
                {
                    "name": "清流",
                    "operators": ["清流", "森蚺", "溫蒂"]
                }
            ],
            "rooms": {
                // 房間資訊，必選
                // 取值範圍 "control" / "manufacture" / "trading" / "power" / "meeting" / "hire" / "dormitory" / "processing" / "training" / "recycle"
                // 缺少某個則該設施使用預設演算法進行換班。
                // 設施陣列中的房間物件少於實際房間數時，從未配置的第一個房間起，該設施剩餘房間均不換班，並發送一條整設施的未配置提示。
                // 設施陣列為空（明確寫了設施鍵但內容為 []）時整個設施保持原樣不換班，與缺少該設施鍵（使用預設演算法換班）相區分；會客室線索、訓練室繼續訓練等設施級操作照常執行。
                // 寫了房間物件但未給出選人意圖（operators/candidates 為空或缺失、未配置 autofill 與編組）時該房間僅跳過換人，保持原進駐不動並發出 ｢房間保持原樣｣ 回呼；產物等其他欄位的處理不受影響
                // skip 跳過該房間的換人與換產物；無人機照常使用（不換班時 Pre/Post 無時序區別）；設施級操作不受影響（如會客室的線索流程、訓練室的繼續訓練照常執行），只記日誌、不發 ｢房間保持原樣｣ 回呼
                // 若想不對某個房間換班請使用 skip 欄位，或直接在 MAA ｢任務設定 - 基建換班｣ 中取消該設施的勾選
                "control": [
                    {
                        "operators": [
                            "夕", // 使用 OCR 進行，需要傳入對應用戶端語言的幹員名稱
                            "令",
                            "凱爾希",
                            "阿米婭",
                            "瑪恩納"
                        ]
                    }
                ],
                "manufacture": [
                    {
                        "operators": ["芬", "稀音", "克洛絲"],
                        "sort": false // 是否排序（按照上面 operators 的順序），可選，預設 false
                        // 範例：當使用稀音、帕拉斯、巫戀等幹員且 "sort": false，幹員順序可能會被打亂，導致暖機效果遺失。
                        //      使用 "sort": true，可以避免這個問題
                    },
                    {
                        "skip": true // 是否跳過目前房間（陣列序號對應），可選，預設 false
                        // 若為 true，其他欄位均可為空。跳過換幹員與換產物操作，無人機照常使用，設施級操作不受影響（如會客室的線索交流、訓練室的繼續訓練等仍會正常進行）
                    },
                    {
                        "operators": ["Castle-3"],
                        "autofill": true, // 使用原先的演算法，自動填充剩下的位置，可選，預設 false
                        // 若 operators 為空或缺失，該房間按照上文的空意圖規則保持原樣
                        // 若 operators 不為空，將僅考慮單幹員效率，而不考慮整個組合效率
                        // 注意可能與後面自訂的幹員產生衝突，請謹慎使用，或將 autofill 的房間順序放到最後
                        "product": "Battle Record" // 目前製造產物，可選。
                        // 若辨識到目前設施與作業中設定的產物不符，介面會彈出紅色字樣提示
                        // 取值範圍： "Battle Record" | "Pure Gold" |  "Dualchip" | "Originium Shard" | "LMD" | "Orundum"
                    },
                    {
                        "operators": ["多蘿西"],
                        "candidates": [
                            // 備選幹員，可選。這裡面有誰用誰，選滿為止；不會選中正在其他設施工作中的幹員（選人階段即排除）；
                            // 心情低於基建換班心情閾值的跳過；宿舍相反，心情已滿的跳過讓出休息位，未滿的（含正在本宿舍休息的）保持可選
                            // 與 autofill=true 不相容，即該陣列不為空時，autofill 需為 false
                            "星源",
                            "白面鴞",
                            "赫默"
                        ]
                    },
                    {
                        "use_operator_groups": true, // 設定為 true 以使用 groups 中的幹員編組，預設為 false
                        "operators": [
                            // 啟用後，operators 中的名稱將被解釋為編組名稱
                            "古+銀", // 將按照心情門檻值以及設定順序選擇編組
                            "清流" // 如 ｢古+銀｣ 組中有幹員心情低於門檻值，將使用 ｢清流｣ 組
                            // 所有編組均不可用時，候補名單（candidates）照常生效；未配置候補且未配置 autofill 時保持該房間原樣不動，不回落預設演算法自動選人
                        ]
                    }
                ],
                "meeting": [
                    {
                        "autofill": true // 這個房間內整個 autofill
                    }
                ],
                "training": [ // 訓練室，配置訓練位幹員與協助者
                    {
                        // 協助者候選，可選，缺省時不選擇協助者；協助者位無訓練位那類準入限制，可進駐任意幹員，
                        // 但與通用備選欄位一致，不會選中正在其他設施工作中的幹員（選人階段即排除）；
                        // 協助者候補同樣按生產設施的語義套用基建換班心情閾值，心情低於閾值的候選在選人階段跳過；全部候選被排除時協助者保持原樣；
                        // 適用於滿足其他設施幹員 ｢基建中有同陣營幹員進駐時提升效率｣ 一類技能條件的場景（往 operatorsA 放一個對應陣營的幹員即可，與專精無關）；名稱要求同 operators（外服亦須填寫簡體中文名，辨識結果會歸一化為簡體後再比對）
                        "operatorsA": ["杜宾"],
                        // 訓練位幹員候選，可選，缺省時不更換訓練位幹員；按選擇列表順序取第一個出現的幹員進駐；
                        // 進駐受訓練位準入限制，幹員需已精二且仍有未專精滿級的技能；專精全滿的幹員不出現在選擇列表，配置了也等效跳過；
                        // 訓練進行中訓練位鎖定不可更換；僅進駐不開新訓練，開始專精仍需手動；名稱要求同 operators
                        "operatorsB": ["银灰"]
                        // operators/candidates 等通用欄位對訓練室無效，僅 skip 與 operatorsA/operatorsB 生效
                        // 繼續訓練是獨立於排班表的 GUI 勾選項，不屬於排班表欄位，不受 rooms 三態影響；空陣列 / skip / 空意圖時繼續訓練照常執行，僅換人被跳過
                        // 開啟繼續訓練時，換班流程優先嘗試繼續當前幹員的專精，成功則訓練位不動、僅按配置更換協助者；無法繼續（材料不足或已滿級）時按配置更換訓練位幹員
                    }
                ],
                "recycle": [ // 回收站，兩個進駐位無角色區分，寫法同製造站；product 欄位無效
                    {
                        "operators": ["芬", "克洛絲"]
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

[243 極限效率，一天三換](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master-v2/resource/custom_infrast/243_layout_3_times_a_day.json)

[153 極限效率，一天三換](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master-v2/resource/custom_infrast/153_layout_3_times_a_day.json)

## 排班表擴充協定

MAA 基礎排班協定僅包含幹員班次資訊，排班表擴充協定在 MAA 排班協定的基礎上，補充可選的基建布局 `layout`、幹員資訊 `operators` 和來源與版本資訊 `metadata`，使 MAA、[**明日方舟一圖流-排班表自動產生工具**](https://ark.yituliu.cn/tools/scheduleV3)、[**明日方舟一圖流-排班表收益計算器**](https://ark.yituliu.cn/tools/maa-schedule-calculator)**等第三方應用程式可以**共用一份檔案，實現資料雙向互通。擴充欄位均為可選內容，可依第三方應用程式的實際需求進行調整。

| 頂層欄位    | 作用                                                                   |
| ----------- | ---------------------------------------------------------------------- |
| `layout`    | 記錄基建布局，包括設施類型、等級和位置                                 |
| `operators` | 記錄幹員精英階段和等級資訊                                             |
| `metadata`  | 記錄協定版本、產生工具及模組版本等資料，也包括第三方工具所需的額外資料 |

### 擴充欄位一覽

範例需與基礎協定的 `plans` 等欄位合併使用，練度及版本號僅用於示範；實際 JSON 需刪除註解。

```jsonc
{
    // 三個欄位均為頂層可選欄位，可分別提供，不影響 MAA 基礎排班執行
    // 工具匯入、編輯和匯出時應保留擴充資訊
    "layout": [ // 完整的已建造布局，可選；省略表示未提供布局
        // 未列出的設施視為未建造；plans[].rooms 中省略的設施沿用基礎協定預設規則
        // type：設施類型；level：設施等級；position：實際位置
        { "type": "control", "level": 5 },
        // 同類型房間順序對應 plans[].rooms 中的順序及無人機 index 編號
        { "type": "trading", "level": 3, "position": "B101" },
        { "type": "trading", "level": 3, "position": "B102" },
        { "type": "trading", "level": 1, "position": "B103" },
        { "type": "manufacture", "level": 3, "position": "B201" },
        { "type": "manufacture", "level": 3, "position": "B202" },
        { "type": "manufacture", "level": 2, "position": "B203" },
        { "type": "manufacture", "level": 2, "position": "B301" },
        { "type": "power", "level": 3, "position": "B302" },
        { "type": "power", "level": 3, "position": "B303" },
        { "type": "meeting", "level": 3 },
        { "type": "processing", "level": 3, "position": "B105" },
        { "type": "hire", "level": 3, "position": "B205" },
        { "type": "training", "level": 3, "position": "B305" },
        { "type": "dormitory", "level": 1, "position": "B104" },
        { "type": "dormitory", "level": 1, "position": "B204" },
        { "type": "dormitory", "level": 1, "position": "B304" },
        { "type": "dormitory", "level": 1, "position": "B404" },
        { "type": "recycle", "level": 1, "position": "B405" }
    ],
    "operators": { // 幹員練度，可選；以遊戲幹員 ID 為鍵
        // 不取代房間內的人員安排
        "char_002_amiya": {
            "name": "阿米婭", // 幹員名稱，可選
            "elite": 2, // 精英階段：0、1、2
            "level": 80 // 目前精英階段下的等級
        },
        "char_124_kroos": {
            "name": "克洛絲",
            "elite": 1,
            "level": 55
        }
    },
    "metadata": { // 來源與版本等附加資訊，可選
        "extensionVersion": "1.0", // 整套擴充協定的版本字串，可選；未提供時按無版本格式處理
        "generator": { // 原始產生工具資訊
            "id": "yituliu-riic-schedule-generator",
            "name": "明日方舟一图流-排班表自动生成工具",
            "url": "https://ark.yituliu.cn/tools/scheduleV3"
        },
        "moduleVersions": { // 產生時的模組版本；模組識別碼由產生工具定義，配合 generator.id 解讀
            "layout": "v20260924.2314", // 布局模組
            "data": "v20260924.2314", // 資料模組
            "team": "v20260928.1118", // 班組模組
            "assembler": "v20260822.2233", // 組裝器
            "yield": "v20261002.1334", // 收益計算器
            "recommendation": "v20260928.1102" // 除錯資訊
        }
    }
}
```
