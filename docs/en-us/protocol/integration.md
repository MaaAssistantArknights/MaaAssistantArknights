---
order: 1
icon: bxs:book
---

# Integration

## API

### `AsstAppendTask`

#### Prototype

```cpp
TaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### Description

Appends a task.

#### Return Value

- `TaskId`  
    The task ID if the task is successfully appended, for the following configuration;
    0 if the task is not successfully appended.

#### Parameter Description

- `AsstHandle handle`  
    Instance handle
- `const char* type`  
    Task type
- `const char* params`  
    Task parameters in JSON

##### List of Task Types

- `StartUp`  
    Start-up

```json
// Corresponding task parameters
{
    "enable": bool,              // Whether to enable this task, optional, true by default
    "client_type": string,       // Client version, optional, empty by default
                                 // Options: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "start_game_enabled": bool   // Whether to launch client automatically, optional, false by default
}
```

- `CloseDown`  
    Close Game Client  

```json
// Corresponding task parameters
{
    "enable": bool,              // Whether to enable this task, optional, true by default
}
```

- `Fight`  
    Operation

```json
// Corresponding task parameters
{
    "enable": bool,             // Whether to enable this task, optional, true by default
    "stage": string,            // Stage name, optional, by default crrent / last stage. Editing in run-time is not supported.
                                // Supports all mainline stages, such as "1-7", "S3-2", etc.
                                // At the end of the level, enter Normal/Hard to switch between Normal and Tough difficulty
                                // Annihilation. The input should be `Annihilation`.
                                // Certain side story stages. The input should be complete with stage number.
    "medicine": int,            // Maximum number Sanity Potion used, optional, by default 0
    "expiring_medicine": int,   // Maximum number of expired Sanity Potion within 48 hours, optional, by default 0
    "stone": int,               // Maximum number of Originite Prime used, optional, by default 0
    "times": int,               // Maximum times, optional, by default infinite
    "series": int,              // Number of series, optional, 1~6
    "drops": {                  // Specifying the number of drops, optional, no specification by default
        "30011": int,           // Key: item ID; value: number of items. Key refers to resource/item_index.json
        "30062": int            // OR combination
    },
    /* Items are combined with OR operators, i.e. the task stops when any condition meets. */

    "report_to_penguin": bool,  // Whether to upload data to Pengiun Stats, optional, by default false
    "penguin_id": string,       // Penguin Stats ID, optional, by default empty. Available only when `report_to_penguin` is `true`.
    "server": string,           // Server, optional, by default "CN", will affect the drop recognition and upload
                                // Options："CN" | "US" | "JP" | "KR"
    "client_type": string,      // Client versino, optional, empty by default. Used to reconnect after the game crashed. Empty means to disable this feature
                                // Options: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "DrGrandet": bool,          // Save sanity by using Originites, Optional, false by default. effective only when Originites may be used
                                // Wait in the using Originites confirmation screen until the 1 point of sanity has been restored and then immediately use the Originite.
}
```

Supports some of the special stages,Please refer to [autoLocalization example](..\..\..\tools\AutoLocalization\example\en-us.xaml#L260).

- `Recruit`  
    Recruitment

```json
// Corresponding task parameters
{
    "enable": bool,             // Whether to enable this task, optional, by default true
    "refresh": bool,            // Whether to refresh 3★ tags, optional, by default false
    "select": [                 // Tag level to click, required
        int,
        ...
    ],
    "confirm": [                // Tag level for confirmation, required. Can be set to empty array for calculation only.
        int,
        ...
    ],
    "first_tags": [             // Preferred Tags, valid only if 3★ tags. Optional, by default empty
        string,                 // When Tag is level-3, as many Tags here as possible (if any) will be selected.
        ...                     // It's a forced selection, i.e. it ignores all "unselect 3★ Tags" settings.
    ],
    "extra_tags_mode": int,     // Select more tags, optional
                                // 0 - default
                                // 1 - click 3 tags anyway, even if they are in conflict
                                // 2 - select more combinations if possible, even if their tags are in conflict
    "times": int,               // The times of recruitment, optional, by default 0. Can be set to 0 for calculation only.
    "set_time": bool,           // Whether to set time to 9 hours, available only when `times` is 0, optional, by default true
    "expedite": bool,           // Whether to use expedited plans, optional, by default false
    "expedite_times": int,      // The times of using expedited plans, available only when `expedite` is `true`
                                // Optional, by default infinity until `times` reaches its limitation.
    "skip_robot": bool,         // Whether to skip when robot tag is recognized, optional, skip by default.
    "recruitment_time": {       // Tags level and set duration in minutes, optional, by default 540 (i.e. 9 hours)
        "3": int,
        "4": int,
        ...
    },
    "report_to_penguin": bool,  // Whether to report to Penguin Stats, optional, by default false.
    "penguin_id": string,       // Penguin Stats user id, optional, by default empty. Valid only if report_to_penguin is true.
    "report_to_yituliu": bool,  // Whether to report to YITULIU, optional, by default false.
    "yituliu_id": string,       // YITULIU user id, optional, by default empty. Valid only if report_to_yituliu is true.
    "server": string,           // Server, optional, by default "CN", will affect upload
}
```

- `Infrast`  
    Infrastructure shifting

```json
{
    "enable": bool,             // Whether to enable this task, optional, by default true
    "mode": int,            // Shift mode, optional. Editing in run-time is not supported.
                            // 0 - By Default, auto shift
                            // 10000 - Custom Mode, please refer 3.6-INFRASTRUCTURE_SCHEDULING_SCHEMA
    "facility": [           // Facilities for shifting, required. Editing in run-time is not supported.
        string,             // Facility name: "Mfg" | "Trade" | "Power" | "Control" | "Reception" | "Office" | "Dorm"
        ...
    ],
    "drones": string,       // Usage of drones, optional, by default "_NotUse"
                            // "_NotUse"、"Money"、"SyntheticJade"、"CombatRecord"、"PureGold"、"OriginStone"、"Chip"
    "threshold": float,     // Morale threshold with range [0, 1.0], optional, by default 0.3
    "replenish": bool,      // Whether to replenish Originium Shard, optional, by default false
    "dorm_notstationed_enabled": bool, // Whether to enbale Not Stationed in Dorm, by default false
    "dorm_trust_enabled": bool, // Whether to fill operators in dorm by trust, by default false

    /* The following is only valid when mode == 10000 */
    "filename": string,     // Custom config json file path
    "plan_index": int,      // custom config plan index
}
```

- `Mall`  
    Collecting Credits and auto-purchasing
    Will buy items in order following `buy_first` list, buy other items from left to right ignoring items in `blacklist`, and buy other items from left to right ignoring the `blacklist` while credit overflows.

```json
// Corresponding task parameters
{
    "enable": bool,         // Whether to enable this task, optional, by default true
    "shopping": bool,       // Whether to buy items from the store, optional, by default false. Editing in run-time is not supported.
    "buy_first": [          // Items to be purchased with priority, optional. Editing in run-time is not supported.
        string,             // Item name, e.g. "招聘许可" (Recruitment Permit), "龙门币" (LMD), etc.
        ...
    ],
    "blacklist": [          // Blacklist, optional. Editing in run-time is not supported.
        string,             // Item name, e.g. "加急许可" (Expedited Plan), "家具零件" (Furniture Part), etc.
        ...
    ],
    "force_shopping_if_credit_full": bool // Whether to ignore the Blacklist if credit overflows, by default true
    "only_buy_discount": bool // Whether to purchase only discounted items, applicable only on the second round of purchases, by default false.
    "reserve_max_credit": boll // Whether to stop purchasing when credit points fall below 300, applicable only on the second round of purchases, by default false.
}
```

- `Award`  
    Collecting daily awards.  

```json
// Corresponding task parameters
{
    "enable": bool          // Whether to enable this task, optional, by default true
}
```

- `Roguelike`  
    Integrated Strategies

```json
{
    "enable": bool,         // Whether to enable this task, optional, by default true
    "theme": string,        // Name of the theme, optional, by default "Phantom"
                            // Phantom - 傀影与猩红血钻
                            // Mizuki  - 水月与深蓝之树
    "mode": int,            // Mode, optional, by default 0
                            // 0 - For candle, plays as much as you can with stable strategy
                            // 1 - For Originium Ingots, exits after first level
                            // 2 - For both, Plays until invests
                            // 3 - For pass, plays as much as you can with aggressive strategy
    "starts_count": int,    // Number of starts, optional, by default INT_MAX
    "investment_enabled": bool, // by default true

    "investments_count": int,
                            // Number of investments, optional, by default INT_MAX
    "stop_when_investment_full": bool,
                            // Stop the task when investment is full, optional, by default false
    "squad": string,        // Squad name like "assault squad", optional, by default "Default Squad"
    "roles": string,        // Roles, optional
    "core_char": string,    // Operator name, optional. Will recognize auto-selection of levels.
    "use_support": bool,    // Whether "core_char" is a support unit, optional, by default false
    "use_nonfriend_support": bool,  // Whether support unit can be non-friend support unit, optional, by default false, valid when use_support=true
    "refresh_trader_with_dice": bool  // Whether refresh trader with dice to buy special item, optional, by default false
}
```

- `Copilot`  
    Copilot auto-combat feature

```json
{
    "enable": bool,             // Whether to enable this task, optional, by default true
    "filename": string,         // Filename and path of the task JSON, supporting absolute/relative paths. Editing in run-time is not supported.
    "formation": bool           // Whether to "quick build", optional, by default false. Editing in run-time is not supported.
}
```

For more details about auto-copilot JSON, please refer to [Copilot Schema](./copilot-schema.md)

- `SSSCopilot`  
    Copilot auto-combat feature for STATIONARY SECURITY SERVICE

```json
{
    "enable": bool,             // Whether to enable this task, optional, by default true
    "filename": string,         // Filename and path of the task JSON, supporting absolute/relative paths. Editing in run-time is not supported.
    "loop_times": int
}
```

For more details about auto-copilot JSON, please refer to [Copilot Schema](./copilot-schema.md)

- `Depot`
    Depot recognition

```json
// Corresponding task parameters
{
    "enable": bool          // Whether to enable this task, optional, by default true
}
```

- `OperBox`  
    Operator box recognition

```json
// Corresponding task parameters
{
    "enable": bool          // Whether to enable this task, optional, by default true
}
```

- `ReclamationAlgorithm`  
    ReclamationAlgorithm ( A new mode in CN client)

```json
{
    "enable": bool,
    "theme": int,           // Theme, optional, 1 by default
                            // 0 - *Fire Within the Sand*
                            // 1 - *Tales Within the Sand*
    "mode": int,            // Mode, optional, 0 by default
                            // 0 - Farm badges & construction pts (exiting the stage immediately)
                            // 1 - Fire Within the Sand: Farm Crude Gold (forging Gold at headquarter after purchasing water)
                            //     Tales Within the Sand: Automatically craft items and load to earn currency
    "product": string       // Automatically crafted items, optional, glow stick by default 
                            // Suggested fill in the substring
}
```

- `Custom`  
    Custom Task

```json
{
    "enable": bool,
    "task_names": [     // Execute the task on the first match in the array (and subsequent next, etc.)
                        // If you want to perform multiple tasks, you can append Custom task multiple times
        string,
        ...
    ]
}
```

- `SingleStep`  
    Single-step task (currently only supports copilot)

```json
{
    "enable": bool,
    "type": string,     // currently only supports "copilot"
    "subtask": string,  // "stage" | "start" | "action"
                        // "stage" to set stage name, eg: "details": { "stage": "xxxx" }
                        // "start" to start mission, without details
                        // "action": single battle action, details is single action in Copilot,
                        //           eg: "details": { "name": "史尔特尔", "location": [ 4, 5 ], "direction": "左" }，详情参考 3.3-战斗流程协议.md
    "details": {
        ...
    }
}
```

- `VideoRecognition`  
    Video recognition, currently only supports operation (combat) video

```json
{
    "enable": bool,
    "filename": string,
}
```

### `AsstSetTaskParams`

#### Prototype

```cpp
bool ASSTAPI AsstSetTaskParams(AsstHandle handle, TaskId id, const char* params);
```

#### Description

Set task parameters

#### Return Value

- `bool`  
    Whether the parameters are successfully set.

#### Parameter Description

- `AsstHandle handle`  
    Instance handle
- `TaskId task`  
    Task ID, the return value of `AsstAppendTask`
- `const char* params`  
    Task parameter in JSON, same as `AsstAppendTask`
    For those fields that do not mention "Editing in run-time is not supported" can be changed during run-time. Otherwise these changes will be ignored when the task is running.

### `AsstSetStaticOption`

#### Prototype

```cpp
bool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### Description

Set process-level parameters

#### Return Value

- `bool`  
    Is the setup successful

#### Parameter Description

- `AsstStaticOptionKey key`  
    key
- `const char* value`  
    value

##### List of Key and value

None

### `AsstSetInstanceOption`

#### Prototype

```cpp
bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### Description

Set instance-level parameters

#### Return Value

- `bool`  
    Is the setup successful

#### Parameter Description

- `AsstHandle handle`  
    handle
- `AsstInstanceOptionKey key`  
    key
- `const char* value`  
    value

##### List of Key and value

```cpp
    enum InstanceOptionKey
    {
        Invalid = 0,
        // Deprecated // MinitouchEnabled = 1,   // Is minitouch enabled
                                // If you can't use minitouch, it's useless to turn it on.
                                // "1" - on, "0" - off
        TouchMode = 2,          // Touch mode, minitouch by default
                                // minitouch | maatouch | adb
        DeploymentWithPause = 3,    // Deployment with Pause (Works for IS, Copilot and 保全派驻)
                                    // "1" | "0"
        AdbLiteEnabled = 4,     // Enable AdbLite or not, "0" | "1"
        KillAdbOnExit = 5,       // Release Adb on exit, "0" | "1"
    };
```
