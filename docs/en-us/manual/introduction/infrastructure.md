---
order: 4
icon: material-symbols:view-quilt-rounded
---

# Base Management

## Normal Mode

### Shift Strategy

- Automatically calculates and selects the **optimal solution within a single facility**, supporting all generic skill combinations and special skill synergies.
- Automatically identifies EXP Records, Gold Bars, Originium Shards, and Chips, deploying appropriate operator combinations for each.
- Automatically uses drones according to the selected `Drone Usage` setting.
- Automatically detects morale levels and assigns operators with remaining morale percentage below the `Base Facility Morale Threshold` to dormitories.

### Additional Notes

- Base shift management currently optimizes for single-facility efficiency, not cross-facility global optimization.
  - Recognizable and usable examples: `Shamare + Tequila`, `Vermeil + Scene`.
  - Unrecognizable examples: `Rosmontis System`, `Pinus Sylvestris Knights`.
- When `Drone Usage` is set to `Trading Post - LMD`, the `Shamare Group` will be prioritized.
- Reception Room selects operators with corresponding clue affinity when only one clue type is missing; otherwise selects generic operators.
- Reception Room only sends clues when your collection is full, and sends only three at a time. To customize the number sent, modify the `ClueSelected` - `maxTimes` field in `resource/tasks/tasks.json` in the MAA directory.
- Enabling `Do not place stationed operators in dormitory` prevents operators like `Irene` and `Logos` from being assigned to dormitories when not training in the Training Room, but also prevents operators with low morale in the Processing Station from being moved to dormitories.
- Control Center strategy is complex; currently only considers `Amiya`, `Swire`, `Kal'tsit`, `Team Rainbow`, and other operators with +0.05 morale bonus. This will be gradually optimized.
- You can select which facility types MAA should manage (all are selected by default).

## Preset Rotation Mode

- This mode requires preset squads configured in-game. MAA will automatically rotate through them.

## Custom Base Mode

- The [Schedule Generator](https://ark.yituliu.cn/tools/schedule) created by community experts can help you create custom schedules. Refer to the [Base Facility Protocol Documentation](../../protocol/base-scheduling-schema.md) for usage.
- The MAA folder `/resource/custom_infrast/` contains built-in theoretically maximum-efficiency presets. Not recommended for direct use due to their extreme operator and elite/skill level requirements.
