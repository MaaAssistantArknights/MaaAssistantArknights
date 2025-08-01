---
order: 3
icon: hugeicons:brain-02
---

# Combat

::: important This page may be outdated.
:::

## General Settings

- Combat options include `Use Sanity Potion + Use Originium`, `Perform Battles` and `Material`, you can specify any of them. The combat task stops once one of the specifications is met.

  - `Use Sanity Potion` specifies the maximum used amount of sanity potions. Multiple potions may be used at a time.
  - `Use Originium` specifies the maximum used amount of Originium. It is used one at a time. The Originium will not be used when there are Sanity Potions in the depot.
  - `Perform Battles` specifies the number of battles to perform at most.
  - `Material` specifies the number of materials to collect.

- `Material` and `Stage` are independent options. `Material` is not going to automatically navigate to the stage for the specified material. You still need to manually configure the stage option.
- `Use Originium` will only be used after `Use Sanity Option`, because MAA will only use Originium to replenish sanity when there are no Sanity Potions left. Therefore, after checking `Use Originium`, MAA will lock the number of `Use Sanity Potion` to 999, making sure to consume all the Sanity Potions to avoid skipping the `Use Originium` judgment.

::: details Example

| Use Sanity Potion | Use Originium | Perform Battles | Material | Result |
| :------: | :----: | :------: | :------: | -------------------------------------------------------------------------------------------------------------------------------------- |
| | | | | Ends after using up the existing Sanity. |
| 2 | | | | Uses up the existing Sanity first, then consumes a Sanity Potion once, totalling `2` times. Ends after using up the Sanity. |
| _999_ | 2 | | | Uses up the existing Sanity and all Sanity Potions, then uses Originium, totalling `2` times. Ends after using up the Sanity. |
| | | 2 | | Ends after performing the selected stage `2` times. |
| | | | 2 | Ends after farming and obtaining `2` of the specified materials. |
| 2 | | 4 | | Ends after performing the selected stage `4` times, using up to `2` Sanity Potions. |
| 2 | | | 4 | Ends after farming and obtaining `4` of the specified materials, using up to `2` Sanity Potions. |
| 2 | | 4 | 8 | Ends after performing the selected stage `4` times, using up to `2` Sanity Potions. However, if `8` of the specified materials are obtained before completing `4` times, it will end early. |
| _999_ | 4 | 8 | 16 | Ends after performing the selected stage `8` times, using up all Sanity Potions and `4` Originium. However, if `16` of the specified materials are obtained before completing `8` times, it will end early. |
| | 2 | | | Uses up the existing Sanity first. If there are Sanity Potions in storage, it ends; if there are no Sanity Potions, it uses `2` Originium and ends after using up the Sanity. _Not a MAA GUI behavior_ |
| 2 | 4 | | | Uses up the existing Sanity first. If there are still some left after consuming `2` Sanity Potions, it ends; if there are no Sanity Potions after consuming ≤`2` Sanity Potions, it continues to use `4` Originium and ends after using up the Sanity. _Not a MAA GUI behavior_ |

:::

### Operations

- If the stage you need is unavailable in the selection, please choose `Cur/Last` in MAA and manually locate the stage in the game.
  Ensure the screen stays on the stage detail page with the **Start** and **Auto-Deploy** buttons available.
