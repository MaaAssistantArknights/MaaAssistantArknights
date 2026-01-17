---
order: 1
icon: bxs:book
---

# 集成文檔

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
   若添加成功，返回該任務 ID, 可用於後續設定任務參數；  
   若添加失敗，返回 0

#### 參數說明

:::: field-group  
::: field name="handle" type="AsstHandle" required  
實例句柄  
:::  
::: field name="type" type="const char*" required  
任務類型  
:::  
::: field name="params" type="const char*" required  
任務參數，json string  
:::  
::::

##### 任務類型一覽

- `StartUp`  
  開始喚醒

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="client_type" type="string" required  
用戶端版本。  
<br>
選項：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::: field name="start_game_enabled" type="boolean" optional default="false"  
是否自動啟動用戶端。  
:::  
::: field name="account_name" type="string" optional  
切換賬號，預設不切換。  
<br>
僅支援切換至已登錄的賬號，使用登錄名進行查找，保證輸入內容在所有已登錄賬號唯一即可。  
<br>
官服：`123****4567`，可輸入 `123****4567`、`4567`、`123`、`3****4567`  
<br>
B服：`張三`，可輸入 `張三`、`張`、`三`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "client_type": "Official",
   "start_game_enabled": true,
   "account_name": "123****4567"
}
```

</details>

- `CloseDown`  
   關閉遊戲

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="client_type" type="string" required  
用戶端版本，填空則不執行。  
<br>
選項：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "client_type": "Official"
}
```

</details>

- `Fight`  
   理智作戰

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="stage" type="string" optional  
關卡名，預設為空，辨識當前/上次的關卡。不支援執行中設定。  
目前支援導航的關卡有：
- 全部主線關卡。可在關卡末尾添加 `-NORMAL` 或 `-HARD` 來切換標準或磨難關卡。
- 龍門幣、作戰記錄的 5 / 6 關，但必須輸入 `CE-6` / `LS-6`。MAA 會在第六關無法代理的情況下自動切換至第五關。
- 技能書、採購憑證、碳本第 5 關，必須輸入 `CA-5` / `AP-5` / `SK-5`。
- 所有芯片本。必須輸入完整關卡編號，如 `PR-A-1`。
- 剿滅模式支援以下傳入值，必須使用對應的 Value：
  - 當期剿滅：Annihilation
  - 切爾諾伯格：Chernobog@Annihilation
  - 龍門外環：LungmenOutskirts@Annihilation
  - 龍門市區：LungmenDowntown@Annihilation
