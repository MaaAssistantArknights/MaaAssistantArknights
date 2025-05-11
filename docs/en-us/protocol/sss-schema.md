---
order: 7
icon: game-icons:prisoner
---

# Stationary Security Service Schema

::: tip
Please note that JSON files do not support comments. The comments in the text are for demonstration purposes only and should NOT be copied directly.
:::

```json
{
    "type": "SSS", // Protocol type, SSS represents Contingency Contract, required, cannot be modified
    "stage_name": "Dossoles Under Construction", // Contingency Contract map name, required
    "minimum_required": "v4.9.0", // Minimum required MAA version, required
    "doc": {
        // Description, optional
        "title": "Low-investment high-success-rate strategy",
        "title_color": "dark",
        "details": "This strategy has very low level requirements balabala...", // Suggestion: write your name (author), reference video links, etc. here
        "details_color": "dark"
    },
    "buff": "Adaptive Supply Component", // Initial power component selection, optional
    "equipment": [
        // Initial equipment selection, counted horizontally, optional
        // Not implemented in current version, will only display in the interface
        "A",
        "A",
        "A",
        "A",
        "B",
        "B",
        "B",
        "B"
    ],
    "strategy": "Optimal Strategy", // Or "Free Strategy", optional
    // Not implemented in current version, will only display in the interface
    "opers": [
        // Specific operators, optional
        {
            "name": "Thorns",
            "skill": 3,
            "skill_usage": 1
        }
    ],
    "tool_men": {
        // Remaining required operators by class, selected by cost order, optional
        // Not implemented in current version, will only display in the interface
        "Pioneer": 13,
        "Guard": 2, // Both Chinese and English accepted
        "Medic": 2
    },
    "drops": [
        // Priority for recruiting operators and acquiring equipment at battle start and during battle
        "Archetto",
        "Exusiai", // Supports operator names, class names
        "Vanguard", // Class names in both Chinese and English
        "Support",
        "No Additional Operators Needed", // Don't recruit anyone
        "Reorganization Power Component", // Supports equipment names
        "Counter Power Component",
        "Combat Ready Valve", // Mid-stage optional equipment, also listed here
        "Reassignment Transmitter"
    ],
    "blacklist": [
        // Blacklist, optional. These operators won't be selected in drops.
        // In future versions with squad formation support, these operators won't be selected as utility operators either
        "Nightmare",
        "Mayer"
    ],
    "stages": [
        {
            "stage_name": "Swarming", // Individual stage name, required
            // Supports name, stageId, levelId; stageId or levelId recommended
            // Please don't use code (e.g., LT-1) as it may conflict with other CC stages
            "strategies": [
                // Required
                // Each check proceeds from top to bottom sequentially, skipping completed strategies
                // If utility operators for current strategy are fully deployed:
                //     If no core, consider strategy complete
                //     If core exists and can be deployed, deploy core and consider strategy complete
                //     If core is generating DP, wait and skip subsequent strategies
                // If utility operators for current strategy are not fully deployed:
                //     If deployment area has no required utility operators, check next strategy
                //     If deployment area has required utility operators:
                //         If none can be immediately deployed, wait
                //         If some can be immediately deployed, prioritize lowest cost
                // For strategies at the same position:
                //     If no core, allow checking subsequent strategies at same position when deployment area lacks required utility operators (regardless of DP)
                //     If core exists, ignore subsequent strategies at same position until complete
                // Multiple core operators can be specified for same position; non-final core operators effectively pass turn after their strategy completes (not counted as utility operators)
                {
                    "core": "Thorns",
                    "tool_men": {
                        "Pioneer": 1, // Both Chinese and English accepted
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
                    // Not specifying core can be used to deploy utility operators for support
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
            "draw_as_possible": true, // "Deploy Operator" button, whether to use as soon as available, optional, default is true
            "actions": [
                // Optional
                // Reuses the Copilot logic, see protocol/copilot-schema.md
                // If action condition is met, execute action; otherwise, execute strategies logic above
                {
                    "type": "Deploy Operator" // New type, "Deploy Operator" button, clicks once; ineffective when "draw_as_possible" is true
                },
                {
                    "type": "CheckIfStartOver", // New type, checks if operator is present, restarts if not
                    "name": "Thorns"
                },
                {
                    "name": "Myrtle",
                    "location": [
                        4,
                        5
                    ],
                    "direction": "Left"
                },
                {
                    "kills": 10,
                    "type": "Retreat",
                    "name": "Myrtle"
                }
            ],
            "retry_times": 3 // Number of retries on battle failure, abandons entire run if exceeded
        },
        {
            "stage_name": "Sharing the Spoils"
            // ...
        }
        // The script will play as many stages as you define; for example, if only 4 stages are defined, it will automatically restart after stage 4
    ]
}
```

## Example Files

<https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/>
