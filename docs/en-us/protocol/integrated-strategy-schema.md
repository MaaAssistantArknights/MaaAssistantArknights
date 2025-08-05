---
order: 5
icon: ri:game-fill
---

# Integrated Strategy (I.S.) Schema

::: tip
Please note that JSON files do not support comments. Comments are for demonstration purposes only and should not be copied directly
:::

## Integrated Strategy resources location

- `resource/roguelike/` the files for each theme are below
  - Theme folder
    `Phantom/` for Phantom's I.S. ("傀影" - Phantom & Crimson Solitaire)
    `Mizuki/` for Mizuki's I.S. ("水月" - Mizuki & Caerula Arbor)
    `Sami/` for Sami's I.S. ("萨米" - Expeditioner's Jǫklumarkar)
    `Sarkaz/` for Sarkaz's I.S. ("萨卡兹" - Sarkaz Endless Tale)
    `JieGarden/` for JieGarden's I.S. ("界园" - Boundary Garden)
    - `autopilot/` combat json for each level
      - `level_name_in_chinese.json` combat logic of the level
    - `encounter/` encounter nodes logic
      - `default.json` levelling mode
      - `deposit.json` investing mode
    - `recruitment.json` operators recruitment logic
    - `shopping.json` trade store purchasing logic

- Specifically, in `Sami/`:
  - `foldartal.json` defines the usage logic for Sami's Foldartals
  - `collapsal_paradigms.json` defines the types of Sami's Collapsal Paradigms
  - `autopilot/关卡名_collapse.json` combat logic for stages (Collapsal Paradigm farming mode)
  - `encounter/collapse.json` encounter logic for Collapsal Paradigm farming mode

- In `Sarkaz/`:
  - `fragments.json` stores basic information about Sarkaz's Thoughts
  - `map.json` stores template image information for Sarkaz's Blueprint navigation

## Integrated Strategy Step 1: Operator Recruitment

`resource/roguelike/theme_name/recruitment.json` describes the logic of the operator recruitment

The tools in `tools/RoguelikeRecruitmentTool` and `tools/RoguelikeOperSearch` can help view and edit the recruitment files.

```json
{
    "theme": "Phantom",              // I.S. Theme (In this case Phantom)
    "priority": [                    // Group's priority, in order
             ...
        ],
    "team_complete_condition": [     // Conditions for squad completness
             ...
        ]
} 
```

### Operator classification

Split operators in different ***groups*** according to your understanding of the game (Group concept references [Copilot Schema](./copilot-schema.md))

::: info

1. Operators and summons within the same group must be deployed in the same way (i.e. both melee or ranged) and should have **similar attack ranges**

2. Allows the same operator or summon to be sorted into different groups, depending on its usage.

3. Please do not change the name of an existing group, as this may cause previous versions of the task to be unavailable when MAA is updated!

4. Please try not to add new groups, instead try to implement new operators added to the task into existing groups according to their usage
:::

::: tip
By default, only E1 Level 55 operators will be recruited
:::

```json
{
    "theme": "Phantom",              
    "priority": [                     // Groups, in order
        {
            "name": "棘刺",           // Group name ("棘刺" = "Thorns")
            "opers": [                // Group contains operators, ordered, representing the deployment priority
                                      // (e.g. if you need to deploy the group's operators, test for Thorns, deploy Thorns if it's available, test for Horn if Thorns is not available)
                {
                    "name": "棘刺",   // Operator's name ("棘刺" = "Thorns")
                    ...
                },
                {
                    "name": "号角",   // Operator's name ("号角" = "Horn")
                    ...
                }
            ]
        },
       "team_complete_condition": [     
             ...
        ]
    ]
}
```

