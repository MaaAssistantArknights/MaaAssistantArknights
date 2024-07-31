---
order: 3
---

# Combat

::: important Translation Required
This page is outdated and maybe still in Simplified Chinese. Translation is needed.
:::

## General Settings

- Fight options include `Use Sanity Potion + Use Originium`, `Perform Battles` and `Material`, you can specific any of them. The fight tasks stops once one of the specifications is met.

  - `Use Sanity Potion` specifies the number of sanity potions to use at most. Multiple medicines may be used at a time.
  - `Use Originium` specifies the number of Originium to use at most. It is used one at a time. The Origin Stone will not be used when there is Sanity Potion in depot.
  - `Perform Battles` specifies the number of battles to perform at most.
  - `Material` specifies the number of materials to collect.

- `Material` and `Stage` are independent options. `Material` is not going to automatically navigate to the the stage for the specified material. You still need to manually configure the stage option.
- `Use Originium` will only be used after `Use Sanity Option`, because MAA will only use Originium to replenish sanity when there are no Sanity Potions left. Therefore, after checking `Use Originium`, MAA will lock the number of `Use Sanity Potion` to 999, making sure to consume all the Sanity Potions to avoid skipping the `Use Originium` judgment.

::: details Example
| Use Sanity Potion | Use Originium | Perform Battles | Material | Result |
| :------: | :----: | :------: | :------: | -------------------------------------------------------------------------------------------------------------------------------------- |
| | | | | Ends after using up the existing Sanity. |
| 2 | | | | Uses up the existing Sanity first, then consumes a Sanity Potion once, totaling `2` times. Ends after using up the Sanity. |
| _999_ | 2 | | | Uses up the existing Sanity and all Sanity Potions, then uses Originium, totaling `2` times. Ends after using up the Sanity. |
| | | 2 | | Ends after performing the selected stage `2` times. |
| | | | 2 | Ends after farming and obtaining `2` of the specified materials. |
| 2 | | 4 | | Ends after performing the selected stage `4` times, using up to `2` Sanity Potions. |
| 2 | | | 4 | Ends after farming and obtaining `4` of the specified materials, using up to `2` Sanity Potions. |
| 2 | | 4 | 8 | Ends after performing the selected stage `4` times, using up to `2` Sanity Potions. However, if `8` of the specified materials are obtained before completing `4` times, it will end early. |
| _999_ | 4 | 8 | 16 | Ends after performing the selected stage `8` times, using up all Sanity Potions and `4` Originium. However, if `16` of the specified materials are obtained before completing `8` times, it will end early. |
| | 2 | | | Uses up the existing Sanity first. If there are Sanity Potions in storage, it ends; if there are no Sanity Potions, it uses `2` Originium and ends after using up the Sanity. _Not a MAA GUI behavior_ |
| 2 | 4 | | | Uses up the existing Sanity first. If after consuming `2` Sanity Potions there are still Sanity Potions left, it ends; if there are no Sanity Potions after consuming ≤`2` Sanity Potions, it continues to use `4` Originium and ends after using up the Sanity. _Not a MAA GUI behavior_ |

:::

### Operations

- If the stage you need is not available in the selection, please choose `Cur/Last` in MAA and manually locate the stage in the game.
  Make sure the screen stays on the stage detail page with the **Start** and **Auto-Deploy** buttons available.
