---
order: 2
icon: material-symbols:u-turn-left
---

# Callback Schema

::: info Note
Callback messages are rapidly evolving with version updates, and this document may be outdated. For the latest information, please refer to the [C# Integration Source Code](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/MaaWpfGui/Main/AsstProxy.cs)
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
      Destroyed         = 5,           // Instance has been destroyed

      /* TaskChain Info */
      TaskChainError     = 10000,      // TaskChain execution/recognition error
      TaskChainStart     = 10001,      // TaskChain started
      TaskChainCompleted = 10002,      // TaskChain completed
      TaskChainExtraInfo = 10003,      // TaskChain extra information
      TaskChainStopped   = 10004,      // TaskChain manually stopped

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
  Custom parameter from caller, passed directly from the `custom_arg` parameter of the `AsstCreateEx` interface, C-like languages can use this parameter to pass a `this` pointer

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

### Common `What` Fields

- `ConnectFailed`  
  Connection failed
- `Connected`  
  Connected, note that the `uuid` field is empty at this point (it will be obtained in the next step)
- `UuidGot`  
  Device unique ID has been obtained
- `UnsupportedResolution`  
  Resolution not supported
- `ResolutionError`  
  Error getting resolution
- `Reconnecting`  
  Connection lost (adb/emulator crashed), attempting to reconnect
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
    "async_call_id": int,       // Asynchronous request ID, the return value of AsstAsyncXXX
    "details": {
        "ret": bool,            // Actual return value of the call
        "cost": int64,          // Time cost in milliseconds
    }
}
```

### AllTasksCompleted

```json
{
    "taskchain": string,            // Last task chain
    "uuid": string,                 // Device unique ID
    "finished_tasks": [             // Task IDs that have been executed
        int,
        ...
    ]
}
```

#### Common `taskchain` Fields

- `StartUp`  
  Start wakeup
- `CloseDown`  
  Close game
- `Fight`  
  Farm sanity
- `Mall`  
  Credit store shopping
- `Recruit`  
  Auto recruitment
- `Infrast`  
  Base shift change
- `Award`  
  Collect daily rewards
- `Roguelike`  
  Integrated Strategies farming
- `Copilot`  
  Auto stage copy
- `SSSCopilot`  
  Auto Contingency Contract copy
- `Depot`  
  Inventory recognition
- `OperBox`  
  Operator box recognition
- `ReclamationAlgorithm`  
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
    "taskid": int,                  // Current task ID
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
    "taskid": int,                 // Current task ID
    "details": object,             // Details
    "uuid": string                 // Device unique ID
}
```

#### Common `subtask` Fields

- `ProcessTask`  

  ```json
  // Example of corresponding details field
  {
    "task": "StartButton2", // Task name
    "action": 512,
    "exec_times": 1, // Times already executed
    "max_times": 999, // Maximum execution times
    "algorithm": 0
  }
  ```

- Todo others

##### Common `task` Fields

- `StartButton2`  
  Start battle
- `MedicineConfirm`  
  Use sanity potion
- `ExpiringMedicineConfirm`  
  Use sanity potion that expires within 48 hours
- `StoneConfirm`  
  Use Originite Prime
- `RecruitRefreshConfirm`  
  Refresh recruitment tags
- `RecruitConfirm`  
  Confirm recruitment
- `RecruitNowConfirm`  
  Use expedited plan
- `ReportToPenguinStats`  
  Report to Penguin Stats
- `ReportToYituliu`  
  Report to Yituliu Database
- `InfrastDormDoubleConfirmButton`  
  Base dormitory double confirmation button, only appears when operators conflict, please notify the user
- `StartExplore`  
  Start Integrated Strategies exploration
- `StageTraderInvestConfirm`  
  Invested Originium Ingots in Integrated Strategies
- `StageTraderInvestSystemFull`  
  Investment reached game limit in Integrated Strategies
- `ExitThenAbandon`  
  Abandoned current Integrated Strategies exploration
- `MissionCompletedFlag`  
  Integrated Strategies combat completed
- `MissionFailedFlag`  
  Integrated Strategies combat failed
- `StageTraderEnter`  
  Integrated Strategies stage: Encounter Trader
- `StageSafeHouseEnter`  
  Integrated Strategies stage: Safe House
- `StageEncounterEnter`  
  Integrated Strategies stage: Encounter/Treasure
- `StageCombatDpsEnter`  
  Integrated Strategies stage: Normal Combat
- `StageEmergencyDps`  
  Integrated Strategies stage: Emergency Combat
- `StageDreadfulFoe`  
  Integrated Strategies stage: Dangerous Enemy