1. Introduction to existing groups

    Taking Sami's I.S. as an example: operators are mainly divided into:

    | Group  | Considerations | Class | Operators |  
    | :--- | :--- | :--- | :--- |
    | ***地面阻挡*** (Ground Blocking)  | Field presence and clearing enemies | Defenders, Guards | Healing defenders, cornerstones, Blacknight, Mountain, M3, Ling and Scene's summons, Spot, Defender reserve operators |
    | ***地面单切/处决者*** (Ground Single-Target/Executors) | Solo elite enemies | Executor specialists | SilverAsh, Yato the Elegy, Kirara R Yato, M3, Red |
    | ***高台C*** (Ranged Core) | Sustained and decisive output | Snipers, Casters | Weathered, Logos, Ch'en the Holungday, Chongyue |
    | ***高台输出*** (Ranged DPS) | Anti-air and sustained output | Snipers, Casters | Archetto, Exusiai, Kroos, Steward |
    | ***速狙*** (Fast Snipers) | Physical damage, standard range | Snipers | Eyjafjalla, Exusiai, Yeye, Kroos |
    | ***术师*** (Casters) | Arts damage, standard range | Single-target Casters | Eyjafjalla, Logos, Steward |
    | ***辅助*** (Supporters) | Can hit 1 tile behind | Supporters | Suzuran, Lily of the Valley, Yeye, Orchid |
    | ***狙击*** (Long-range Snipers) | Longer range high ground | Artilleryman, Spreadshooter | Weathered, Toddifons, Rosa |
    | ***奶*** (Healers) | Healing ability | Medics, Supporters | Kal'tsit, Skadi the Corrupting Heart, Hibiscus, Ansel |
    | ***单奶*** (Single-target Healers) | Attack range depth >= 4 | Medics | Kal'tsit, Reed the Flame Shadow, Ptilopsis, Perfumer, Hibiscus, Ansel |
    | ***群奶*** (AoE Healers) | Attack range depth < 4, can heal behind | Medics, Supporters | Nightingale, Ptilopsis, Skadi the Corrupting Heart |
    | ***回费*** (DP Recovery) | Cost recovery | Vanguards | Myrtle, Ines, Fang, Vanilla |
    | ***地刺*** (Ground Traps) | No blocking, provides DPS or slowing in front of defenders | Ambushers | Gladiia, Ascalon, Manticore |
    | ***地面远程*** (Ground Ranged) | Ground long-range, can output behind shields | Instructors, Lords | Horn, Pallas, Thorns, SilverAsh |
    | ***领主*** (Lords) | Ground attack range depth > 4, can target air | Fortress, Lord | Thorns, SilverAsh, Chouyu |
    | ***盾法*** (Shield Casters) | Short attack range, some tanking ability | Arts Fighter Casters | Lin, Carnelian |
    | ***炮灰*** (Fodder) | Absorb attacks, redeploy | Specialists, summons | M3, Red, Myrtle, reserve operators |
    | ***大龙*** (Big Dragon) | Tank in front of blockers, easy to combine | Summons | Ling's dragons, Jessica the Flame Purifier's shield |
    | ***补给站*** (Supply Stations) | Accelerate main DPS operator skill rotation | Summons | Support Supply Station, Artificer operator summons |
    | ***无人机*** (Drones) | Ignore height restrictions for healing summons | Summons | Skadi's Seaborn, Silence's Medical Drone |
    | ***支援陷阱*** (Support Traps) | Deployable ground explosives | Summons | Support Mist Generator, Support Rumble-Rumble |
    | ***障碍物*** (Obstacles) | Don't take deployment slots, attract aggro or block | Summons | Cage, obstacles |
    | ***其他地面*** (Other Ground) | Ground operators not preferred for priority use | Push/pull, block-1 vanguards, swordmasters | Bagpipe, Croissant |
    | ***高台预备/其他高台*** (High Ground Reserve/Other High Ground) | High ground operators not preferred for priority use | AoE casters, chain casters, tacticians | Orchid, reserve operators |

    ::: tip
    "Ground Blocking" mainly considers an operator's overall defensive capabilities (sometimes killing everything is also a form of defense), including "Ground Ranged" and "Lord" groups.

    "Healers" mainly considers overall healing ability, including single-target and AoE healers. Consider attack range coverage when deciding whether to use single-target healers (healing 4 tiles) or AoE healers (healing behind).

    "Ranged DPS" only considers output ability, mainly a mixed ordering of sniper and caster professions. Consider damage type, attack range and other restrictions when using specific groups like fast snipers, casters, supporters, or shield casters.

    For trap-type summons, because there are many of them, don't put them in the support trap group. It works better to let MAA deploy them automatically.
    :::

2. Groups requiring special handling

    In addition to those general groups, sometimes we need more custom tweaks for some operators or types such as

    | Group | Operators | Features |
    | :--- | :--- | :--- |
    | 益达 (Weathered) | 维什戴尔 (Weathered) | High ground DPS, prioritizing deployment can reduce pressure |
    | 棘刺 (Thorns) | 棘刺 (Thorns), 号角 (Horn) | Ground ranged output, Phantom I.S. has some maps with very suitable positions |
    | 召唤类 (Summoners) | 凯尔希 (Kal'tsit), 令 (Ling), 稀音 (Scene) | Self-contained ground blocking, some maps need priority deployment, summons can be used as blockers or fodder |
    | 情报官 (Agents) | 晓歌 (Cantabile), 伊内丝 (Ines) | Can recover DP, provide side output, and can single cut |
    | 浊心斯卡蒂 (Skadi Alter) | 浊心斯卡蒂 (Skadi the Corrupting Heart) | Decent healing under low pressure, but special range, some maps have suitable positions |
    | 焰苇 (Reed Alter) | 焰影苇草 (Reed the Flame Shadow) | Commonly used opening operator in Sami I.S., combines healing and output, some maps have optimal positions |
    | 玛恩纳 (SilverAsh) | 玛恩纳 (Closure), 银灰 (SilverAsh) | Ground large-area decisive output, can be deployed against bosses |
    | 史尔特尔 (Surtr) | 史尔特尔 (Surtr) | Since Surtr always carries S3 at E2, field presence ability is almost zero, deployment priority is extremely low |
    | 骰子 (Dice) | 骰子 (Dice) | In Mizuki I.S. the dice needs to be operated separately |

::: info
Currently fixed to group unidentified ground crews behind the penultimate formation (other ground), and unidentified high-platform crews behind the penultimate formation (other high-platform)
:::

::: tip
Newly implemented operators need to be manually added to each theme's `recruitment.json`, and developers may not always remember to make adaptations.
If you notice this issue, you can open an issue to remind developers, or you can submit a PR directly.
:::

### Preset groups--Complete line-up testing

In a team that you expect to pass or reach the top, what are the basic core players? Are they essential? How many do you need?

::: info
The current script's recruiting logic is to only recruit 0 hope and key agents before the lineup meets the lineup readiness level, and save hope for high star key agents. So don't set the threshold number too high, it is recommended that the number of all the operators needed (the basic core lineup) should add up to 4-8 operators.

(The current implementation manually marks high-strength operators and three-star operators as key operators, and only recruits key operators during recruitment) (TODO: Identify 0 hope operators)

After the team meets the lineup completeness check, each time you get a recruitment ticket, it will recruit based on the operator's rating and E2 priority, so to avoid wasting hope, operators you don't want to recruit can be set with no rating or set below the same profession's three-star/reserve operator rating. (For operators you definitely want or absolutely don't want, you can specially assign extremely high or extremely low scores, see `Sami`'s `止颂` (Heidi) as an example)

When forming combat squads, the default sorting of the operator selection interface (from top to bottom, left to right) will be read and stored in order. Based on this order, keeping the relative order, the first operators that meet the lineup completeness will be moved to the beginning until the lineup completeness is all satisfied or there are no operators that meet the lineup completeness. Then reserve operators will be moved to the end, forming a new order, and selection will be based on this new order. In particular, since six-star temporary recruited operators are at the very beginning of the squad by default, it's easy to include six-star temporary recruited operators in the actual squad. (TODO: Exclude temporary recruited E1 operators that can't be used)

