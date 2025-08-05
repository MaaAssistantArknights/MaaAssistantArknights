---
order: 7
icon: ri:game-fill
---

# Auto Integrated Strategy

MAA selects the latest theme by default, which can be changed in `Auto Integrated Strategy` - `Integrated Strategy Theme`.

::: warning
All features involving automatic combat require a stable game frame rate of at least 60 FPS, including but not limited to Auto Combat and Integrated Strategy.
:::

- Please pin the corresponding Integrated Strategy theme at the terminal in-game. Although automatic navigation is currently possible, long-term functionality is not guaranteed.
- Please manually end explorations of **non-target themes** in advance to ensure the target theme is ready to start exploration.
- You can select theme, difficulty, squad, starting operator (single operator name only), and more in settings.
- MAA can use support operators to start, by first entering the target operator name in `General Settings` → `Starting Operator` and then enabling `Advanced Settings` → `Use support unit for "Starting Operator"`.
- When playing for the **first time** in a newly installed client:
  - The difficulty selection button will not be displayed. Please manually select a difficulty once in-game, after which MAA can select it automatically.
  - Tutorials will appear after entering the map screen. Please read and close them manually.

## Recommended Starts

::: details _Last updated 2025/8/2_

| Theme   | Difficulty        | Squad                                   | Strategy            | Operator  |
| ------- | ----------------- | --------------------------------------- | ------------------------- | --------- |
| Phantom | Formal Investigation·3~7 | Leader Squad / Tactical Assault Squad | Overcoming Your Weaknesses | Thorns |
| Mizuki  | Surging Waves·3~7 | People-Oriented Squad / Mind Over Matter Squad | Slow and Steady Wins the Race | Wiš'adel |
| Sami    | Braving Nature·4~10 | Special Training Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |
| Sarkaz  | Face the Spirit·4~10 | Blueprint Mapping Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |
| Garden  | Enter the Garden·3~10 | Leader Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |

The recommended difficulties consider factors like `enemy difficulty`, `hope consumption`, and `score multiplier`, and have been tested to be stable with high-level operators. Feel free to adjust based on your situation and needs.

| Theme   | Notes |
| ------- | ----- |
| Phantom | `Formal Investigation·4` and higher difficulties may give hope-reducing collectibles at start, preventing recruitment of six-star operators. |
| Mizuki  | `Surging Waves·4` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Mind Over Matter Squad`.<br>`People-Oriented Squad` is better for high-level accounts, while `Mind Over Matter Squad` requires luck. |
| Sami    | `Braving Nature·6` and higher difficulties increase six-star operator hope cost by +1, potentially making it impossible to recruit them with `Special Training Squad`. |
| Sarkaz  | `Face the Spirit·15` and higher difficulties increase six-star operator hope cost by +1, potentially making it impossible to recruit them with `Blueprint Mapping Squad`.<br>With `Blueprint Mapping Squad`, MAA will adopt an avoidance strategy to quickly obtain `Spirit Bookmarks`, but generally cannot complete endings.<br>When using the Originium Ingot farming strategy with `Point-Stab Ingot Squad`, MAA will use shop refreshing to speed up the process. |
| Garden  | `Enter the Garden·15` and higher difficulties increase six-star operator hope cost by +1, potentially making it impossible to recruit them with `Leader Squad`.<br>When set to `Enter the Garden·3` difficulty with the Originium Ingot farming strategy and `Leader Squad`, MAA will use the `End of Time` stage-skip strategy to speed up the process. |

:::

## Combat Strategy

All MAA operations in Integrated Strategy use preset strategies, with all combat stages using built-in operation files. When no operation exists for a specific stage, a basic (not very intelligent) decision tree handles combat.

For details, see the [Integrated Strategy Protocol](../../protocol/integrated-strategy-schema.md).

- Automatically recognizes operators and their levels, selecting optimal operators and skills.
- Recognizes shop items and prioritizes purchasing more powerful collectibles.

## Exception Handling

- Automatically reconnects and continues tasks after disconnections or the 4 AM server reset.
- For battles exceeding 5 minutes, automatically retreats all ground units; after 6 minutes, abandons the battle to avoid time-consuming deadlocks.
- If a task encounters problems, automatically abandons the current exploration and retries.

If the program repeatedly gets stuck at the same point, please submit an Issue with logs