- 別傳中的 OF-1 / OF-F3 / GT-5。
- 當期 SS 活動 後三關。可訪問 [API](https://api.maa.plus/MaaAssistantArknights/api/gui/StageActivity.json) 獲取支援的關卡列表。需額外加載 [tasks.json](https://api.maa.plus/MaaAssistantArknights/api/resource/tasks.json) 檔案中的活動關卡導航。
- 複刻的 SS 活動。輸入 `SSReopen-<關卡前綴>` ，可一次性刷完 XX-1 ~ XX-9 關，如 `SSReopen-IC`。
:::  
::: field name="medicine" type="number" optional default="0"  
最大使用理智藥數量。  
:::  
::: field name="expiring_medicine" type="number" optional default="0"  
最大使用 48 小時內過期理智藥數量。  
:::  
::: field name="stone" type="number" optional default="0"  
最大吃石頭數量。  
:::  
::: field name="times" type="number" optional default="2147483647"  
戰鬥次數。  
:::  
::: field name="series" type="number" optional  
連戰次數, -1~6。
<br>
`-1` 為禁用切換。
<br>
`0` 為自動切換為當前可用的最大次數, 如當前理智不夠6次, 則選擇最低可用次數。
<br>
`1~6` 為指定連戰次數。  
:::  
::: field name="drops" type="object" optional  
指定掉落數量，預設不指定。key 為 item_id, value 為數量。key 可參考 `resource/item_index.json` 檔案。  
<br>
例如: `{ "30011": 10, "30062": 5 }`  
<br>
以上全部是或的關係，即任一達到即停止任務。  
:::  
::: field name="report_to_penguin" type="boolean" optional default="false"  
是否匯報企鵝數據。  
:::  
::: field name="penguin_id" type="string" optional  
企鵝數據匯報 id, 預設為空。僅在 `report_to_penguin` 為 true 時有效。  
:::  
::: field name="report_to_yituliu" type="boolean" optional default="false"  
是否匯報一圖流。  
:::  
::: field name="yituliu_id" type="string" optional  
一圖流匯報 id, 預設為空。僅在 `report_to_yituliu` 為 true 時有效。  
:::  
::: field name="server" type="string" optional default="CN"  
伺服器，會影響掉落辨識及上傳。
<br>
選項：`CN` | `US` | `JP` | `KR`  
:::  
::: field name="client_type" type="string" optional  
用戶端版本，預設為空。用於遊戲崩潰時重啟並連回去繼續刷，若為空則不啟用該功能。
<br>
選項：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::: field name="DrGrandet" type="boolean" optional default="false"  
節省理智碎石模式，僅在可能產生碎石效果時生效。
<br>
在碎石確認介面等待，直到當前的 1 點理智恢復完成後再立刻碎石。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "stage": "1-7",
   "medicine": 1,
   "expiring_medicine": 0,
   "stone": 0,
   "times": 10,
   "series": 0,
   "drops": {
      "30011": 10
   },
   "report_to_penguin": true,
   "penguin_id": "123456",
   "report_to_yituliu": true,
   "yituliu_id": "123456",
   "server": "CN",
   "client_type": "Official",
   "DrGrandet": false
}
```

</details>

- `Recruit`  
  公開招募

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="refresh" type="boolean" optional default="false"  
是否刷新三星 Tags。  
:::  
::: field name="select" type="array<number>" required  
會去點擊標籤的 Tag 等級。  
:::  
::: field name="confirm" type="array<number>" required  
會去點擊確認的 Tag 等級。若僅公招計算，可設定為空陣列。  
:::  
::: field name="first_tags" type="array<string>" optional  
首選 Tags，僅在 Tag 等級為 3 時有效。預設為空。
<br>
當 Tag 等級為 3 時，會盡可能多地選擇這裡的 Tags（如果有），而且是強制選擇，也就是會忽略所有"讓 3 星 Tag 不被選擇"的設定。  
:::  
::: field name="extra_tags_mode" type="number" optional default="0"  
選擇更多的 Tags。
<br>
`0` - 預設行為
<br>
`1` - 選 3 個 Tags, 即使可能衝突
<br>
`2` - 如果可能, 同時選擇更多的高星 Tag 組合, 即使可能衝突  
:::  
::: field name="times" type="number" optional default="0"  
招募多少次。若僅公招計算，可設定為 0。  
:::  
::: field name="set_time" type="boolean" optional default="true"  
是否設定招募時限。僅在 `times` 為 0 時生效。  
:::  
::: field name="expedite" type="boolean" optional default="false"  
是否使用加急許可。  
:::  
::: field name="expedite_times" type="number" optional  
加急次數，僅在 `expedite` 為 true 時有效。預設無限使用（直到 `times` 達到上限）。  
:::  
::: field name="skip_robot" type="boolean" optional default="true"  
是否在辨識到小車詞條時跳過。  
:::  
::: field name="recruitment_time" type="object" optional  
Tag 等級（大於等於 3）和對應的希望招募時限，單位為分鐘，預設值都為 540（即 09:00:00）。
<br>
例如: `{ "3": 540, "4": 540 }`  
:::  
::: field name="report_to_penguin" type="boolean" optional default="false"  
是否匯報企鵝數據。  
:::  
::: field name="penguin_id" type="string" optional  
企鵝數據匯報 id, 預設為空。僅在 `report_to_penguin` 為 true 時有效。  
:::  
::: field name="report_to_yituliu" type="boolean" optional default="false"  
是否匯報一圖流數據。  
:::  
::: field name="yituliu_id" type="string" optional  
一圖流匯報 id, 預設為空。僅在 `report_to_yituliu` 為 true 時有效。  
:::  
::: field name="server" type="string" optional default="CN"  
伺服器，會影響上傳。
<br>
選項：`CN` | `US` | `JP` | `KR`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "refresh": true,
   "select": [5, 4],
   "confirm": [4, 3],
   "first_tags": ["高级资深干员"],
   "extra_tags_mode": 1,
   "times": 4,
   "set_time": true,
   "expedite": false,
   "expedite_times": 0,
   "skip_robot": true,
   "recruitment_time": {
      "3": 540,
      "4": 540
   },
   "report_to_penguin": false,
   "penguin_id": "123456",
   "report_to_yituliu": false,
   "yituliu_id": "123456",
   "server": "CN"
}
```