For beginner accounts: If 10 recruitments don't satisfy half the team's key operators, the completeness check will be abandoned, and operators will be used based on their ratings, so beginner accounts without trained three-stars may end up with just two or three six-stars and a bunch of reserve operators.
:::

```json
{
    "theme": "Phantom",              // I.S. Theme
    "priority": [                    // Group
             ...
        ],
    "team_complete_condition": [     // Formation completness test
        {                            // A strategy group (condition)
            "groups": [              // Which group members are needed?
                "高台输出"            // ("高台输出" = "Ranged DPS") (This indicates a need for a Ranged DPS operator)
            ],
            "threshold": 1           // How many of these operators are needed?
        },
        {                            // Can have multiple strategy groups
            "groups": [
                "棘刺",              // ("棘刺" = "Thorns") (This indicates a minimum of 2 operators needed from Thorns, Ground Blocking, Ground Single-Cut, Fodder groups)
                "地面阻挡",          // ("地面阻挡" = "Ground Blocking")
                "地面单切",          // ("地面单切" = "Ground Single-Cut")
                "炮灰"              // ("炮灰" = "Fodder")
            ],
            "threshold": 2
        },
        {
            "groups": [
                "奶"                 // ("奶" = "Healers") (This indicates a need for 1 healer)
            ],
            "threshold": 1 
        }
        ...
    ]
} 
```

::: caution
When an operator appears in multiple operator groups, it is only counted once.
For example: Operator `棘刺` (Thorns) may appear in both the `棘刺` (Thorns) group and the `地面阻挡` (Ground Blocking) group. Within this strategy group, operator `棘刺` (Thorns) is only counted once.

But each strategy group counts separately.
For example: Operator `焰影苇草` (Reed the Flame Shadow) may appear in both the `高台输出` (Ranged DPS) group and the `奶` (Healers) group. In this case, both strategy groups can count operator `焰影苇草` (Reed the Flame Shadow).
:::

### Adjusting parameters for operator recruitment

1. The order within a group represents the priority of the deployment detection

2. Temporary recruited operators get a +600 score bonus to their original rating

3. Random direct promotion operators' scores are calculated by adding recruitment score and E2 score together

