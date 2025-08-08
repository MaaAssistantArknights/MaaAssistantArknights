---
order: 6
icon: material-symbols:view-quilt-rounded
---

# Base Scheduling Protocol

Usage and field descriptions for `resource/custom_infrast/*.json`

::: tip
Please note that JSON files do not support comments. The comments in this document are for demonstration purposes only. Do not copy them directly into your JSON files.
:::

[Visual Schedule Generator Tool](https://ark.yituliu.cn/tools/schedule)

## Complete Field Reference

```json
{
    "title": "小号的换班方案", // Task name, optional ("小号的换班方案" = "Alt Account Shift Plan")
    "description": "哈哈哈哈", // Task description, optional ("哈哈哈哈" = "Hahaha")
    "plans": [
        {
            "name": "早班", // Plan name, optional ("早班" = "Morning Shift")
            "description": "lol", // Plan description, optional
            "description_post": "", // Description displayed after plan execution, optional
            "period": [
                // Shift time period, optional
                // If current time is within this range, this plan will be automatically selected (JSON file may contain multiple plans)
                // If this field doesn't exist, automatically switches to the next plan after each shift ends
                // Core doesn't process this field; if you're using MAA through integration, implement this logic yourself
                [
                    "22:00", // Format required: hh:mm, currently just compares number sizes, for cross-day periods follow this example
                    "23:59"
                ],
                [
                    "00:00",
                    "06:00"
                ]
            ],
            "duration": 360, // Work duration (minutes), reserved field, currently not used. May be used for shift change reminders in the future
            "Fiammetta": {
                // Which operator to use "Fiammetta" on, optional, omit if not using
                "enable": true, // Whether to use "Fiammetta", optional, default true
                "target": "巫恋", // Target operator, uses OCR, requires operator name in client language ("巫恋" = "Shamare")
                "order": "pre" // Use before entire shift change or after, optional, values "pre"/"post", default "pre"
            },
            "drones": {
                // Drone usage, optional, omit if not using drones
                "enable": true, // Whether to use drones, optional, default true
                "room": "trading", // Which room type to use drones on, values "trading"/"manufacture"
                "index": 1, // Which instance of that room type to use drones on, corresponds to left tab number, range [1, 5]
                "rule": "all", // Usage rule, reserved field, currently not used. May support plug/unplug operations in future
                "order": "pre" // Use before operator changes or after, optional, values "pre"/"post", default "pre"
            },
            "groups": [
                // For "control"/"manufacture"/"trading", you can set operator groups
                {
                    "name": "古+银", // ("古+银" = "Gummy+Silver")
                    "operators": ["古米", "银灰", "梅"] // ("古米" = "Gummy", "银灰" = "SilverAsh", "梅" = "Plum")
                },
                {
                    "name": "清流", // ("清流" = "Purestream")
                    "operators": ["清流", "森蚺", "温蒂"] // ("清流" = "Purestream", "森蚺" = "Eunectes", "温蒂" = "Weedy")
                }
            ],
            "rooms": {
                // Room information, required
                // Values: "control"/"manufacture"/"trading"/"power"/"meeting"/"hire"/"dormitory"/"processing"
                // Missing rooms use default algorithm for shift change.
                // To skip a room, use skip field or uncheck the facility in software Task Settings - Base Management - General Settings
                "control": [
                {
                    "operators": [
                        "夕", // Uses OCR, requires operator name in client language ("夕" = "Dusk")
                        "令", // ("令" = "Ling")
                        "凯尔希", // ("凯尔希" = "Kal'tsit")
                        "阿米娅", // ("阿米娅" = "Amiya")
                        "玛恩纳" // ("玛恩纳" = "Closure")
                    ]
                }
                ],
                "manufacture": [
                {
                    "operators": ["芬", "稀音", "克洛丝"], // ("芬" = "Fang", "稀音" = "Scene", "克洛丝" = "Kroos")
                    "sort": false // Whether to sort (by operators order above), optional, default false
                    // Example: When using Scene, Pallas, Shamare, etc. with "sort": false, operator order may be scrambled, losing warm-up effect.
                    // Using "sort": true can avoid this issue
                },
                {
                    "skip": true // Whether to skip current room (by array index), optional, default false
                    // If true, other fields can be empty. Only skips operator change operation, other actions like drone usage, clue exchange still occur
                },
                {
                    "operators": ["Castle-3"],
                    "autofill": true, // Use original algorithm to auto-fill remaining positions, optional, default false
                    // If operators is empty, room uses original algorithm completely
                    // If operators not empty, considers only single operator efficiency, not combination efficiency
                    // May conflict with custom operators defined later, e.g., using operators needed later, use cautiously or place autofill rooms last
                    "product": "Battle Record" // Current manufacturing product, optional.
                    // If detected facility product doesn't match task setting, UI will show red warning; may have more uses in future
                    // Values: "Battle Record"|"Pure Gold"|"Dualchip"|"Originium Shard"|"LMD"|"Orundum"
                },
                {
                    "operators": ["多萝西"], // ("多萝西" = "Dorothy")
                    "candidates": [
                        // Backup operators, optional. Uses whoever's available until positions filled
                        // Incompatible with autofill=true, i.e., autofill must be false when this array not empty
                        "星源", // ("星源" = "Astgenne")
                        "白面鸮", // ("白面鸮" = "Ptilopsis")
                        "赫默" // ("赫默" = "Silence")
                    ]
                },
                {
                    "use_operator_groups": true, // Set true to use operator groups from groups field, default false
                    "operators": [
                        // When enabled, names in operators are interpreted as group names
                        "古+银", // Will select groups by mood threshold and setting order ("古+银" = "Gummy+Silver")
                        "清流" // If Gummy+Silver group has operators below threshold, will use Purestream group ("清流" = "Purestream")
                    ]
                }
                ],
                "meeting": [
                    {
                        "autofill": true // Completely autofill this room
                    }
                ]
            }
        },
        {
        "name": "晚班" // ("晚班" = "Night Shift")
        // ...
        }
    ]
}
```

## Example

[243_layout_3_times_a_day](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/243_layout_3_times_a_day.json)

[153_layout_3_times_a_day](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/153_layout_3_times_a_day.json)
