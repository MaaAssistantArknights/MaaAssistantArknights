---
order: 7
icon: ri:game-fill
---

# Auto Integrated Strategy

MAA selects the latest theme by default, which can be changed in `Auto Integrated Strategy` - `Integrated Strategy Theme`.

::: warning
All features involving Copilot require the following prerequisites, including but not limited to Copilot and Auto I. S.

- At least 60 frames of **stable** framerate
- Touch mode set to Minitouch or MaaTouch

:::

- Please pin the corresponding Integrated Strategy theme at the terminal in-game. Although automatic navigation is currently possible, long-term functionality is not guaranteed.
- Please manually end explorations of **non-target themes** in advance to ensure the target theme is ready to start exploration.
- You can select theme, difficulty, squad, starting operator (single operator name only), and more in settings.
- MAA can use support operators to start, by first entering the target operator name in `General Settings` → `Starting Operator` and then enabling `Advanced Settings` → `Use support unit for "Starting Operator"`.
- When playing for the **first time** in a newly installed client:
  - The difficulty selection button will not be displayed. Please manually select a difficulty once in-game, after which MAA can select it automatically.
  - Tutorials will appear after entering the map screen. Please read and close them manually.

## Recommended Starts

::: details _Last updated on 2025/8/30_

| Theme   | Difficulty             | Squad                                           | Strategy                      | Operator |
| ------- | ---------------------- | ----------------------------------------------- | ----------------------------- | -------- |
| Phantom | Formal Investigation·2 | Leader Squad / Tactical Assault Squad           | Overcoming Your Weaknesses    | Thorns   |
| Mizuki  | Surging Waves·3~7      | People-Oriented Squad / Mind Over Matter Squad  | Slow and Steady Wins the Race | Wiš'adel |
| Sami    | Braving Nature·4~10    | Special Training Squad / Tactical Ranged Squad  | Slow and Steady Wins the Race | Wiš'adel |
| Sarkaz  | Facing Souls·4~10      | Blueprint Mapping Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |
| Garden  | Guided Tour·3~10       | Leader Squad / Tactical Ranged Squad            | Slow and Steady Wins the Race | Wiš'adel |

The recommended difficulties consider factors like `enemy difficulty`, `hope consumption`, and `score multiplier`, and have been tested to be stable with high-level operators. Feel free to adjust based on your situation and needs.

| Theme   | Notes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Phantom | `Formal Investigation·3` and higher difficulties may offer hope-reducing collectibles at start, preventing recruitment of six-star operators.                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| Mizuki  | `Surging Waves·4` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Mind Over Matter Squad`.<br>`People-Oriented Squad` is better for high-level accounts, while `Mind Over Matter Squad` requires luck.                                                                                                                                                                                                                                                                                                                                       |
| Sami    | `Braving Nature·6` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Special Training Squad`.                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Sarkaz  | `Facing Souls·15` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Blueprint Mapping Squad` unless `Blueprint Mapping Squad Enhancement Ⅱ` has been activated in `Historical Reconstruction`.<br>When `Blueprint Mapping Squad` is selected, MAA will use a combat-avoidance strategy to quickly collect `Soul Bookmarks`, though it typically makes it impossible to witness an ending.<br>When the Originium Ingot farming strategy and the `Ingots Squad` are selected, MAA will use the shop refreshing strategy to speed up the process. |
| Garden  | `Guided Tour·15` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Leader Squad`.<br>When `Guided Tour·3` difficulty, the Originium Ingot farming strategy and `Leader Squad` are selected, MAA will use the `End of Time` stage-skip strategy to speed up the process.                                                                                                                                                                                                                                                                        |

:::

## Combat Strategy

All MAA operations in Integrated Strategy use preset strategies, with all combat stages using built-in operation files. When no operation exists for a specific stage, a basic (not very intelligent) decision tree handles combat.

For details, see the [Integrated Strategy Protocol](../../protocol/integrated-strategy-schema.md).

- Automatically recognizes operators and their levels, selecting optimal operators and skills.
- Recognizes shop items and prioritizes purchasing more powerful collectibles.

## Exception Detection

- If a task encounters problems, it will automatically abandon the current exploration and retry.
- If battle duration exceeds 5 minutes, all ground units will automatically retreat; if it exceeds 6 minutes, the current battle will be automatically abandoned to avoid time-consuming stalemates.
- To automatically reconnect and continue tasks after intermittent disconnections or updates, please also enable the Start Wake-Up Task and set the corresponding time for forced scheduled start.

If the program gets stuck at the same location multiple times, please submit an Issue with logs and screenshots.
