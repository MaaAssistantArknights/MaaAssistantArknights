---
order: 8
icon: ri:game-fill
---

# Auto Integrated Strategy

MAA selects the Phantom theme by default, which can be changed in `Auto Integrated Strategy` - `Integrated Strategy Theme`.

::: warning  
All features involving Copilot require the following prerequisites, including but not limited to Copilot and Auto I. S.

- At least 60 frames of **stable** framerate
- Touch mode set to Minitouch, MaaTouch, or MuMu Touch Enhancement

:::

- Please pin the corresponding Integrated Strategy theme at the terminal in-game. Although automatic navigation is currently possible, long-term functionality is not guaranteed.
- Please manually end explorations of **non-target themes** in advance to ensure the target theme is ready to start exploration.
- You can select theme, difficulty, squad, starting operators, and more in settings. Only the 1st starting operator is available by default; the 2nd and 3rd can be enabled in `Advanced Settings`, and the operators in the three slots must be of different classes.
- Recruitment only searches the first few pages for the specified operators, so operators with lower hope cost may be missed; in that case the slot falls back to the default policy. It is recommended to place operators with higher hope cost first.
- MAA can use support operators to start, by first entering the target operator name in `General Settings` → `Starting Operator` and then enabling the use-support checkboxes for each starting operator in `Advanced Settings`. Each slot can independently choose whether to use a support unit, but using a support unit in the 1st slot is mutually exclusive with rerolling for Elite 2 (rerolling for Elite 2 works only with owned operators).
- When playing for the **first time** in a newly installed client:
  - The difficulty selection button will not be displayed. Please manually select a difficulty once in-game, after which MAA can select it automatically.
  - Tutorials will appear after entering the map screen. Please read and close them manually.

## Recommended Starts

::: details _Last updated on 2026/10/26_

<!-- prettier-ignore -->
| Theme | Difficulty | Squad | Strategy | Operator |
| - | - | - | - | - |
| Phantom | Formal Investigation·2 | Leader Squad / Tactical Assault Squad | Overcoming Your Weaknesses | Degenbrecher/Thorns |
| Mizuki | Surging Waves·3~10 | People-Oriented Squad / Mind Over Matter Squad | Slow and Steady Wins the Race | Wiš'adel |
| Sami | Braving Nature·4~10 | Special Training Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |
| Sarkaz | Facing Souls·4~10 | Blueprint Mapping Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |
| Garden | Guided Tour·3~10 | Leader Squad / Tactical Ranged Squad | Slow and Steady Wins the Race | Wiš'adel |
| 黑流树海 (BlackFlow) | _None yet_ <!-- 保密等级·0~15 --> | Special Squad / Tactical Fortification Squad | Slow and Steady Wins the Race / Indestructible | `Mechanic` (Elite 2 with talent fully unlocked and `Effect Improvement II` of `Recruitment Mastery II` required) |

The recommended difficulties consider factors like `enemy difficulty`, `hope consumption`, and `score multiplier`, and have been tested to be stable with high-level operators. Feel free to adjust based on your situation and needs.

