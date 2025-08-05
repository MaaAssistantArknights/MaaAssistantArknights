---
order: 7
icon: ri:game-fill
---

# Auto Integrated Strategy

MAA selects the latest theme by default, and can be changed in `Auto I.S.` - `I.S. Theme`.

::: warning
All features involving auto-battle require a stable game frame rate of at least 60 frames, including but not limited to Copilot and Auto I.S..
:::

- Please pin the corresponding I.S. theme to the terminal of the game. Although automatic navigation is currently available, long-term usability is not guaranteed.
- Please manually end explorations of **non-target themes** in advance to ensure the target theme can start exploration.
- You can select theme, difficulty, squad, starting operator (single operator name only), etc. in settings.
- MAA can start with a support operator. First fill in the target operator name in `General Settings` → `Starting Operator`, then enable `Advanced Settings` → `Use support unit for "Starting Operator"`.
- When playing for the **first time** on a newly installed client:
  - The difficulty selection button will not be displayed. Please manually select difficulty once in-game, after which MAA can automatically select it.
  - Tutorial popups will appear after entering the overworld map. Please manually close them after reading.

## Recommended Starting Strategies

::: details _Last updated 2025/8/2_

| Theme   | Difficulty        | Squad                                   | Strategy                   | Operators |
| ------- | ----------------- | --------------------------------------- | -------------------------- | --------- |
| Phantom | Formal Investigation·3~7 | Leader Squad / Tactical Assault Squad | Overcoming Your Weaknesses | Thorns |
| Mizuki  | Surging Waves·3~7 | People-Oriented Squad / Mind Over Matter Squad | Slow and Steady Wins the Race | Wiš'adel |
| Sami    | Braving Nature·4~10 | Special Training Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |
| Sarkaz  | Face the Spirit·4~10 | Blueprint Mapping Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |
| Garden of Wandering | Enter the Garden·3~10 | Leader Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |

The recommended difficulty comprehensively considers `Enemy Difficulty`, `Hope Consumption`, `Score Multiplier` and other factors. It is relatively stable when tested at high practice levels and can be freely adjusted according to actual conditions and needs.

| Theme   | Notes |
| ------- | ----- |
| Phantom | `Formal Investigation·4` and higher difficulties may obtain hope-reducing collectibles at the start, making it impossible to recruit 6-star operators at the beginning. |
| Mizuki  | `Surging Waves·4` and higher difficulties increase 6-star operator recruitment hope cost by +1. Using `Mind Over Matter Squad` at the start may make it impossible to recruit 6-star operators.<br>`People-Oriented Squad` is suitable for accounts with high practice levels, `Mind Over Matter Squad` requires luck. |
| Sami    | `Braving Nature·6` and higher difficulties increase 6-star operator recruitment hope cost by +1. Using `Special Training Squad` at the start may make it impossible to recruit 6-star operators. |
| Sarkaz  | `Face the Spirit·15` and higher difficulties increase 6-star operator recruitment hope cost by +1. Using `Blueprint Mapping Squad` at the start may make it impossible to recruit 6-star operators.<br>If `Blueprint Mapping Squad` is selected, an avoidance strategy will be adopted, which can quickly obtain `Spirit Bookmarks` but basically cannot clear endings.<br>When using the Originium Ingot farming strategy with `Point-Stab Ingot Squad` as the starting squad, a shop refresh strategy will be used to accelerate the process. |
| Garden of Wandering | `Enter the Garden·15` and higher difficulties increase 6-star operator recruitment hope cost by +1. Using `Leader Squad` at the start may make it impossible to recruit 6-star operators.<br>When the difficulty is set to `Enter the Garden·3` and using the Originium Ingot farming strategy with `Leader Squad` as the starting squad, the `End of Time` stage-skip strategy will be used to accelerate the process. |

:::

## Battle Strategy

All operations in MAA's I.S. automation use preset strategies, and all stage battles call built-in job files. When the corresponding stage job does not exist, a not-very-smart general decision tree will be used for battle.

For details, please refer to [I.S. Schema](../../protocol/integrated-strategy-schema.md).

- Supports automatic recognition of operators and proficiency, automatically selecting better operators and skills.
- Supports identifying shop items and prioritizing purchase of more powerful collectibles.

## Abnormal Detection

- Supports automatic reconnection after disconnection or 4 AM server reset and continues tasks.
- Combat lasting longer than 5 minutes will automatically retreat all ground units, longer than 6 minutes will automatically abandon the current battle to avoid mutual scraping and time waste.
- If tasks encounter problems, will automatically abandon the current exploration and retry.

However, if it frequently gets stuck in the same place, please submit an Issue with logs and screenshots for feedback.