- If you are not on this page, `Cur/Last` will automatically navigate to the last stage played according to the record in the lower right corner of the terminal homepage.
- You can also enable `Manual entry of stage names` in `Combat` - `Advanced` and enter the stage number manually. Currently supported stages include:
  - All main theme stages, where `-NORMAL` or `-HARD` can be added at the end to switch between standard and challenge modes.
  - LMD stages and Battle Record stages 5/6. The input must be `CE-6` or `LS-6` even if you have not unlocked it yet. In that case, the program will automatically switch to the corresponding stage 5.
  - Skill Summary, Shop Voucher, and Carbon Stages 5. The input also must be `CA-5`, `AP-5`, and `SK-5` respectively.
  - Chip stages. The input must be complete with stage number, such as `PR-A-1`.
  - The Annihilation mode supports the following values:

    - Current Annihilation: Annihilation
    - Chernobog: Chernobog@Annihilation
    - Lungmen Outskirts: LungmenOutskirts@Annihilation
    - Lungmen Downtown: LungmenDowntown@Annihilation

  - Some side story stages, now contains `OF-1`, `OF-F3` and `GT-5`.
  - The last three stages of the current SS event. This is available after downloading updates automatically from the [API](https://api.maa.plus/MaaAssistantArknights/api/gui/StageActivity.json) when the event is on. The prompt will be shown on the main page when this is available.
  - For the SS event rerun, you can enter `SSReopen-XX` to clear XX-1 ~ XX-9 levels once. Example `SSReopen-IC`.

::: details Example
![Example](/images/en-us/combat-start-interface-example.png)
:::

### Annihilation Mode

- MAA navigates via the Annihilation button at the top-right of the terminal homepage. Ensure the selected Annihilation stage has unlocked `Full Delegation` and that you have enough "PRTS Proxy Annihilation Cards".
- It is only recommended to use automatic Annihilation for stages where you have already achieved 400 kills.

## Advanced Settings

### Alternative Stage

The alternative stage is determined based on the availability of stages on the given day, i.e., the first available stage will be selected for battle.
This is a function similar to a schedule and cannot be used as a backup stage when the stage selection task fails.

1. Example: Stage selection `CE-6/5`, alternatives `1-7` and `LS-6/5`:
   - If `CE-6/5` is available on the day, it will proceed to `CE-6/5` and not to `1-7` or `LS-6/5`. If the player has not unlocked the `CE-6/5` mission node, the sanity farming task will fail.
   - If `CE-6/5` is not available on the day, it will proceed to `1-7` and not to `CE-6/5`. If the player has not unlocked the 1-7 mission node, the sanity farming task will fail.
   - Since `1-7` is a permanent stage before `LS-6/5`, MAA will never proceed to `LS-6/5` in this scenario.
2. If the stage selection is `Annihilation Mode`, then:
   - The selection logic of the remaining backup stages will not be affected by the result of the Annihilation, even if the Annihilation fails, the sanity farming task will not fail.
   - The remaining backup stages will only inherit the settings of `Use Sanity Potion` and `Series`, and will not be controlled by `Use Originium`, `Perform Battles`, or `Material`.

### Remaining Sanity

It starts after the `Combat` task ends and is not controlled by `Use Sanity Potion`, `Use Originium`, `Perform Battles`, `Material`, or `Series`. It ends when the current remaining sanity is exhausted.

- Suitable for clearing the remaining "corner" sanity (e.g., going to 1-7) after sanity is insufficient in the `Stage Selection` stage.
- Also suitable for automatically using single battles to use up sanity when the set number of consecutive battles is too high and sanity is insufficient (e.g., setting 1-7 to battle 6 times, but only having 30 sanity, thus automatically switching to 5 single battles of 1-7).
- If the remaining sanity is still insufficient, the task will end (e.g., less than 6 sanity).
- If the selected stage for remaining sanity is unopened, the sanity farming task will fail.

### Series

MAA will fight according to the number of consecutive battles set by the user:

- **AUTO mode** (0):
  - Automatically identify the maximum number of consecutive battles in the level, maintain the maximum number of consecutive battles and do not overflow sanity
  - Enter the `Remaining Sanity` process after completion (if set)

- **Numerical mode** (1~6):
  - Perform consecutive battles according to the set number of times
  - If the current sanity is not enough to complete the set number of times (such as only 5 times but set to 6 times), it will directly end the task and enter the `Remaining Sanity` process (if set)

- **Disable mode** (-1):
  - Do not adjust the number of consecutive battles in the game
  - If the sanity is not enough to complete the current set number of times in the game, directly end the task and enter the `Remaining Sanity` process (if set)

### Perform Battles

MAA will execute a maximum of the specified number of battles

Example: Assuming the current sanity is 100, and the level consumes 6 sanity

- Set the `Perform Battles` to 10, and the proxy multiplier to 4: 2 (start actions) x 4 (multiple proxy) = 8 operations will be executed (2 x floor(10 / 4) = 8), consuming 8 x 6 = 48 sanity. At this time, if another 4x proxy is performed, it will reach 12 operations, which exceeds the set 10 times, so it will not be performed again, and the mission will end with 8 operations

- Set the `Perform Battles` to 10, and the proxy multiplier to AUTO: 1 6x proxy + 1 4x proxy = 10 operations will be executed (6 * floor(10 / 6) + (10 % 6) = 10), consuming 10 x 6 = 60 sanity

### Drop Recognition

- Material drops are automatically recognized and printed to the program log. The data also gets uploaded to [Penguin Stats](https://penguin-stats.io/) and [Yituliu](https://ark.yituliu.cn/).
- You can manually set your Penguin Stats user ID in the settings.

## Abnormal Detection

- `Auto-Deploy` will be automatically selected if not already in case you forget to do so.
- After disconnection or forced server reset at 4 am, it will automatically reconnect and continue to play the last stage selected in the game. If you need to cross the day, please check the last stage selection.
- An account level-up situation can be automatically handled as well as a failed operation in which case the operation will be abandoned and the battle will be retried.
