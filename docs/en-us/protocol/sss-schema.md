---
order: 7
icon: game-icons:prisoner
---

# Stationary Security Service Schema

::: tip
Please note that JSON files do not support comments. The comments in this document are for demonstration purposes only. Do not copy them directly into your JSON files.
:::

```json
{
    "type": "SSS", // Protocol type, SSS indicates Stationary Security Service, required, cannot be modified
    "stage_name": "多索雷斯在建地块", // Map name, required ("多索雷斯在建地块" = "Dossoles Under Construction")
    "minimum_required": "v4.9.0", // Minimum required MAA version, required
    "doc": {
        // Description, optional
        "title": "低练度高成功率作业", // ("低练度高成功率作业" = "Low-investment high-success rate operation")
        "title_color": "dark",
        "details": "对练度要求很低balabala……", // Recommend adding your name (author), reference video links, etc. here ("对练度要求很低balabala……" = "Very low level requirements etc...")
        "details_color": "dark"
    },
    "buff": "自适应补给元件", // Starting navigation element selection, optional ("自适应补给元件" = "Adaptive Supply Component")
    "equipment": [
        // Starting equipment selection, read horizontally, optional
        // Not implemented in current version, only displayed in interface
        "A",
        "A",
        "A",
        "A",
        "B",
        "B",
        "B",
        "B"
    ],
    "strategy": "优选策略", // Or "自由策略" ("Free strategy"), optional
    // Not implemented in current version, only displayed in interface
    "opers": [
        // Specified operators, optional
        {
            "name": "棘刺", // ("棘刺" = "Thorns")
            "skill": 3,
            "skill_usage": 1
        }
    ],
    "tool_men": {
        // Remaining required operators by profession, picked by cost, optional
        // Not implemented in current version, only displayed in interface
        "Pioneer": 13,
        "近卫": 2, // Both Chinese and English supported ("近卫" = "Guard")
        "Medic": 2
    },
    "drops": [
        // Recruitment priority for operators and equipment at battle start and during battle
        "空弦", // ("空弦" = "Archetto")
        "能天使", // Supports operator names, class names ("能天使" = "Exusiai")
        "先锋", // Class names in Chinese or English ("先锋" = "Vanguard")
        "Support",
        "无需增调干员", // Don't recruit anyone ("无需增调干员" = "No recruitment needed")
        "重整导能组件", // Supports equipment names ("重整导能组件" = "Reorganization Navigation Component")
        "反制导能组件", // ("反制导能组件" = "Counter Navigation Component")
        "战备激活阀", // In-stage optional equipment, also listed here ("战备激活阀" = "Combat Preparation Actuator")
        "改派发讯器" // ("改派发讯器" = "Modified Device Signal")
    ],
    "blacklist": [
        // Blacklist, optional. These won't be selected in drops.
        // In future versions with squad formation, these won't be selected as tool operators either
        "夜半", // ("夜半" = "Nightingale")
        "梅尔" // ("梅尔" = "Mayer")
    ],
    "stages": [
        {
            "stage_name": "蜂拥而上", // Single stage name, required ("蜂拥而上" = "Swarming Advance")
            // Supports name, stageId, levelId; recommend stageId or levelId
            // Don't use code (e.g., LT-1) as it conflicts with other SSS stages
            "strategies": [
                // Required
                // Each check processes from top to bottom in sequence, skipping completed strategies
                // If current strategy's tool operators are fully deployed:
                //     If no core, consider strategy complete
                //     If core exists and can be deployed, deploy core and consider strategy complete
                //     If core is converting DP, wait and skip subsequent strategies
                // If current strategy's tool operators aren't fully deployed:
                //     If deployment area has no required tool operators, check next strategy
                //     If deployment area has required tool operators:
                //         If none can be deployed immediately, wait
                //         If some can be deployed immediately, prioritize lower cost ones
                // For strategies at same location:
                //     If no core, allows checking subsequent strategies at same location when needed tool operators unavailable
                //     If core exists, ignores subsequent strategies at same location until current one completes
                // Multiple core operators can be written for same location; non-last core operators act as passthrough after strategy completes
                {
                    "core": "棘刺", // ("棘刺" = "Thorns")
                    "tool_men": {
                        "Pioneer": 1, // Both Chinese and English supported
                        "Warrior": 1,
                        "Medic": 1
                    },
                    "location": [
                        10,
                        1
                    ],
                    "direction": "Left"
                },
                {
                    "core": "泥岩", // ("泥岩" = "Mudrock")
                    "tool_men": {
                        "Pioneer": 1,
                        "Warrior": 1,
                        "Medic": 1
                    },
                    "location": [
                        2,
                        8
                    ],
                    "direction": "Left"
                },
                {
                    // No core field, can be used to deploy auxiliary passthrough operators
                    "tool_men": {
                        "Support": 100
                    },
                    "location": [
                        2,
                        8
                    ],
                    "direction": "Left"
                }
            ],
            "draw_as_possible": true, // "Deploy Operator" button, whether to use when ready, optional, default true
            "actions": [
                // Optional
                // Reuses copywriting logic, refer to protocol/copilot-schema.md
                // Executes action when conditions met, otherwise executes strategies logic above
                {
                    "type": "调配干员" // New type, "Deploy Operator" button, clicks once, ineffective when "draw_as_possible" is true ("调配干员" = "Deploy Operator")
                },
                {
                    "type": "CheckIfStartOver", // New type, checks if operator is present, exits and restarts if not
                    "name": "棘刺" // ("棘刺" = "Thorns")
                },
                {
                    "name": "桃金娘", // ("桃金娘" = "Myrtle")
                    "location": [
                        4,
                        5
                    ],
                    "direction": "左" // ("左" = "Left")
                },
                {
                    "kills": 10,
                    "type": "撤退", // ("撤退" = "Retreat")
                    "name": "桃金娘" // ("桃金娘" = "Myrtle")
                }
            ],
            "retry_times": 3 // Battle failure retry count, abandons entire run if exceeded
        },
        {
            "stage_name": "见者有份" // ("见者有份" = "Share the Spoils")
            // ...
        }
        // Write as many stages as you want to play, e.g., if only up to stage 4 is written, automatically restarts after completing stage 4
    ]
}
```

## Example Files

<https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/>