</details>

- `Infrast`  
   基建換班

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="mode" type="number" optional default="0"  
換班工作模式。
<br>
`0` - `Default`: 預設換班模式，單設施最適解。
<br>
`10000` - `Custom`: 自訂換班模式，讀取使用者配置，可參考 [基建排班協議](./base-scheduling-schema.md)。
<br>
`20000` - `Rotation`: 一鍵輪換模式，會跳過控制中樞、發電站、宿舍以及辦公室，其餘設施不進行換班但保留基本操作（如使用無人機、會客室邏輯）。  
:::  
::: field name="facility" type="array<string>" required  
要換班的設施（有序）。不支援執行中設定。
<br>
設施名：`Mfg` | `Trade` | `Power` | `Control` | `Reception` | `Office` | `Dorm` | `Processing` | `Training`  
:::  
::: field name="drones" type="string" optional default="\_NotUse"  
無人機用途。`mode = 10000` 時該欄位無效。
<br>
選項：`_NotUse` | `Money` | `SyntheticJade` | `CombatRecord` | `PureGold` | `OriginStone` | `Chip`  
:::  
::: field name="threshold" type="number" optional default="0.3"  
工作心情閾值，取值範圍 [0, 1.0]。
<br>
`mode = 10000` 時該欄位僅針對 "autofill" 有效。
<br>
`mode = 20000` 時該欄位無效。  
:::  
::: field name="replenish" type="boolean" optional default="false"  
貿易站"源石碎片"是否自動補貨。  
:::  
::: field name="dorm_notstationed_enabled" type="boolean" optional default="false"  
是否啟用宿舍"未進駐"選項。  
:::  
::: field name="dorm_trust_enabled" type="boolean" optional default="false"  
是否將宿舍剩餘位置填入信賴未滿幹員。  
:::  
::: field name="reception_message_board" type="boolean" optional default="true"  
是否領取會客室資訊板信用。  
:::  
::: field name="reception_clue_exchange" type="boolean" optional default="true"  
是否進行線索交流。  
:::  
::: field name="reception_send_clue" type="boolean" optional default="true"  
是否贈送線索。  
:::  
::: field name="filename" type="string" required  
自訂配置路徑。不支援執行中設定。
<br>
<Badge type="warning" text="僅在 mode = 10000 時生效" />  
:::  
::: field name="plan_index" type="number" required  
使用配置中的方案序號。不支援執行中設定。
<br>
<Badge type="warning" text="僅在 mode = 10000 時生效" />  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "mode": 0,
   "facility": ["Mfg", "Trade", "Reception"],
   "drones": "PureGold",
   "threshold": 0.3,
   "replenish": true,
   "dorm_notstationed_enabled": false,
   "dorm_trust_enabled": true,
   "reception_message_board": true,
   "reception_clue_exchange": true,
   "reception_send_clue": true,
   "filename": "schedules/base.json",
   "plan_index": 1
}
```

</details>

- `Mall`  
   領取信用及商店購物。  
   會先有序的按 `buy_first` 購買一遍，再從左到右並避開 `blacklist` 購買第二遍，在信用溢出時則會無視黑名單從左到右購買第三遍直到不再溢出

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="visit_friends" type="boolean" optional default="true"  
是否訪問好友基建以獲得信用。  
:::  
::: field name="shopping" type="boolean" optional default="true"  
是否購物。  
:::  
::: field name="buy_first" type="array<string>" optional default="[]"  
優先購買列表。商品名，如 `"招聘许可"`、`"龙门币"` 等。  
:::  
::: field name="blacklist" type="array<string>" optional default="[]"  
購物黑名單列表。商品名，如 `"加急許可"`、`"家具零件"` 等。  
:::  
::: field name="force_shopping_if_credit_full" type="boolean" optional default="false"  
是否在信用溢出時無視黑名單。  
:::  
::: field name="only_buy_discount" type="boolean" optional default="false"  
是否只購買折扣物品，只作用於第二輪購買。  
:::  
::: field name="reserve_max_credit" type="boolean" optional default="false"  
是否在信用點低於 300 時停止購買，只作用於第二輪購買。  
:::  
::: field name="credit_fight" type="boolean" optional default="false"  
是否借助戰打一局 OF-1 關卡以便在第二天獲得更多信用。  
:::  
::: field name="formation_index" type="number" optional default="0"  
打 OF-1 時所使用的編隊欄位的編號。
<br>
為 0–4 的整數，其中 0 表示選擇當前編隊，1-4 分別表示第一、二、三、四編隊。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "visit_friends": true,
   "shopping": true,
   "buy_first": ["招聘许可", "龙门币"],
   "blacklist": ["家具零件"],
   "force_shopping_if_credit_full": false,
   "only_buy_discount": true,
   "reserve_max_credit": false,
   "credit_fight": false,
   "formation_index": 0
}
```

