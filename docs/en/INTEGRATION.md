# Integration

## API

### `AsstAppendTask`

#### Prototype

```c++
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
    No parameters required

- `Fight`  
    Operation

```jsonc
// Corresponding task parameters
{
    "stage": string,            // Stage name, optional, by default "当前关卡". Editing in run-time is not supported.
    "medicine": int,            // Maximum number Sanity Potion used, optional, by default 0
    "stone": int,               // Maximum number of Originite Prime used, optional, by default 0
    "times": int,               // Maximum times, optional, by default infinite
    "report_to_penguin": bool,  // Whether to upload data to Pengiun Stats, optional, by default false
    "penguin_id": string,       // Penguin Stats ID, optional, by default empty. Available only when `report_to_penguin` is `true`.
    "server": string            // Server, optional, by default "CN", will affect the drop recognition and upload
                                // Options："CN" | "US" | "JP" | "KR"
}
```

- `Recruit`  
    Recruitment

```jsonc
// Corresponding task parameters
{
    "refresh": bool,            // Whether to refresh 3★ tags, optional, by default false
    "select": [                 // Tag level to click, required
        int,
        ...
    ],
    "confirm": [                // Tag level for confirmation, required. Can be set to empty array for calculation only.
        int,
        ...
    ],
    "times": int,               // The times of recruitment, optional, by default 0. Can be set to 0 for calculation only.
    "set_time": bool,           // Whether to set time to 9 hours, available only when `times` is 0, optional, by default true
    "expedite": bool,           // Whether to use expedited plans, optional, by default false
    "expedite_times": int,      // The times of using expedited plans, available only when `expedite` is `true`
                                // Optional, by default infinity until `times` reaches its limitation.
}
```

- `Infrast`  
    Infrastructure shifting

```jsonc
{
    "mode": int,            // Shift mode, reserved, optional. Editing in run-time is not supported.
    "facility": [           // Facilities for shifting, required. Editing in run-time is not supported.
        string,             // Facility name: "Mfg" | "Trade" | "Power" | "Control" | "Reception" | "Office" | "Dorm"
        ...
    ],
    "drones": string,       // Usage of drones, optional, by default "_NotUse"
                            // "_NotUse"、"Money"、"SyntheticJade"、"CombatRecord"、"PureGold"、"OriginStone"、"Chip"
    "threshold": float,     // Morale threshold with range [0, 1.0], optional, by default 0.3
    "replenish": bool,      // Whether to replenish Originium Shard, optional, by default false
}
```

- `Visit`  
    Visiting reception room of friends  
    Settings are not supported.

- `Mall`  
    Collecting Credits and auto-purchasing
    Will buy items in order following `buy_first` list, and buy other items from left to right ignoring items iin `blacklist`.

```jsonc
// Corresponding task parameters
{
    "shopping": bool,       // Whether to buy items from the store, optional, by default false. Editing in run-time is not supported.
    "buy_first": [          // Items to be purchased with priority, optional. Editing in run-time is not supported.
        string,             // Item name, e.g. "招聘许可" (Recruitment Permit), "龙门币" (LMD), etc.
    ],
    "blacklist": [          // Blacklist, optional. Editing in run-time is not supported.
        string,             // Item name, e.g. "加急许可" (Expedited Plan), "家具零件" (Furniture Part), etc.
        ...
    ]
}
```

- `Award`  
    Collecting daily awards.  
    Settings are not supported.

- `Roguelike`  
    无限刷肉鸽

```jsonc
{
    "mode": int,            // 模式，可选项。默认 0
                            // 0 - 尽可能一直往后打
                            // 1 - 第一层投资完源石锭就退出
                            // 2 - 投资过后再退出，没有投资就继续往后打
}
```

- `Copilot`  
    Copilot auto-combat feature

```jsonc
{
    "stage_name": string,       // Stage name, same as the `stage_name` field in JSON. Editing in run-time is not supported.
    "filename": string,         // Filename and path of the task JSON, supporting absolute/relative paths. Editing in run-time is not supported.
    "formation": bool           // Whether to "quick build", optional, by default false. Editing in run-time is not supported.
}
```

For more details about task JSON, please refer to [Task Schema](TASK_SCHEMA.md)

### `AsstSetTaskParams`

#### Prototype

```c++
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
