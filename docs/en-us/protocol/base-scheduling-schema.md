---
order: 6
icon: material-symbols:view-quilt-rounded
---

# Infrastructure Schedule Schema

This document is machine-translated. If you have the ability, please refer to the Chinese version. We would greatly appreciate any errors or suggestions for improvement.

Usage and Field Description for `resource/custom_infrast/*.json` files

::: tip
As JSON format does not support comments, please remove the comments when using the examples below.
:::

[Visual Schedule Generation Tool](https://ark.yituliu.cn/tools/schedule)

## Complete Field List

```json
{
    "title": "小号的换班方案",       // Assignment name, optional
    "description": "哈哈哈哈",       // Assignment description, optional
    "plans": [
        {
            "name": "早班",         // Plan name, optional
            "description": "lol",   // Plan description, optional
            "description_post": "", // The description to display after executing the plan, optional
            "period": [             // Shift change period, optional
                                    // If the current time is within this interval, the plan will be automatically selected (there may be multiple plans in the entire JSON file)
                                    // If this field does not exist, it will automatically switch to the next plan after each running.
                                    // Core does not process this field. If you are using interface integration with MAA, please implement this logic on your own
                [
                    "22:00",        // In the format of hh:mm, currently only simple numerical comparison is used. If it crosses midnight, please refer to the example in this file
                    "23:59"
                ],
                [
                    "00:00",
                    "06:00"
                ]
            ],
            "duration": 360,        // Work duration (in minutes), reserved field, currently not used. In the future, there may be reminders or automatic shift changes when the time comes
            "Fiammetta": {          // Which operator is using "Fiammetta", optional, not required if not in use
                "enable": true,     // Whether to use "Fiammetta", optional, default is true
                "target": "巫恋",   // Target operator, which is obtained through OCR and needs to be passed in the name of the operator in the corresponding client language
                "order": "pre",     // Whether to use it before or after the entire shift change, optional, with a value range of "pre" / "post", default is "pre"
            },
            "drones": {              // For using drones. Optional. If not specified, drones won't be used.
                "enable": true,      // Whether to use drones. Optional. Default is true.
                "room": "trading",   // Which type of room to use drones for. Possible values: "trading" / "manufacture"
                "index": 1,          // The index of the room to use drones for. Corresponds to the tab on the left. Possible values: [1, 5].
                "rule": "all",       // Usage rules. Reserved field. Currently unused, but may be used in the future to support plug-ins and other operations.
                "order": "pre"       // Whether to use drones before or after changing operators. Optional. Possible values: "pre" / "post". Default is "pre".
            },
            "groups":[              // For "control" / "manufacture" / "trading", operator groups can be set
                {
                    "name":"A",
                    "operators":[
                        "古米",
                        "银灰",
                        "梅"
                    ]
                },
                {
                    "name":"B",
                    "operators":[
                        "清流",
                        "森蚺",
                        "温蒂"
                    ]
                }
            ],
            "rooms": {                 // Room information. Required.
                           // Possible values: "control" / "manufacture" / "trading" / "power" / "meeting" / "hire" / "dormitory" / "processing".
                           // If a room is not specified, the default algorithm will be used to change operators.
                           // To avoid changing operators in a specific room, use the "skip" field, or uncheck the corresponding facility in the software's infrastructure settings.
                "control": [
                    {
                        "operators": [  // The operators assigned to this room. Identified through OCR. Must be passed in the corresponding client language.
                            "夕",
                            "令",
                            "凯尔希",
                            "阿米娅",
                            "玛恩纳"
                        ]
                    }
                ],
                "manufacture": [
                    {
                        "operators": [  // The operators assigned to this room.
                            "芬",
                            "稀音",
                            "克洛丝"
                        ],
                        "sort": false,   // Whether to sort the operators according to the above order. Optional. Default is false.
                                         // For example: if using operators like "稀音", "帕拉斯", "巫恋", etc. and "sort": false, the order of operators may be disrupted, leading to a loss of preheating effect.
                                         // If "sort": true is used, this issue can be avoided.
                    },
                    {
                        "skip": true    // Whether to skip this room (corresponding to the array index). Optional. Default is false.
                                        // If true, all other fields can be empty. Only the operator change will be skipped. Other actions, such as drone usage and clue exchange, will proceed as usual.
                    },
                    {
                        "operators": [
                            "Castle-3"
                        ],
                        "autofill": true, // Use the original algorithm to automatically fill the remaining positions. Optional, default to false.
                                          // If the operators array is empty, the scheduling of the entire room will be based on the original algorithm.
                                          // If the operators array is not empty, only the efficiency of individual operators will be considered, not the efficiency of the entire combination.
                                          // Be careful that conflicts may arise with the custom operators defined later, for example, if the operators needed later are used here, please use it with caution, or place the autofill room order at the end.
                        "product": "Battle Record" // The current manufactured product, optional.
                                                   // If the product recognized by the facility does not match the one set in the task, the interface will display a red warning message. More functions may be added in the future.
                                                   // Possible values: "Battle Record" | "Pure Gold" | "Dualchip" | "Originium Shard" | "LMD" | "Orundum"
                    },
                    {
                        "operators": [
                            "多萝西"
                        ],
                        "candidates": [ // Optional backup operators. Use whoever is listed until all positions are filled.
                                        // Not compatible with autofill=true. When this array is not empty, autofill should be set to false.
                            "星源",
                            "白面鸮",
                            "赫默"
                        ]
                    },
                    {
                        "use_operator_groups":true,  // Set to true to use the grouping of operators in groups, default is false
                        "operators":[                // When enabled, names in operators will be interpreted as group names
                            "A",                     // Grouping will be selected according to mood threshold and setting order
                            "B"                      // If any operator in group A whose mood is below the threshold, group B will be used
                        ]
                    }
                ],
                "meeting": [
                    {
                        "autofill": true // Use autofill for the entire room.
                    }
                ]
            }
        },
        {
            "name": "晚班"
            // ...
        }
    ]
}
```

## Example

[243_layout_3_times_a_day](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/243_layout_3_times_a_day.json)

[153_layout_3_times_a_day](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/153_layout_3_times_a_day.json)