</details>

- `Award`  
   領取各種獎勵

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="award" type="boolean" optional default="true"  
領取每日/每週任務獎勵。  
:::  
::: field name="mail" type="boolean" optional default="false"  
領取所有郵件獎勵。  
:::  
::: field name="recruit" type="boolean" optional default="false"  
領取限定池子贈送的每日免費單抽。  
:::  
::: field name="orundum" type="boolean" optional default="false"  
領取幸運牆的合成玉獎勵。  
:::  
::: field name="mining" type="boolean" optional default="false"  
領取限時開採許可的合成玉獎勵。  
:::  
::: field name="specialaccess" type="boolean" optional default="false"  
領取五週年贈送的月卡獎勵。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "award": true,
   "mail": true,
   "recruit": true,
   "orundum": false,
   "mining": true,
   "specialaccess": false
}
```

</details>

- `Roguelike`  
   無限刷肉鴿

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="theme" type="string" optional default="Phantom"  
主題。
<br>
`Phantom` - 傀影與猩紅血鑽
<br>
`Mizuki` - 水月與深藍之樹
<br>
`Sami` - 探索者的銀霜止境
<br>
`Sarkaz` - 薩卡茲的無終奇語
<br>
`JieGarden` - 界園  
:::  
::: field name="mode" type="number" optional default="0"  
模式。
<br>
`0` - 刷分/獎勵點數，盡可能穩定地打更多層數。
<br>
`1` - 刷源石錠，第一層投資完就退出。
<br>
`2` - <Badge type="danger" text="已棄用" /> 兼顧模式 0 與 1，投資過後再退出，沒有投資就繼續往後打。
<br>
`3` - 開發中...
<br>
`4` - 凹開局，先在 0 難度下到達第三層後重開，再到指定難度下凹開局獎勵，若不為熱水壺或希望則回到 0 難度下重新來過；若在 Phantom 主題下則不切換難度，僅在當前難度下嘗試到達第三層、重開、凹開局。
<br>
`5` - 刷坍縮範式；僅適用於 Sami 主題；通過戰鬥漏怪等方式加快坍縮值積累，若遇到的第一個的坍縮範式在 `expected_collapsal_paradigms` 列表中則停止任務，否則重開。
<br>
`6` - 刷月度小隊蚊子腿，除了針對模式的適配以外和模式 0 相同。
<br>
`7` - 刷深入調查蚊子腿，除了針對模式的適配以外和模式 0 相同。
:::  
::: field name="squad" type="string" optional default="指挥分队"  
開局分隊名。  
:::  
::: field name="roles" type="string" optional default="取长补短"  
開局職業組。  
:::  
::: field name="core_char" type="string" optional  
開局幹員名。僅支援單個幹員**中文名**，無論區服；若留空或設定為空字元串 `""` 則根據練度自動選擇。  
:::  
::: field name="use_support" type="boolean" optional default="false"  
開局幹員是否為助戰幹員。  
:::  
::: field name="use_nonfriend_support" type="boolean" optional default="false"  
是否可以是非好友助戰幹員。僅在 `use_support` 為 true 時有效。  
:::  
::: field name="starts_count" type="number" optional default="2147483647"  
開始探索的次數。達到後自動停止任務。  
:::  
::: field name="difficulty" type="number" optional default="0"  
指定難度等級。若未解鎖難度，則會選擇當前已解鎖的最高難度。  
:::  
::: field name="stop_at_final_boss" type="boolean" optional default="false"  
是否在第 5 層險路惡敵節點前停止任務。僅適用於**除 Phantom 以外**的主題。  
:::  
::: field name="stop_at_max_level" type="boolean" optional default="false"  
是否在肉鴿等級刷滿後停止任務。  
:::  
::: field name="investment_enabled" type="boolean" optional default="true"  
是否投資源石錠。  
:::  
::: field name="investments_count" type="number" optional default="2147483647"  
投資源石錠的次數。達到後自動停止任務。  
:::  
::: field name="stop_when_investment_full" type="boolean" optional default="false"  
是否在投資到達上限後自動停止任務。  
:::  
::: field name="investment_with_more_score" type="boolean" optional default="false"  
是否在投資後嘗試購物。僅適用於模式 1。  
:::  
::: field name="start_with_elite_two" type="boolean" optional default="false"  
是否在凹開局的同時凹幹員精二直升。僅適用於模式 4。  
:::  
::: field name="only_start_with_elite_two" type="boolean" optional default="false"  
是否只凹開局幹員精二直升而忽視其他開局條件。僅在模式為 4 且 `start_with_elite_two` 為 true 時有效。  
:::  
::: field name="refresh_trader_with_dice" type="boolean" optional default="false"  
是否用骰子刷新商店購買特殊商品。僅適用於 Mizuki 主題，用於刷指路鱗。  
:::  
::: field name="first_floor_foldartal" type="string" optional  
希望在第一層遠見階段得到的密文版。僅適用於 Sami 主題，不限模式；若成功凹到則停止任務。  
:::  
::: field name="start_foldartal_list" type="array<string>" optional default="[]"  
凹開局時希望在開局獎勵階段得到的密文板。僅在主題為 Sami 且模式為 4 時有效。
<br>
僅當開局擁有列表中所有的密文板時才算凹開局成功。
<br>
注意，此參數須與 "生活至上分隊" 同時使用，其他分隊在開局獎勵階段不會獲得密文板。  
:::  
::: field name="collectible_mode_start_list" type="object" optional  
凹開局時期望的獎勵，預設全為 false。僅在模式為 4 時有效。
<br>
`hot_water`: 熱水壺獎勵，常用於觸發燒水機制（通用）。
<br>
`shield`: 護盾獎勵，約等於額外生命值（通用）。
<br>
`ingot`: 源石錠獎勵（通用）。
<br>
`hope`: 希望獎勵（通用，注意：JieGarden 主題下無 hope 獎勵）。
<br>
`random`: 隨機獎勵選項：遊戲中指"消耗所有源石錠換一個隨機收藏品"（通用）。
<br>
`key`: 鑰匙獎勵，僅在 Mizuki 主題時有效。
<br>
`dice`: 骰子獎勵，僅在 Mizuki 主題時有效。
<br>
`ideas`: 2 构想獎勵，僅在 Sarkaz 主題時有效。
:::  
::: field name="use_foldartal" type="boolean" optional  
是否使用密文板。模式 5 下預設值 `false`，其他模式下預設值 `true`。僅適用於 Sami 主題。  
:::  
::: field name="check_collapsal_paradigms" type="boolean" optional  
是否檢測獲取的坍縮範式。模式 5 下預設值 `true`，其他模式下預設值 `false`。  
:::  
::: field name="double_check_collapsal_paradigms" type="boolean" optional default="true"  
是否執行坍縮範式檢測防漏措施。僅在主題為 Sami 且 `check_collapsal_paradigms` 為 true 時有效。模式 5 下預設值 `true`，其他模式下預設值 `false`。  
:::  
::: field name="expected_collapsal_paradigms" type="array<string>" optional default="['目空一些', '睁眼瞎', '图像损坏', '一抹黑']"  
希望觸發的坍縮範式。僅在主題為 Sami 且模式為 5 時有效。  
:::  
::: field name="monthly_squad_auto_iterate" type="boolean" optional  
是否啟動月度小隊自動切換。  
:::  
::: field name="monthly_squad_check_comms" type="boolean" optional  
是否將月度小隊通信也作為切換依據。  
:::  
::: field name="deep_exploration_auto_iterate" type="boolean" optional  
是否啟動深入調查自動切換。  
:::  
::: field name="collectible_mode_shopping" type="boolean" optional default="false"  
燒水是否啟用購物。  
:::  
::: field name="collectible_mode_squad" type="string" optional  
燒水時使用的分隊, 預設與squad同步, 當squad為空字元串且未指定collectible_mode_squad值時為指挥分队。  
:::  
::: field name="start_with_seed" type="boolean" optional default="false"  
使用種子刷錢。
<br>
僅在 Sarkaz 主題，Investment 模式，"點刺成錠分隊" or "後勤分隊" 時可能為 true。
<br>
使用固定種子。  
:::  
::::  

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "theme": "Sami",
   "mode": 5,
   "squad": "指挥分队",
   "roles": "取长补短",
   "core_char": "塑心",
   "use_support": false,
   "use_nonfriend_support": false,
   "starts_count": 3,
   "difficulty": 8,
   "stop_at_final_boss": false,
   "stop_at_max_level": false,
   "investment_enabled": true,
   "investments_count": 2,
   "stop_when_investment_full": false,
   "investment_with_more_score": false,
   "start_with_elite_two": false,
   "only_start_with_elite_two": false,
   "refresh_trader_with_dice": false,
   "first_floor_foldartal": "",
   "start_foldartal_list": [],
   "collectible_mode_start_list": {
      "hot_water": true,
      "shield": false,
      "ingot": false,
      "hope": true,
      "random": false,
      "key": false,
      "dice": false,
      "ideas": false
   },
   "use_foldartal": true,
   "check_collapsal_paradigms": true,
   "double_check_collapsal_paradigms": true,
   "expected_collapsal_paradigms": ["目空一些", "睁眼瞎"],
   "monthly_squad_auto_iterate": false,
   "monthly_squad_check_comms": false,
   "deep_exploration_auto_iterate": false,
   "collectible_mode_shopping": false,
   "collectible_mode_squad": "",
   "start_with_seed": false
}
```

