---
order: 3
icon: ph:sword-bold
---

# Combat Operation Protocol

Usage of `resource/copilot/*.json` files and field descriptions

::: tip
Please note that JSON files do not support comments. The comments in this document are for demonstration purposes only. Do not copy them directly into your JSON files.
:::

## Complete Field Reference

```json
{
    "stage_name": "暴君", // Stage name, required. Chinese name, code, stageId, levelId, etc. are all acceptable as long as they uniquely identify the stage. ("暴君" = "Patriot")
    "opers": [
        // Specified operators
        {
            "name": "重岳", // Operator name ("重岳" = "Chongyue")
            "skill": 3, // Skill number. Optional, default is 1, range [1, 3]
            "skill_usage": 2, // Skill usage method. Optional, default is 0
            // 0 - Do not use automatically (depends on "actions" field)
            // 1 - Use whenever ready, as many times as possible (e.g., Thorns S3, Myrtle S1, etc.)
            // 2 - Use X times (e.g., Mountain S2 once, Chongyue S3 five times, set via "skill_times" field)
            // 3 - Auto-determine usage timing (placeholder)
            // For auto-trigger skills, use 0
            "skill_times": 5, // Number of skill activations. Optional, default is 1
            "requirements": {
                // Level requirements. Reserved interface, not yet implemented. Optional, default is empty
                "elite": 2, // Elite level. Optional, default is 0, no elite requirement
                "level": 90, // Operator level. Optional, default is 0
                "skill_level": 10, // Skill level. Optional, default is 0
                "module": 1, // Module number. Optional, default is 0
                "potentiality": 1 // Potential requirement. Optional, default is 0
            }
        }
    ],
    "groups": [
        {
            "name": "任意正常群奶", // Group name, required ("任意正常群奶" = "Any Medic")
            // Use any name that matches the "name" field in the deploy section below
            "opers": [
                // Choose any one operator, unordered, higher level operators preferred, usage same as opers field above
                {
                    "name": "夜莺", // Operator name ("夜莺" = "Nightingale")
                    "skill": 3,
                    "skill_usage": 2 // For specific timing, different operators may have different skill CDs, please be aware
                },
                {
                    "name": "白面鸮", // Operator name ("白面鸮" = "Ptilopsis")
                    "skill": 2,
                    "skill_usage": 2
                }
            ]
        }
    ],
    "actions": [
        // Combat operations. Ordered, executes the next one only after completing the previous one. Required
        {
            "type": "部署", // Operation type, optional, default is "Deploy" ("部署" = "Deploy")
            // "Deploy" | "Skill" | "Retreat" | "SpeedUp" | "BulletTime" | "SkillUsage" | "Output" | "SkillDaemon" | "MoveCamera"
            // "部署"   |  "技能"  |  "撤退"   | "二倍速"   |  "子弹时间"  |  "技能用法"   | "打印"  |  "摆完挂机" | "移动镜头"
            // Both English and Chinese are supported (e.g., "部署" for "Deploy")
            // For "Deploy", waits until enough DP is available (unless timeout)
            // For "Skill", waits until skill is ready (unless timeout)
            // "SpeedUp" toggles between normal and 2x speed
            // "BulletTime" is 1/5 speed after clicking an operator; any following action returns to normal speed
            //      "name" or "location" must be specified to indicate which operator to enter bullet time with (auto-detected)
            //      If next action is "Skill"/"Retreat", use the same "name" or "location" as the next action
            //      If next action is "Deploy", use any operator (but avoid using the operator to be deployed as it affects recognition)
            // "Output" doesn't show this step in UI, used only to output doc content (for subtitles, etc.)
            // "SkillDaemon" only uses "use when ready" skills, does nothing else until battle ends
            // "MoveCamera" for Guide mode, requires the distance field
            // Currently the four conditions below use AND relationship
            "kills": 0, // Kill count condition, waits until reached. Optional, default is 0, executes immediately
            "costs": 50, // DP condition, waits until reached. Optional, default is 0, executes immediately
            // DP can be affected by potential, may not be entirely accurate, suitable for battles with loose timing requirements.
            // Otherwise use cost_changes below
            // Recognition is more accurate for two-digit DP; three-digit DP may be misrecognized, not recommended
            "cost_changes": 5, // DP change condition, waits until reached. Optional, default is 0, executes immediately
            // Calculated from when this action begins executing (using DP at the end of previous action as baseline)
            // Supports negative values (e.g., when operators like Jaye reduce DP)
            // Recognition is more accurate for two-digit DP; three-digit DP may be misrecognized, not recommended
            "cooling": 2, // Number of operators on cooldown condition, waits until reached. Optional, default is -1, no recognition
            // TODO: Other conditions
            // TODO: "condition_type": 0,    // Relationship between execution conditions, optional, default is 0
            //                        // 0 - AND; 1 - OR
            "name": "棘刺", // Operator name or group name, required when type is "Deploy", optional for "Skill"|"Retreat" ("棘刺" = "Thorns")
            "location": [
                5,
                5
            ], // Operator deployment position.
            // Required when type is "Deploy".
            // Optional when type is "Skill"|"Retreat".
            // "Skill": Recommended only for automatic devices on the field, use location without name to activate skill. For normal deployed operators, use name
            // "Retreat": Recommended only when multiple summons share the same name, use location without name to retreat. For normal deployed operators, use name
            "direction": "左", // Operator facing direction. Required when type is "Deploy" ("左" = "Left")
            // "Left" | "Right" | "Up" | "Down" | "None"
            // "左"   |  "右"   | "上"  | "下"   |  "无"
            // Both English and Chinese are supported (e.g., "左" for "Left")
            "skill_usage": 1, // Change skill usage method. Required when type is "SkillUsage"
            // Example: Initially need Myrtle to help attack without using skill, later need auto skill activation
            // Can set to 1 at the appropriate time
            "skill_times": 5, // Number of skill activations. Optional, default is 1
            "pre_delay": 0, // Pre-delay. Optional, default is 0, unit is milliseconds
            // After all conditions for the current action are met, starts timing, executes the corresponding action when timing ends
            "post_delay": 0, // Post-delay. Optional, default is 0, unit is milliseconds
            // After the current action is executed, starts timing, begins next action when timing ends
            //
            // "timeout": 999999999,   // Reserved field, not yet implemented.
            // Timeout. Optional when type is "Deploy"|"Skill". Default INT_MAX, unit is milliseconds
            // Abandons current action and executes next action when timeout occurs
            "distance": [
                4.5,
                0
            ], // Required when type is "MoveCamera"
            // [x movement in tiles, y movement in tiles], can be decimal
            // Note that during "MoveCamera", operators on field cannot be recognized, need to use sleep to cover entire animation
            "doc": "下棘刺了！", // Description, optional. Displayed in UI, no actual function ("下棘刺了！" = "Deploying Thorns!")
            "doc_color": "orange" // Description text color, optional, default is gray. Displayed in UI, no actual function
        },
        // Example 1
        {
            "name": "任意正常群奶", // ("任意正常群奶" = "Any Medic")
            "location": [
                5,
                6
            ],
            "direction": "右" // ("右" = "Right")
        },
        // Example 2
        {
            "name": "史尔特尔", // ("史尔特尔" = "Silverash")
            "location": [
                4,
                5
            ],
            "direction": "左", // ("左" = "Left")
            "doc": "你史尔特尔奶奶来啦！", // ("你史尔特尔奶奶来啦！" = "Here comes your Silverash grandma!")
            "doc_color": "red"
        },
        // Example 3
        {
            "type": "二倍速" // ("二倍速" = "SpeedUp")
        }
    ],
    "minimum_required": "v4.0", // Minimum required MAA version, required. Reserved field, not yet implemented
    "doc": {
        // Description, optional.
        "title": "低练度高成功率作业", // ("低练度高成功率作业" = "Low-investment high-success rate operation")
        "title_color": "dark",
        "details": "对练度要求很低balabala……", // ("对练度要求很低balabala……" = "Very low level requirements etc...") Recommend adding your name (author), reference video links, etc. here
        "details_color": "dark"
    },
    "difficulty": 0 // Operation difficulty, optional, default value is 0.
    // 0: Default, not set
    // 1: Supports normal difficulty
    // 2: Supports challenge mode
    // 3: Supports both normal and challenge modes
}
```

## Examples

[OF-1](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/OF-1_credit_fight.json)
