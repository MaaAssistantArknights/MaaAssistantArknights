---
order: 12
icon: octicon:tools-16
---

# Tools

## Recruitment Recognition

Open the Recruitment tag interface and click Start Recognition to initiate.

If you have previously run Operator Recognition, potential data will be displayed after the recognized operator's name.

## Operator Recognition

Can recognize lists of both owned and unowned operators, and identify potential levels to provide data for Recruitment Recognition.

After enabling `Fetch Operator data from Yituliu` in `Settings - Third-party services` and filling in a Yituliu read-only token, Operator Recognition no longer connects to the emulator for screenshot recognition, and instead reads the progression data stored on Yituliu; the exported Markdown/CSV tables will then include additional columns for skill level, skill masteries, and modules. Fetching from Yituliu is implemented by the UI, while the default screenshot recognition is still done by Core.

Currently supports the following export formats:

- Clipboard
- JSON
- Markdown table
- CSV table

## Depot Recognition

Supports automatically entering the depot from the main page. If automatic navigation fails, please manually switch to the `Upgrade Materials` screen and **scroll all the way to the left** before starting the function.

The recognized depot data is cached and used by the [Depot Maintain](./depot-maintain.md) task and the `Target Inventory` mode in [Combat](./combat.md). It can also be synced automatically via the [Update Doctor Data](./user-data-update.md) task. When recognition returns no items, the existing inventory data is kept.

Currently supports the following export formats:

- [Penguin Statistics Planner](https://penguin-stats.io/planner) (ArkPlanner)
- [Arknights Toolbox](https://arkntools.app/#/material) (Lolicon)
- Markdown table
- CSV table

If you are the administrator of a data site, we welcome you to contact us to adapt your site's material JSON protocol!

More useful features may be added in the future. (Teaser)

## Gacha

::: center  
**It really uses your account to pull operators&emsp;&emsp;It really uses your account to pull operators&emsp;&emsp;It really uses your account to pull operators**&emsp;&emsp;  
**It really uses your account to pull operators&emsp;&emsp;It really uses your account to pull operators&emsp;&emsp;It really uses your account to pull operators**&emsp;&emsp;  
**It really uses your account to pull operators&emsp;&emsp;It really uses your account to pull operators&emsp;&emsp;It really uses your account to pull operators**&emsp;&emsp;  
:::

Couldn't join the MAA Gacha Party livestreams? No problem! Now you can have your own personal gacha assistant! Screenshots will be taken during the pulling process and displayed in the center of the interface!

## Peep

Want to see the world through MAA's eyes? Now you can!

MAA will attempt to take screenshots of the emulator at the target frame rate and display them in real-time in the window.

When the screenshot speed is significantly slower than the target frame rate, MAA will automatically reduce the target frame rate to match the screenshot speed. ~~Can be used as a stress test~~

## Mini Game

Used to automatically farm mini-games in events. Supports selecting names and switching between permanent and current limited-time activities in settings.

Built-in features include:

- Permanent Activities
  - Hidden Front (supports selecting endings and event branches)
  - Reclamation Algorithm
- Permanent Features
  - Event shop exchange
  - Green ticket shop exchange
  - Yellow ticket shop exchange
  - Reclamation Algorithm shop exchange
  - Automatically Raise Potential
  - Missing Material Synthesis

### Reclamation Algorithm

#### Tales Within the Sand

**Default Mode**

1. You need to start the task from the main screen where "Start Reclamation" appears at the bottom-right
2. Cannot be used when you already have an existing save file (delete the existing save to use)
3. Cannot be used when operators are present in the Reclamation Algorithm squad (clear the current squad, especially after completing a wave)

**Manufacturing Points Farm**

1. You need to start from the page where you can see the base camp after entering Reclamation Algorithm
2. If no item name is specified, Glow Sticks will be crafted by default
3. Prerequisite: It must be the first day after settlement, with no enemy raids entering the base camp for the next three days
4. If the manufacturable quantity is exactly a multiple of 99, the process may get stuck. You can use up a small amount before saving; this issue will be fixed in the future

#### Relaunch Anchor

**RA-1**

- Income reference: ~159 tokens + coordination points per run, ~2 min 10 sec per cycle
- Operator requirement: None
- Prerequisite: Complete RA-1 in the main storyline
- Open RA-1 on the world map, start the task when "Start Construction" appears in the bottom-right corner
- Note: If you have unlocked technologies that grant extra starting items, please remove facilities that cause this (e.g. Food Supply Station, Drink Supply Station, Large Beast Pen, etc.)
- Task flow: Automatically executes intensive farming, construction, resource delivery, and settlement loop

**RA-4**

- Reward estimate: ~417 tokens + coordination points per run, ~1 min 40 sec per round
- Operator requirement: Wis'adel (support unit allowed)
- Prerequisite:
  1. Advance the main story until RA-4 is cleared.
  2. Unlock Strategy Planning Management.
  3. If you have your own Wis'adel, manually enter the stage and form a team once, ensuring any 5 operators with lower DP cost than Wis'adel + Wis'adel, and Wis'adel is in the 6th slot (i.e., the last one selected). Then confirm recruitment, abandon the current construction, and start from "Start Construction".
  4. If using a support Wis'adel, ensure that Wis'adel is on the front page of sniper support units (consider using a friend). Manually enter the stage and add any 5 operators with lower DP cost than Wis'adel, select support Wis'adel for the 6th slot, then confirm recruitment, abandon the current construction, and start from "Start Construction" (ensure the first five slots have operators and the 6th slot is empty).
- Open RA-4 on the world map, start the task when "Start Construction" appears in the bottom-right corner
- Note: If you have unlocked technologies that grant extra starting items, please remove facilities that cause this (e.g. Food Supply Station, Drink Supply Station, Large Beast Pen, etc.)
- Task flow: Use the Gold from Strategy Planning Management to unlock areas, and use Wis'adel to complete the boss elimination mission.

**RA-15**

- Income reference: ~500 tokens + coordination points per run, ~3 min per cycle
- Operator requirement: Civilight Eterna (can use support unit)
- Prerequisites:
  1. Complete RA-15 in the main storyline
  2. If you own Civilight Eterna, manually open the stage and set up the squad once: 5 Vanguards (no level requirement) + Civilight Eterna in the 6th slot, then save and exit
  3. If using support Civilight Eterna, ensure she appears on the first page of Caster supports
- Open RA-15 on the world map, start the task when "Start Construction" appears in the bottom-right corner
- Note: If you have unlocked technologies that grant extra starting items, please remove facilities that cause this (e.g. Food Supply Station, Drink Supply Station, Large Beast Pen, etc.)
- Task flow: Complete the 60-kill mission with Civilight Eterna
