---
order: 3
icon: ph:sword-bold
---

# Copilot Schema

Usage and field descriptions for `resource/copilot/*.json` files

::: tip
Please note that JSON files do not support comments. The comments in the text are for demonstration only, do not copy them directly for use
:::

## Complete Field List

```json
{
    "stage_name": "暴君", // Stage name, required. Can be Chinese stage name, code, stageId, levelId, etc., as long as it's unique.
    "opers": [
        // Specified operators
        {
            "name": "重岳", // Operator name
            "skill": 3, // Skill number. Optional, default is 1, value range [1, 3]
            "skill_usage": 2, // Skill usage. Optional, default is 0
            // 0 - Do not use automatically (relies on "actions" field)
            // 1 - Use whenever ready, as many times as available (e.g., Thorns skill 3, Myrtle skill 1, etc.)
            // 2 - Use X times (e.g., Mountain skill 2 use 1 time, Horn skill 3 use 5 times, set via "skill_times" field)
            // 3 - Automatically determine when to use (future feature)
            // If it's a fully automatic skill, fill in 0
            "skill_times": 5, // Number of skill uses. Optional, default is 1
            "requirements": {
                // Upgrade requirements. Reserved interface, not yet implemented. Optional, default is empty
                "elite": 2, // Elite level. Optional, default is 0, no elite level requirement
                "level": 90, // Operator level. Optional, default is 0
                "skill_level": 10, // Skill level. Optional, default is 0
                "module": 1, // Module number. Optional, default is 0
                "potentiality": 1 // Potential requirement. Optional, default is 0
            }
        }
    ],
    "groups": [
        {
            "name": "任意正常群奶", // Group name, required
            // You can name it whatever you want, just make sure it matches the name in deploy below
            "opers": [
                // Choose any one operator, unordered, will prioritize higher level operators, usage same as opers field above
                {
                    "name": "夜莺",
                    "skill": 3,
                    "skill_usage": 2 // If using at specific moments, different operators may have different skill CDs, please be aware
                },
                {
                    "name": "白面鸮",
                    "skill": 2,
                    "skill_usage": 2
                }
            ]
        }
    ],
    "actions": [
        // Operations during combat. Ordered, will only execute the next one after completing the previous one. Required
        {
            "type": "部署", // Operation type, optional, default is "Deploy"
            // "Deploy" | "Skill" | "Retreat" | "SpeedUp" | "BulletTime" | "SkillUsage" | "Output" | "SkillDaemon" | "MoveCamera"
            // "部署"   |  "技能"  |  "撤退"   | "二倍速"   |  "子弹时间"  |  "技能用法"   | "打印"  |  "摆完挂机" | "移动镜头"
            // Both Chinese and English are accepted, with the same effect
            // If "Deploy", will wait until enough DP is available (unless timeout)
            // If "Skill", will wait until the skill CD is ready (unless timeout)
            // "SpeedUp" is toggleable, using once makes it 2x speed, using again returns to 1x speed
            // "BulletTime" is the 1/5 speed when clicking any operator, will return to normal speed after any action
            //      "name" or "location" must be filled in, indicating which operator to click to enter bullet time, can be operators on field or in deployment list (will be automatically determined)
            //      If the next action is "Skill"/"Retreat", need to fill in the same "name" or "location" as the next action
            //      If the next action is "Deploy", fill in anyone (but preferably not the one to be deployed, as clicking the portrait may affect recognition)
            // "Output" doesn't show this step in the interface, only used to output the content in doc (for subtitles etc.)
            // "SkillDaemon" only uses skills with "Use when ready" setting, does nothing else until the battle ends
            // "MoveCamera" is used for "Navigator Trial" mode, also needs to fill in the distance field
            // Currently the following four conditions are in AND relationship, i.e. &&
            "kills": 0, // Kill count condition, will wait until reached. Optional, default is 0, execute immediately
            "costs": 50, // DP condition, will wait until reached. Optional, default is 0, execute immediately
            // DP is affected by potential and other factors, may not be completely accurate, only suitable for battles with loose timing requirements.
            // Otherwise, please use cost_changes below
            // Also, DP recognition is more accurate with two digits, three-digit DP may be misrecognized, not recommended
            "cost_changes": 5, // DP change amount condition, will wait until reached. Optional, default is 0, execute immediately
            // Note this is calculated from when this action starts executing (i.e. the DP at the end of the previous action is used as baseline)
            // Supports negative values, i.e., DP decreased (e.g., due to operators like "Jaye" that consume DP)
            // Also, DP recognition is more accurate with two digits, three-digit DP may be misrecognized, not recommended
            "cooling": 2, // Number of operators in cooldown condition, will wait until reached. Optional, default is -1, don't recognize
            // TODO: Other conditions
            // TODO: "condition_type": 0,    // Relationship between execution conditions, optional, default is 0
            //                        // 0 - AND; 1 - OR
            "name": "棘刺", // Operator name or group name, required when type is "Deploy", optional for "Skill"|"Retreat"
            "location": [
                5,
                5
            ], // Position to deploy the operator.
            // Required when type is "Deploy".
            // Optional when type is "Skill"|"Retreat",
            // "Skill": Only recommended for automatic devices on the field, don't fill in name, and use location to activate skills. For normally deployed operators, it's recommended to use name to activate skills
            // "Retreat": Only recommended when there are multiple summons with same name, don't fill in name, and use location to retreat. For normally deployed operators, it's recommended to use name for retreat
            "direction": "左", // Direction the deployed operator faces. Required when type is "Deploy"
            // "Left" | "Right" | "Up" | "Down" | "None"
            // "左"   |  "右"   | "上"  | "下"   |  "无"
            // Both Chinese and English are accepted, with the same effect
            "skill_usage": 1, // Modify skill usage. Required when type is "SkillUsage"
            // Example: Just deployed Myrtle needs to help deal with some enemies, can't automatically use skills, but in mid-late game when stable needs to use skills automatically
            // Then you can set to 1 at the appropriate time
            "skill_times": 5, // Number of skill uses. Optional, default is 1
            "pre_delay": 0, // Pre-delay. Optional, default is 0, unit milliseconds
            // After all condition parameters of the current action are met, start timing, execute the corresponding action after timing ends
            "post_delay": 0, // Post-delay. Optional, default is 0, unit milliseconds
            // After the action corresponding to the current action's type is completed, start timing, start the next action after timing ends
            //
            // "timeout": 999999999,   // Reserved field, not yet implemented.
            // Timeout. Optional when type is "Deploy"|"Skill". Default INT_MAX, unit milliseconds
            // If timeout, abandon current action and execute the next action
            "distance": [
                4.5,
                0
            ], // Required when type is "MoveCamera"
            // [x movement in tiles, y movement in tiles], can be decimal
            // Note that during "MoveCamera", it cannot recognize what's standing in the field, need to use sleep to fully cover the entire movement animation
            "doc": "Deploying Thorns!", // Description, optional. Will show in the interface, has no actual effect
            "doc_color": "orange" // Description text color, optional, default is gray. Will show in the interface, has no actual effect.
        },
        // Example 1
        {
            "name": "任意正常群奶",
            "location": [
                5,
                6
            ],
            "direction": "右"
        },
        // Example 2
        {
            "name": "史尔特尔",
            "location": [
                4,
                5
            ],
            "direction": "左",
            "doc": "你史尔特尔奶奶来啦！",
            "doc_color": "red"
        },
        // Example 3
        {
            "type": "二倍速"
        }
    ],
    "minimum_required": "v4.0", // Minimum required MAA version, required. Reserved field, not yet implemented
    "doc": {
        // Description, optional.
        "title": "低练度高成功率作业",
        "title_color": "dark",
        "details": "对练度要求很低balabala……", // It's recommended to write your name here! (author name), reference video strategy links, etc.
        "details_color": "dark"
    },
    "difficulty": 0 // Difficulty corresponding to the strategy, optional, default is 0.
    // 0: Default, not set
    // 1: Supports normal difficulty
    // 2: Supports challenge mode
    // 3: Supports both normal and challenge modes
}
```

## Examples

[OF-1](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/OF-1_credit_fight.json)
