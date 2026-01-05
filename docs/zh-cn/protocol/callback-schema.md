---
order: 2
icon: material-symbols:u-turn-left
---

# 回调消息协议

::: info 注意
回调消息随版本更新飞速迭代中，本文档可能过时。如需获取最新内容可参考 [C# 集成源码](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/MaaWpfGui/Main/AsstProxy.cs)
:::

## 回调函数原型

```cpp
typedef void(ASST_CALL* AsstCallback)(int msg, const char* details, void* custom_arg);
```

## 参数总览

- `int msg`  
  消息类型

  ```cpp
  enum class AsstMsg
  {
      /* Global Info */
      InternalError     = 0,           // 内部错误
      InitFailed        = 1,           // 初始化失败
      ConnectionInfo    = 2,           // 连接相关信息
      AllTasksCompleted = 3,           // 全部任务完成
      AsyncCallInfo     = 4,           // 外部异步调用信息
      Destroyed         = 5,           // 实例已销毁

      /* TaskChain Info */
      TaskChainError     = 10000,      // 任务链执行/识别错误
      TaskChainStart     = 10001,      // 任务链开始
      TaskChainCompleted = 10002,      // 任务链完成
      TaskChainExtraInfo = 10003,      // 任务链额外信息
      TaskChainStopped   = 10004,      // 任务链手动停止

      /* SubTask Info */
      SubTaskError      = 20000,       // 原子任务执行/识别错误
      SubTaskStart      = 20001,       // 原子任务开始
      SubTaskCompleted  = 20002,       // 原子任务完成
      SubTaskExtraInfo  = 20003,       // 原子任务额外信息
      SubTaskStopped    = 20004,       // 原子任务手动停止

      /* Web Request */
      ReportRequest     = 30000,       // 上报请求
  };
  ```

- `const char* details`  
  消息详情，json 字符串，详见 [字段解释](#字段解释)
- `void* custom_arg`  
  调用方自定义参数，会原样传出 `AsstCreateEx` 接口中的 `custom_arg` 参数，C 系语言可利用该参数传出 `this` 指针

## 字段解释

### InternalError

`details` 字段为空。

### InitFailed

:::: field-group
::: field name="what" type="string" required
错误类型。
:::  
::: field name="why" type="string" required
错误原因。
:::  
::: field name="details" type="object" required
错误详情。
:::  
::::

### ConnectionInfo

:::: field-group
::: field name="what" type="string" required
信息类型。
:::
::: field name="why" type="string" required
信息原因。
:::
::: field name="uuid" type="string"
设备唯一码（连接失败时为空）。
:::
::: field name="details" type="object" required
连接详情。其结构如下：

- `adb` (string, required): `AsstConnect` 接口 `adb_path` 参数。
- `address` (string, required): `AsstConnect` 接口 `address` 参数。
- `config` (string, required): `AsstConnect` 接口 `config` 参数。

:::
::::

### 常见 `What` 字段

- `ConnectFailed`  
  连接失败
- `Connected`  
  已连接，注意此时的 `uuid` 字段值为空（下一步才是获取）
- `UuidGot`  
  已获取到设备唯一码
- `UnsupportedResolution`  
  分辨率不被支持
- `ResolutionError`  
  分辨率获取错误
- `Reconnecting`  
  连接断开（adb / 模拟器 炸了），正在重连
- `Reconnected`  
  连接断开（adb / 模拟器 炸了），重连成功
- `Disconnect`  
  连接断开（adb / 模拟器 炸了），并重试失败
- `ScreencapFailed`  
  截图失败（adb / 模拟器 炸了），并重试失败
- `TouchModeNotAvailable`  
  不支持的触控模式

### AsyncCallInfo

:::: field-group
::: field name="uuid" type="string" required
设备唯一码。
:::
::: field name="what" type="string" required
回调类型，例如 `Connect` | `Click` | `Screencap` 等。
:::
::: field name="async_call_id" type="number" required
异步请求 id，即调用 `AsstAsyncXXX` 时的返回值。
:::
::: field name="details" type="object" required
异步调用详情。其结构如下：

- `ret` (boolean, required): 实际调用的返回值。
- `cost` (number, required): 耗时，单位毫秒。

:::
::::

### AllTasksCompleted

:::: field-group
::: field name="taskchain" type="string" required
最后的任务链。
:::
::: field name="uuid" type="string" required
设备唯一码。
:::
::: field name="finished_tasks" type="array<number>" required
已经运行过的任务 id 列表。
:::
::::

#### 常见 `taskchain` 字段

- `StartUp`  
  开始唤醒
- `CloseDown`  
  关闭游戏
- `Fight`  
  理智作战
- `Mall`  
  信用点及购物
- `Recruit`  
  自动公招
- `Infrast`  
  基建换班
- `Award`  
  领取日常奖励
- `Roguelike`  
  无限刷肉鸽
- `Copilot`  
  自动抄作业
- `SSSCopilot`  
  自动抄保全作业
- `Depot`  
  仓库识别
- `OperBox`  
  干员 box 识别
- `Reclamation`  
  生息演算
- `Custom`  
  自定义任务
- `SingleStep`  
  单步任务
- `VideoRecognition`  
  视频识别任务
- `Debug`  
  调试

### TaskChain 相关消息

:::: field-group
::: field name="taskchain" type="string" required
当前的任务链。
:::
::: field name="taskid" type="number" required
当前任务 TaskId。
:::
::: field name="uuid" type="string" required
设备唯一码。
:::
::::

### TaskChainExtraInfo

`details` 字段为空。

### SubTask 相关消息

:::: field-group
::: field name="subtask" type="string" required
子任务名。
:::
::: field name="class" type="string" required
子任务符号名。
:::
::: field name="taskchain" type="string" required
当前任务链。
:::
::: field name="taskid" type="number" required
当前任务 TaskId。
:::
::: field name="details" type="object" required
详情。
:::
::: field name="uuid" type="string" required
设备唯一码。
:::
::::

#### 常见 `subtask` 字段

- `ProcessTask`  
  `details` 字段内容如下：

  :::: field-group
  ::: field name="task" type="string" required
  任务名。
  :::
  ::: field name="action" type="number" required
  Action ID。
  :::
  ::: field name="exec_times" type="number" required
  已执行次数。
  :::
  ::: field name="max_times" type="number" required
  最大执行次数。
  :::
  ::: field name="algorithm" type="number" required
  识别算法。
  :::
  ::::

- Todo 其他

##### 常见 `task` 字段

- `StartButton2`  
  开始战斗
- `MedicineConfirm`  
  使用理智药
- `ExpiringMedicineConfirm`  
  使用 48 小时内过期的理智药
- `StoneConfirm`  
  碎石
- `RecruitRefreshConfirm`  
  公招刷新标签
- `RecruitConfirm`  
  公招确认招募
- `RecruitNowConfirm`  
  公招使用加急许可
- `ReportToPenguinStats`  
  汇报到企鹅数据统计
- `ReportToYituliu`  
  汇报到一图流大数据
- `InfrastDormDoubleConfirmButton`  
  基建宿舍的二次确认按钮，仅当干员冲突时才会有，请提示用户
- `StartExplore`  
  肉鸽开始探索
- `StageTraderInvestConfirm`  
  肉鸽投资了源石锭
- `StageTraderInvestSystemFull`  
  肉鸽投资达到了游戏上限
- `ExitThenAbandon`  
  肉鸽放弃了本次探索
- `MissionCompletedFlag`  
  肉鸽战斗完成
- `MissionFailedFlag`  
  肉鸽战斗失败
- `StageTraderEnter`  
  肉鸽关卡：诡异行商
- `StageSafeHouseEnter`  
  肉鸽关卡：安全的角落
- `StageEncounterEnter`  
  肉鸽关卡：不期而遇/古堡馈赠
- `StageCombatDpsEnter`  
  肉鸽关卡：普通作战
- `StageEmergencyDps`  
  肉鸽关卡：紧急作战
- `StageDreadfulFoe`  
  肉鸽关卡：险路恶敌
- `StartGameTask`
  打开客户端失败（配置文件与传入 client_type 不匹配）
- Todo 其他

### SubTaskExtraInfo

:::: field-group
::: field name="taskchain" type="string" required
当前任务链。
:::
::: field name="class" type="string" required
子任务类型。
:::
::: field name="what" type="string" required
信息类型。
:::
::: field name="details" type="object" required
信息详情。
:::
::: field name="uuid" type="string" required
设备唯一码。
:::
::::

#### 常见 `what` 及 `details` 字段

- `StageDrops`  
  关卡材料掉落信息。`details` 字段结构如下：
  - `drops` (array, required): 本次识别到的掉落材料，数组每一项包含：
    - `itemId` (string, required): 材料 ID。
    - `quantity` (number, required): 掉落数量。
    - `itemName` (string, required): 材料名称。
  - `stage` (object, required): 关卡信息，包含：
    - `stageCode` (string, required): 关卡编号。
    - `stageId` (string, required): 关卡 ID。
  - `stars` (number, required): 行动结束星级。
  - `stats` (array, required): 本次执行期间总的材料掉落，数组每一项包含：
    - `itemId` (string, required): 材料 ID。
    - `itemName` (string, required): 材料名称。
    - `quantity` (number, required): 总计数量。
    - `addQuantity` (number, required): 本次新增的掉落数量。

- `RecruitTagsDetected`  
  公招识别到了 Tags。`details` 字段内容如下：

  :::: field-group
  ::: field name="tags" type="array<string>" required
  识别到的 Tag 列表。
  :::
  ::::

- `RecruitSpecialTag`  
  公招识别到了特殊 Tag。`details` 字段内容如下：

  :::: field-group
  ::: field name="tag" type="string" required
  特殊 Tag 名称，例如 `高级资深干员`。
  :::
  ::::

- `RecruitResult`  
  公招识别结果。`details` 字段结构如下：
  - `tags` (array, required): 所有识别到的 tags，目前固定为 5 个。
  - `level` (number, required): 组合的最高星级。
  - `result` (array, required): 具体的组合结果，数组每一项包含：
    - `tags` (array, required): 参与组合的 tags。
    - `level` (number, required): 这组 tags 的星级。
    - `opers` (array, required): 可能招募到的干员，数组每一项包含：
      - `name` (string, required): 干员名称。
      - `level` (number, required): 干员星级。

- `RecruitTagsRefreshed`  
  公招刷新了 Tags。`details` 字段内容如下：

  :::: field-group
  ::: field name="count" type="number" required
  当前槽位已刷新次数。
  :::
  ::: field name="refresh_limit" type="number" required
  当前槽位刷新次数上限。
  :::
  ::::

- `RecruitNoPermit`  
  公招无招聘许可。`details` 字段内容如下：

  :::: field-group
  ::: field name="continue" type="boolean" required
  是否继续刷新。
  :::
  ::::

- `RecruitTagsSelected`  
  公招选择了 Tags。`details` 字段内容如下：

  :::: field-group
  ::: field name="tags" type="array<string>" required
  选择的 Tag 列表。
  :::
  ::::

- `RecruitSlotCompleted`  
  当前公招槽位任务完成。`details` 字段为空。

- `RecruitError`  
  公招识别错误。`details` 字段为空。

- `EnterFacility`  
  基建进入了设施。`details` 字段内容如下：

  :::: field-group
  ::: field name="facility" type="string" required
  设施名。
  :::
  ::: field name="index" type="number" required
  设施序号。
  :::
  ::::

- `NotEnoughStaff`  
  基建可用干员不足。`details` 字段内容如下：

  :::: field-group
  ::: field name="facility" type="string" required
  设施名。
  :::
  ::: field name="index" type="number" required
  设施序号。
  :::
  ::::

- `ProductOfFacility`  
  基建产物。`details` 字段内容如下：

  :::: field-group
  ::: field name="product" type="string" required
  产物名。
  :::
  ::: field name="facility" type="string" required
  设施名。
  :::
  ::: field name="index" type="number" required
  设施序号。
  :::
  ::::

- `StageInfo`  
  自动作战关卡信息。`details` 字段内容如下：

  :::: field-group
  ::: field name="name" type="string" required
  关卡名。
  :::
  ::::

- `StageInfoError`  
  自动作战关卡识别错误。`details` 字段为空。

- `PenguinId`  
  企鹅物流 ID。`details` 字段内容如下：

  :::: field-group
  ::: field name="id" type="string" required
  企鹅物流 ID。
  :::
  ::::

- `Depot`  
  仓库识别结果。`details` 字段结构如下：
  - `done` (boolean, required): 是否已经识别完了，为 `false` 表示仍在识别中（过程中的数据）。
  - `arkplanner` (object, required): [ArkPlanner](https://penguin-stats.cn/planner) 格式的数据。
  - `lolicon` (object, required): [lolicon](https://arkntools.app/#/material) (Arkntools) 格式的数据。

- `OperBox`  
  干员识别结果。`details` 字段结构如下：
  - `done` (boolean, required): 是否已经识别完了，为 `false` 表示仍在识别中（过程中的数据）。
  - `all_oper` (array, required): 全干员列表，数组每一项包含：
    - `id` (string, required): 干员 ID。
    - `name` (string, required): 干员名称。
    - `own` (boolean, required): 是否拥有。
    - `rarity` (number, required): 干员稀有度 [1, 6]。
  - `own_opers` (array, required): 已拥有干员的详细信息列表，数组每一项包含：
    - `id` (string, required): 干员 ID。
    - `name` (string, required): 干员名称。
    - `own` (boolean, required): 是否拥有。
    - `elite` (number, required): 精英等级 [0, 2]。
    - `level` (number, required): 干员等级。
    - `potential` (number, required): 干员潜能 [1, 6]。
    - `rarity` (number, required): 干员稀有度 [1, 6]。

- `UnsupportedLevel`  
  自动抄作业，不支持的关卡名。`details` 字段为空。

### ReportRequest

该字段主要用于核心模块向 UI 层传递网络请求信息，UI 负责执行具体的 HTTP 上报操作。

:::: field-group
::: field name="url" type="string" required
请求的完整 URL，例如 `https://penguin-stats.io/PenguinStats/api/v2/report`。
:::
::: field name="headers" type="object" required
请求头键值对（不包含 `Content-Type`，UI 层自行添加）。
:::
::: field name="body" type="string" required
请求体内容（通常是 JSON 格式的字符串）。
:::
::: field name="subtask" type="string" required
子任务名称，标识具体上报任务，如 `ReportToPenguinStats`、`ReportToYituliu`。
:::
::::
