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
  };
  ```

- `const char* details`  
  消息详情，json 字符串，详见 [字段解释](#字段解释)
- `void* custom_arg`  
  调用方自定义参数，会原样传出 `AsstCreateEx` 接口中的 `custom_arg` 参数，C 系语言可利用该参数传出 `this` 指针

## 字段解释

### InternalError

Todo

### InitFailed

```json
{
    "what": string,     // 错误类型
    "why": string,      // 错误原因
    "details": object   // 错误详情
}
```

### ConnectionInfo

```json
{
    "what": string,     // 信息类型
    "why": string,      // 信息原因
    "uuid": string,     // 设备唯一码（连接失败时为空）
    "details": {
        "adb": string,     // AsstConnect 接口 adb_path 参数
        "address": string, // AsstConnect 接口 address 参数
        "config": string   // AsstConnect 接口 config 参数
    }
}
```

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

```json
{
    "uuid": string,             // 设备唯一码
    "what": string,             // 回调类型，"Connect" | "Click" | "Screencap" | ...
    "async_call_id": int,       // 异步请求 id，即调用 AsstAsyncXXX 时的返回值
    "details": {
        "ret": bool,            // 实际调用的返回值
        "cost": int64,          // 耗时，单位毫秒
    }
}
```

### AllTasksCompleted

```json
{
    "taskchain": string,            // 最后的任务链
    "uuid": string,                 // 设备唯一码
    "finished_tasks": [             // 已经运行过的任务 id
        int,
        ...
    ]
}
```

#### 常见 `taskchain` 字段

- `StartUp`  
  开始唤醒
- `CloseDown`  
  关闭游戏
- `Fight`  
  刷理智
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
- `ReclamationAlgorithm`  
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

```json
{
    "taskchain": string,            // 当前的任务链
    "taskid": int,                  // 当前任务 TaskId
    "uuid": string                  // 设备唯一码
}
```

### TaskChainExtraInfo

Todo

### SubTask 相关消息

```json
{
    "subtask": string,             // 子任务名
    "class": string,               // 子任务符号名
    "taskchain": string,           // 当前任务链
    "taskid": int,                 // 当前任务 TaskId
    "details": object,             // 详情
    "uuid": string                 // 设备唯一码
}
```

#### 常见 `subtask` 字段

- `ProcessTask`  

  ```json
  // 对应的 details 字段举例
  {
    "task": "StartButton2", // 任务名
    "action": 512,
    "exec_times": 1, // 已执行次数
    "max_times": 999, // 最大执行次数
    "algorithm": 0
  }
  ```

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

```json
{
    "taskchain": string,           // 当前任务链
    "class": string,               // 子任务类型
    "what": string,                // 信息类型
    "details": object,             // 信息详情
    "uuid": string,                // 设备唯一码
}
```

#### 常见 `what` 及 `details` 字段

- `StageDrops`  
  关卡材料掉落信息

  ```json
  // 对应的 details 字段举例
  {
    "drops": [
      // 本次识别到的掉落材料
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
    "stage": {
      // 关卡信息
      "stageCode": "CA-5",
      "stageId": "wk_fly_5"
    },
    "stars": 3, // 行动结束星级
    "stats": [
      // 本次执行期间总的材料掉落
      {
        "itemId": "3301",
        "itemName": "技巧概要·卷1",
        "quantity": 4,
        "addQuantity": 2 //本次新增的掉落数量
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
  公招识别到了 Tags

  ```json
  // 对应的 details 字段举例
  {
    "tags": ["费用回复", "防护", "先锋干员", "辅助干员", "近战位"]
  }
  ```

- `RecruitSpecialTag`  
  公招识别到了特殊 Tag

  ```json
  // 对应的 details 字段举例
  {
    "tag": "高级资深干员"
  }
  ```

- `RecruitResult`  
  公招识别结果

  ```json
  // 对应的 details 字段举例
  {
    "tags": [
      // 所有识别到的 tags，目前来说一定是 5 个
      "削弱",
      "减速",
      "术师干员",
      "辅助干员",
      "近战位"
    ],
    "level": 4, // 总的星级
    "result": [
      {
        "tags": ["削弱"],
        "level": 4, // 这组 tags 的星级
        "opers": [
          {
            "name": "初雪",
            "level": 5 // 干员星级
          },
          {
            "name": "陨星",
            "level": 5
          },
          {
            "name": "槐琥",
            "level": 5
          },
          {
            "name": "夜烟",
            "level": 4
          },
          {
            "name": "流星",
            "level": 4
          }
        ]
      },
      {
        "tags": ["减速", "术师干员"],
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
        "tags": ["削弱", "术师干员"],
        "level": 4,
        "opers": [
          {
            "name": "夜烟",
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
  // 对应的 details 字段举例
  {
    "count": 1, // 当前槽位已刷新次数
    "refresh_limit": 3 // 当前槽位刷新次数上限
  }
  ```

- `RecruitNoPermit`  
  公招无招聘许可

  ```json
  // 对应的 details 字段举例
  {
    "continue": true // 是否继续刷新
  }
  ```

- `RecruitTagsSelected`  
  公招选择了 Tags

  ```json
  // 对应的 details 字段举例
  {
    "tags": ["减速", "术师干员"]
  }
  ```

- `RecruitSlotCompleted`  
  当前公招槽位任务完成

- `RecruitError`  
  公招识别错误

- `EnterFacility`  
  基建进入了设施

  ```json
  // 对应的 details 字段举例
  {
    "facility": "Mfg", // 设施名
    "index": 0 // 设施序号
  }
  ```

- `NotEnoughStaff`  
  基建可用干员不足

  ```json
  // 对应的 details 字段举例
  {
    "facility": "Mfg", // 设施名
    "index": 0 // 设施序号
  }
  ```

- `ProductOfFacility`  
  基建产物

  ```json
  // 对应的 details 字段举例
  {
    "product": "Money", // 产物名
    "facility": "Mfg", // 设施名
    "index": 0 // 设施序号
  }
  ```

- `StageInfo`  
  自动作战关卡信息

  ```json
  // 对应的 details 字段举例
  {
      "name": string  // 关卡名
  }
  ```

- `StageInfoError`  
  自动作战关卡识别错误

- `PenguinId`  
  企鹅物流 ID

  ```json
  // 对应的 details 字段举例
  {
      "id": string
  }
  ```

- `Depot`  
  仓库识别结果

  ```json
  // 对应的 details 字段举例
  "done": bool,       // 是否已经识别完了，为 false 表示仍在识别中（过程中的数据）
  "arkplanner": {     // https://penguin-stats.cn/planner
      "object": {
          "items": [
              {
                  "id": "2004",
                  "have": 4,
                  "name": "高级作战记录"
              },
              {
                  "id": "mod_unlock_token",
                  "have": 25,
                  "name": "模组数据块"
              },
              {
                  "id": "2003",
                  "have": 20,
                  "name": "中级作战记录"
              }
          ],
          "@type": "@penguin-statistics/depot"
      },
      "data": "{\"@type\":\"@penguin-statistics/depot\",\"items\":[{\"id\":\"2004\",\"have\":4,\"name\":\"高级作战记录\"},{\"id\":\"mod_unlock_token\",\"have\":25,\"name\":\"模组数据块\"},{\"id\":\"2003\",\"have\":20,\"name\":\"中级作战记录\"}]}"
  },
  "lolicon": {     // https://arkntools.app/#/material
      "object": {
          "2004" : 4,
          "mod_unlock_token": 25,
          "2003": 20
      },
      "data": "{\"2003\":20,\"2004\": 4,\"mod_unlock_token\": 25}"
  }
  // 目前只支持 ArkPlanner 和 Lolicon (Arkntools) 的格式，以后可能会兼容更多网站
  ```

- `OperBox`  
  干员识别结果

  ```json
  // 对应的 details 字段举例
  "done": bool,       // 是否已经识别完了，为 false 表示仍在识别中（过程中的数据）
  "all_oper": [
      {
          "id": "char_002_amiya",
          "name": "阿米娅",
          "own": true,
          "rarity": 5
      },
      {
          "id": "char_003_kalts",
          "name": "凯尔希",
          "own": true,
          "rarity": 6
      },
      {
          "id": "char_1020_reed2",
          "name": "焰影苇草",
          "own": false,
          "rarity": 6
      },
  ]
  "own_opers": [
      {
          "id": "char_002_amiya", // 干员id
          "name": "阿米娅", // 干员名称
          "own": true, // 是否拥有
          "elite": 2, // 精英度 0,1,2
          "level": 50, // 干员等级
          "potential": 6, // 干员潜能 [1, 6]
          "rarity": 5 // 干员稀有度 [1, 6]
      },
      {
          "id": "char_003_kalts",
          "name": "凯尔希",
          "own": true,
          "elite": 2,
          "level": 50,
          "potential": 1,
          "rarity": 6
      }
  ]
  ```

- `UnsupportedLevel`  
  自动抄作业，不支持的关卡名
