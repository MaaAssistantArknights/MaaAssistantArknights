---
order: 1
icon: bxs:book
---

# Integration Documentation

## API Introduction

### `AsstAppendTask`

#### Interface Prototype

```cpp
AsstTaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### Description

Add a task

#### Return Value

- `AsstTaskId`  
    If successful, returns the task ID which can be used for subsequent parameter settings;  
    If failed, returns 0

#### Parameter Description

- `AsstHandle handle`  
    Instance handle
- `const char* type`  
    Task type
- `const char* params`  
    Task parameters, JSON string

##### Task Types Overview

- `StartUp`  
  Start wakeup  

```json5
// Corresponding task parameters
{
    "enable": bool,              // Whether to enable this task, optional, default is true
    "client_type": string,       // Client version, optional, default is empty
                                 // Options: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "start_game_enabled": bool,  // Whether to automatically start the client, optional, default is false
    "account_name": string       // Switch account, optional, default is not to switch
                                 // Only supports switching to already logged-in accounts, using the login name for search, ensure input is unique among all logged-in accounts
                                 // Official server: 123****4567, you can input 123****4567, 4567, 123, 3****4567
                                 // B server: Zhang San, you can input Zhang San, Zhang, San
}
```

- `CloseDown`  
    Close game  

```json5
// Corresponding task parameters
{
    "enable": bool,              // Whether to enable this task, optional, default is true
    "client_type": string,       // Client version, required, will not execute if empty
                                 // Options: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
}
```

- `Fight`  
    Farm sanity

```json5
// Corresponding task parameters
{
    "enable": bool,             // Whether to enable this task, optional, default is true
    "stage": string,            // Stage name, optional, default is empty, recognizes current/last stage. Not supported during runtime
                                // Supports all main story stages, such as "1-7", "S3-2", etc.
                                // You can add Normal/Hard at the end of the stage to switch between standard and challenge difficulties
                                // For Annihilation, you must input "Annihilation"
                                // For current SS event last three stages, you must input the complete stage code
    "medicine": int,            // Maximum number of sanity potions to use, optional, default is 0
    "expiring_medicine": int,   // Maximum number of sanity potions expiring within 48 hours to use, optional, default is 0
    "stone": int,               // Maximum number of Originite Prime to use, optional, default is 0
    "times": int,               // Number of battles, optional, default is int32.max
    "series": int,              // Number of consecutive battles, optional, -1~6
                                // -1  to disable switching
                                // 0   to automatically switch to the maximum number currently available, if current sanity is not enough for 6 times, choose the minimum available number
                                // 1~6 to specify the number of consecutive battles
    "drops": {                  // Specify drop quantities, optional, default is not specified
        "30011": int,           // key - item_id, value quantity. key can be found in resource/item_index.json file
        "30062": int            // These are in OR relationship
    },
    /* All of the above are in OR relationship, meaning the task stops when any condition is met */

    "report_to_penguin": bool,  // Whether to report to Penguin Statistics, optional, default is false
    "penguin_id": string,       // Penguin Statistics reporting ID, optional, default is empty. Only effective when report_to_penguin is true
    "server": string,           // Server, optional, default is "CN", affects drop recognition and uploading
                                // Options: "CN" | "US" | "JP" | "KR"
    "client_type": string,      // Client version, optional, default is empty. Used to restart and reconnect when the game crashes, feature disabled if empty
                                // Options: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "DrGrandet": bool,          // Save sanity when using Originite Prime mode, optional, default is false, only effective when it may produce Originite Prime usage.
                                // Waits at the Originite Prime confirmation screen until the current 1 point of sanity is restored, then immediately uses Originite Prime
}
```

For support of some resource stage names, please refer to [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/tools/AutoLocalization/example/en-us.xaml#L260)

- `Recruit`  
  Recruitment

```json5
// Corresponding task parameters
{
    "enable": bool,             // Whether to enable this task, optional, default is true
    "refresh": bool,            // Whether to refresh 3-star Tags, optional, default is false
    "select": [                 // Tag levels to click, required
        int,
        ...
    ],
    "confirm": [                // Tag levels to confirm, required. If only calculating recruitment, can be set to empty array
        int,
        ...
    ],
    "first_tags": [             // Preferred Tags, only effective when Tag level is 3. Optional, default is empty
        string,                 // When Tag level is 3, will try to select as many Tags from here as possible (if any)
        ...                     // This is forced selection, meaning it will ignore all "do not select 3-star Tag" settings
    ],
    "extra_tags_mode": int,     // Select more Tags, optional, default is 0
                                // 0 - Default behavior
                                // 1 - Select 3 Tags, even if they may conflict
                                // 2 - If possible, select more high-star Tag combinations, even if they may conflict
    "times": int,               // Number of recruitments, optional, default is 0. If only calculating recruitment, can be set to 0
    "set_time": bool,           // Whether to set recruitment duration. Only effective when times is 0, optional, default is true
    "expedite": bool,           // Whether to use expedited plans, optional, default is false
    "expedite_times": int,      // Number of expedited plans to use, only effective when expedite is true.
                                // Optional, default is unlimited use (until times reaches upper limit)
    "skip_robot": bool,         // Whether to skip when robot tag is detected, optional, default is skip
    "recruitment_time": {       // Tag level (greater than or equal to 3) and corresponding desired recruitment duration in minutes, default values are all 540 (i.e. 09:00:00)
        "3": int,
        "4": int,
        ...
    },
    "report_to_penguin": bool,  // Whether to report to Penguin Statistics, optional, default is false
    "penguin_id": string,       // Penguin Statistics reporting ID, optional, default is empty. Only effective when report_to_penguin is true
    "report_to_yituliu": bool,  // Whether to report to Yituliu, optional, default is false
    "yituliu_id": string,       // Yituliu reporting ID, optional, default is empty. Only effective when report_to_yituliu is true
    "server": string,           // Server, optional, default is "CN", affects uploading
                                // Options: "CN" | "US" | "JP" | "KR"
}
```

- `Infrast`  
    Base shift

```json5
{
    "enable": bool,         // Whether to enable this task, optional, default is true
    "mode": int,            // Shift work mode, optional, default is 0 (Default)
                            // 0     - Default: Default shift mode, single facility optimal solution
                            // 10000 - Custom: Custom shift mode, reads user configuration, refer to protocol/base-scheduling-schema.md
                            // 20000 - Rotation: One-click rotation mode, will skip Control Center, Power Plant, Dormitory, and Office, other facilities won't shift but retain basic operations (such as using drones, reception room logic)

    "facility": [           // Facilities to shift (ordered), required. Not supported during runtime
        string,             // Facility name, "Mfg" | "Trade" | "Power" | "Control" | "Reception" | "Office" | "Dorm"
        ...
    ],
    "drones": string,       // Drone usage, optional, default is _NotUse
                            // This field is ignored when mode = 10000
                            // "_NotUse", "Money", "SyntheticJade", "CombatRecord", "PureGold", "OriginStone", "Chip"
    "threshold": float,     // Work mood threshold, optional, range [0, 1.0], default is 0.3
                            // When mode = 10000, this field is only effective for "autofill"
                            // When mode = 20000, this field is ignored
    "replenish": bool,      // Whether to automatically replenish "Originium Shard" in Trading Post, optional, default is false

    "dorm_notstationed_enabled": bool, // Whether to enable the "Not Stationed" option for dormitories, optional, default is false
    "dorm_trust_enabled": bool, // Whether to fill remaining dormitory positions with operators who haven't reached max trust, optional, default is false
    "reception_message_board": bool, // Whether to collect credit from the reception room message board, optional, default is true

    /* The following parameters are only effective when mode = 10000, otherwise they are ignored */
    "filename": string,     // Custom configuration path, required. Not supported during runtime
    "plan_index": int,      // Plan index to use in the configuration, required. Not supported during runtime
}
```

- `Mall`  
    Collect credit and shop.  
    Will first purchase according to `buy_first` in order, then purchase from left to right while avoiding `blacklist` for a second round, and when credit is overflowing, it will ignore the blacklist and purchase from left to right for a third round until no longer overflowing

```json5
// Corresponding task parameters
{
    "enable": bool,         // Whether to enable this task, optional, default is true
    "shopping": bool,       // Whether to shop, optional, default is false. Not supported during runtime
    "buy_first": [          // Priority purchase list, optional. Not supported during runtime
        string,             // Product name, such as "Recruitment Permit", "LMD", etc.
        ...
    ],
    "blacklist": [          // Blacklist, optional. Not supported during runtime
        string,             // Product name, such as "Expedited Plan", "Furniture Parts", etc.
        ...
    ],
   "force_shopping_if_credit_full": bool // Whether to ignore the blacklist when credit is overflowing, default is true
    "only_buy_discount": bool // Whether to only purchase discounted items, only applies to the second round of purchases, default is false
    "reserve_max_credit": bool // Whether to stop purchasing when credit is below 300, only applies to the second round of purchases, default is false
}
```

- `Award`  
    Collect various rewards

```json5
// Corresponding task parameters
{
    "enable": bool,            // Whether to enable this task, optional, default is true
    "award": bool,             // Collect daily/weekly task rewards, default is true
    "mail": bool,              // Collect all mail rewards, default is false
    "recruit": bool,           // Collect free daily single pull from limited banner, default is false
    "orundum": bool,           // Collect Orundum rewards from the lucky wall, default is false
    "mining": bool,            // Collect Orundum rewards from limited mining permits, default is false
    "specialaccess": bool      // Collect monthly card rewards gifted by the 5th anniversary, default is false
}
```

- `Roguelike`  
    Infinite Integrated Strategies

```json5
// Corresponding task parameters
{
    "enable": bool,  // Whether to enable this task, optional, default value is true
    "theme": string, // Theme, optional, default value is "Phantom"
                     //   Phantom - Phantom & Crimson Solitaire
                     //   Mizuki  - Mizuki & Caerula Arbor
                     //   Sami    - Explorer's Jǫklumarkar
                     //   Sarkaz  - Endless Reverie of the Sarkaz
    "mode": int,     // Mode, optional, default value is 0
                     //   0 - Farm points/reward points, try to reach higher floors stably
                     //   1 - Farm Originium Ingots, exit after investing on the first floor
                     //   2 - [Deprecated] Balance between modes 0 and 1, exit after investing, continue if there's no investment
                     //   3 - In development...
                     //   4 - Reset for opening conditions, first reach the third floor at difficulty 0 then restart, then switch to specified difficulty to try for opening rewards,
                     //       if not Hot Water or Hope, return to difficulty 0 to try again;
                     //       If in Phantom theme, don't switch difficulty, just try to reach third floor at current difficulty, restart, and try for opening rewards
                     //   5 - Farm for Collapsal Paradigms; only for Sami theme; accelerate collapse value accumulation through methods like letting enemies leak,
                     //       if the first Collapsal Paradigm encountered is in the expected_collapsal_paradigms list then stop the task, otherwise restart
                     //   6 - Farm for Monthly Squad minor rewards, same as mode 0 except for mode-specific adaptations
                     //   7 - Farm for Deep Exploration minor rewards, same as mode 0 except for mode-specific adaptations
    "squad": string,                // Starting squad name, optional, default value is "Command Squad";
    "roles": string,                // Starting profession group, optional, default value is "Complementary Strength";
    "core_char": string,            // Starting operator name, optional; only supports single operator **Chinese name**, regardless of server; if left empty or set to empty string "" will automatically select based on level
    "use_support": bool,            // Whether the starting operator is a support operator, optional, default value is false
    "use_nonfriend_support": bool,  // Whether non-friend support operators are allowed, optional, default value is false; only effective when use_support is true
    "starts_count": int,                // Number of times to start exploration, optional, default value is INT_MAX; stops task automatically when reached
    "difficulty": int,                  // Specified difficulty level, optional, default value is 0; only applicable to themes **except Phantom**;
                                        // If difficulty not unlocked, will select the highest unlocked difficulty
    "stop_at_final_boss": bool,         // Whether to stop task before the final boss node on floor 5, optional, default value is false; only applicable to themes **except Phantom**
    "stop_at_max_level": bool,          // Whether to stop task when roguelike level is maxed out, optional, default value is false
    "investment_enabled": bool,         // Whether to invest Originium Ingots, optional, default value is true
    "investments_count": int,           // Number of Originium Ingot investments, optional, default value is INT_MAX; stops task automatically when reached
    "stop_when_investment_full": bool,  // Whether to automatically stop task when investment reaches limit, optional, default value is false
    "investment_with_more_score": bool, // Whether to try shopping after investment, optional, default value is false; only applicable to mode 1
    "start_with_elite_two": bool,       // Whether to also try for Elite 2 direct promotion when resetting for opening conditions, optional, default value is false; only applicable to mode 4
    "only_start_with_elite_two": bool,  // Whether to only try for operator Elite 2 direct promotion and ignore other opening conditions, optional, default value is false;
                                        // Only effective when mode is 4 and start_with_elite_two is true
    "refresh_trader_with_dice": bool,   // Whether to use dice to refresh shop to buy special items, optional, default value is false; only applicable to Mizuki theme, used for farming Pointer Scales
    "first_floor_foldartal": string,    // Desired Foldartal to get during first floor foresight phase, optional; only applicable to Sami theme, any mode; stops task if successfully obtained
    "start_foldartal_list": [           // Desired Foldartals to get during opening rewards phase when resetting for opening conditions, optional, default value is []; only effective when theme is Sami and mode is 4;
      string,                           // Only counts as successful opening reset when all Foldartals in the list are present at opening;
      ...                               // Note: this parameter must be used together with "Life-Focused Team", other squads won't get Foldartals during opening rewards phase;
    ],
    "start_with_two_ideas": bool,       // Whether to reset for 2 ideas at opening, optional, default value is false; only effective when theme is Sarkaz and mode is 4
    "use_foldartal": bool,                    // Whether to use Foldartals, default value is false in mode 5, true in other modes; only applicable to Sami theme
    "check_collapsal_paradigms": bool,        // Whether to check obtained Collapsal Paradigms, default value is true in mode 5, false in other modes
    "double_check_collapsal_paradigms": bool, // Whether to perform Collapsal Paradigm detection anti-miss measures, default value is true in mode 5, false in other modes;
                                              // Only effective when theme is Sami and check_collapsal_paradigms is true
    "expected_collapsal_paradigms": [         // Desired Collapsal Paradigms to trigger, default value is ["Blind Spot", "Blind Eye", "Image Damage", "A Touch of Black"];
        string,                               // Only effective when theme is Sami and mode is 5
        ...
    ],
    "monthly_squad_auto_iterate": bool,    // Whether to enable Monthly Squad auto-switching
    "monthly_squad_check_comms": bool,     // Whether to also use Monthly Squad communications as switching criteria
    "deep_exploration_auto_iterate": bool, // Whether to enable Deep Exploration auto-switching
    "collectible_mode_shopping": bool,  // Whether to enable shopping in Hot Water mode, default value is false
    "collectible_mode_squad": string,   // Squad to use in Hot Water mode, defaults to synchronize with squad, when squad is empty string and collectible_mode_squad is not specified, uses Command Squad
    "collectible_mode_start_list": {    // Expected rewards for Hot Water mode, default all false, key range:
        "hot_water": bool,              // [hot_water: Hot Water, shield: Shield, ingot: Originium Ingot, hope: Hope, random: Random Reward, key: Key, dice: Dice, ideas: Ideas]
        ...
    },
    "start_with_seed": bool,        // Use seed to farm money, effective when true
                                    // Only possible when true in Sarkaz theme, Investment mode, "Pointake to Ingot Squad" or "Logistics Squad"
                                    // Uses fixed seed
}
```

For Collapsal Paradigm farming features, please refer to [Integrated Strategy Protocol](./integrated-strategy-schema.md#sami-iscollapsal-paradigms)

- `Copilot`  
    Auto stage copy

```json5
{
    "enable": bool,             // Whether to enable this task, optional, default is true
    "filename": string,         // Path to the task JSON file, both absolute and relative paths are supported. Not supported during runtime
    "formation": bool           // Whether to perform "Quick Formation", optional, default is false. Not supported during runtime
}
```

For task JSON, please refer to [Combat Flow Protocol](./copilot-schema.md)

- `SSSCopilot`  
    Auto Contingency Contract stage copy

```json5
{
    "enable": bool,             // Whether to enable this task, optional, default is true
    "filename": string,         // Path to the task JSON file, both absolute and relative paths are supported. Not supported during runtime
    "loop_times": int           // Number of execution loops
}
```

For Contingency Contract task JSON, please refer to [Contingency Contract Protocol](./sss-schema.md)

- `Depot`  
    Inventory recognition

```json5
// Corresponding task parameters
{
    "enable": bool          // Whether to enable this task, optional, default is true
}
```

- `OperBox`  
    Operator box recognition

```json5
// Corresponding task parameters
{
    "enable": bool          // Whether to enable this task, optional, default is true
}
```

- `Reclamation`  
    Reclamation Algorithm

```json5
{
    "enable": bool,
    "theme": string,            // Theme, optional. Default is 1
                                // Fire  - *Fire Within the Sand*
                                // Tales - *Tales Within the Sand*
    "mode": int,                // Mode, optional. Default is 0
                                // 0 - Farm points and construction points, enter battle then exit immediately
                                // 1 - Fire Within the Sand: Farm crude gold, liason buys water then forge at base;
                                //     Tales Within the Sand: Automatically craft items and load save to farm currency
    "tools_to_craft": [
        string,                 // Items to automatically craft, optional, default is glow stick
        ...
    ] 
                                // Recommend filling in substring
    "increment_mode": int,      // Click type, optional. Default is 0
                                // 0 - Rapid click
                                // 1 - Long press
    "num_craft_batches": int    // Maximum number of craft batches per session, optional. Default is 16
}
```

- `Custom`  
    Custom task

```json5
{
    "enable": bool,
    "task_names": [     // Execute the first matching task in the array (and subsequent next etc.)
                        // If you want to execute multiple tasks, you can append Custom task multiple times
        string,
        ...
    ]
}
```

- `SingleStep`  
    Single step task (currently only supports combat)

```json5
{
    "enable": bool,
    "type": string,     // Currently only supports "copilot"
    "subtask": string,  // "stage" | "start" | "action"
                        // "stage" sets stage name, requires "details": { "stage": "xxxx" }
                        // "start" starts operation, no details
                        // "action": single combat operation, details must be a single action from the combat protocol,
                        //           for example: "details": { "name": "SilverAsh", "location": [ 4, 5 ], "direction": "left" }, see protocol/copilot-schema.md for details
    "details": {
        ...
    }
}
```

- `VideoRecognition`  
  Video recognition, currently only supports task (combat) videos

```json5
{
    "enable": bool,
    "filename": string, // Path to the video file, both absolute and relative paths are supported. Not supported during runtime
}
```

### `AsstSetTaskParams`

#### Interface Prototype

```cpp
bool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);
```

#### Description

Set task parameters

#### Return Value

- `bool`  
    Returns whether setting was successful

#### Parameter Description

- `AsstHandle handle`  
    Instance handle
- `AsstTaskId task`  
    Task ID, the return value from `AsstAppendTask` interface
- `const char* params`  
    Task parameters, JSON string, same as the `AsstAppendTask` interface.  
    Fields not marked as "Not supported during runtime" support real-time modification; otherwise if the current task is running, the corresponding fields will be ignored

### `AsstSetStaticOption`

#### Interface Prototype

```cpp
bool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### Description