</details>

刷坍縮範式功能具體請參考 [肉鴿輔助協議](./integrated-strategy-schema.md#薩米肉鴿——坍縮範式)

- `Copilot`  
   自動抄作業

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="filename" type="string"  
單一作業 JSON 檔案的路徑，與 copilot_list 二選一（必填）；相對路徑與絕對路徑均可。  
:::  
::: field name="copilot_list" type="array<object>"  
作業列表，與 filename 二選一（必填）；當 filename 與 copilot_list 同時存在時，忽視 copilot_list；此參數生效時僅可執行 set_params 一次。
<br>
每個對象包含：
<br>

- `filename`: 作業 JSON 檔案的路徑；相對路徑與絕對路徑均可
  <br>
- `stage_name`: 關卡名，具體請參考 [PRTS.Map](https://map.ark-nights.com)
  <br>
- `is_raid`: 是否切換為突襲模式，可選，預設值 false
  :::  
  ::: field name="loop_times" type="number" optional default="1"  
  循環次數。僅在單一作業模式下（即指定 filename 時）有效；此參數生效時僅可執行 set_params 一次。  
  :::  
  ::: field name="use_sanity_potion" type="boolean" optional default="false"  
  是否允許在剩餘理智不足時使用理智藥。  
  :::  
  ::: field name="formation" type="boolean" optional default="false"  
  是否進行自動編隊。  
  :::  
  ::: field name="formation_index" type="number" optional default="0"  
  自動編隊所使用的編隊欄位的編號。僅在 formation 為 true 時有效。
  <br>
  為 0–4 的整數，其中 0 表示選擇當前編隊，1-4 分別表示第一、二、三、四編隊。  
  :::  
  ::: field name="user_additional" type="array<object>" optional default="[]"  
  自訂追加幹員列表。僅在 formation 為 true 時有效。
  <br>
  每個對象包含：
  <br>
- `name`: 幹員名，可選，預設值 ""，若留空則忽視此幹員
  <br>
- `skill`: 需要攜帶的技能，可選，預設值 1；為 1–3 的整數，若不在此範圍內則遵從遊戲內預設的技能選擇  
  :::  
  ::: field name="add_trust" type="boolean" optional default="false"  
  是否在自動編隊時以信賴值升序自動填充空餘欄位。僅在 formation 為 true 時有效。  
  :::  
  ::: field name="ignore_requirements" type="boolean" optional default="false"  
  是否在自動編隊時忽視幹員屬性要求。僅在 formation 為 true 時有效。  
  :::  
  ::: field name="support_unit_usage" type="number" optional default="0"  
  助戰幹員的使用模式。為 0–3 的整數。僅在 formation 為 true 時有效。
  <br>
  `0` - 表示不使用助戰幹員
  <br>
  `1` - 如果有且僅有一名缺失幹員則嘗試尋找助戰幹員補齊編隊，如果無缺失幹員則不使用助戰幹員
  <br>
  `2` - 如果有且僅有一名缺失幹員則嘗試尋找助戰幹員補齊編隊，如果無缺失幹員則使用指定助戰幹員  
  <br>
  `3` - 如果有且僅有一名缺失幹員則嘗試尋找助戰幹員補齊編隊，如果無缺失幹員則使用隨機助戰幹員  
  :::  
  ::: field name="support_unit_name" type="string" optional default=""  
  指定助戰幹員名。僅在 support_unit_usage 為 2 時有效。  
  :::  
  ::::

作業 JSON 請參考 [戰鬥流程協議](./copilot-schema.md)

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "copilot/1-7.json",
   "loop_times": 2,
   "use_sanity_potion": false,
   "formation": true,
   "formation_index": 1,
   "user_additional": [
      {
         "name": "能天使",
         "skill": 3
      }
   ],
   "add_trust": true,
   "ignore_requirements": false,
   "support_unit_usage": 2,
   "support_unit_name": "艾雅法拉"
}
```

</details>

- `SSSCopilot`  
   自動抄保全作業

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="filename" type="string" required  
作業 JSON 的檔案路徑，絕對、相對路徑均可。不支援執行期設定。  
:::  
::: field name="loop_times" type="number" optional  
循環執行次數。  
:::  
::::  
保全作業 JSON 請參考 [保全派駐協議](./sss-schema.md)

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "sss/plan.json",
   "loop_times": 1
}
```

</details>

- `ParadoxCopilot`
  自動抄悖論模擬作業

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="filename" type="string" required  
單個作業 JSON 的檔案路徑，絕對、相對路徑均可。不支援執行期設定。必選，與 list 二選一。  
:::  
::: field name="list" type="array<string>" required  
作業 JSON 列表，絕對、相對路徑均可。不支援執行期設定。必選，與 filename 二選一。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "paradox/exusiai.json",
   "list": []
}
```

</details>

- `Depot`  
   倉庫辨識

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true
}
```

