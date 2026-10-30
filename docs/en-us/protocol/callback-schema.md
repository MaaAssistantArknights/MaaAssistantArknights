---
order: 2
icon: material-symbols:u-turn-left
---

# Callback Message Protocol

::: info Note  
Callback messages are rapidly evolving with each version update, so this document may become outdated. For the latest information, please refer to the [C# Integration Source Code](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/src/MaaWpfGui/Main/AsstProxy.cs)  
:::

## Callback Function Prototype

```cpp
typedef void(ASST_CALL* AsstApiCallback)(AsstMsgId msg, const char* details_json, void* custom_arg);
```

Here `AsstMsgId` is an alias of `int32_t`.

## Parameter Overview

- `AsstMsgId msg`  
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

      /* Web Request */
      ReportRequest     = 30000,       // Report request
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
- `MuMuExtrasInputStatus`  
  The actual status of MuMu touch enhancement, `details` structure:
  - `available` (boolean, required): Whether it has taken effect.
  - `deferred` (boolean, required): Whether the status is not determined yet (the game was not rendering when connected; it will be rechecked automatically once rendering starts).
- `ResolutionGot`  
  Resolution retrieved
- `ResolutionChanged`  
  Resolution changed while running; the connection is invalidated and the current task is interrupted, `details` structure:
  - `width` (number, required): The current width.
  - `height` (number, required): The current height.
- `FastestWayToScreencap`  
  Fastest screenshot method found, `details` structure:
  - `method` (string, required): Fastest screenshot method.
  - `cost` (number, required): Time cost in milliseconds.
  - `alternatives` (array`<object>`, required): All candidate methods and their costs.
- `ScreencapCost`  
  Screenshot cost statistics (reported every 10 screenshots), `details` structure:
  - `min` (number, required): Minimum cost in milliseconds.
  - `max` (number, required): Maximum cost in milliseconds.
  - `avg` (number, required): Average cost in milliseconds.
  - `fault_times` (number): Number of failures (only present when there are failures).

- `EmulatorFPS`  
  Emulator refresh rate (checked every 1 minute), `details` structure:
  - `fps` (number, required): Emulator/system refresh rate (FPS).
  - `refresh_period_ns` (number, required): Frame refresh period in nanoseconds.

### AsyncCallInfo

```json
{
    "uuid": string,             // Device unique ID
    "what": string,             // Callback type, "Connect" | "AttachWindow" | "Click" | "Screencap" | ...
    "async_call_id": int,       // Asynchronous request ID, i.e., return value when calling AsstAsyncXXX
    "details": {
        "ret": bool,            // Actual call return value
        "cost": int64,          // Time cost, in milliseconds
        "error": string,        // Unhandled exception type, only present when the call failed due to an unhandled exception
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
- `ParadoxCopilot`  
  Auto Paradox Simulation
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

`TaskChainError` / `TaskChainStart` / `TaskChainCompleted` / `TaskChainExtraInfo` / `TaskChainStopped` share the following fields:

```json
{
    "taskchain": string,            // Current task chain
    "taskid": int,                  // Current task TaskId
    "uuid": string                  // Device unique ID
}
```

When a task chain is interrupted by an unhandled exception, `TaskChainError` additionally carries a `details` field:

- `error` (string, required): The exception type, one of `OpenCVException` | `OutOfMemory` | `GpuDeviceRemoved` | `UnhandledException` | `UnknownException`. When `OutOfMemory` or `GpuDeviceRemoved` (GPU inference device removed, e.g. due to a graphics driver update or GPU reset) occurs, the remaining tasks are cleared, and the program must be restarted to continue.

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
    "action": "ClickSelf", // Action name, e.g. "ClickSelf" | "DoNothing" | "Swipe"
    "exec_times": 1, // Execution times
    "max_times": 999, // Maximum execution times
    "algorithm": "MatchTemplate", // Recognition algorithm name, e.g. "MatchTemplate" | "OcrDetect" | "FeatureMatch" | "JustReturn"
    "result": {} // Recognition result of this run, structure varies by algorithm; empty object when there is none
  }
  ```

  Additionally, when the task execution count reaches its limit, a `SubTaskExtraInfo` message with `what` set to `ExceededLimit` is reported first; its `details` contains `task` (task name), `exec_times` (execution times), and `max_times` (maximum execution times).

  If the list of tasks to be recognized contains undefined task names (e.g. task resources not matching the program version), a `SubTaskError` with `what` set to `UnknownTask` is reported back before recognition starts and the whole task chain is stopped; its `details` contains `tasks` (string array, the list of undefined task names).

- `ReportToPenguinStats` / `ReportToYituliu`  
  Report battle drops to Penguin Statistics / Yituliu big data (a `SubTaskError` is reported back when the upload fails)
- `StartGameTask`  
  Failed to open client (config file does not match passed client_type)
- Todo others

##### Common `task` Field Values

- `StartButton2`  
  Start battle
- `MedicineConfirm`  
  Use sanity potion
- `StoneConfirm`  
  Use Originium Prime
- `RecruitRefreshConfirm`  
  Refresh recruitment tags
- `RecruitConfirm`  
  Confirm recruitment
- `RecruitNowConfirm`  
  Use Expedited Plan
- `InfrastDormDoubleConfirmButton`  
  Base double confirmation button, appears only when the operator to be assigned is already stationed in another facility (MAA will click confirm automatically), please notify users
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
- `StageCombatOpsEnter`  
  Integrated Strategy stage: Combat
- `StageEmergencyOps`  
  Integrated Strategy stage: Emergency Combat
- `StageDreadfulFoe`  
  Integrated Strategy stage: Dreadful Foe
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
    ],
    "cur_times": 2, // Number of consecutive battles recognized on the settlement screen (only present when recognized)
    "annihilation_weekly_process": [1600, 1800] // Weekly acquisition progress of Annihilation mode, format: [completed count, limit] (only present on Annihilation stages)
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

