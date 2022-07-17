# Integrated Strategy (I.S.) Schema

Usage of `resource/roguelike_copilot.json` and description of each field

## Overview

```jsonc
[
    {
        "stage_name": "驯兽小屋",       // Stage name, required
        "replacement_home": [          // Replace the position of home, optional, empty by default
            [ 2, 8 ]                   // The coordinates of the home
        ],
        "key_kills": [                 // The number of key kills, optional, empty by default
            15,                        // If this value is not empty, the operators will keep their skills and cast them when the number of kills reach the value, instead of casting when ready.
            ...                        // When all elements are processed, operators will cast skills when they are ready.
        ]
    },
    {
        "stage_name": "巡逻队",
        "replacement_home": [
            [ 2, 8 ]
        ]
    }
]
```