4. Meaning of each field and script-related logic for group members

   ```json
   {
       "theme": "Phantom",
       "priority": [
           "name": "地面阻挡",                                // Group name ("地面阻挡" = "Ground Blocking")
           "doc": "标准线为1档(清杂能力或者站场能力比山强)>山>2档(阻挡>2,可自回)>斑点,站场能力小于斑点放到单切或者炮灰组",
                                                             // Doc fields are just notes. Has no effect on the program
           "opers": [                                        // What operators should be included, in ordered, represents deployment priority.
               {
                   "name": "百炼嘉维尔",                      // Operator name ("百炼嘉维尔" = "Gavial the Invincible", the first position in the group indicates that
                                                             // when it is necessary to deploy a ground blocking group, the first thing to check is whether it is Gavial Alter or not.)
                   "skill": 3,                               // Which skill to use (In this case skill 3)
                   "skill_usage": 2,                         // Skill usage mode, refer to Combat Operation Protocol, difference is default is 1 if empty
                                                             // (0 is don't use automatically, 1 is use when ready, 2 is use x times (x is set by "skill_times" field), 3 is not supported for now)
                   "skill_times": 2,                         // Skill usage times, default is 1, effective when "skill_usage" field is 2
                   "alternate_skill": 2,                     // Alternative skills used when there is no designated skill, usually 6-star operators who have not 
                                                             // E2'd and use 3 skills after promotion
                                                             // (in this case, use skill 2 when skill 3 is not available)
                   "alternate_skill_usage": 1,               // Skill use mode for alternative skills (this field has not yet been implemented)
                   "alternate_skill_times": 1,               // Number of skill uses for alternative skills (this field has not yet been implemented)
                   "recruit_priority": 900,                  // Recruitment priority, bigger number, higher priority, more than 900 belongs to must be recruited
                                                             // 400 or below recruitment priority is lower than the recruitment of ordinary three-star operators
                                                             // Temporarily recruited operators get priority automatically +600
                   "promote_priority": 600,                  // Advancement priority, bigger number, higher priority, above 900 is a guaranteed E2 if there's enough hope
                                                             // Below 400 recruitment priority is lower than the recruitment of ordinary three-star operators
                                                             // Tip: When you lower recruitment priority or don't write it, and raise the priority of some of
                                                             // your E2, you're actually raising the priority of the E2 for these operators when temporarily recruited
                   "is_key": true,                           // If true, the operator is a key element. Default to false if empty.
                                                             // If the lineup completeness test is not passed, only key and 0 hope are recruited, saving hope.
                   "is_start": true,                         // If true, the operator is a starter. Default to false if empty. If there is no start player in the team,
                                                             // only start players and 0 hope players will be recruited, and user-filled players will be recruited.
                   "auto_retreat": 0,                        // Auto-retreat after a few full-seconds of deployment, takes effect when greater than 0, mainly used for specialists and vanguards, 
                                                             // since I.S. usually starts at 2x speed, it is recommended to set it to skill duration divided by 2
                   "recruit_priority_when_team_full": 850,   // No need to set separately, recruitment priority when lineup completeness is met, default is recruitment priority -100
                   "promote_priority_when_team_full": 850,   // No need to set separately, promotion priority when lineup completeness is met, default is E2 priority +300
                   "recruit_priority_offsets": [             // Adjust recruitment priority based on current lineup
                       {
                           "groups": [                       // Which groups are required to fulfill the conditions, counted by group
                                                             // Don't have duplicate operators in these groups or they will be counted multiple times
                               "凯尔希",                     // ("凯尔希" = "Kal'tsit")
                               "地面阻挡",                   // ("地面阻挡" = "Ground Blocking")
                               "棘刺"                        // ("棘刺" = "Thorns")
                           ],
                           "is_less": false,                 // Whether the condition is greater than or less than, false is greater than or equal to, true is less than or equal to. Default to false if empty.
                           "threshold": 2,                   // Number of conditions met. Default to 0 if empty.
                           "offset": -300                    // Adjustments to recruitment priorities after fulfillment. Default to 0 if empty.
                                                             // (This means that when there are 2 or more operators from Kal'tsit, Ground Blocking, and Thorns groups,
                                                             // Gavial the Invincible's recruitment priority is reduced by 300)
                       }
                   ]
               },
               ...
           ],
       ],
       "team_complete_condition": [
           ...
       ]
   }
   ```

   ::: info
   The groups in `recruit_priority_offsets` should not have duplicate operators.

   After setting `auto_retreat`, you generally don't need to set `retreat_plan` for it in the battle plan.
   :::

5. Add groups and operators as you see fit

    When you add a new group, you can copy operators from existing groups. Refer to the ratings already given by the devs, and modify them on that basis.

## Integrated Strategy Step 2: Battle Logic

`resource/roguelike/theme_name/autopilot/level_name.json` Describes the combat strategy for each level

### Basic I.S. fighting logic of MAA

(Effective when the combat logic file for the level name does not exist)

1. Perform basic combat operations based on the tile grid of the map

    - MAA performs basic combat operations based on whether the grid on the map is a blue or red box, whether it's a high platform or ground, and whether it can be deployed or not.

    - MAA decides which job to use based on the name or number of the map only, and does not judge the map's **Standard**, **Emergency**, **Road Network**, **Foldartal Use**, etc.

    - MAA does not judge **in combat the situation of undefined squares on the map**, e.g. the position of the altar in the `Taming Hut`, the `follower effect` of monsters coming out of the left side or the right side.

   So in the future, you need to try to design a set of combat logic that can cope with **all the different scenarios of a map name** (the above-mentioned scenarios), and be careful of being hung up on the issue that this map operates in Emergency Mode.

2. MAA's Basic Battle Strategy -- Blocking the Blue Entry Point

    1. Ground Crews are preferably deployed on (or around) the grid of the Blue Box (why the grid, scroll down), orientated towards the Red box (auto-calculated), and are not deployed on the Red box

    2. Prioritise the ground units before healers and ranged operators, in a circle from the blue box to the perimeter

    3. It will keep deploying things that can be deployed according to the logic above (operators, summons, support items, etc.)

### Optimise basic combat strategies