<!-- prettier-ignore -->
| Theme | Notes |
| - | - |
| Phantom | `Formal Investigation·3` and higher difficulties may offer hope-reducing collectibles at start, preventing recruitment of six-star operators. |
| Mizuki | `Surging Waves·4` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Mind Over Matter Squad`.<br>`People-Oriented Squad` is better for high-level accounts, while `Mind Over Matter Squad` requires luck. |
| Sami | `Braving Nature·6` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Special Training Squad`. |
| Sarkaz | `Facing Souls·15` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Blueprint Mapping Squad` unless `Blueprint Mapping Squad Enhancement Ⅱ` has been activated in `Historical Reconstruction`.<br>When `Blueprint Mapping Squad` is selected, MAA will use a combat-avoidance strategy to quickly collect `Soul Bookmarks`, though it typically makes it impossible to witness an ending.<br>When the Originium Ingot farming strategy and the `Ingots Squad` are selected, MAA will use the shop refreshing strategy to speed up the process. |
| Garden | `Guided Tour·15` and higher difficulties increase six-star operator hope cost by +1, making it potentially impossible to recruit them with `Leader Squad`.<br>When `Guided Tour·3` difficulty is selected, with the Originium Ingot farming strategy and `Leader Squad` selected, MAA will use the `End of Time` stage-skip strategy to speed up the process. |
| 黑流树海 (BlackFlow) | There are currently no dedicated combat strategies; battles are decided automatically by recruitment scores.<br>Both Originium Ingot farming and the fast flight to floor 3 revolve around the `Mechanic`'s flight: the run only starts with a `Structural Principle` part (fly anywhere on the map 3 times) if the Mechanic is at Elite 2 with the `Effect Improvement II` of the `Recruitment Mastery II` talent unlocked, and that talent is easy to unlock (just sell 10 parts in total after Elite 2). With the recommended setup, once the Mechanic is recruited, the promotion check is skipped and the strategy runs as if it were Elite 2 for efficiency; the fast flight to floor 3 requires the Mechanic's talents to be fully unlocked, and the flight fails otherwise. Some events have different options before the first clear and may get stuck; clearing N1 once satisfies both the Elite 2 requirement and the event options.<br>With the level farming strategy, the fast flight only happens when `Fly to Floor 3 quickly` is checked; when checked, the squad, roles and starting operators are automatically switched to the setup above, and when unchecked, the run plays toward the target ending. Originium Ingot farming still works without an Elite 2 Mechanic with talents fully unlocked, but battles will most likely fail.<br>With the level farming strategy, when `Fly to Floor 3 quickly` is checked and `Invest ingots` is unchecked, shops are skipped. |

:::

## BlackFlow Starting Reward Farming

When the 黑流树海 (BlackFlow) theme is set to the starting reward farming strategy, MAA plays preparation runs and target runs repeatedly to obtain a satisfying start. The following options are available:

- `Desired Rewards` / `Feathered Serpent Rewards` (the latter requires selecting `Swaddled Feathered Serpent` as the cultivation target): all entries checked in the two lists count as candidates, and the target run is satisfied once any of them appears at the start.
- `Reroll until Floor 1 is 实托邦 (a Floor 1 variant)`: after a desired reward appears on the support selection page of the target run, a card is picked by `Starting Reward Pick Priority` and floor 1 is entered; if floor 1 is 实托邦, the task exits keeping this run; if not, the run is abandoned and a new preparation run starts. Requires difficulty 6 or higher.
- `Starting Reward Pick Priority`: drag to adjust the order; on the support selection page, cards present on the page are picked from top to bottom by this order.
- `Fly to Floor 3 quickly`: the preparation run always starts with the `Special Squad`, `Slow and Steady Wins the Race`, and your own Mechanic, and recruits only the Mechanic; on floor 2, it flies straight to floor 3 with the carried Structural Principle. The flight fails if the Mechanic's talents are not fully unlocked, so please do not check this option.

## Combat Strategy

All MAA operations in Integrated Strategy use preset strategies, with all combat stages using built-in operation files. When no operation exists for a specific stage, a basic (not very intelligent) decision tree handles combat.

For details, see the [Integrated Strategy Protocol](../../protocol/integrated-strategy-schema.md).

- Automatically recognizes operators and their levels, selecting optimal operators and skills.
- Recognizes shop items and prioritizes purchasing more powerful collectibles.

## Exception Detection

- If a task encounters problems, it will automatically abandon the current exploration and retry.
- If battle duration exceeds 8 minutes, all ground units will automatically retreat; if it exceeds 10 minutes, the current battle will be automatically abandoned to avoid time-consuming stalemates.

If the program gets stuck at the same location multiple times, please submit an Issue with logs and screenshots.
