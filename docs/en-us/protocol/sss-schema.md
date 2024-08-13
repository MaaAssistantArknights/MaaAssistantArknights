---
order: 7
icon: game-icons:prisoner
---

# Security Presence Schema

This document is machine-translated. If you have the ability, please refer to the Chinese version. We would greatly appreciate any errors or suggestions for improvement.

::: tip
As the JSON format does not support comments, please remove them when using the examples below.
:::

```json
{
    "type": "SSS",                         // Protocol type, SSS means a stationed preservation, required, unchangeable
    "stage_name": "Duos Lore building site", // The name of the preservation location, required
    "minimum_required": "v4.9.0",          // The minimum required version of MAA, required
    "doc": {                               // Description, optional
        "title": "Low-skill high-success-rate operation",
        "title_color": "dark",
        "details": "Low skill requirements balabala... It is suggested to write your name (author's name), reference to video strategy links, etc. here.",
        "details_color": "dark"
    },
    "buff": "Adaptive Supply Component",    // The choice of starting navigation element, optional
    "equipment": [                         // The choice of starting equipment, counted horizontally, optional
                                           // Not implemented in the current version, only displayed on the interface
        "A", "A", "A", "A",
        "B", "B", "B", "B"
    ],
    "strategy": "Optimized strategy",      // Or "Free strategy", optional
                                           // Not implemented in the current version, only displayed on the interface
    "opers": [                             // Specified operator, optional
        {
            "name": "Thorns",
            "skill": 3,
            "skill_usage": 1
        }
    ],
    "tool_men": {                          // The remaining number of personnel of each profession needed, sorted by cost, optional
                                           // Not implemented in the current version, only displayed on the interface
        "Pioneer": 13,
        "Guards": 2,
        "Medic": 2
    },
    "drops": [                             // Priority of recruiting operators and obtaining equipment at the beginning and during the battle
        "Silence",
        "Exusiai",
        "Vanguard",
        "Support",
        "No need to increase the adjustment operator", // Not recruiting anyone
        "Reorganization Navigation Module",            // Supports the name of the equipment, written together
        "Counter Navigation Module",
        "Combat Preparation Actuator",                 // Optional equipment in the middle of the level, also put here
        "Modified Device Signal",
    ],
    "blacklist": [ // Blacklist, optional. These people will not be selected in drops.
                   // In subsequent versions, the formation tool people will not choose these people either.
        "Midnight",
        "Mayer"
    ],
    "stages": [
        {
            "stage_name": "Swarm",          // Name of a single-level stage, required
                                           // Supports name, stageId, levelId, recommended stageId or levelId
                                           // Please do not use codes (e.g. LT-1), that conflict with other preservation stages


            "strategies": [                // Required
                                         // It will deploy the tool_men in each object according to the order.
                                         // If the current hand does not have any tool_men, it will deploy the next object.
                {
                    "core": "Thorns",
                    "tool_men": {
                        "Pioneer": 1,     // Both Chinese and English names are acceptable.
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
                    "core": "Mudrock",
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
                    // If "core" is not provided, it can be used to deploy tool_men that assist with passing.
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
            "draw_as_possible": true,   // "Deploy Operator" button. Whether to use it when available. Optional, default is true.
            "actions": [                // Optional
                                        // It reuses the logic of copying homework.
                                        // If the condition of the action is met, the action is executed. Otherwise, the strategies above are executed.
                {
                    "type": "Deploy Operator" // New type. Click the "Deploy Operator" button. It is invalid when "draw_as_possible" is true.
                },
                {
                    "type": "CheckIfStartOver", // New type. Check if the operator is available. If not, exit and restart.
                    "name": "Thorns"
                },
                {
                    "name": "Firewatch",
                    "location": [
                        4,
                        5
                    ],
                    "direction": "Left"
                },
                {
                    "kills": 10,
                    "type": "Retreat",
                    "name": "Firewatch"
                }
            ],
            "retry_times": 3 // The number of times to retry when the battle fails. If it exceeds this limit, it will give up on the entire stage.
        },
        {
            "stage_name": "Grani and the Knights' Treasure"
            // ...
        }
        // Write as many stages as you want to play. For example, if only 4 is written, it will automatically restart after completing stage 4.
    ]
}
```

## Example

<https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/>