</details>

- `OperBox`  
   幹員 box 辨識

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true
}
```

</details>

- `Reclamation`  
   生息演算

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="theme" type="string" optional default="Fire"  
主題。
<br>
`Fire` - _沙中之火_
<br>
`Tales` - _沙洲遺聞_  
:::  
::: field name="mode" type="number" optional default="0"  
模式。
<br>
`0` - 刷分與建造點，進入戰鬥直接退出。
<br>
`1` - 沙中之火：刷赤金，聯絡員買水後基地鍛造；沙洲遺聞：自動製造物品並讀檔刷貨幣。  
:::  
::: field name="tools_to_craft" type="array<string>" optional default="[&quot;荧光棒&quot;]"  
自動製造的物品，建議填寫子串。  
:::  
::: field name="increment_mode" type="number" optional default="0"  
點擊類型。
<br>
`0` - 連點
<br>
`1` - 長按  
:::  
::: field name="num_craft_batches" type="number" optional default="16"  
單次最大製造輪數。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "theme": "Fire",
   "mode": 1,
   "tools_to_craft": ["荧光棒", "发电机"],
   "increment_mode": 0,
   "num_craft_batches": 12
}
```

</details>

- `Custom`  
   自訂任務

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="task_names" type="array<string>" required  
執行陣列中第一個匹配上的任務（及後續 next 等）。若想執行多個任務，可多次 append Custom task。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "task_names": ["StartUp", "Infrast", "Fight"]
}
```

</details>

- `SingleStep`  
   單步任務（目前僅支援戰鬥）

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="type" type="string" required default="copilot"  
目前僅支援 `"copilot"`。  
:::  
::: field name="subtask" type="string" required  
子任務類型。
<br>
`stage` - 設定關卡名，需要 `"details": { "stage": "xxxx" }`。
<br>
`start` - 開始作戰，無 `details`。
<br>
`action` - 單步作戰操作，`details` 需為作戰協議中的單個 action，例如：`"details": { "name": "史爾特爾", "location": [ 4, 5 ], "direction": "左" }`，詳情參考 [戰鬥流程協議](./copilot-schema.md)。  
:::  
::: field name="details" type="object" optional  
子任務的詳細參數。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "type": "copilot",
   "subtask": "stage",
   "details": {
      "stage": "1-7"
   }
}
```