1. Blue box Alternative

    It's obviously not smart to just pile up your operators in front of the blue box, some levels have tiles where you can't get through, and the defence is very efficient here

    Or if there are levels with multiple blue boxes, and the MAA doesn't know which blue box corresponds to which red box, they may deploy randomly

    At this point, you'll need to open the [map wiki](https://map.ark-nights.com/areas?coord_override=maa) while imagining the battle in your head

    First switch the `Coordinate Display` to `MAA` in `Settings`

    Then, based on your experience, find the coordinates and orientation of the points that need to be prioritised for defence, and write them into the `replacement_home` of the json.

    ```json
    {
        "stage_name": "蓄水池",        // Level name ("蓄水池" = "Cistern")
        "replacement_home": [         // Entry points (blue boxes replacement points),  
                                      // at least 1 to be complete
            {
                "location": [         // Grid coordinates, obtained from the map wiki
                    6,           
                    4
                ],
                "direction_Doc1": "Preferred direction, it doesn't mean it's definitely in that direction (the algorithm makes its own judgement)",
                "direction_Doc2": "Default is none, i.e. there is no recommended direction, it is entirely up to the algorithm to decide",
                "direction_Doc3": "none / left / right / up / down / 无 / 上 / 下 / 左 / 右",
                "direction": "left"   // (This indicates priority deployment of operators
                                      // to the grid at coordinates 6,4 to the left.)
            }
        ],
        ...
    ```

    ::: tip
    The blue box alternative will take effect when all steps in `deploy_plan` are completed but there are still operators to deploy in the waiting area, following the same logic as the general combat strategy
    :::

2. Deployment tile blacklist

    There are points of priority for defence and points of priority for not deploying operators, such as where the fireball passes through, under the boss's feet, and some locations that are not good for output.

    At this point, we introduce `blacklist_location` to blacklist the tiles that we don't want MAA to deploy operators on.
    ::: info
    The tiles added here will not be used, even if they are included in the deployment strategy later on.
    :::

    ```json
        ...
        "blacklist_location_Doc": "This is an example of usage, not that the map 'Cistern' needs ban.",
        "blacklist_location": [ // Locations where the deployment of the operators is prohibited
            [
                0,
                0
            ],
            [
                1,
                1
            ]
        ],
    ```

3. Alternative map strategies

    For example, if there is a monster in the blue box in Mizuki I.S. should we use the dice to ease the pressure of stacking monsters?

    ```json
        "not_use_dice_Doc": "When the Blue box operator is retreated does MAA need to use the dice. Defaults to false if empty",
        "not_use_dice": false,
    ```

### Or is it an Emergency Operation? It's time to show your true white skills - customised combat strategies

Customisation using `"deploy_plan"` and `"retreat_plan"`

The customised strategy takes precedence over the basic combat strategy, and when all the steps in the customised strategy have been attempted, the saved or fallen re-deployed operator will be deployed per the basic combat strategy.

There is no need to set up too many customised plans when there is a problem. It may be better to hand over to MAA after completing the key steps, or a combination of both.

1. Deployment of operators using groups

    ```json
    "deploy_plan": [ // Deployment logic: order from top-to-bottom, left-to-right
                     // Tries to deploy the first operator it finds, or skips it if it doesn't.
        {
            "groups": ["百嘉", "基石", "地面C", "号角", "挡人先锋"], // Looks for operators from these groups
                                                                  // ("百嘉" = "Gavial Alter", "基石" = "Cornerstone", "地面C" = "Ground C", "号角" = "Horn", "挡人先锋" = "Blocking Vanguard")
            "location": [ 6, 4 ],                     // Deploys the first agent it finds to coordinates 6,4 and faces left.
            "direction": "left",                      // If it doesn't find it, proceed to the next deployment operation.
        },                                    
        {
            "groups": [ "召唤" ],                     // ("召唤" = "Summoner")
            "location": [ 6, 3 ],
            "direction": "left"
        },
        {
            "groups": [ "单奶", "群奶" ],             // ("单奶" = "Single-target Healer", "群奶" = "AoE Healer")
            "location": [ 6, 2 ],
            "direction": "down"
        }
    ]
    ```

    ::: info
    MAA flattens all deployment commands and then executes the highest-priority deployment operations
    Example: deploys [ "百嘉" (Gavial), "基石" (Cornerstone), "地面C" (Ground C)] on [6,4], deploys [ "基石" (Cornerstone), "地面C" (Ground C)] on [6,3], then MAA will flatten the deployment commands to [ "百嘉" (Gavial), "基石" (Cornerstone), "地面C" (Ground C), "基石" (Cornerstone), "地面C" (Ground C)]
    If a "百嘉" (Gavial) operator on [6,4] is retreated during battle, the "基石" (Cornerstone) operator in hand, if available, will be deployed on [6,4], instead of [6,3]

    This means that from a macro perspective, after each deployment action is completed, it will start checking for executable strategies from the beginning (the current step's position has no already placed operators, and there are operators in the deployment area belonging to this step's operator group)
    :::

    ::: tip
    Some commonly used operator group usages:

    1. In many operations, the main defensive point combination is [ "地面阻挡" (Ground Blocking), "处决者" (Executor), "其他地面" (Other Ground)], which means when the main tanking operator dies, it will try to use an executor to delay the CD; when there's too much survival pressure on this point, consider using [ "重装" (Heavy Defender), "地面阻挡" (Ground Blocking), "处决者" (Executor), "炮灰" (Fodder), "其他地面" (Other Ground)]; for positions behind defenders, prioritize [ "地面远程" (Ground Ranged), "地面阻挡" (Ground Blocking), "处决者" (Executor), "其他地面" (Other Ground)]; to purely attract aggro or sacrifice, use [ "炮灰" (Fodder), "障碍物" (Obstacles), "其他地面" (Other Ground)]

    2. High ground position combinations commonly use [ "高台输出" (Ranged DPS), "其他高台" (Other High Ground)], if you want any high ground to be placed you can use [ "高台输出" (Ranged DPS), "狙击" (Sniper), "辅助" (Supporter), "盾法" (Shield Caster), "其他高台" (Other High Ground)]

    3. Some ground positions are suitable for both SilverAsh and ground trap operators [ "玛恩纳" (SilverAsh), "地刺" (Ground Traps)]
    :::