- `RecruitPreservedTag`  
  Recruitment preserved tag detected

  ```json
  // Corresponding details field example
  {
    "tag": "支援机械" // "Robot"
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

- `RecruitPermitReserved`
  The current recruitment permit count is at or below the reserve configured for 3★ recruitment, so the current slot is skipped.

  ```json
  // Corresponding details field example
  {
    "current": 8
  }
  ```

- `RecruitPermitCountRecognitionFailed`
  Recruitment permit count recognition failed while the 3★ permit reserve was enabled, so the current slot is skipped. The `details` field is empty.

- `RecruitTagsSelected`  
  Recruitment tags selected

  ```json
  // Corresponding details field example
  {
    "tags": ["减速", "术师干员"] // ["Slow", "Caster"]
  }
  ```

- `RecruitError`  
  Recruitment recognition error. The `details` field is empty; if triggered because the current slot reached its refresh limit, it contains `refresh_limit` (the slot refresh limit).

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

- `CustomInfrastRoomKeptUnchanged`  
  The room is kept unchanged during custom infrastructure scheduling. The `details` field always includes `facility` (facility name) and `index` (facility index), plus:

  ```json
  // Corresponding details field example
  {
    "reason": "candidates_working" // Reason for keeping the room unchanged, see below
  }
  ```

  `reason` values:
  - `facility_empty`: the custom rooms array is empty.
  - `room_no_intent`: the room is configured but has no operator selection intent.
  - `candidates_unavailable`: none of the candidate operators is available, usually a misconfigured list (typo or unowned operator).
  - `candidates_working`: all candidate operators are working in other facilities.
  - `candidates_low_mood`: all candidate operators are below the mood threshold.

- `CustomInfrastRoomsNotConfigured`  
  The facility is not covered by the custom rooms array and is kept unchanged. The `details` field contains only the default `facility` (facility name) and `index` (facility index).

- `CustomInfrastTraineeNotFound`  
  None of the training room candidate operators is matched. The `details` field always includes `facility` (facility name) and `index` (facility index), plus:

  ```json
  // Corresponding details field example
  {
    "candidates": ["艾雅法拉", "焰影苇草"] // List of candidate operators for the training room
  }
  ```

- `CustomInfrastAssistantNotFound`  
  None of the assistant candidate operators is matched. The `details` field always includes `facility` (facility name) and `index` (facility index), plus:

  ```json
  // Corresponding details field example
  {
    "candidates": ["凯尔希"],        // List of candidate operators for the assistant
    "working_excluded": ["琴柳"],    // Optional; candidates excluded because working in other facilities, only present when non-empty
    "low_mood_excluded": ["歌蕾蒂娅"] // Optional; candidates excluded because their mood is below the threshold, only present when non-empty
  }
  ```

- `CustomInfrastRoomGroupsMatchFailed`  
  Operator group matching failed during custom infrastructure scheduling. The `details` field always includes `facility` (facility name) and `index` (facility index), plus:

  ```json
  // Corresponding details field example
  {
    "reason": "all_unavailable", // Reason for the failure, see below
    "groups": ["古+銀"] // Optional; undefined or unavailable group names, only present with `group_not_defined` and `all_unavailable`
  }
  ```

  `reason` values:
  - `group_not_defined`: none of the referenced group names is defined in groups.
  - `all_unavailable`: operators of all groups are unavailable.
  - `recognition_failed`: the operator list recognition failed.

- `CopilotPrecheckFile`  
  Reported before pre-matching each copilot file when "Assist formation with Yituliu data" is enabled in multi-file copilot mode. The `details` field contains:

  ```json
  // Corresponding details field example
  {
    "nav_name": "CV-EX-1", // Stage name
    "filename": "作业.json" // Copilot file name
  }
  ```

- `StageInfo`  
  Auto combat stage information

  ```json
  // Corresponding details field example
  {
      "name": string,  // Stage name
      "size": number   // Number of map grid cells
  }
  ```

- `StageInfoError`  
  Auto combat stage recognition error

- `DepotInfo`  
  Depot recognition result

  ```json
  // Corresponding details field example
  "done": bool,   // Whether recognition is complete, false means still in progress (data during process)
  "data": "{\"2001\":18000,\"31043\":317}"  // JSON string, format: {"itemId": quantity, ...}
  ```

- `OperBoxInfo`  
  Operator recognition result

  ```json
  // Corresponding details field example
  "done": bool,       // Whether recognition is complete, false means still in progress (data during process)
  "all_opers": [
      {
          "id": "char_002_amiya",
          "name": "阿米娅", // "Amiya"
          "name_en": "Amiya", // Operator English name
          "name_jp": "アーミヤ", // Operator Japanese name
          "name_kr": "아미야", // Operator Korean name
          "name_tw": "阿米婭", // Operator Traditional Chinese name
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

- `BlackFlowStartRun`  
  Progress of BlackFlow starting reward farming (roguelike mode 30003). (machine-translated)

  ```json
  // Corresponding details field example
  {
      "status": "reward_missed",       // Current progress, see below
      "rewards": ["空间租赁", "金乌"]   // Optional; reward names recognized on the target run's support selection page
  }
  ```

  `status` values:
  - `preparing`: the preparation run started.
  - `target_run`: the target run started.
  - `reward_missed`: rewards were recognized on the support selection page but none is desired; the run is abandoned and a new preparation run starts.
  - `reward_picked`: with `blackflow_start_utopia` on, a desired reward was found on the support selection page; a reward was picked by `blackflow_start_reward_priority` and the run enters floor 1 to check for 实托邦.
  - `completed`: a desired reward was found; the task exited to the theme page keeping the run, and the task ends. With `blackflow_start_utopia` on, this means floor 1 is 实托邦.
  - `reward_unrecognized`: rewards on the support selection page could not be recognized; the task stops on that page.
  - `reward_page_missing`: the target run did not show the support selection page; the task stops.
  - `reward_recognition_failed`: the preparation run or the target run failed to pick a reward on the support selection page; the task stops.

  `rewards` is only reported with `reward_missed`, `reward_picked`, `completed`, `reward_unrecognized` and `reward_recognition_failed`, and is omitted when `blackflow_start_rewards` is 0 with `blackflow_start_utopia` off (rewards are not recognized) or when no reward was recognized. Reward names are reported in Simplified Chinese.

- `BlackFlowScrapTradeAborted`  
  The BlackFlow scrap trade is aborted due to an anomaly. The `details` field contains:

  ```json
  // Corresponding details field example
  {
    "reason": "sale_failed", // Reason for aborting the trade
    "item": "晶体元件" // Name of the item that triggered the abort
  }
  ```

### ReportRequest

This field is mainly used by the core module to pass network request information to the UI layer, which is responsible for performing the actual HTTP reporting operations.

```json
{
  "url": "string",            // The full request URL, e.g., https://penguin-stats.io/PenguinStats/api/v2/report
  "headers": {                // Request headers key-value pairs (excluding Content-Type, which is added by the UI)
    "authorization": "PenguinID 1234567890",
    "User-Agent": "MaaAssistantArknights/1.2.3 cpr/4.5.6"
  },
  "body": "string",           // Request body content (usually a JSON-formatted string)
  "subtask": "string"         // Subtask name identifying the specific reporting task, e.g., "ReportToPenguinStats", "ReportToYituliu"
}
```
