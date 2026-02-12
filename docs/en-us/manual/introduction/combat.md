---
order: 3
icon: hugeicons:brain-02
---

# Sanity Farming

## General Settings

- The `Use Sanity Potion` + `Use Originium` and `Perform Battles`+ `Material` options work as OR conditions - the task will stop when any of these conditions is met.
  - `Use Sanity Potion` specifies how many times to replenish sanity (may use multiple potions at once).
  - `Use Originium` specifies how many Originium to use (one at a time). Originium won't be used if sanity potions are available.
  - `Perform Battles` specifies the number of battles to complete (e.g., "stop after 15 runs").
  - `Material` specifies how many of a specific material to obtain (e.g., "stop after getting 5 Orirock").

- `Material` and `Stage Selection` are independent options. `Material` only uses the material count as a stopping condition and doesn't automatically navigate to stages that drop that material.
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
  - All main story stages. You can add `-NORMAL` or `-HARD` at the end to switch between normal and challenge modes.
  - Resource stages like CE-6/LS-6 (LMD/EXP). Enter the exact code like `CE-6` or `LS-6`. MAA will automatically switch to level 5 if level 6 isn't available.
  - Skill summary, voucher, and carbon stages (level 5 only). Enter exact codes like `CA-5`, `AP-5`, or `SK-5`.
  - All chip stages. Enter complete stage codes like `PR-A-1`.
  - For Annihilation mode, use these specific values:
    - Current annihilation: Annihilation
    - Chernobog: Chernobog@Annihilation
    - Lungmen Outskirts: LungmenOutskirts@Annihilation
    - Lungmen Downtown: LungmenDowntown@Annihilation

  - Side story stages like `OF-1`, `OF-F3`, and `GT-5`.
  - The last three stages of the current Side Story event. These will be shown at the bottom of the interface after automatically downloading updates from the [API](https://api.maa.plus/MaaAssistantArknights/api/gui/StageActivityV2.json).
  - Rerun Side Story events: Enter `SSReopen-<stage prefix>` to farm all stages XX-1 through XX-9 in sequence, like `SSReopen-IC`.

::: details Example Screen
![Example Screen](/images/zh-cn/combat-start-interface-example.png)
:::

### Annihilation Mode

- MAA navigates to Annihilation using the button at the top-right of the terminal home screen. Ensure your selected Annihilation stage has unlocked `Full Delegation` and that you have enough `PRTS Annihilation Delegation Cards`.
- This feature is only recommended for stages where you've already achieved the 400-kill milestone.

## Advanced Settings

### Alternative Stages

Alternative stages are selected based on daily stage availability - MAA will choose the first available stage in the list.
This functions like a schedule, not as a fallback if the primary stage selection fails.

Example: Alternative Stages are `CE-6/5`, `1-7` and `LS-6/5`:

- If `CE-6/5` is open today, MAA will run it and ignore the alternatives. If you haven't unlocked auto-deploy for CE-6/5, the task will fail.
- If `CE-6/5` is closed today, MAA will run `1-7` instead. If you haven't unlocked auto-deploy for 1-7, the task will fail.
- Since `1-7` is a permanent stage that appears before `LS-6/5` in the list, MAA will never run `LS-6/5` in this scenario.

### Multiplier

MAA will use the specified battle multiplier setting:

- **AUTO mode** (0):
  - Automatically identifies and uses the maximum possible multiplier without wasting sanity

- **Fixed value mode** (1-6):
  - Uses exactly the specified multiplier
  - If current sanity is insufficient for the set multiplier (e.g., only enough for 5× but set to 6×), ends the task

- **Disabled mode** (-1):
  - Doesn't change the in-game multiplier setting
  - If sanity is insufficient for the current in-game multiplier setting, ends the task

### Perform Battles

MAA will run up to the specified number of battles.

Example: Assuming you have 100 sanity and the stage costs 6 sanity:

- If `Perform Battles` is 10 and multiplier is 4: MAA will do 2 runs × 4× multiplier = 8 battles (floor(10/4) × 4 = 8), using 48 sanity. It won't do another 4× run since that would be 12 battles, exceeding the set limit of 10.
- If `Perform Battles` is 10 and multiplier is AUTO: MAA will do one 6× run plus one 4× run = 10 battles (6 + 4 = 10), using 60 sanity.

### Drop Recognition

- Automatically recognizes and counts material drops, uploading data to both [Penguin Statistics](https://penguin-stats.cn/) and [Yituliu](https://ark.yituliu.cn/).
- You can set a custom Penguin Statistics user ID if desired.

## Error Handling

- Automatically checks `Auto Deploy` if it's available.
- Automatically reconnects and continues tasks after disconnections or the daily 4 AM server reset.
- Continues tasks after level-ups.
- If auto-deploy fails, abandons the current operation and retries the battle.
