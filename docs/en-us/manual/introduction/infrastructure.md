---
order: 4
---

# Base

::: important This page may be outdated.
:::

## Workshift Strategy

- Automatically calculate and choose the **optimal solution within a single facility**. Supports all general and special skill combinations.
- Supports recognition of Battle Record, Pure Gold, Originium Shard, Chip and so on for different operators.
- Automatically use drones according to the selected `drone usage`.
- Recognizes the percentage of the Morale bar. When Morale is below some threshold, the operator will be moved to the dormitory.

## Note

- The work shift strategy is based on the optimal solution within a single facility instead of multiple facilities. Combinations such as: `Shamare-Tequila`, `Vermeil-Scene` within a single facility can be recognized correctly; while combinations like `Rosmontis`, `Pinus Sylvestris` among facilities are not supported yet.
- If `Usage of Drone` is selected with the option `Trading Post-LMD`, it will recognize `Shamare` and reserve it for her.
- Operators of the corresponding faction will be selected when only one Clue is needed in the Reception Room; otherwise, general operators will be chosen.
- The reception Room will send out Clues only when your Clues are full. Three Clues will be sent out at most. If wanted, you can edit the number of sent Clues in the `SelectClue` - `maxTimes` field in `resource/tasks.json`.
- If you do not want operators like `Irene` or someone else to be put into the dormitory when the training room is not in use, you can switch off `Working operator shall not be put into the dormitory` in the settings. Note that this may cause the operators with non-full fatigue to not enter the dormitory as well.
- Due to the complexity of the Control Center, only `Amiya`, `Swire`, `Kal'tsit`, `Team Rainbow` and other Morale+0.05 operators will be considered. To be improved in future.
- Some alternate operators may have conflicts in Infrastructure. Please take note if there are "Operator conflict" warnings on the UI, and double check the Infrastructure to shift manually (e.g. some facilities may not have any operator).
- You can choose the facility categories that need to be handled by MAA, with all selected by default.

## Custom infrastructure shift change (Beta)

- Several sets of extremely efficient tasks are built-in under the MAA folder `/resource/custom_infrast/`, which can be used as a reference. Due to its high requirements for operators and their levels, it is not recommended for direct use.
