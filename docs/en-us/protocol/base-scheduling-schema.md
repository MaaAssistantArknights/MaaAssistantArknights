---
order: 6
icon: material-symbols:view-quilt-rounded
---

# Infrastructure Schedule Schema

This document is primarily machine-translated. If you have the ability, please take a look at the Chinese version. We would greatly appreciate any errors or suggestions for improvement.

Usage and Field Description for `resource/custom_infrast/*.json` files

::: tip
Please note that JSON files do not support comments. The comments in the text are for demonstration only, do not copy them directly for use
:::

[Visual Schedule Generation Tool](https://ark.yituliu.cn/tools/schedule)

## Complete Field List

```json
{
    "title": "小号的换班方案", // Task name, optional
    "description": "哈哈哈哈", // Task description, optional
    "plans": [
        {
            "name": "早班", // Plan name, optional
            "description": "lol", // Plan description, optional
            "description_post": "", // Description to display after plan execution, optional
            "period": [
                // Shift time period, optional
                // If current time is within this interval, this plan will be automatically selected (the entire json file may contain multiple plans)
                // If this field does not exist, it will automatically switch to the next plan after each shift change
                // Core does not process this field, if you use interface integration with maa, please implement this logic yourself
                [
                    "22:00", // Required format hh:mm, currently only simple comparison of numbers, if crossing days please follow the example in this sample
                    "23:59"
                ],
                [
                    "00:00",
                    "06:00"
                ]
            ],
            "duration": 360, // Work duration (minutes), reserved field, currently has no effect. In the future, a popup may remind when it's time to change shifts, or even change automatically
            "Fiammetta": {
                // "菲亚梅塔" for which operator to use, optional, leave empty if not using
                "enable": true, // Whether to use "菲亚梅塔", optional, default true
                "target": "巫恋", // Target operator, using OCR, need to input operator name in corresponding client language
                "order": "pre" // Use before the entire shift change, or after completion, optional, value range "pre" / "post", default "pre"
            },
            "drones": {
                // Drone usage, optional, if not filled in, drones will not be used
                "enable": true, // Whether to use drones, optional, default true
                "room": "trading", // Which type of room to use for, value range "trading" / "manufacture"
                "index": 1, // Which number of this room type to use, corresponds to left tab bar sequence, value range [1, 5]
                "rule": "all", // Usage rule, reserved field, currently no effect. May be used for plug-in operations in future
                "order": "pre" // Use before changing operators or after, optional, value range "pre" / "post", default "pre"
            },
            "groups": [
                // For "control" / "manufacture" / "trading", operator groups can be set
                {
                    "name": "古+银",
                    "operators": ["古米", "银灰", "梅"]
                },
                {
                    "name": "清流",
                    "operators": ["清流", "森蚺", "温蒂"]
                }
            ],
            "rooms": {
                // Room information, required
                // Value range "control" / "manufacture" / "trading" / "power" / "meeting" / "hire" / "dormitory" / "processing"
                // Missing ones will use default algorithm for shift change.
                // If you want to skip changing operators in a room, use the skip field, or directly uncheck the facility in software Farming - Base Settings - General Settings
                "control": [
                {
                    "operators": [
                        "夕", // Using OCR, need to input operator name in Chinese language only
                        "令",
                        "凯尔希",
                        "阿米娅",
                        "玛恩纳"
                    ]
                }
                ],
                "manufacture": [
                {
                    "operators": ["芬", "稀音", "克洛丝"],
                    "sort": false // Whether to sort (according to the operators order above), optional, default false
                    // Example: when using operators like 稀音, 帕拉斯, 巫恋, etc. and "sort": false, operator order may be disrupted, causing preheating effect to be lost.
                    //     Using "sort": true can avoid this problem
                },
                {
                    "skip": true // Whether to skip the current room (corresponding to array sequence), optional, default false
                    // If true, other fields can be empty. Only skips operator change operation, others like using drones, clue exchange etc. will still proceed normally
                },
                {
                    "operators": ["Castle-3"],
                    "autofill": true, // Use the original algorithm to automatically fill remaining positions, optional, default false
                    // If operators is empty, the room will completely use the original algorithm for scheduling
                    // If operators is not empty, will only consider individual operator efficiency, not the entire combination efficiency
                    // Note that conflicts may occur with custom operators defined later, such as using operators needed later here, please use with caution, or place autofill room order at the end
                    "product": "Battle Record" // Current manufacturing product, optional.
                    // If the recognized facility product doesn't match the schedule setting, interface will show a red text prompt, may have more functions in future
                    // Value range: "Battle Record" | "Pure Gold" | "Dualchip" | "Originium Shard" | "LMD" | "Orundum"
                },
                {
                    "operators": ["多萝西"],
                    "candidates": [
                        // Backup operators, optional. Will use whoever is available to fill positions
                        // Not compatible with autofill=true, i.e. when this array is not empty, autofill needs to be false
                        "星源",
                        "白面鸮",
                        "赫默"
                    ]
                },
                {
                    "use_operator_groups": true, // Set to true to use operator groups in groups, default is false
                    "operators": [
                        // When enabled, names in operators will be interpreted as group names
                        "古+银", // Groups will be selected according to mood threshold and setting order
                        "清流" // If any operator in 古+银 group has mood below threshold, will use 清流 group
                    ]
                }
                ],
                "meeting": [
                    {
                        "autofill": true // Autofill this entire room
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

## Examples

[243 maximum efficiency, three shifts per day](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/243_layout_3_times_a_day.json)

[153 maximum efficiency, three shifts per day](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/153_layout_3_times_a_day.json)