- `StartGameTask`
  Failed to open client (config file doesn't match provided client_type)
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

#### Common `what` and `details` Fields

- `StageDrops`  
  Stage drop information

  ```json
  // Example of corresponding details field
  {
    "drops": [
      // Materials dropped this time
      {
        "itemId": "3301",
        "quantity": 2,
        "itemName": "Skill Summary - 1"
      },
      {
        "itemId": "3302",
        "quantity": 1,
        "itemName": "Skill Summary - 2"
      },
      {
        "itemId": "3303",
        "quantity": 2,
        "itemName": "Skill Summary - 3"
      }
    ],
    "stage": {
      // Stage information
      "stageCode": "CA-5",
      "stageId": "wk_fly_5"
    },
    "stars": 3, // Operation end stars
    "stats": [
      // Total material drops during this execution
      {
        "itemId": "3301",
        "itemName": "Skill Summary - 1",
        "quantity": 4,
        "addQuantity": 2 // Amount newly added in this drop
      },
      {
        "itemId": "3302",
        "itemName": "Skill Summary - 2",
        "quantity": 3,
        "addQuantity": 1
      },
      {
        "itemId": "3303",
        "itemName": "Skill Summary - 3",
        "quantity": 4,
        "addQuantity": 2
      }
    ]
  }
  ```

- `RecruitTagsDetected`  
  Recruitment tags detected

  ```json
  // Example of corresponding details field
  {
    "tags": ["DP-Recovery", "Defense", "Vanguard", "Supporter", "Melee"]
  }
  ```

- `RecruitSpecialTag`  
  Special recruitment tag detected

  ```json
  // Example of corresponding details field
  {
    "tag": "Top Operator"
  }
  ```

- `RecruitResult`  
  Recruitment recognition result

  ```json
  // Example of corresponding details field
  {
    "tags": [
      // All detected tags, currently always 5
      "Debuff",
      "Slow",
      "Caster",
      "Supporter",
      "Melee"
    ],
    "level": 4, // Overall rarity
    "result": [
      {
        "tags": ["Debuff"],
        "level": 4, // Rarity of this tag combination
        "opers": [
          {
            "name": "Pramanix",
            "level": 5 // Operator rarity
          },
          {
            "name": "Meteorite",
            "level": 5
          },
          {
            "name": "Waai Fu",
            "level": 5
          },
          {
            "name": "Haze",
            "level": 4
          },
          {
            "name": "Meteor",
            "level": 4
          }
        ]
      },
      {
        "tags": ["Slow", "Caster"],
        "level": 4,
        "opers": [
          {
            "name": "Nightmare",
            "level": 5
          },
          {
            "name": "Greyy",
            "level": 4
          }
        ]
      },
      {
        "tags": ["Debuff", "Caster"],
        "level": 4,
        "opers": [
          {
            "name": "Haze",
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
  // Example of corresponding details field
  {
    "count": 1, // Current slot refresh count
    "refresh_limit": 3 // Current slot refresh count limit
  }
  ```

- `RecruitNoPermit`  
  No recruitment permits

  ```json
  // Example of corresponding details field
  {
    "continue": true // Whether to continue refreshing
  }
  ```

- `RecruitTagsSelected`  
  Recruitment tags selected

  ```json
  // Example of corresponding details field
  {
    "tags": ["Slow", "Caster"]
  }
  ```

- `RecruitSlotCompleted`  
  Current recruitment slot task completed

- `RecruitError`  
  Recruitment recognition error

- `EnterFacility`  
  Entered a facility in the base

  ```json
  // Example of corresponding details field
  {
    "facility": "Mfg", // Facility name
    "index": 0 // Facility index
  }
  ```

- `NotEnoughStaff`  
  Not enough available operators in base

  ```json
  // Example of corresponding details field
  {
    "facility": "Mfg", // Facility name
    "index": 0 // Facility index
  }
  ```

- `ProductOfFacility`  
  Base product

  ```json
  // Example of corresponding details field
  {
    "product": "Money", // Product name
    "facility": "Mfg", // Facility name
    "index": 0 // Facility index
  }
  ```

- `StageInfo`  
  Auto battle stage information

  ```json
  // Example of corresponding details field
  {
      "name": string  // Stage name
  }
  ```

- `StageInfoError`  
  Auto battle stage recognition error

- `PenguinId`  
  Penguin Logistics ID

  ```json
  // Example of corresponding details field
  {
      "id": string
  }
  ```

- `Depot`  
  Inventory recognition result

  ```json
  // Example of corresponding details field
  "done": bool,       // Whether recognition is complete, false means still in progress (data during the process)
  "arkplanner": {     // https://penguin-stats.cn/planner
      "object": {
          "items": [
              {
                  "id": "2004",
                  "have": 4,
                  "name": "Advanced Battle Record"
              },
              {
                  "id": "mod_unlock_token",
                  "have": 25,
                  "name": "Module Data Block"
              },
              {
                  "id": "2003",
                  "have": 20,
                  "name": "Frontline Battle Record"
              }
          ],
          "@type": "@penguin-statistics/depot"
      },
      "data": "{\"@type\":\"@penguin-statistics/depot\",\"items\":[{\"id\":\"2004\",\"have\":4,\"name\":\"Advanced Battle Record\"},{\"id\":\"mod_unlock_token\",\"have\":25,\"name\":\"Module Data Block\"},{\"id\":\"2003\",\"have\":20,\"name\":\"Frontline Battle Record\"}]}"
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
  // Example of corresponding details field
  "done": bool,       // Whether recognition is complete, false means still in progress (data during the process)
  "all_oper": [
      {
          "id": "char_002_amiya",
          "name": "Amiya",
          "own": true,
          "rarity": 5
      },
      {
          "id": "char_003_kalts",
          "name": "Kal'tsit",
          "own": true,
          "rarity": 6
      },
      {
          "id": "char_1020_reed2",
          "name": "Reed the Flame Shadow",
          "own": false,
          "rarity": 6
      },
  ]
  "own_opers": [
      {
          "id": "char_002_amiya", // Operator ID
          "name": "Amiya", // Operator name
          "own": true, // Whether owned
          "elite": 2, // Elite level 0,1,2
          "level": 50, // Operator level
          "potential": 6, // Operator potential [1, 6]
          "rarity": 5 // Operator rarity [1, 6]
      },
      {
          "id": "char_003_kalts",
          "name": "Kal'tsit",
          "own": true,
          "elite": 2,
          "level": 50,
          "potential": 1,
          "rarity": 6
      }
  ]
  ```

- `UnsupportedLevel`  
  Auto stage copy, unsupported stage name
