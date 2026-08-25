---
order: 5
icon: material-symbols:view-quilt-rounded
---

# Base Management

## Normal Mode

### Shift Strategy

- Automatically calculates and selects **efficient operator combinations**, supporting all generic skill combinations, special skill synergies, and cross-facility combinations — a hassle-free choice.
- Automatically identifies EXP Records, Gold Bars, Originium Shards, and Chips, deploying appropriate operator combinations for each.
- Automatically uses drones according to the selected `Drone Usage` setting.
- Automatically detects morale levels and assigns operators with remaining morale percentage below the `Base Facility Morale Threshold` to dormitories.

### Cross-Facility Teams

The following cross-facility operator teams can be enabled as needed in the base settings:

- `Pinus Sylvestris Knights`: required operators `Flametail` (E2), `Viviana` (E2), and at least one of `Wild Mane`/`Ashlock`/`Fartooth` (E2); `Gravel` participates in the calculation.
- `Perception Information`: required operators `Whisperain` (E2), `Rosmontis` (E2), and `Ebenholz` (E2); `Dusk`, `Virtuosa`, `Iris`, and `Czerny` participate in the calculation. Takes priority over `Worldly Plight`.
- `Worldly Plight`: required operators `Mulberry` (E2) and `Stainless` (E2); `Chongyue`, `Ling`, and `Dusk` participate in the calculation.
- `Abyssal Hunters`: required operators `Gladiia` (E2), `Ulpianus`, `Skadi`, `Specter`, and `Andreana`. Due to algorithm limitations, they will not participate in scheduling together with `Pinus Sylvestris Knights` when both are selected.

### Shift Order

In Normal Mode, the shift order is planned automatically by the algorithm (Dormitory → Power Plant → Office → Control Center → Factory → Trading Post → Reception Room → Dormitory → Processing Station → Training Room) to support cross-facility combinations and morale-recovery interactions. The facility list only determines which facility types MAA processes; the order of the list does not take effect. To schedule shifts in a custom order, use the `Custom Base Mode`.

### Additional Notes

- When `Drone Usage` is set to `Trading Post - LMD`, the `Shamare Group` will be prioritized.
- Fiammetta recovery targets: up to 3 recovery targets (`Purestream`, `Closure`, `Proviso`, `Shamare`, `Tequila`, `Gladiia`) can be selected. During shift change, the target operator with the lowest morale is placed in a dormitory together with Fiammetta first. Since the 007 schedule of `Shamare` and `Tequila` is difficult to achieve with automatic shift changes, selecting these two operators is not recommended.
- The Reception Room will attempt to send clues on every rotation. The Official (and Bilibili) clients added a "One‑click Send Duplicate Clues" feature on 2025-12-05: when MAA detects this button, it will automatically click it to batch-send duplicate clues; EN/JP/KR are expected to support this feature in about 6 months, and TW in about 1 year (TW may get it earlier due to accelerated updates).
- The Reception Room selects operators by fixed priority (e.g. `Forcer` and `Caper` first); U-Official's clue affinity skill is disabled.
- If Clue Exchange cannot be started, all clues on the clue board are taken off during shift change; if it can be started, one-click placement is used instead. This is because the personal clue limit is 10 (including clues already placed in the Reception Room), and placed clues do not count toward duplicate detection. Pre-placing too many clues may therefore fill the limit while the clues in hand are mutually non-duplicate, leaving no way to one-click send them or receive new ones — only friends picking them up can help. For example, with 6 clue types on the board (7 distinct types are required to start the exchange) and 4 mutually non-duplicate clues in hand, no new clues can be received, and one-click sending cannot send anything either.
- Enabling `Do not place stationed operators in dormitory` prevents operators like `Irene` and `Logos` from being assigned to dormitories when not training in the Training Room, but also prevents operators with low morale in the Processing Station from being moved to dormitories.
- The Control Center selects operators by fixed priority rules (manufacturing/trading/office acceleration, morale reduction for other facilities, and cross-facility team interactions; same-type bonuses do not stack on multiple slots).
- You can select which facility types MAA should manage (all are selected by default).

## Preset Rotation Mode

- This mode requires preset squads configured in-game. MAA will automatically rotate through them.

## Custom Base Mode

- The [Schedule Generator](https://ark.yituliu.cn/tools/scheduleV3) created by community experts can help you create custom schedules. Refer to the [Base Facility Protocol Documentation](../../protocol/base-scheduling-schema.md) for usage.
- The MAA folder `/resource/custom_infrast/` contains built-in theoretically maximum-efficiency presets. Not recommended for direct use due to their extreme operator and elite/skill level requirements.
