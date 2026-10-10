---
order: 8
icon: mdi:account-arrow-up
---

# Operator Progression

The Operator Progression task follows the plan list in order and automatically develops the specified operators up to the configured **Elite**, **basic skill level**, and **skill mastery** targets. When materials are insufficient it crafts them at the Workshop, produces them at the Factory, or buys them from the Purchase Certificate Store on its own, with no manual in-game operations needed.

## How It Works

Each plan corresponds to one operator, and the three kinds of targets can be set at the same time without conflicting with each other.

- **Elite**: promotes stage by stage; before each promotion the operator is first leveled up to the max level of the current stage.
- **Automatic material completion**: missing materials are automatically crafted at the Workshop; the Dualchips required to promote 5★ and 6★ operators to Elite 2 are produced at the Factory, and missing Chip Catalysts are bought from the Purchase Certificate Store. If a formula is not unlocked yet, or the materials still cannot be completed, that target is recorded as failed and the task continues with the remaining targets of this plan and the following plans.
- **Skill mastery**: mastery occupies the Training Room for a period of time. A single run starts at most one mastery training, for one operator; once that training is started, the mastery targets of all remaining operators in the same run are skipped (if the Training Room is already occupied by another operator, that entry is recorded as skipped). Reaching the planned mastery level therefore usually requires running the task several times.
- While the task runs, development results are output entry by entry, and a summary of completed, failed, and skipped entries is output at the end; entries that have reached their targets are removed from the plan list automatically.
- The plan list cannot be edited while the task is running.

Entries in the plan list can be **reordered by dragging**, and the task develops them one by one in that order.

## Importing a Plan

Besides adding operators one by one and setting targets manually in the UI, you can import a JSON plan directly:

1. Copy the plan JSON to the clipboard; you can also click the "Edit the plan online with Arknights Toolbox" link in the settings and export after editing online in the [Arknights Toolbox](https://arkntools.app/#/material).
2. Click "Read from Clipboard" in the Operator Progression settings; the parsed plan is appended to the end of the current list.

Each array item of the plan corresponds to one operator:

| Field           | Type   | Required | Description                                                                                                                                       |
| :-------------- | :----- | :------- | :------------------------------------------------------------------------------------------------------------------------------------------------ |
| `role`          | string | No       | Operator profession, see the list below; inferred from `name` when omitted, and must be given explicitly if the name maps to several professions  |
| `name`          | string | Yes      | Operator name, e.g. `Amiya`, must not be empty                                                                                                    |
| `elite`         | int    | No       | Target Elite stage, `1` or `2`; omitted / `0` means not set                                                                                       |
| `skill_level`   | int    | No       | Target basic skill level, `2` ~ `7`; omitted / `0` means not set                                                                                  |
| `skill_mastery` | int[]  | No       | Mastery target levels for skill 1 / 2 / 3 in order, each `0` ~ `3`; for low-rarity operators shorter arrays such as int[1] or int[2] are accepted |

`role` takes the same values as `profession` in the unpacked data: `Pioneer` (Vanguard), `Warrior` (Guard), `Tank` (Defender), `Sniper` (Sniper), `Caster` (Caster), `Medic` (Medic), `Support` (Supporter), `Special` (Specialist).
The `level` field is reserved and not supported yet.

Example:

```json
[{"role": "Warrior","name":"Ch'en","elite":2,"skill_level": 7, "skill_mastery":[0,0,3]}]
```

The example above develops the Guard "Ch'en" to Elite 2 and basic skill level 7, and masters skill 3 to mastery 3.

::: tip
Field names are case-sensitive, and fields other than those listed above are not accepted. If any single plan cannot be parsed, the whole import takes no effect and the reason is written to the log.

Importing does not complete prerequisites automatically. For example, if you set only `skill_mastery` without `skill_level`, and the operator's current basic skill level is below 7, that mastery target will fail at runtime because the prerequisite is not met. Set `skill_level: 7` together with it.
:::
