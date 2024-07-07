---
order: 4
---

# Infrastructure

::: important Translation Required
This page is outdated and maybe still in Simplified Chinese. Translation is needed.
:::

## Shifting Strategy

- Automatically calculate and choose the **optimal solution within a single facility**. Supports all general and special skill combinations.
- Supports recognition of Battle Record, Pure Gold, Originium Shard, Chip and so on for different operators.
- 自动按照 `无人机用途` 选择的方式使用无人机。
- Recognizes the percentage of the Morale bar. When Morale is below some threshold, the operator will be moved to the dormitory.

## Note

- The shifting strategy is based on the optimal solution within a single facility instead of multiple facilities. Combination such as: `Shamare-Tequila`, `Vermeil-Scene` within a single facility can be recognized correctly; while combination like `Rosmontis`, `Pinus Sylvestris` among facilities is not supported yet.
- If `Usage of Drone` is selected with the option `Trading Post-LMD`, it will recognize `Shamare` and reserve it for her.
- Operators of corresponding fraction will be selected when only one Clue is needed Reception Room; otherwise general operators will be chosen.
- Reception Room will send out Clues only when your Clues are full. Three Clues will be send out at most. You can edit `SelectClue` - `maxTimes` field in `resource/tasks.json` to edit number of Clues sent if you want.
- If you do not want operators like `Irene` or someone else to be put into the dormitory when the training room is not in use, you can switch off `Working operator shall not be put into the dormitory` in the settings. Note that this may cause the operators with non-full fatigue not entering the dormitory as well.
- Due to the complexity of Control Center, only `Amiya`, `Swire`, `Kal'tsit`, `Team Rainbow` and other Morale+0.05 operators will be considered. To be improved in future.
- Some alternate operators may have conflicts in Infrastructure. Please notice if there is "Operator conflict" warnings on the UI, and double check the Infrastructure to shift manually (e.g. some facilities may not have any operator).
- 可自行选择需要 MAA 处理的设施类别，默认全选。

## Custom infrastructure shift change (Beta)

- Several sets of extremely efficient tasks are built-in under the MAA folder `/resource/custom_infrast/`, which can be used as a reference. 由于其对干员及练度的需求极高，不推荐直接使用。