2. Deployment of operators at a point in time
    ::: tip
    Suitable for certain single-cutting operators or usage scenarios that require cannon fodder
    :::

    ```json
    "deploy_plan": [
            {
                "groups": [ "异德", "刺客", "挡人先锋", "其他地面" ],
                                  // ("异德" = "Strange Virtue", "刺客" = "Assassin", "挡人先锋" = "Blocking Vanguard", "其他地面" = "Other Ground")
                "location": [ 5, 3 ],
                "direction": "left",
                "condition": [ 0, 3 ]   // This operation is only performed when the number of kills is 0 - 3
            },
            {
                "groups": [ "异德", "刺客", "挡人先锋", "其他地面" ],
                "location": [ 5, 3 ],
                "direction": "left",
                "condition": [ 6, 10 ]  
            },
            ...
        ]
    ```

3. Retrieval of the operators at some point
   ::: tip
   Sometimes the fodder is too strong to hold the field or you need to deploy to move the lineup. What should I do? Retreat!

   Deployment and retreat commands for the same position should have non-overlapping condition numbers, otherwise it will instantly retreat after deploying
   :::

    ```json
     "retreat_plan": [                  // Retrieval targets at specific points in time
            {
                "location": [ 4, 1 ],
                "condition": [ 7, 8 ]   // At 7 - 8 kills, remove operator on [4,1], skip if there are none.
            }
        ]
    ```

4. Disable a skill at a certain point in time (to do)

5. Additional fields (not recommended)

    ```json
        "role_order_Doc": "Operator type deployment order, the unspecified parts will be filled in with the order of Guard, Vanguard, Medic, Defender, Sniper, Caster, Supporter, Specialist, Summoner, and so on.",
        "role_order": [                 // Not recommended, please configure the deploy_plan field.
            "warrior",
            "pioneer",
            "medic",
            "tank",
            "sniper",
            "caster",
            "support",
            "special",
            "drone"
        ],
        "force_air_defense_when_deploy_blocking_num_Doc": "When there are 10000 blocking units on the field, it will start to force deploying a total of 1 pair of empty units (fill in or not will not affect the normal deployment logic), during this period, it does not prohibit the deployment of medical units (no default is false).",
        "force_air_defense_when_deploy_blocking_num": { // Not recommended, please configure the deploy_plan field.
            "melee_num": 10000,
            "air_defense_num": 1,
            "ban_medic": false
        },
        "force_deploy_direction_Doc": "These points force deployment directions for certain occupation",
        "force_deploy_direction": [     // Not recommended, please configure the deploy_plan field.
            {
                "location": [
                    1,
                    1
                ],
                "role_Doc": "Application of mandatory orientation to the occupation entered",
                "role": [
                    "warrior",
                    "pioneer"
                ],
                "direction": "up"
            },
            {
                "location": [
                    3,
                    1
                ],
                "role": [
                    "sniper"
                ],
                "direction": "left"
            }
        ],
    ```

::: info
When MAA cannot find a customized combat strategy for the current level, it will automatically execute the general combat strategy.

When MAA fails to recognize the current level name, it will not execute any combat logic.
:::

### Have a special understanding of an operator's playing style? -- Refined operation of specific operators

Please group this operator separately

When writing assignments, please consider the sequential priority of this operator with existing assignments

Or you could just forget about it, and write a combat logic for this agent alone

It's also possible to use just one operator! Use MAA to clear the level (due to other imperfections in logic, the possibility is very low)

Reference examples: 1. Thorns in Phantom I.S. 2. Strange Virtue in Mizuki I.S. 3. Reed the Flame Shadow/Weathered in Sami I.S. 4. Weathered in Sarkaz I.S.

## Integrated Strategy Step 3: Encounters Node logic

`resource/roguelike/theme_name/encounter/default.json` Strategy for the selection of Encounter events in the leveling mode

`resource/roguelike/theme_name/encounter/deposit.json` Strategy for the selection of Encounter events in the investment mode

### MAA currently has a method for judging Encounters

OCR recognises Encounters, but the option is operated in a fixed position.

If the Encounter is not recognized, it will click the choice at the bottom

Generally, it only requires slight adjustment or no adjustment at all (the devs have already taken care of this)

A common reason for getting stuck is when there's an option type icon but the option is not selectable (TODO)

### Optimise the priority of the Encounter options