- If you are not on this page, `Cur/Last` will automatically navigate to the last stage played according to the record in the lower right corner of the terminal homepage.
- You can also enable `Manual entry of stage names` in `Combat` - `Advanced` and enter the stage number manually. Currently supported stages include:
  - All main theme stages, where `-NORMAL` or `-HARD` can be added at the end to switch between standard and challenge modes.
  - LMD stages and Battle Record stages 5/6. The input must be `CE-6` or `LS-6` even if you have not unlocked it yet. In that case, the program will automatically switch to corresponding stage 5.
  - Skill Summary, Shop Voucher, and Carbon Stages 5. The input also must be `CA-5`, `AP-5`, and `SK-5` respectively.
  - Chip stages. The input must be complete with stage number, such as `PR-A-1`.
  - Annihilation. The input must be `Annihilation`.
  - Some side story stages, now contains `OF-1`, `OF-F3` and `GT-5`.
  - The last three stages of the current SS event. This is available after downloading updates automatically from the [API](https://ota.maa.plus/MaaAssistantArknights/api/gui/StageActivity.json) when the event is on. Prompt will be shown in the main page when this is available.
  - For the SS event rerun, you can enter `SSReopen-XX` to clear XX-1 ~ XX-9 levels once. Example `SSReopen-IC`.

::: details Example
![Example](/image/en-us/combat-start-interface-example.png)
:::

### Annihilation Mode

- MAA 只会通过终端首页右上角的剿灭按钮进行跳转，请确保当期剿灭已解锁 `全权委托` 并持有足够数量的 “PRTS 剿灭代理卡”。
- 在当期剿灭刷新或重新安装明日方舟后，从剿灭 `开始战斗` 页面返回会自动展开 `情报汇总`，请提前关闭此页面以防止任务卡住。
- 仅建议当期剿灭已“400 杀”的玩家使用 MAA 自动剿灭。

## Advanced Settings

### Alternative Stage

备选关卡根据当天关卡开放情况决定战斗关卡，即选择第一个开放关卡进行战斗。

1. 例子：关卡选择 `龙门币-6/5`，备选选择 `1-7` 和 `经验-6/5`：
   - 如果当天开放 `龙门币-6/5`，就会前往 `龙门币-6/5`，不会前往 `1-7` 和 `经验-6/5`。如果玩家此时未解锁 `龙门币-6/5` 代理，则刷理智任务出错。
   - 如果当天未开放 `龙门币-6/5`，则会前往 `1-7`，不会前往 `龙门币-6/5`。如果玩家此时未解锁 1-7 代理，则刷理智任务出错。
   - 由于 `经验-6/5` 前存在常驻关卡 `1-7`，在这种情况下，MAA 永远也不会前往 `经验-6/5` 战斗。
2. 如关卡选择为 `剿灭模式`，则不会因为剿灭的结果影响其余备选关卡的选择逻辑。即使剿灭出错，刷理智任务也不会出错。

### Remaining Sanity Stage

在 `刷理智` 任务结束后启动，不受吃理智药、吃源石、指定次数、指定材料、连战次数等的控制，刷完理智即结束。

- 适用于在 `关卡选择` 关卡理智不足后，继续前往 `剩余理智` 关卡清理剩余的“边角”理智 (如前往 1-7)。
- 亦适用于在连战次数设置过高而理智不足时自动以单次连战刷光理智收尾 (设置 1-7 连战 6 次，但只有 30 理智，于是自动转为刷 5 次不连战的 1-7)。
- 若剩余理智仍然不足则会结束任务 (如少于 6 理智)。
- 如果剩余理智选择关卡为未开放关卡，则刷理智任务出错。

### Series

- MAA 目前仅会按照用户设定的次数进行连战，尚未支持自动识别最大连战次数。
- 若设置的次数过多但理智不足，MAA 会直接进行 `吃理智药` 或 `吃源石` 操作，并继续尝试连战。
- 若未设置 `吃理智药` 或 `吃源石`，MAA 会直接认为理智不足，终止刷理智任务。若设置了 `剩余理智` ，MAA 会直接开始刷 `剩余理智` 关卡。

### Drop Recognition

- Material drops are automatically recognized and printed to the program log. The data also gets uploaded to [Penguin Stats](https://penguin-stats.io/) and [Yituliu](https://ark.yituliu.cn/).
- You can manually set your Penguin Stats user ID in the settings.

## Abnormal Detection

- `Auto-Deploy` will be automatically selected if not already in case you forget to do so.
- After disconnection or flashing at 4 am, it will automatically reconnect and continue to play the last stage selected in the game. If you need to cross the day, please check the last stage selection.
- A level up situation can be automatically handled as well as a failed delegation in which case this time of the operation will be given up.
