---
order: 2
icon: material-symbols:u-turn-left
---

# 回呼訊息協定

::: info 注意
回呼訊息 (Callback) 隨版本更新快速迭代中，本文件可能過時。如需獲取最新內容可參考 [C# 整合原始碼](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/src/MaaWpfGui/Main/AsstProxy.cs)
:::

## 回呼函式原型

```cpp
typedef void(ASST_CALL* AsstApiCallback)(AsstMsgId msg, const char* details_json, void* custom_arg);
```

其中 `AsstMsgId` 為 `int32_t` 的別名。

## 參數總覽

- `AsstMsgId msg`  
   消息類型

  ```cpp
  enum class AsstMsg
  {
   /* Global Info */
   InternalError     = 0,           // 內部錯誤
   InitFailed        = 1,           // 初始化失敗
   ConnectionInfo    = 2,           // 連線相關資訊
   AllTasksCompleted = 3,           // 全部任務完成
   AsyncCallInfo     = 4,           // 外部非同步呼叫資訊
   Destroyed         = 5,           // 執行個體已銷毀

   /* TaskChain Info */
   TaskChainError     = 10000,      // 任務鏈執行 / 辨識錯誤
   TaskChainStart     = 10001,      // 任務鏈開始
   TaskChainCompleted = 10002,      // 任務鏈完成
   TaskChainExtraInfo = 10003,      // 任務鏈額外資訊
   TaskChainStopped   = 10004,      // 任務鏈手動停止

   /* SubTask Info */
   SubTaskError      = 20000,       // 原子任務執行 / 辨識錯誤
   SubTaskStart      = 20001,       // 原子任務開始
   SubTaskCompleted  = 20002,       // 原子任務完成
   SubTaskExtraInfo  = 20003,       // 原子任務額外資訊
   SubTaskStopped    = 20004,       // 原子任務手動停止

   /* Web Request */
   ReportRequest     = 30000,       // 彙報請求
  };
  ```

- `const char* details`  
   消息詳情，json 字串，詳見 [欄位解釋](#欄位解釋)
- `void* custom_arg`  
   呼叫端自訂參數。會原樣傳出 `AsstCreateEx` 介面中的 `custom_arg` 參數，C 系語言可利用此參數傳出 `this` 指標。

## 欄位解釋

### InternalError

`details` 欄位為空。

### InitFailed

:::: field-group
::: field what
@type string
@required
錯誤類型。
:::  
::: field why
@type string
@required
錯誤原因。
:::  
::: field details
@type object
@required
錯誤詳情。
:::  
::::

### ConnectionInfo

:::: field-group
::: field what
@type string
@required
資訊類型。
:::
::: field why
@type string
@required
資訊原因。
:::
::: field uuid
@type string
設備唯一碼（連線失敗時為空）。
:::
::: field details
@type object
@required
連線詳情。其結構如下：

- `adb` (string, required): `AsstConnect` 介面 `adb_path` 參數。
- `address` (string, required): `AsstConnect` 介面 `address` 參數。
- `config` (string, required): `AsstConnect` 介面 `config` 參數。

:::
::::

### 常見 `What` 欄位

- `ConnectFailed`  
   連線失敗
- `Connected`  
   已連接，注意此時的 `uuid` 欄位值為空（下一步才是獲取）
- `UuidGot`  
   已獲取到設備唯一碼
- `UnsupportedResolution`  
   解析度不被支援
- `ResolutionError`  
   解析度獲取錯誤
- `Reconnecting`  
   連線中斷（adb / 模擬器 炸了），正在重連
- `Reconnected`  
   連線中斷（adb / 模擬器 炸了），重連成功
- `Disconnect`  
   連線中斷（adb / 模擬器 炸了），並重試失敗
- `ScreencapFailed`  
   截圖失敗（adb / 模擬器 炸了），並重試失敗
- `TouchModeNotAvailable`  
   不支援的觸控模式
- `MuMuExtrasInputStatus`  
   MuMu 觸控增強的實際生效狀態，`details` 結構如下：
  - `available` (boolean, required): 是否已生效。
  - `deferred` (boolean, required): 是否尚未判定（連線時遊戲尚未開始渲染，會在開始渲染後自動重試）。
- `ResolutionGot`  
   已獲取到解析度
- `ResolutionChanged`  
   執行中解析度被修改，連線已斷開並中斷目前任務，`details` 結構如下：
  - `width` (number, required): 目前寬度。
  - `height` (number, required): 目前高度。
- `FastestWayToScreencap`  
   已找到最快的截圖方式，`details` 結構如下：
  - `method` (string, required): 最快的截圖方式。
  - `cost` (number, required): 耗時，單位毫秒。
  - `alternatives` (array`<object>`, required): 各候選方式及其耗時。

- `ScreencapCost`  
   截圖耗時統計（每 10 次截圖回傳一次），`details` 結構如下：
  - `min` (number, required): 最小耗時，單位毫秒。
  - `max` (number, required): 最大耗時，單位毫秒。
  - `avg` (number, required): 平均耗時，單位毫秒。
  - `fault_times` (number): 失敗次數（僅有失敗時存在）。

- `EmulatorFPS`  
   模擬器更新率（每 1 分鐘檢測一次），`details` 結構如下：
  - `fps` (number, required): 模擬器/系統更新率（FPS）。
  - `refresh_period_ns` (number, required): 每幀更新週期，單位奈秒。

### AsyncCallInfo

:::: field-group
::: field uuid
@type string
@required
設備唯一碼。
:::
::: field what
@type string
@required
回呼類型，例如 `Connect` | `AttachWindow` | `Click` | `Screencap` 等。
:::
::: field async_call_id
@type number
@required
非同步請求 ID，即呼叫 `AsstAsyncXXX` 時的回傳值。
:::
::: field details
@type object
@required
非同步呼叫詳情。其結構如下：

- `ret` (boolean, required): 實際呼叫的回傳值。
- `cost` (number, required): 耗時，單位毫秒。
- `error` (string): 未處理例外的類型（僅呼叫因未處理例外失敗時存在）。

:::
::::

### AllTasksCompleted

:::: field-group
::: field taskchain
@type string
@required
最後的任務鏈。
:::
::: field taskid
@type number
@required
最後一條任務鏈對應的任務 TaskId。
:::
::: field uuid
@type string
@required
設備唯一碼。
:::
::: field finished_tasks
@type array<number>
@required
已經執行過的任務 ID 列表。
:::
::::

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
- `ParadoxCopilot`  
   自動抄悖論模擬作業
- `Depot`  
   倉庫辨識
- `OperBox`  
   幹員 Box 辨識
- `Reclamation`  
   生息演算
- `Custom`  
   自訂任務
- `SingleStep`  
   單步任務
- `VideoRecognition`  
   影片辨識任務
- `Debug`  
   偵錯

### TaskChain 相關消息

`TaskChainError` / `TaskChainStart` / `TaskChainCompleted` / `TaskChainExtraInfo` / `TaskChainStopped` 共用以下欄位：

:::: field-group
::: field taskchain
@type string
@required
目前的任務鏈。
:::
::: field taskid
@type number
@required
目前任務 TaskId。
:::
::: field uuid
@type string
@required
設備唯一碼。
:::
::::

其中 `TaskChainError` 在任務鏈因未處理異常中斷時會額外攜帶 `details` 欄位：

- `error` (string, required): 異常類型，取值為 `OpenCVException` | `OutOfMemory` | `UnhandledException` | `UnknownException`。

### TaskChainExtraInfo

預設僅攜帶上述公共欄位。肉鴿（界園主題）路徑規劃判定無法避開過多戰鬥而主動重開時，訊息會額外攜帶：

- `what` (string, required): 固定為 `RoutingRestart`。
- `why` (string, required): 固定為 `TooManyBattlesAhead`。
- `node_cost` (number, required): 規劃得到的下一節點代價。

### SubTask 相關消息

:::: field-group
::: field subtask
@type string
@required
子任務名稱。
:::
::: field class
@type string
@required
子任務符號名稱。
:::
::: field taskchain
@type string
@required
目前任務鏈。
:::
::: field taskid
@type number
@required
目前任務 TaskId。
:::
::: field details
@type object
@required
詳情。
:::
::: field uuid
@type string
@required
設備唯一碼。
:::
::::

#### 常見 `subtask` 欄位

- `ProcessTask`  
  `details` 欄位內容如下：

  :::: field-group
  ::: field task
  @type string
  @required
  任務名稱。
  :::
  ::: field action
  @type string
  @required
  動作名稱，例如 `ClickSelf` | `DoNothing` | `Swipe`。
  :::
  ::: field exec_times
  @type number
  @required
  已執行次數。
  :::
  ::: field max_times
  @type number
  @required
  最大執行次數。
  :::
  ::: field algorithm
  @type string
  @required
  辨識演算法名稱，例如 `MatchTemplate` | `OcrDetect` | `FeatureMatch` | `JustReturn`。
  :::
  ::: field result
  @type object
  @required
  本次辨識結果，各演算法結構不同；無辨識結果時為空物件。
  :::
  ::::

  此外，當任務執行次數達到上限時，會先回傳一條 `SubTaskExtraInfo` 訊息，`what` 為 `ExceededLimit`，`details` 包含 `task`（任務名稱）、`exec_times`（已執行次數）、`max_times`（最大執行次數）。

- `ReportToPenguinStats` / `ReportToYituliu`  
  彙報戰鬥掉落到企鵝數據統計 / 一圖流大數據（上報失敗時以 `SubTaskError` 回傳）。
- `StartGameTask`  
  打開用戶端失敗（設定檔與傳入 `client_type` 不匹配）。
- Todo 其他

##### 常見 `task` 欄位

- `StartButton2`  
   開始戰鬥
- `MedicineConfirm`  
   使用理智藥
- `StoneConfirm`  
   碎石
- `RecruitRefreshConfirm`  
   公招刷新標籤
- `RecruitConfirm`  
   公招確認招募
- `RecruitNowConfirm`  
   公招使用加急許可
- `InfrastDormDoubleConfirmButton`  
   基建的二次確認按鈕。僅當待進駐幹員已進駐其他設施時才會出現（MAA 將自動點擊確認），請提示使用者。
- `StartExplore`  
   肉鴿開始探索
- `StageTraderInvestConfirm`  
   肉鴿投資源石錠
- `StageTraderInvestSystemFull`  
   肉鴿投資達到遊戲上限
- `ExitThenAbandon`  
   肉鴿放棄本次探索
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
- `StageCombatOpsEnter`  
   肉鴿關卡：普通作戰
- `StageEmergencyOps`  
   肉鴿關卡：緊急作戰
- `StageDreadfulFoe`  
   肉鴿關卡：險路惡敵
- Todo 其他

### SubTaskExtraInfo

:::: field-group
::: field taskchain
@type string
@required
目前任務鏈。
:::
::: field class
@type string
@required
子任務類型。
:::
::: field what
@type string
@required
資訊類型。
:::
::: field details
@type object
@required
資訊詳情。
:::
::: field uuid
@type string
@required
設備唯一碼。
:::
::::

#### 常見 `what` 及 `details` 欄位

- `StageDrops`  
  關卡材料掉落資訊。`details` 欄位結構如下：
  - `drops` (array, required)：本次辨識到的掉落材料，陣列每一項包含：
    - `itemId` (string, required)：材料 ID。
    - `quantity` (number, required)：掉落數量。
    - `itemName` (string, required)：材料名稱。
  - `stage` (object, required)：關卡資訊，包含：
    - `stageCode` (string, required)：關卡編號。
    - `stageId` (string, required)：關卡 ID。
  - `stars` (number, required)：行動結束星級。
  - `stats` (array, required)：本次執行期間總計材料掉落，陣列每一項包含：
    - `itemId` (string, required)：材料 ID。
    - `itemName` (string, required)：材料名稱。
    - `quantity` (number, required)：總計數量。
    - `addQuantity` (number, required)：本次新增的掉落數量。
  - `cur_times` (number, optional)：結算畫面辨識到的連戰次數（辨識到時存在）。
  - `annihilation_weekly_process` (array, optional)：剿滅模式的本週獲取進度，格式為 `[已完成數, 上限]`（僅剿滅關卡存在）。

- `RecruitTagsDetected`  
  公招辨識到了 Tags。`details` 欄位內容如下：

  :::: field-group
  ::: field tags
  @type array<string>
  @required
  辨識到的 Tag 列表。
  :::
  ::::

- `RecruitSpecialTag`  
  公招辨識到了特殊 Tag。`details` 欄位內容如下：

  :::: field-group
  ::: field tag
  @type string
  @required
  特殊 Tag 名稱，例如 高級資深幹員。
  :::
  ::::

- `RecruitPreservedTag`  
   公招辨識到了已設定為保留的 Tag。`details` 欄位內容如下：

  :::: field-group
  ::: field tag
  @type string
  @required
  觸發保留的 Tag 名稱，例如 `支援機械`。
  :::
  ::::

- `RecruitResult`  
  公招辨識結果。`details` 欄位結構如下：
  - `tags` (array, required)：所有辨識到的 tags，目前固定為 5 個。
  - `level` (number, required)：組合的最高星級。
  - `result` (array, required)：具體的組合結果，陣列每一項包含：
    - `tags` (array, required)：參與組合的 tags。
    - `level` (number, required)：這組 tags 的星級。
    - `opers` (array, required)：可能招募到的幹員，陣列每一項包含：
      - `id` (string, required)：幹員 ID。
      - `name` (string, required)：幹員名稱。
      - `level` (number, required)：幹員星級。

- `RecruitTagsRefreshed`  
  公招刷新了 Tags。`details` 欄位內容如下：

  :::: field-group
  ::: field count
  @type number
  @required
  目前槽位已刷新次數。
  :::
  ::: field refresh_limit
  @type number
  @required
  目前槽位刷新次數上限。
  :::
  ::::

- `RecruitNoPermit`  
  公招無招聘許可。`details` 欄位內容如下：

  :::: field-group
  ::: field continue
  @type boolean
  @required
  是否繼續刷新。
  :::
  ::::

- `RecruitTagsSelected`  
  公招選擇了 Tags。`details` 欄位內容如下：

  :::: field-group
  ::: field tags
  @type array<string>
  @required
  選擇的 Tag 列表。
  :::
  ::::

- `RecruitError`  
  公招辨識錯誤。`details` 欄位為空；若因目前槽位刷新次數達到上限觸發，則包含 `refresh_limit`（槽位刷新次數上限）。

- `EnterFacility`  
  基建進入了設施。`details` 欄位內容如下：

  :::: field-group
  ::: field facility
  @type string
  @required
  設施名稱。
  :::
  ::: field index
  @type number
  @required
  設施序號。
  :::
  ::::

- `NotEnoughStaff`  
  基建可用幹員不足。`details` 欄位內容如下：

  :::: field-group
  ::: field facility
  @type string
  @required
  設施名稱。
  :::
  ::: field index
  @type number
  @required
  設施序號。
  :::
  ::::

- `ProductOfFacility`  
  基建產物。`details` 欄位內容如下：

  :::: field-group
  ::: field product
  @type string
  @required
  產物名稱。
  :::
  ::: field facility
  @type string
  @required
  設施名稱。
  :::
  ::: field index
  @type number
  @required
  設施序號。
  :::
  ::::

- `StageInfo`  
  自動作戰關卡資訊。`details` 欄位內容如下：

  :::: field-group
  ::: field name
  @type string
  @required
  關卡名稱。
  :::
  ::: field size
  @type number
  @required
  地圖格點數量。
  :::
  ::::

- `StageInfoError`  
  自動作戰關卡辨識錯誤。`details` 欄位為空。

- `DepotInfo`  
  倉庫辨識結果。`details` 欄位結構如下：
  - `done` (boolean, required)：是否已經辨識完了，為 false 表示仍在辨識中（過程中的數據）。
  - `data` (string, required)：JSON 字串，格式為 `{"物品ID": 數量, ...}`，例如 `{"2001":18000,"31043":317}`。

- `OperBoxInfo`  
  幹員辨識結果。`details` 欄位結構如下：
  - `done` (boolean, required)：是否已經辨識完了，為 false 表示仍在辨識中（過程中的數據）。
  - `all_opers` (array, required)：全幹員列表，陣列每一項包含：
    - `id` (string, required)：幹員 ID。
    - `name` (string, required)：幹員名稱。
    - `name_en` (string, required)：幹員英文名稱。
    - `name_jp` (string, required)：幹員日文名稱。
    - `name_kr` (string, required)：幹員韓文名稱。
    - `name_tw` (string, required)：幹員繁中名稱。
    - `own` (boolean, required)：是否擁有。
    - `rarity` (number, required)：幹員稀有度 [1, 6]。
  - `own_opers` (array, required)：已擁有幹員的詳細資訊列表，陣列每一項包含：
    - `id` (string, required)：幹員 ID。
    - `name` (string, required)：幹員名稱。
    - `own` (boolean, required)：是否擁有。
    - `elite` (number, required)：精英等級 [0, 2]。
    - `level` (number, required)：幹員等級。
    - `potential` (number, required)：幹員潛能 [1, 6]。
    - `rarity` (number, required)：幹員稀有度 [1, 6]。

- `UnsupportedLevel`  
  自動抄作業，不支援的關卡名稱。`details` 欄位內容如下：

  :::: field-group
  ::: field level
  @type string
  @required
  不支援的關卡名稱。
  :::
  ::::

### ReportRequest

該欄位主要用於核心模組向 UI 層傳遞網路請求資訊，UI 負責執行具體的 HTTP 彙報操作。

:::: field-group
::: field url
@type string
@required
請求的完整 URL，例如 `https://penguin-stats.io/PenguinStats/api/v2/report`。
:::
::: field headers
@type object
@required
請求 Headers 的 Key-Value 設定（不包含 `Content-Type`，由 UI 層自行加入）。
:::
::: field body
@type string
@required
請求 Body 內容（通常為 json 格式的字串）。
:::
::: field subtask
@type string
@required
子任務名稱，辨識具體彙報任務，如 `ReportToPenguinStats`、`ReportToYituliu`。
:::
::::