Please refer to [prts.wiki](https://prts.wiki/w/%E9%9B%86%E6%88%90%E6%88%98%E7%95%A5) to see the effects of each Encouter's options, note that the options are not necessarily fixed.

The Encounter options can be modified to guide MAA towards special endings

```json
{
    "theme": "Sami",                              // I.S. Theme
    "stage": [                                    // Encounter event
        {
            "name": "低地市集",                    // Name of the Encounter ("低地市集" = "Lowland Market")
            "option_num": 3,                      // There are several options in total (In this case: 3)
            "choose": 3,                          // Which option should you choose first (the third one is preferred here)
                                                  // If you can't choose it, choose the escape option (basically the last one)

            "choices": [                          // Requirements for selecting options
                                                  // (it does not affect program operation for the time being
                                                  // only marking applicable situations for easy modification)
                {
                    "name": "选择碎草药",          // Option name ("选择碎草药" = "Selection of herbs")
                    "ChaosLevel": {               // Immunity / Light level
                        "value": "3",             // Required number
                        "type": ">"               // Is it greater than or less than
                                                  // (here it means that the Immunity / Light level is greater than 3
                                                  // only when the option of "Selection of herbs" is activated)
                    }
                },
                {
                    "name": "选择好看的织物",      // ("选择好看的织物" = "Choosing good-looking fabrics")
                    "ChaosLevel": {
                        "value": "3",
                        "type": ">"
                    }
                },
                ...
```

### Dynamically prioritise certain options according to the team situation (TODO)

## Integrated Strategy Step 4: Prioritising Trade Store collection

`resource/roguelike/theme_name/shopping.json` Describes strategies for purchasing collectibles in the store (and selecting collectibles after combat?)

```json
{
    "theme": "Phantom",                                       // I.S. Theme
    "priority": [                                             // Order is the priority
                                                              // The higher the order, the higher the priority to buy,
                                                              // However, the priority judgment is before the screening of chars and roles.
                                                              // high-priority products may be screened out and nothing is purchased.
        {
            "name": "金酒之杯",                                // Collectible name ("金酒之杯" = "Golden Chalice")
            "no_longer_buy": true,                             // true means don't spend money on the item after getting it
                                                               // false or empty means continue to spend money on the item
            "ignore_no_longer_buy": true,                      // True means that "no_longer_buy" is ignored when the store has the collectible
                                                               // that is, it will be bought
                                                               // False or omitted, means that the store has it.
            "effect": "每有5源石锭, 所有我方单位的攻击速度+7",    // Collectible effect ("For every 5 Originium Ingots, all friendly units get +7 ASPD")
                                                               // (does not affect the operation, useful for convenient sorting)
            "no": 167                                          // Collectible number (can be found in the wiki, does not affect the operation)
        },
        
        ...
        {
            "name": "扩散之手",                                 // ("扩散之手" = "Hand of Diffusion")
            "chars": [                                         // Buy this item when you have these operators in the team
                "异客"                                          // ("异客" = "Passenger") (This means that if you have Passenger in your team,
                                                               // you will try to buy the Hand of Diffusion when you encounter it)
            ],
            "effect": "【扩散术师】、【链术师】和【轰击术师】每对一个单位造成伤害就回复2点技力值", // ("[Diffusion Artist], [Chain Artist], and [Blast Artist] regain 2 SP for each unit they deal damage to.")
            "no": 136                                          
        },
        ...

        {
            "name": "折戟-破釜沉舟",                            // ("折戟-破釜沉舟" = "Folding the halberd - Breaking the cauldron")
            "roles": [                                         // Buy this collectible when you have these classes in your team
                "WARRIOR"                                      // (This means that if you have a Guard operator in your team
                                                               // you will try to buy a Halberd-Breaker when you encounter it)
            ],
            "effect": "所有【近卫】干员的防御力-40%，但攻击力+40%，攻击速度+30", // ("All [Guard] Operators members have -40% Defence, but +40% Attack Power and +30% Attack Speed.")
            "no": 16                                           
        },
        ...

        {
            "name": "Miss.Christine摸摸券",                     // ("Miss.Christine摸摸券" = "Miss.Christine Touch coupon")
            "promotion": 2,                                   // Purchased when there are 2 operators in the team to be promoted
            "effect": "立即进阶两个干员（不消耗希望）",           // ("Immediate promote two operators (Does not consume Hope)")
            "no": 15
        },
        ...

        {
            "name": "警戒篱木",                                 // ("警戒篱木" = "Alert Fence")
            "effect": "坍缩值-2，目标生命上限+2",                // ("Collapse value -2, target max life +2")
            "No": 198,
            "decrease_collapse": true                          // true means getting this collectible will reduce collapse value. Will not be purchased when mode is 5
        },
        ...

    "others":                                                  // MAA won't buy these collectibles, like ending collectibles and the crane
        {
            "name": "无人起重机"                               // ("无人起重机" = "Unmanned Crane")
        },
```

## Integrated Strategy Special Mechanisms

### Sami Integrated Strategy - Foldartals

`resource/roguelike/Sami/foldartal.json` describes the strategy for Sami's Foldartals

```json
{
    "theme": "Sami",                                         // I.S. Theme (Sami)
    "groups": [                                              // Groups for usage situations and methods
        {
            "usage": "SkipBattle",                           // Usage (Skip battle, used for money farming and hot water mode combat nodes, use plates to skip battles to save time)
            "doc": "跳过战斗,刷钱和烧开水模式",                // ("Skip battle, money farming and hot water mode")
            "pairs": [                                       // Plate pairs (when encountering corresponding nodes, will check if there are matching plate pairs as defined below,
                                                            // if yes, will use all available ones, if no, will enter the node directly)
                {                                            // (Here "伤痕" (Scar) can only link with "空无" (Void))
                    "up": [                                  // Upper plate
                        "伤痕"                               // ("伤痕" = "Scar")
                    ],
                    "down": [                                // Lower plate
                        "空无"                               // ("空无" = "Void")
                    ]
                },
                {                                            // (Here will search in order "黜人"+"惊讶","黜人"+"疑惑","黜人"+...,"猎手"+"惊讶","猎手"+"疑惑","猎手"+... , ...)
                    "up": [
                        "黜人",                              // ("黜人" = "Ejection")
                        "猎手",                              // ("猎手" = "Hunter")
                        ...
                    ],
                    "down": [
                        "惊讶",                              // ("惊讶" = "Surprise")
                        "疑惑",                              // ("疑惑" = "Doubt")
                        ...
                    ]
                }
            ]
        },
        {
            "usage": "Boss",                                 // (Here indicates plate pairs to be used when encountering boss nodes)
            "doc": "有的用全用了",                           // ("Use all available ones")
            ...
        }
    ],
    "foldartal": [                                          // Foldartal effects notes (these are just notes for easy reference of plate effects, do not affect program operation)
        {
            "name": "布局",                                  // Foldartal type (upper or lower plate)
            "foldartal": [
                {
                    "name": "黜人",                           // Foldartal name ("黜人" = "Ejection")
                    "effect": "选择所有右侧邻近的战斗节点"     // Foldartal effect ("Select all combat nodes adjacent to the right")
                },
```

### Sami Integrated Strategy - Collapsal Paradigms

When `check_collapsal_paradigms` is `true`, MAA will check for Collapsal Paradigms in two different ways:

- Click on the upper middle of the screen at the level selection interface to expand the collapse status bar, hereinafter referred to as Panel Check;
- Observe whether there is a Collapsal Paradigm notification on the right side of the screen, hereinafter referred to as Banner Check.

There are various ways to get collapse values, we have considered the following situations:

1. After combat, the collapse value increases due to imperfect combat, perform Banner Check.
2. After combat, the collapse value changes due to obtaining collectibles, perform Banner Check.
3. In encounter nodes, etc., the collapse value changes due to selection options, perform Banner Check.
4. In the trader node, the collapse value changes due to purchasing collectibles, perform Banner Check.
5. The collapse value decreases due to using Foldartals, perform Banner Check.
6. The collapse value increases due to entering a new floor, perform Panel Check.
7. If during Banner Check it is found that the Collapsal Paradigm has receded, since we don't know if two layers can recede at once (even if they can, the probability is extremely low), an additional Panel Check will be triggered the next time we return to the level selection interface.
8. When `double_check_collapsal_paradigms` is `true`, an additional Panel Check will be triggered each time we return to the level selection interface to verify whether there were any missed or extra recorded Collapsal Paradigms.

Here is an example task configuration for farming hidden Collapsal Paradigms:

```json
{
    "theme": "Sami",
    "mode": 5,
    "investment_enabled": false,
    "squad": "远程战术分队",                   // ("远程战术分队" = "Ranged Tactics Squad")
    "roles": "稳扎稳打",                       // ("稳扎稳打" = "Steady Approach")
    "core_char": "维什戴尔",                   // ("维什戴尔" = "Weathered")
    "expected_collapsal_paradigms": ["目空一些", "睁眼瞎", "图像损坏", "一抹黑"] 
                                               // ("目空一些" = "Blank Somewhat", "睁眼瞎" = "Blind Eye", "图像损坏" = "Image Distortion", "一抹黑" = "Pitch Black")
}
```

When `mode` is `5`:

- Priority is given to using combat strategies with `stage_name` as `关卡名_collapse`, for example `resource/roguelike/Sami/autopilot/事不过四_collapse.json`;
- Uses the encounter event selection strategy described in `resource/roguelike/Sami/encounter/collapse.json`,
- Will not purchase collectibles with `decrease_collapse` set to `true`.

When `mode` is not `5` but `check_collapsal_paradigms` is `true`, it will still detect Collapsal Paradigms and stop the task when encountering Collapsal Paradigms in the `expected_collapsal_paradigms` list, but will not restart when encountering other Collapsal Paradigms.

For farming hidden Collapsal Paradigms, N10 difficulty is recommended, with the following teams suggested:

- Weathered + Spot + Steward;
- Reed the Flame Shadow + Orchid + Popukar;
- Toddifons + Spot + Steward.

## Desired Logic (todo)

### Automatic Formation Logic

1. Different map formation completeness tests and skill priorities can be set for different maps

2. You can avoid some difficult battles based on the available lineups

### Optimising the pathfinding algorithm (already initially implemented)

For example, more battles in the first three levels and fewer battles in the later ones, so that the development will be better

For example, identify connections between nodes to determine a more optimal path

### Skill Retention

The operator deployed in a certain frame, wait x seconds for the skill to turn on after the skill has been turned on, easy to align the axes; you can write Skill_hold under Deploy_plan, or you can write Skill_hold_plan

### Skills Shutdown

Useful for operators who have ammo skills
