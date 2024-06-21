---
order: 2
icon: material-symbols:u-turn-left
---

# Callback Schema

**Callback messages are being iterated rapidly with version updates, and this document may be outdated. For the latest content, please refer to [C# Integration Source Code](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/MaaWpfGui/Main/AsstProxy.cs)**

## Prototype

```cpp
typedef void(ASST_CALL* AsstCallback)(int msg, const char* details, void* custom_arg);
```

## Overview

- `int msg`  
    The message type

    ```cpp
    enum class AsstMsg
    {
        /* Global Info */
        InternalError = 0,          // Internal error
        InitFailed,                 // Initialization failure
        ConnectionInfo,             // Connection info
        AllTasksCompleted,          // Whether all tasks have been completed
        AsyncCallInfo,              // Async Call Info
        /* TaskChain Info */
        TaskChainError = 10000,     // Errors in task chain execution/recognition
        TaskChainStart,             // Task chain starts
        TaskChainCompleted,         // Task chain completes
        TaskChainExtraInfo,         // Extra information about task chain
        TaskChainStopped,           // Task chain stopped
        /* SubTask Info */
        SubTaskError = 20000,       // Errors in subtask execution/recognition
        SubTaskStart,               // Subtask starts
        SubTaskCompleted,           // Subtask completes
        SubTaskExtraInfo,           // Extra information about subtask
        SubTaskStopped,             // Subtask stopped
    };
    ```

- `const char* details`  
    Message details, JSON. See also: [Field Description](#field-description)
- `void* custom_arg`  
    Custom arguments of the caller, will pass the `custom_arg` argument of `AsstCreateEx` interface. C-like languages can pass the `this` pointer with it.

## Field Description

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
    "what": string,     // Info type
    "why": string,      // Info reason
    "uuid": string,     // UUID (empty when connection fails)
    "details": {
        "adb": string,     // adb_path argument of AsstConnect interface
        "address": string, // address argument of AsstConnect interface
        "config": string   // config argument of AsstConnect interface
    }
}
```

### Commonly-used `What` Field Values

- `ConnectFailed`  
    Connection failed.
- `Connected`  
    Connected. Note that the `uuid` field is empty now (and will be retrieved in the next step)
- `UuidGot`  
    UUID has been retrieved.
- `UnsupportedResolution`  
    The resolution is not supported.
- `ResolutionError`  
    Cannot get the resolution.
- `Reconnecting`  
    Disconnected (adb/emulator crashed), and reconnecting
- `Reconnected`  
    Disconnected (adb/emulator crashed), and reconnected successfully
- `Disconnect`  
    Disconnected (adb/emulator crashed), and failed to reconnect
- `ScreencapFailed`  
    Screencap Failed (adb/emulator crashed), and failed to reconnect
- `TouchModeNotAvailable`  
    Touch Mode is not available

### AsyncCallInfo

```json
{
    "uuid": string,             // UUID
    "what": string,             // Call type, "Connect" | "Click" | "Screencap" | ...
    "async_call_id": int,       // Async call id, is the return value when the AsstAsyncXXX API is called
    "details": {
        "ret": bool,            // Result
        "cost": int64,          // Time spent in ms
    }
}
```

### AllTasksCompleted

```json
{
    "taskchain": string,            // Last task chain
    "uuid": string,                 // UUID
    "finished_tasks": [             // ID of the last task run
        int,
        ...
    ]
}
```

#### Commonly-used `taskchain` Field Values

- `StartUp`  
    Start-up.
- `CloseDown`  
    Close Game Client
- `Fight`  
    Fighting.
- `Mall`  
    Mall.
- `Recruit`  
    Auto-recruitment.
- `Infrast`  
    Infrastructure.
- `Award`  
    Get Daily Rewards
- `Roguelike`  
    Roguelike.
- `Copilot`  
    Auto-Combat by json file
- `SSSCopilot`  
    Auto-Combat by json file for STATIONARY SECURITY SERVICE
- `Depot`  
    Depot Recognition
- `OperBox`  
    Operator Box Recognition
- `ReclamationAlgorithm`  
    Reclamation Algorithm ( A new mode in CN client)
- `Custom`  
    Custom Task
- `SingleStep`  
    Single Step Task
- `VideoRecognition`  
    Video Recognition for Copilot
- `Debug`  
    Debug.

### Information Related to TaskChain

```json
{
    "taskchain": string,            // Current task chain
    "taskid": int,                  // Current task ID
    "uuid": string                  // UUID
}
```

### TaskChainExtraInfo

Todo

### Information Related to SubTask

```json
{
    "subtask": string,             // Subtask name
    "class": string,               // Subtask class
    "taskchain": string,           // Current task chain
    "taskid": int,                 // Current task ID
    "details": object,             // Details
    "uuid": string                 // UUID
}
```

#### Commonly-used `subtask` Field Values

- `ProcessTask`  

    ```json
    // Example of corresponding details field
    {
        "task": "StartButton2",     // Task name
        "action": 512,
        "exec_times": 1,            // Execution times
        "max_times": 999,           // Maximum execution times
        "algorithm": 0
    }
    ```

- Todo Other

##### Commonly-used `task` Field Values

- `StartButton2`  
    Starting.
- `MedicineConfirm`  
    Confirmation of using Sanity Potion.
- `ExpiringMedicineConfirm`  
    Confirmation of using Expiring Sanity Potion.
- `StoneConfirm`  
    Confirmation of using Originite Prime.
- `RecruitRefreshConfirm`  
    Confirmation of refreshing recruitment list.
- `RecruitConfirm`  
    Confirmation of recruitment.
- `RecruitNowConfirm`  
    Confirmation of expedited recruitment.
- `ReportToPenguinStats`  
    Reporting to Penguin Stats.
- `ReportToYituliu`  
    Reporting to Yituliu Big Data.
- `InfrastDormDoubleConfirmButton`  
    Double confirmation at infrastructure, only happens when there is a conflict with other operators.
- `StartExplore`  
    Integrated Strategy: starting.
- `StageTraderInvestConfirm`  
    Integrated Strategy: trading items with Originium Ingot.
- `StageTraderInvestSystemFull`  
    Integrated Strategy: trading system full.
- `ExitThenAbandon`  
    Integrated Strategy: exit confirmation.
- `MissionCompletedFlag`  
    Integrated Strategy: mission completed.
- `MissionFailedFlag`  
    Integrated Strategy: mission failed.
- `StageTraderEnter`  
    Integrated Strategy: Cunning Merchant
- `StageSafeHouseEnter`  
    Integrated Strategy: Safehouse
- `StageEncounterEnter`  
    Integrated Strategy: Chance Meeting
- `StageCombatDpsEnter`  
    Integrated Strategy: Operation
- `StageEmergencyDps`  
    Integrated Strategy: Emergency Operation
- `StageDreadfulFoe`  
    Integrated Strategy: Dreadful Foe
- `StartGameTask`
    Failed to launch client (incompatible config file with client_type)
- Todo Other

### SubTaskExtraInfo

```json
{
    "taskchain": string,           // Current task chain
    "class": string,               // Subtask class
    "what": string,                // Information type
    "details": object,             // Information details
    "uuid": string,                // UUID
}
```

#### Commonly-used `what` And `details` Field Values

- `StageDrops`  
    Stage drop information

    ```json
    // Example of corresponding details field
    {
        "drops": [              // dropped items
            {
                "itemId": "3301",
                "quantity": 2,
                "itemName": "技巧概要·卷1" // Skill Summary - 1
            },
            {
                "itemId": "3302",
                "quantity": 1,
                "itemName": "技巧概要·卷2" // Skill Summary - 2
            },
            {
                "itemId": "3303",
                "quantity": 2,
                "itemName": "技巧概要·卷3" // Skill Summary - 3
            }
        ],
        "stage": {              // 关卡信息
            "stageCode": "CA-5",
            "stageId": "wk_fly_5"
        },
        "stars": 3,             // Stage clear ★
        "stats": [              // Statistics of drops
            {
                "itemId": "3301",
                "itemName": "技巧概要·卷1", // Skill Summary - 1
                "quantity": 4
            },
            {
                "itemId": "3302",
                "itemName": "技巧概要·卷2", // Skill Summary - 2
                "quantity": 3
            },
            {
                "itemId": "3303",
                "itemName": "技巧概要·卷3", // Skill Summary - 3
                "quantity": 4
            }
        ]
    }
    ```

- `RecruitTagsDetected`  
    Recruitment tags detected

    ```json
    // Example of corresponding details field
    {
        "tags": [
            "费用回复", // DP-Recovery
            "防护", // Defense
            "先锋干员", // Vanguard
            "辅助干员", // Support
            "近战位" // Melee
        ]
    }
    ```

- `RecruitSpecialTag`  
    Special recruitment tags detected

    ```json
    // Example of corresponding details field
    {
        "tag": "高级资深干员" // Senior operator
    }
    ```

- `RecruitResult`  
    Recruitment result

    ```json
    // Example of corresponding details field
    {
        "tags": [                   // All tags, Must be 5
            "削弱",
            "减速",
            "术师干员",
            "辅助干员",
            "近战位"
        ],
        "level": 4,                 // ★ in total
        "result": [
            {
                "tags": [
                    "削弱" // Debuff
                ],
                "level": 4,         // Rarity of these tags
                "opers": [
                    {
                        "name": "初雪", // Pramanix
                        "level": 5  // ★ of the operator
                    },
                    {
                        "name": "陨星", // Meteorite
                        "level": 5
                    },
                    {
                        "name": "槐琥", // Waai Fu
                        "level": 5
                    },
                    {
                        "name": "夜烟", // Haze
                        "level": 4
                    },
                    {
                        "name": "流星", // Meteor
                        "level": 4
                    }
                ]
            },
            {
                "tags": [
                    "减速", // Slow
                    "术师干员" // Caster
                ],
                "level": 4,
                "opers": [
                    {
                        "name": "夜魔", // Nightmare
                        "level": 5
                    },
                    {
                        "name": "格雷伊", // Greyy
                        "level": 4
                    }
                ]
            },
            {
                "tags": [
                    "削弱", // Debuff
                    "术师干员" // Caster
                ],
                "level": 4,
                "opers": [
                    {
                        "name": "夜烟", // Haze
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
        "count": 1,               // Number of times that the slot has been refreshed
        "refresh_limit": 3        // Limits of the number of times of refreshing
    }
    ```

- `RecruitNoPermit`  
    Ran out of recruit permit

    ```json
    // Example of corresponding details field
    {
        "continue": true,   // Whether continue to refresh or not
    }
    ```

- `RecruitTagsSelected`  
    Recruitment tags selected

    ```json
    // Example of corresponding details field
    {
        "tags": [
            "减速", // Slow
            "术师干员" // Caster
        ]
    }
    ```

- `RecruitSlotCompleted`  
    Recruitment slot completed

- `RecruitError`  
    Error when recognizing recruitment

- `EnterFacility`  
    Entering the facility

    ```json
    // Example of corresponding details field
    {
        "facility": "Mfg",  // Facility name
        "index": 0          // Facility ID
    }
    ```

- `NotEnoughStaff`  
    Available operators not enough

    ```json
    // Example of corresponding details field
    {
        "facility": "Mfg",  // Facility name
        "index": 0          // Facility ID
    }
    ```

- `ProductOfFacility`  
    Production of the facility

    ```json
    // Example of corresponding details field
    {
        "product": "Money", // Product
        "facility": "Mfg",  // Facility name
        "index": 0          // Facility ID
    }
    ```

- `StageInfo`  
    Auto-battle stage info

    ```json
    // Example of corresponding details field
    {
        "name": string  // Stage name
    }
    ```

- `StageInfoError`  
    Auto-battle stage info error

- `PenguinId`  
    Penguin ID

    ```json
    // Example of corresponding details field
    {
        "id": string
    }
    ```

- `DepotInfo`  
    Recognition result of depot

    ```json
    // Example of corresponding details field
    // Supports ArkPlanner format only. More formats may be supported in future.
    "arkplanner": {
        "object": {
            "items": [
                {
                    "id": "2004",
                    "have": 4,
                    "name": "高级作战记录" // Strategic Battle Record
                },
                {
                    "id": "mod_unlock_token",
                    "have": 25,
                    "name": "模组数据块" // Module Data Block
                },
                {
                    "id": "2003",
                    "have": 20,
                    "name": "中级作战记录" // Tactical Battle Record
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
    // Only ArkPlanner and Lolicon (Arkntools) formats are supported. More websites will be supported in future.
    ```

- `OperBoxInfo`  
    Recognition result of operator box

    ```json
    // Example of corresponding details field
    "done": bool,       // Whether the recognition has been completed, false means it is still in progress (data during the process)
    "all_oper": [
        {
            "id": "char_002_amiya",
            "name": "阿米娅",
            "own": true
        },
        {
            "id": "char_003_kalts",
            "name": "凯尔希",
            "own": true
        },
        {
            "id": "char_1020_reed2",
            "name": "焰影苇草",
            "own": false
        },
    ]
    "own_opers": [
        {
            "id": "char_002_amiya", // Operator ID
            "name": "阿米娅", // Operator name
            "own": true, // Whether owned
            "elite": 2, // Elite level 0, 1, 2
            "level": 50, // Operator level
            "potential": 6, // Operator potential [1, 6]
            "rarity": 5     // [1, 6]
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
    Unsupported level name