Set process-level parameters

#### Return Value

- `bool`  
    Returns whether setting was successful

#### Parameter Description

- `AsstStaticOptionKey key`  
    Key
- `const char* value`  
    Value

##### Key-Value Overview

None yet

### `AsstSetInstanceOption`

#### Interface Prototype

```cpp
bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### Description

Set instance-level parameters

#### Return Value

- `bool`  
    Returns whether setting was successful

#### Parameter Description

- `AsstHandle handle`  
    Instance handle
- `AsstInstanceOptionKey key`  
    Key
- `const char* value`  
    Value

##### Key-Value Overview

```cpp
    enum InstanceOptionKey
    {
        Invalid = 0,
        // Deprecated // MinitouchEnabled = 1,   // Whether to enable minitouch
                                // Even if enabled, it doesn't mean it will definitely work, device may not support it, etc.
                                // "1" on, "0" off
        TouchMode = 2,          // Touch mode setting, default is minitouch
                                // minitouch | maatouch | adb
        DeploymentWithPause = 3,    // Whether to pause when deploying operators, affects stage copy, Integrated Strategies, and Contingency Contract
                                    // "1" | "0"
        AdbLiteEnabled = 4,     // Whether to use AdbLite, "0" | "1"
        KillAdbOnExit = 5,       // Whether to kill the Adb process on exit, "0" | "1"
    };
```
