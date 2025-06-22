---
order: 4
icon: material-symbols:view-quilt-rounded
---

# Base

## Normal Mode

### Shift Strategy

- Automatically calculates and selects the **optimal solution within a single facility**, supporting all combinations of generic skills and special skills.
- Automatically identifies EXP Records, Pure Gold Ingots, Originium Shards, and Chips, deploying corresponding Operator combinations for each.
- Automatically uses Drones according to the selected `Drone Usage`.
- Automatically detects morale levels and assigns Operators with remaining morale percentage below the `Base Facility Morale Threshold` to Dormitories.

### Additional Notes

- Base shift management currently optimizes for single-facility efficiency, not cross-facility global optimization.
    - Recognizable and usable examples: `Shamare + Tequila`, `Vermeil + Scene`.
    - Unrecognizable examples: `Rosmontis System`, `Pinus Sylvestris Knights`.
- When `Drone Usage Purpose` is set to `Trading Post - LMD`, the `Shamare Group` will be additionally recognized and prioritized.
- Reception Room selects Operators with corresponding Clue affinity when only one Clue type is missing; otherwise selects generic Operators.
- Reception Room sends Clues only when self-owned Clues are full, limited to three per send. Customize send quantity by modifying `ClueSelected` - `maxTimes` in `resource/tasks/tasks.json` under MAA folder.
- Enabling `Do not move stationed Operators to Dormitories` prevents Operators like `Irene` and `Logos` from being assigned to Dormitories when not training in Training Room, but also prevents Processing Station Operators with insufficient morale from entering Dormitories.
- Control Center strategy is complex; currently only considers `Amiya`, `Swire`, `Kal'tsit`, `Team Rainbow`, and other Operators with +0.05 morale bonus. Will be gradually optimized.
- Select which facility types MAA should manage (default: all selected).

## One-click Rotation Mode

- This mode requires preset squads configured in-game. MAA will automatically rotate them.

## Custom Base Mode

- [Schedule Generator](https://ark.yituliu.cn/tools/schedule) created by strategy experts. Refer to [Base Facility Protocol Documentation](../../protocol/base-scheduling-schema.md) for usage.
- MAA folder `/resource/custom_infrast/` contains built-in theoretically maximum-efficiency presets. Not recommended for direct use due to extreme Operator and development level requirements.
