---
order: 3
icon: hugeicons:brain-02
---

# Sanity Farming

::: info UI-Only Feature
Some features on this page are implemented by the UI layer (such as Target Inventory, Weekly Schedule, and multi-task ordering). See [Getting Started](../newbie.md#about-this-documentation) for details.
:::

## General Settings

- The `Use Sanity Potion` + `Use Originium` and `Perform Battles`+ `Material` options work as OR conditions - the task will stop when any of these conditions is met.
  - `Use Sanity Potion` specifies how many times to replenish sanity (may use multiple potions at once).
  - `Use Originium` specifies how many Originium to use (one at a time). Originium won't be used if sanity potions are available.
  - `Perform Battles` specifies the number of battles to complete (e.g., "stop after 15 runs").
  - `Material` specifies how many of a specific material to obtain (e.g., "stop after getting 5 Orirock"), with two counting modes:
    - `Drop Quantity`: Counts the number of that material dropped during this task.
    - `Target Inventory`: References the depot data saved in [Depot Recognition](./tools.md#depot-recognition) and only farms up to the set inventory level. Requires depot data obtained via [Update Doctor Data](./user-data-update.md) or [Depot Recognition](./tools.md#depot-recognition). This mode is implemented by the UI (Core only supports drop quantity mode).

- `Material` and `Stage Selection` are independent options. `Material` only uses the material count as a stopping condition and doesn't automatically navigate to stages that drop that material.
- The `Target Inventory` mode shares the start-time check with [Depot Maintain](./depot-maintain.md): when the task starts, the shortfall is recalculated with the latest depot data, and the task is skipped entirely if already reached. It is also skipped if some fight in this queue run has reported sanity and the current sanity estimated from the report time (1 point per 6 minutes, fractions of 6 minutes count as a full 6 minutes, capped at the sanity limit) is below the stage's minimum entry cost, the task has no potion/Originium budget, and expiring potions are used up (proof conditions: [Depot Maintain · Use expiring sanity potions within 48 hours](./depot-maintain.md#use-expiring-sanity-potions-within-48-hours)). Neither case enters the stage.
- To manage multiple material inventory targets at once, use the [Depot Maintain](./depot-maintain.md) task, which supports multiple plans farmed in sequence.
- `Use Originium` is only checked after `Use Sanity Potion`. Since MAA only uses Originium when no sanity potions remain, checking `Use Originium` will automatically set `Use Sanity Potion` to 999, ensuring all potions are used first.

::: details Examples

| Use Sanity Potion | Use Originium | Perform Battles | Material | Result                                                                                                                                                                            |
| :---------------: | :-----------: | :-------------: | :------: | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|                   |               |                 |          | Uses current sanity and stops.                                                                                                                                                    |
|         2         |               |                 |          | Uses current sanity, then uses sanity potions up to 2 times, then stops.                                                                                                          |
|       _999_       |       2       |                 |          | Uses current sanity, then all sanity potions, then Originium up to 2 times, then stops.                                                                                           |
|                   |               |        2        |          | Runs the selected stage 2 times, then stops.                                                                                                                                      |
|                   |               |                 |    2     | Farms until 2 of the specified material are obtained, then stops.                                                                                                                 |
|         2         |               |        4        |          | Runs the selected stage up to 4 times, using up to 2 sanity potions if needed, then stops.                                                                                        |
|         2         |               |                 |    4     | Farms until 4 of the specified material are obtained, using up to 2 sanity potions if needed, then stops.                                                                         |
|         2         |               |        4        |    8     | Runs the selected stage up to 4 times, using up to 2 sanity potions if needed. Stops early if 8 of the specified material are obtained before reaching 4 runs.                    |
|       _999_       |       4       |        8        |    16    | Runs the selected stage up to 8 times, using all sanity potions and up to 4 Originium if needed. Stops early if 16 of the specified material are obtained before reaching 8 runs. |
|                   |       2       |                 |          | Uses current sanity, then stops if any sanity potions are available. If no potions, uses up to 2 Originium. _Not MAA GUI behavior_                                                |
|         2         |       4       |                 |          | Uses current sanity, then up to 2 sanity potions. If potions remain, stops; if no potions remain after using ≤2 potions, uses up to 4 Originium. _Not MAA GUI behavior_           |

:::

### Stage Selection

- If your desired stage isn't in the selection menu, choose `Current/Last` in MAA, then manually navigate to the stage in-game.
  Ensure you're on the stage details screen with the stage name and remaining sanity in the upper right and auto-deploy/start buttons in the lower right.
- If you're not on this screen, `Current/Last` will automatically enter the "last operation" stage shown on the bottom right of the terminal home screen.
- You can also enable `Manual entry of stage names` in `Task Settings` - `Sanity Farming` - `Advanced Settings` to manually input stage codes. Currently supported stages include:
  - All main story stages. You can add `-NORMAL` or `-HARD` at the end to switch difficulty: Chapters 10-14 map to Standard/Adverse, while Chapters 15+ map to Normal/Raid.
  - Resource stages like CE-6/LS-6 (LMD/EXP). Enter the exact code like `CE-6` or `LS-6`. MAA will automatically switch to level 5 if level 6 isn't available.
  - Skill summary, voucher, and carbon stages (level 5 only). Enter exact codes like `CA-5`, `AP-5`, or `SK-5`.
  - All chip stages. Enter complete stage codes like `PR-A-1`.
  - For Annihilation mode, use these specific values:
    - Current annihilation: Annihilation
    - Chernobog: Chernobog@Annihilation
    - Lungmen Outskirts: LungmenOutskirts@Annihilation
    - Lungmen Downtown: LungmenDowntown@Annihilation

  - Side story stages `OF-1` and `OF-F3`.
  - The last three stages of the current Side Story event. These will be shown at the bottom of the interface after automatically downloading updates from the [API](https://api.maa.plus/MaaAssistantArknights/api/gui/StageActivityV2.json).
  - Rerun Side Story events: Enter `SSReopen-<stage prefix>` to farm all stages XX-1 through XX-9 in sequence, like `SSReopen-IC`.

::: details Example Screen
![Example Screen](/images/zh-cn/combat-start-interface-example.png)
:::

### Annihilation Mode

- MAA navigates to Annihilation using the button at the top-right of the terminal home screen. Ensure your selected Annihilation stage has unlocked `Full Delegation` and that you have enough `PRTS Annihilation Delegation Cards`.
- This feature is only recommended for stages where you've already achieved the 400-kill milestone.
- If the Annihilation entry cannot be found during navigation, the weekly Annihilation is treated as completed and the task ends immediately (not counted as a failure).
- During navigation, MAA checks whether `Full Delegation` (sweep) is available. If not, the task fails and exits.
- During runtime, MAA does **not** continuously check whether sweep tickets are sufficient. If tickets run out mid-run, later battles may fall back to normal auto-deploy and take much longer.
- At settlement, MAA recognizes the weekly Orundum progress (e.g. `1800 / 1800`) and automatically stops when the weekly cap is reached.
- Annihilation is a permanent stage: if it is selected in stage selection / alternative stages, later alternative stages will not continue to be recognized or run.
- Annihilation drops are not uploaded to Penguin Statistics or Yituliu.
- To run Annihilation first, add a separate Sanity Farming task with only Annihilation selected, and drag it above your existing Sanity Farming task. You can enable Weekly Schedule in Advanced Settings and check only Monday so it runs on Mondays only.

## Advanced Settings

### Alternative Stages

Alternative stages are selected based on daily stage availability - MAA will choose the first available stage in the list.
This functions like a schedule, not as a fallback if the primary stage selection fails.

Example: Alternative Stages are `CE-6/5`, `1-7` and `LS-6/5`:

- If `CE-6/5` is open today, MAA will run it and ignore the alternatives. If you haven't unlocked auto-deploy for CE-6/5, the task will fail.
- If `CE-6/5` is closed today, MAA will run `1-7` instead. If you haven't unlocked auto-deploy for 1-7, the task will fail.
- Since `1-7` is a permanent stage that appears before `LS-6/5` in the list, MAA will never run `LS-6/5` in this scenario.
- Likewise, if a permanent stage such as `Annihilation` is selected in the alternatives, later stages will not continue to be recognized.

### Weekly Schedule

- Enable it under `Task Settings` - `Sanity Farming` - `Advanced Settings`. This feature is implemented by the UI.
- After enabling, you can check which **in-game weekdays** (Sunday–Saturday) this Sanity Farming task should run.
- The weekday is calculated from in-game time (client timezone + daily 4:00 reset), not the local calendar day. For example, on CN servers, 3:59 local time still counts as the previous day.
- When starting tasks: if Weekly Schedule is enabled and today is not checked, this Sanity Farming task is **skipped** (log shows task skipped; not a failure), and later tasks continue.
- If Weekly Schedule is not enabled, the task is attempted every day.
- Enabling Weekly Schedule turns off and disables “Hide today's not open stages”.
- Typical use: add a separate Sanity Farming task for Annihilation only, check only Monday in Weekly Schedule, and it will run on Mondays only. See [Annihilation Mode](#annihilation-mode) above.

### Series

MAA will use the specified Series setting:

- **AUTO mode** (0):
  - Automatically selects a multiplier based on remaining battle count (capped at the stage's maximum); reduces medicine usage to avoid sanity overflow, while Originium is used one at a time per setting
  - If sanity is insufficient for a full run of that multiplier, recovers sanity as configured (medicines first, then Originium); ends the task if none are set or exhausted

- **Fixed value mode** (1-10 for CN, 1-6 for overseas servers):
  - Uses exactly the specified multiplier
  - If current sanity is insufficient for a full run of the set multiplier (e.g., only enough for 5× but set to 6×), recovers sanity as configured (medicines first, then Originium); ends the task if none are set or exhausted

- **Disabled mode** (-1):
  - Doesn't change the in-game multiplier setting
  - If sanity is insufficient for a full run of the current in-game multiplier, recovers sanity as configured (medicines first, then Originium); ends the task if none are set or exhausted

### Perform Battles

MAA will run up to the specified number of battles. Actual battle count is floored to whole series runs: `floor(Perform Battles / series) × series`. It will not split a partial series just to fill the remaining count (except AUTO, see below).

Example: Assuming you have 100 sanity, the stage costs 6 sanity, and the stage max series is 10:

- If `Perform Battles` is 12 and series is 5: MAA will do 2 start-ops × 5× = 10 battles (`floor(12 / 5) × 5 = 10`), using 10 × 6 = 60 sanity. Another 5× run would reach 15 and exceed 12, so it stops at 10.
- If `Perform Battles` is 12 and series is AUTO: first take the smaller of remaining count and current max available series, run 10× for 10 battles; then run 2× for the remaining 2, totaling 12 battles and using 12 × 6 = 72 sanity.

### Drop Recognition

- Automatically recognizes and counts material drops, uploading data to both [Penguin Statistics](https://penguin-stats.cn/) and [Yituliu](https://ark.yituliu.cn/).
- You can set a custom Penguin Statistics user ID if desired.

## Error Handling

- Automatically checks `Auto Deploy` if it's available.
- Automatically reconnects and continues tasks after disconnections or the daily 4 AM server reset.
- Continues tasks after level-ups.
- If auto-deploy fails, abandons the current operation and retries the battle.