</details>

- `VideoRecognition`  
  影片辨識，目前僅支援作業（作戰）影片

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
是否啟用本任務。  
:::  
::: field name="filename" type="string" required  
影片的檔案路徑，絕對、相對路徑均可。不支援執行期設定。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "videos/copilot.mp4"
}
```

</details>

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

:::: field-group  
::: field name="handle" type="AsstHandle" required  
實例句柄  
:::  
::: field name="task" type="AsstTaskId" required  
任務 ID, `AsstAppendTask` 接口的返回值  
:::  
::: field name="params" type="const char\*" required  
任務參數，json string，與 `AsstAppendTask` 接口相同。  
未標注"不支援執行中設定"的欄位都支援實時修改；否則若當前任務正在執行，會忽略對應的欄位  
:::  
::::

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

:::: field-group  
::: field name="key" type="AsstStaticOptionKey" required  
鍵  
:::  
::: field name="value" type="const char\*" required  
值  
:::  
::::

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

:::: field-group  
::: field name="handle" type="AsstHandle" required  
實例句柄  
:::  
::: field name="key" type="AsstInstanceOptionKey" required  
鍵  
:::  
::: field name="value" type="const char\*" required  
值  
:::  
::::

##### 鍵值一覽

:::: field-group  
::: field name="Invalid" type="number" optional default="0"  
無效佔位。枚舉值：0。  
:::  
::: field name="MinitouchEnabled" type="boolean" optional  
已棄用。原為是否啟用 minitouch；"1" 開，"0" 關。注意設備可能不支援。枚舉值：1（已棄用）。  
:::  
::: field name="TouchMode" type="string" optional default="minitouch"  
觸控模式設定。可選值：minitouch | maatouch | adb。預設 minitouch。枚舉值：2。  
:::  
::: field name="DeploymentWithPause" type="boolean" optional  
是否暫停下幹員，同時影響抄作業、肉鴿、保全。可用值："1" 或 "0"。枚舉值：3。  
:::  
::: field name="AdbLiteEnabled" type="boolean" optional  
是否使用 AdbLite。可用值："0" 或 "1"。枚舉值：4。  
:::  
::: field name="KillAdbOnExit" type="boolean" optional  
退出時是否殺掉 Adb 進程。可用值："0" 或 "1"。枚舉值：5。  
:::  
::::
