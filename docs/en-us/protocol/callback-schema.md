---
order: 2
icon: material-symbols:u-turn-left
---

# Callback Message Protocol

::: info Note
Callback messages are rapidly evolving with each version update, so this document may become outdated. For the latest information, please refer to the [C# Integration Source Code](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/MaaWpfGui/Main/AsstProxy.cs)
:::

## Callback Function Prototype

```cpp
typedef void(ASST_CALL* AsstCallback)(int msg, const char* details, void* custom_arg);
```

## Parameter Overview

- `int msg`  
  Message type

  ```cpp
  enum class AsstMsg
  {
      /* Global Info */
      InternalError     = 0,           // Internal error
      InitFailed        = 1,           // Initialization failed
      ConnectionInfo    = 2,           // Connection-related information
      AllTasksCompleted = 3,           // All tasks completed
      AsyncCallInfo     = 4,           // External asynchronous call information
      Destroyed         = 5,           // Instance destroyed

      /* TaskChain Info */
      TaskChainError     = 10000,      // Task chain execution/recognition error
      TaskChainStart     = 10001,      // Task chain started
      TaskChainCompleted = 10002,      // Task chain completed
      TaskChainExtraInfo = 10003,      // Task chain extra information
      TaskChainStopped   = 10004,      // Task chain manually stopped

      /* SubTask Info */
      SubTaskError      = 20000,       // Atomic task execution/recognition error
      SubTaskStart      = 20001,       // Atomic task started
      SubTaskCompleted  = 20002,       // Atomic task completed
      SubTaskExtraInfo  = 20003,       // Atomic task extra information
      SubTaskStopped    = 20004,       // Atomic task manually stopped
  };
  ```

- `const char* details`  
  Message details, JSON string, see [Field Explanations](#field-explanations)
- `void* custom_arg`  
  Custom parameter from the caller, returns the `custom_arg` parameter from the `AsstCreateEx` interface unchanged. C-like languages can use this parameter to pass `this` pointers

## Field Explanations

### InternalError

Todo

### InitFailed

```json
{
    "what": string,     // Error type
    "why": string,      // Error reason
    "details": object   // Error details
}
```

### ConnectionInfo

```json
{
    "what": string,     // Information type
    "why": string,      // Information reason
    "uuid": string,     // Device unique ID (empty when connection fails)
    "details": {
        "adb": string,     // AsstConnect interface adb_path parameter
        "address": string, // AsstConnect interface address parameter
        "config": string   // AsstConnect interface config parameter
    }
}
```

### Common `What` Field Values

- `ConnectFailed`  
  Connection failed
- `Connected`  
  Connected, note that the `uuid` field is empty at this point (next step will retrieve it)
- `UuidGot`  
  Device unique ID retrieved
- `UnsupportedResolution`  
  Resolution not supported
- `ResolutionError`  
  Resolution retrieval error
- `Reconnecting`  
  Connection lost (adb/emulator crashed), attempting reconnection
- `Reconnected`  
  Connection lost (adb/emulator crashed), reconnection successful
- `Disconnect`  
  Connection lost (adb/emulator crashed), and reconnection failed
- `ScreencapFailed`  
  Screenshot failed (adb/emulator crashed), and retry failed
- `TouchModeNotAvailable`  
  Unsupported touch mode

### AsyncCallInfo

```json
{
    "uuid": string,             // Device unique ID
    "what": string,             // Callback type, "Connect" | "Click" | "Screencap" | ...
    "async_call_id": int,       // Asynchronous request ID, i.e., return value when calling AsstAsyncXXX
    "details": {
        "ret": bool,            // Actual call return value
        "cost": int64,          // Time cost, in milliseconds
    }
}
```

### AllTasksCompleted

```json
{
    "taskchain": string,            // Last task chain
    "uuid": string,                 // Device unique ID
    "finished_tasks": [             // IDs of tasks that have been run
        int,
        ...
    ]
}
```

#### Common `taskchain` Field Values

- `StartUp`  
  Start up
- `CloseDown`  
  Close down
- `Fight`  
  Sanity farming
- `Mall`  
  Credit store and shopping
- `Recruit`  
  Auto recruitment
- `Infrast`  
  Base management
- `Award`  
  Collect daily rewards
- `Roguelike`  
  Integrated Strategy
- `Copilot`  
  Auto combat
- `SSSCopilot`  
  Auto Stationary Security Service
- `Depot`  
  Depot recognition
- `OperBox`  
  Operator box recognition
- `Reclamation`  
  Reclamation Algorithm
- `Custom`  
  Custom task
- `SingleStep`  
  Single step task
- `VideoRecognition`  
  Video recognition task
- `Debug`  
  Debug

### TaskChain Related Messages

```json
{
    "taskchain": string,            // Current task chain
    "taskid": int,                  // Current task TaskId
    "uuid": string                  // Device unique ID
}
```

### TaskChainExtraInfo

Todo

### SubTask Related Messages

```json
{
    "subtask": string,             // Subtask name
    "class": string,               // Subtask symbol name
    "taskchain": string,           // Current task chain
    "taskid": int,                 // Current task TaskId
    "details": object,             // Details
    "uuid": string                 // Device unique ID
}
```

#### Common `subtask` Field Values

- `ProcessTask`  

  ```json
  // Corresponding details field example
  {
    "task": "StartButton2", // Task name
    "action": 512,
    "exec_times": 1, // Execution times
    "max_times": 999, // Maximum execution times
    "algorithm": 0
  }
  ```

- Todo others

##### Common `task` Field Values

- `StartButton2`  
  Start battle
- `MedicineConfirm`  
  Use sanity potion
- `ExpiringMedicineConfirm`  
  Use sanity potion expiring within 48 hours
- `StoneConfirm`  
  Use Originium Prime
- `RecruitRefreshConfirm`  
  Refresh recruitment tags
- `RecruitConfirm`  
  Confirm recruitment
- `RecruitNowConfirm`  
  Use Expedited Plan
- `ReportToPenguinStats`  
  Report to Penguin Statistics
- `ReportToYituliu`  
  Report to Yituliu big data
- `InfrastDormDoubleConfirmButton`  
  Base dormitory double confirmation button, appears only when operators conflict, please notify users
- `StartExplore`  
  Start Integrated Strategy exploration
- `StageTraderInvestConfirm`  
  Integrated Strategy invested Originium Ingots
- `StageTraderInvestSystemFull`  
  Integrated Strategy investment reached game limit
- `ExitThenAbandon`  
  Integrated Strategy abandoned current exploration
- `MissionCompletedFlag`  
  Integrated Strategy battle completed
- `MissionFailedFlag`  
  Integrated Strategy battle failed
- `StageTraderEnter`  
  Integrated Strategy stage: Merchant
- `StageSafeHouseEnter`  
  Integrated Strategy stage: Safe House
- `StageEncounterEnter`  
  Integrated Strategy stage: Encounter/Castle Gift
- `StageCombatDpsEnter`  
  Integrated Strategy stage: Combat
- `StageEmergencyDps`  
  Integrated Strategy stage: Emergency Combat
- `StageDreadfulFoe`  
  Integrated Strategy stage: Dreadful Foe
- `StartGameTask`
  Failed to open client (config file does not match passed client_type)
- Todo others

### SubTaskExtraInfo

```json
{
    "taskchain": string,           // Current task chain
    "class": string,               // Subtask type
    "what": string,                // Information type
    "details": object,             // Information details
    "uuid": string,                // Device unique ID
}
```

#### Common `what` and `details` Field Values

- `StageDrops`  
  Stage material drop information

  ```json
  // Corresponding details field example
  {
    "drops": [
      // Materials dropped in this recognition
      {
        "itemId": "3301",
        "quantity": 2,
        "itemName": "技巧概要·卷1" // "Skill Summary - 1"
      },
      {
        "itemId": "3302",
        "quantity": 1,
        "itemName": "技巧概要·卷2" // "Skill Summary - 2"
      },
      {
        "itemId": "3303",
        "quantity": 2,
        "itemName": "技巧概要·卷3" // "Skill Summary - 3"
      }
    ],
    "stage": {
      // Stage information
      "stageCode": "CA-5",
      "stageId": "wk_fly_5"
    },
    "stars": 3, // Operation completion stars
    "stats": [
      // Total material drops during this execution
      {
        "itemId": "3301",
        "itemName": "技巧概要·卷1", // "Skill Summary - 1"
        "quantity": 4,
        "addQuantity": 2 // New drop quantity this time
      },
      {
        "itemId": "3302",
        "itemName": "技巧概要·卷2", // "Skill Summary - 2"
        "quantity": 3,
        "addQuantity": 1
      },
      {
        "itemId": "3303",
        "itemName": "技巧概要·卷3", // "Skill Summary - 3"
        "quantity": 4,
        "addQuantity": 2
      }
    ]
  }
  ```

- `RecruitTagsDetected`  
  Recruitment tags detected

  ```json
  // Corresponding details field example
  {
    "tags": ["费用回复", "防护", "先锋干员", "辅助干员", "近战位"] // ["DP-Recovery", "Defense", "Vanguard", "Support", "Melee"]
  }
  ```

- `RecruitSpecialTag`  
  Recruitment special tag detected

  ```json
  // Corresponding details field example
  {
    "tag": "高级资深干员" // "Top Operator"
  }
  ```

- `RecruitResult`  
  Recruitment recognition result

  ```json
  // Corresponding details field example
  {
    "tags": [
      // All detected tags, currently always 5
      "削弱", // "Debuff"
      "减速", // "Slow"
      "术师干员", // "Caster"
      "辅助干员", // "Supporter"
      "近战位" // "Melee"
    ],
    "level": 4, // Overall rarity
    "result": [
      {
        "tags": ["削弱"], // ["Debuff"]
        "level": 4, // This tag combination's rarity
        "opers": [
          {
            "name": "初雪", // "Pramanix"
            "level": 5 // Operator rarity
          },
          {
            "name": "陨星", // "Meteorite"
            "level": 5
          },
          {
            "name": "槐琥", // "Waai Fu"
            "level": 5
          },
          {
            "name": "夜烟", // "Haze"
            "level": 4
          },
          {
            "name": "流星", // "Meteor"
            "level": 4
          }
        ]
      },
      {
        "tags": ["减速", "术师干员"], // ["Slow", "Caster"]
        "level": 4,
        "opers": [
          {
            "name": "夜魔", // "Nightmare"
            "level": 5
          },
          {
            "name": "格雷伊", // "Greyy"
            "level": 4
          }
        ]
      },
      {
        "tags": ["削弱", "术师干员"], // ["Debuff", "Caster"]
        "level": 4,
        "opers": [
          {
            "name": "夜烟", // "Haze"
            "level": 4
          }
        ]
      }
    ]
  }
  ```

- `RecruitTagsRefreshed`  
  Recruitment tags refreshed

  ```json
  // Corresponding details field example
  {
    "count": 1, // Current slot refresh count
    "refresh_limit": 3 // Current slot refresh limit
  }
  ```

- `RecruitNoPermit`  
  No recruitment permits available

  ```json
  // Corresponding details field example
  {
    "continue": true // Whether to continue refreshing
  }
  ```

- `RecruitTagsSelected`  
  Recruitment tags selected

  ```json
  // Corresponding details field example
  {
    "tags": ["减速", "术师干员"] // ["Slow", "Caster"]
  }
  ```

- `RecruitSlotCompleted`  
  Current recruitment slot task completed

- `RecruitError`  
  Recruitment recognition error

- `EnterFacility`  
  Base entered facility

  ```json
  // Corresponding details field example
  {
    "facility": "Mfg", // Facility name
    "index": 0 // Facility index
  }
  ```

- `NotEnoughStaff`  
  Base available operators insufficient

  ```json
  // Corresponding details field example
  {
    "facility": "Mfg", // Facility name
    "index": 0 // Facility index
  }
  ```

- `ProductOfFacility`  
  Base facility product

  ```json
  // Corresponding details field example
  {
    "product": "Money", // Product name
    "facility": "Mfg", // Facility name
    "index": 0 // Facility index
  }
  ```

- `StageInfo`  
  Auto combat stage information

  ```json
  // Corresponding details field example
  {
      "name": string  // Stage name
  }
  ```

- `StageInfoError`  
  Auto combat stage recognition error

- `PenguinId`  
  Penguin Statistics ID

  ```json
  // Corresponding details field example
  {
      "id": string
  }
  ```

- `Depot`  
  Depot recognition result

  ```json
  // Corresponding details field example
  "done": bool,       // Whether recognition is complete, false means still in progress (data during process)
  "arkplanner": {     // https://penguin-stats.cn/planner
      "object": {
          "items": [
              {
                  "id": "2004",
                  "have": 4,
                  "name": "高级作战记录" // "Advanced Battle Record"
              },
              {
                  "id": "mod_unlock_token",
                  "have": 25,
                  "name": "模组数据块" // "Module Data Block"
              },
              {
                  "id": "2003",
                  "have": 20,
                  "name": "中级作战记录" // "Tactical Battle Record"
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
  // Currently only supports ArkPlanner and Lolicon (Arkntools) formats, may support more websites in the future
  ```

- `OperBox`  
  Operator recognition result

  ```json
  // Corresponding details field example
  "done": bool,       // Whether recognition is complete, false means still in progress (data during process)
  "all_oper": [
      {
          "id": "char_002_amiya",
          "name": "阿米娅", // "Amiya"
          "own": true,
          "rarity": 5
      },
      {
          "id": "char_003_kalts",
          "name": "凯尔希", // "Kal'tsit"
          "own": true,
          "rarity": 6
      },
      {
          "id": "char_1020_reed2",
          "name": "焰影苇草", // "Reed the Flame Shadow"
          "own": false,
          "rarity": 6
      },
  ]
  "own_opers": [
      {
          "id": "char_002_amiya", // Operator ID
          "name": "阿米娅", // "Amiya" - Operator name
          "own": true, // Whether owned
          "elite": 2, // Elite level 0,1,2
          "level": 50, // Operator level
          "potential": 6, // Operator potential [1, 6]
          "rarity": 5 // Operator rarity [1, 6]
      },
      {
          "id": "char_003_kalts",
          "name": "凯尔希", // "Kal'tsit"
          "own": true,
          "elite": 2,
          "level": 50,
          "potential": 1,
          "rarity": 6
      }
  ]
  ```

- `UnsupportedLevel`  
  Auto combat, unsupported stage name
