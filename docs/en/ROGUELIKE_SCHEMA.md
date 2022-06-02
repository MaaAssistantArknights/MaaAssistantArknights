# Roguelike Schema

Usage of `resource/roguelike/*.json` and description of each field

## Overview

```jsonc
{
    "stage_name": "驯兽小屋",        // Stage name, required
    "actions": [                    // Actions in order, required. Will be executed sequentially
        {
            "type": "部署",         // Action type, optional, "Deploy" by default
                                    // "Deploy" | "Skill" | "Retreat" | "AllSkill"
                                    // "部署"   |  "技能"  |  "撤退"   | "全部技能"
                                    // Both English and Chinese are supported
                                    // "Deploy" will wait until the cost is enough (unless timeout)
                                    // "Skill" will wait until the skill is ready (unless timeout)
                                    // Use it based on skill effects and CDs of operators (e.g. Mountain: Sweeping Stance, switchable skill)
                                    // "AllSkill" will use all skills of operators with `skill_usage == 3`

            "kills": 0,             // Waiting until the number of kills required is reached, optional, 0 by default
            // TODO: Other conditions
            // TODO: "condition_type": 0,    // Condition aggregation type, optional, 0 by default
            //                        // 0 - AND； 1 - OR

            "roles": [              // Classes of the operators, optional, empty by default, meaning all classes are acceptable
                                    // The list is ordered, with the first item having the highest priority
                                    // Meele/ranged operators are chosen automatically. Non-deployable operators will be ignored.
                                    // The list will be skipped if none of the classes are available.
                "近卫", // Guard
                "先锋", // Vanguard
                "重装", // Defender
                "召唤物" // Summon
            ],
            
            "waiting_cost": false,  // Whether to wait for the cost to be enough for class with higher priority in `roles` list, optional, `false` by default
                                    // `true` means to use existing class in the `roles` list that has the highest priority, waiting for the cost to be enough.
                                    // `false` means to use existing class in the `roles` list that has the highest priority and enough cost.

            "location": [ 5, 5 ],   // Location of the operator, required

            "direction": "左",      // Direction for deployment, required when `type` is "Deploy"
                                    // "Left" | "Right" | "Up" | "Down" | "None"
                                    // "左"   |  "右"   | "上"  | "下"   |  "无"
                                    // Both English and Chinese are supported

            "pre_delay": 0,         // Pre-delay in ms, optional, 0 by default
            "rear_delay": 0,        // Post-delay in ms, optional, 0 by default

            "timeout": 999999999    // Timeout time in ms, optional when `type` is "Deploy", INT_MAX by default
                                    // Will proceed with the next action when timeout
        },
        {                           // Example
            "location": [6, 6],
            "roles": [
                "近卫", // Guard
                "先锋", // Vanguard
                "重装", // Defender
                "召唤物" // Summon
            ],
            "direction": "左"
        }
    ]
}
```
