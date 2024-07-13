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
    `Phantom/` for Phantom's I.S.
    `Mizuki/` for Mizuki's I.S.
    `Sami/` for Sami's I.S.
    - `autopilot/`combat json for each level
      - `level_name_in_chinese.json` combat logic of the level
    - `encounter/` encounter nodes logic
      - `default.json` levelling mode
      - `deposit.json` investing mode
    - `recruitment.json` operators recruitment logic
    - `shopping.json` trade store purchasing logic

## Integrated Strategy Step 1: Operator Recruitment

`resource/roguelike/theme_name/recruitment.json` describes the logic of the operator recruitment

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

Split operators in different ***groups*** according to your understanding of the game(Group, related conceptual references [Copilot Schema](./copilot-schema.md))

::: info

1. Operators and summons within the same group must be deployed in the same way (i.e. both melee or ranged)

2. Allows the same operator or summon to be sorted in different groups, depending on its usage.

3. Please do not change the name of an existing group, as this may cause previous versions of the task to be unavailable when MAA is updated!

4. Please try not to add new group, instead try to implement new operators added to the task, into existing groups according to the usage
:::

::: tip
By default, only E1 Level 55 operators will be recruited
:::

```json
{
    "theme": "Phantom",              
    "priority": [                     // Groups, in order
        {
            "name": "棘刺",           // Group name (In this case Thorns)
            "opers": [                // Group contains operators, ordered, representing the deployment priority
                                      //(e.g. if you need to deploy the group's operators, test for Thorns, deploy Thorns if it's available, test for Horn is Thorns is not available)
                {
                    "name": "棘刺",   // Operator's name (Thorns)
                    ...
                },
                {
                    "name": "号角",   // Operator's name (Horn)
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

    Take Phantom's I.S. as an example: operator's are mainly divided in
    | Group  | Considerations | Class | Operators |  
    | :--- | :--- | :--- | :--- |
    | ***Ground block***  | 站场和清杂  | 重装, 近卫 | 奶盾, 基石, 羽毛笔, 山, M3, 令和稀音的召唤物, 斑点, 重装预备干员 |
    | ***Ground single-block***  | 单独对战精英怪  | 处决者特种 | 史尔特尔, 异德, 麒麟R夜刀, M3, 红 |
    | ***Ranged C***  | 常态和决战输出  | 狙击, 术士 | 假日威龙陈, 澄闪, 艾雅法拉, 肥鸭 |
    | ***Ranged DPS***  | 对空和常态输出  | 狙击, 术士 | 空弦, 能天使, 克洛丝, 史都华德 |
    | ***奶***  | 治疗能力  | 治疗, 辅助 | 凯尔希, 浊心斯卡蒂, 芙蓉, 安赛尔 |
    | ***回费***  | 回复cost  | 先锋 | 桃金娘, 伊内丝, 芬, 香草 |
    | ***炮灰***  | 吸收炮弹, 再部署  | 特种, 召唤物 | M3, 红, 桃金娘, 预备干员 |
    | ***高台预备***  | 有一定输出能力  |  | 梓兰, 预备干员 |

2. Groups requiring special handling

    In addition to those general groups, sometimes we need more custom tweaks for some operators or types such as
    | Group | Operators | Features |
    | :--- | :--- | :--- |
    | Thorns | Thorns, Horn | Long range melee, many maps have good spots |
    | Summoners | Kal'tsit, Ling, Scene | Melee blocking, some maps need deployment prioritization, summons can be used as blocks or fodder |
    | Agent | Cantabile, Ines | Has DP recovery, deals DPS and it's single block |
    | Skadi Alter | Skadi the Corrupting Heart | Low DP cost, special range, some maps have optimal spots |
    | Reed Alter | Reed the Flame Shadow | In Sami I.S. she's commonly used as opening caster, DPS and healing, some maps have optimal spots |
    | SilverAsh | SilverAsh, Młynar | Ground operators with large range , very useful against bosses |
    | Surtr | Surtr | Since Surtr always carries the 3rd skill, her positioning ability is almost zero, deployment priority is extremely low |
    | Dice | Dice | In Mizuki I.S. the dice needs to be operated separately |

::: info
Currently fixed to group unidentified ground crews behind the penultimate formation (other ground), and unidentified high-platform crews behind the penultimate formation (other high-platform)
:::

### Preset groups--Complete line-up testing

In a team that you expect to pass or reach the top, what are the basic core players? Are they essential? How many do you need?

::: info
The current script's recruiting logic is to only recruit 0 hope and key agents before the lineup meets the lineup readiness level, and save hope for high star key agents.
So don't set the threshold number too high, it is recommended that the number of all the operators needed (the basic core lineup) should add up to 4-8 digits.
:::

```json
{
    "theme": "Phantom",              // I.S. Theme
    "priority": [                    // Group
             ...
        ],
    "team_complete_condition": [     // Formation completness test
        {
            "groups": [              // Which group members are needed?
                "C-team"
            ],
            "threshold": 1           // How many of these operators are needed?
        },                           //(This indicates a need for an officer from the C-team on the high platform)
        {
            "groups": [
                "Thorns",              //(A minimum of 2 operators are required from Thorns, GroundBlock, Ground1Block, Fodder)
                "GroundBlock",
                "Ground1Block",
                "Fodder"
            ],
            "threshold": 2
        },
        ...
        ]
} 
```

### Adjusting parameters for operator recruitment

1. The order within a group represents the priority of the deployment detection

2. Meaning of each field and script-related logic for group members

   ```json
   {
       "theme": "Phantom",              
       "priority": [
           "name": "GroundBlocking",                         // Group name (in this case GroundBlocking)
           "doc": "The standard line is 1st gear (clearing ability or field ability is better than mountain) > Mountain > 2nd gear (block>2, can return to itself) > Spot, field ability is less than spot to single cut or cannon fodder group",
                                                             // Doc field are just notes. Has no effect on the program
           "opers": [                                        // What operators should be included, in ordered, represents deployment priority.
               {
                   "name": "Gavial the Invincible",          // Operator name (Gavial the Invincible, the first position in the group indicates that
                                                             //when it is necessary to deploy a ground blocking group, the first thing to check is whether it is a Gavial Alter or not.)
                   "skill": 3,                               // Which skill to use (In this case skill 3)
                   "skill_usage": 2,                         // Skill usage mode, refer to 3.3COPILOT_SCHEMA, default to 1 if empty, 2 is only put x times
                                                             // (x is set by "skill_times" field), 3 is not supported for now.
                   "skill_times": 2,                         // Skill usage, default is 1, effective when "skill_usage" field is 2.
                   "alternate_skill": 2,                     // Alternative skills used when there is no designated skill, usually 6-star operators who have not 
                                                             // E2'd and use 3 skills after promotion
                                                             // (in this case, 2 skills are used when there is no 3 skills).
                   "alternate_skill_usage": 1                // Skill use mode for alternative skills (this field has not yet been implemented)
                   "alternate_skill_times": 1                // Number of skill uses for alternative skills (this field has not yet been implemented)
                   "recruit_priority": 900,                  // Recruitment priority, bigger number, higher priority, more than 900 belongs to must be recruited
                                                             // 400 below the recruitment priority than some of the key operator essence of the second priority is still low
                                                             // Temporarily recruited operator priority automatically +800
                   "promote_priority": 600,                  // Advancement priority, bigger number, higher priority, above 900 is a guaranteed E2 if there's enough hope
                                                             // below 400 recruitment priority is lower than the recruitment of ordinary three-star operators
                                                             // Tip: When you lower your recruitment priority or don't write it in, and raise the priority of some of
                                                             // your E2, you're actually raising the priority of the E2 who temporarily recruited these operators.
                   "is_key": true,                           // If true, the operator is a key element. Default to false if empty.
                                                             // If the lineup completeness test is not passed, only key and 0 hope are recruited, saving hope.
                   "is_start": true,                         // If true, the operator is a stater. Default to false if empty. If there is no start player in the team,
                                                             // only start players and 0 hopeful players will be recruited, and user-filled players will be recruited.
                   "auto_retreat": 0,                        // Auto-retreat after a few full-seconds of deployment, takes effect when greater than 0, mainly used for specialists and vanguards, 
                                                             // since I.S. usually starts at 2x speed, it is recommended to set it to skill duration divided by 2
                   "promote_priority_when_team_full": 850,   
                   "recruit_priority_offsets": [             // Prioritise recruitment according to current line-up
                       {
                           "groups": [                       // Which groups are required to fulfil the conditions
                               "Kaltsit",                      
                               "GroundBlocking",
                               "Thorns"
                           ],
                           "is_less": false,                 // True is less than, false is greater than. Default to false if empty
                           "threshold": 2,                   // Number of conditions met
                           "offset": -300                    // Adjustments to recruitment priorities after fulfilment
                                                             // (This means that when there are 2 or more operators in Kaltsit, GroundBlocking and Thorns,
                                                             // the recruitment priority of Gavial the Invincible is reduced by 300)
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

3. Add groups and opeators as you see fit

    When you add a new group, you can copy the operator from an existing group. Refer to the ratings already given by the devs, and modify them on that basis

## Integrated Strategy Step 2: Battle Logic

`resource/roguelike/theme_name/autopilot/level_name.json` Describes the combat strategy for each level

### Basic I.S. fighting logic of MAA

1. Perform basic combat operations based on the grid type of the map

    - MAA performs basic combat operations based on whether the grid on the map is a blue or red gate, whether it's a high platform or ground, and whether it can be deployed or not.

    - MAA decides which job to use based on the name or number of the map only, and does not judge the map's **Standard**, **Emergency**, **Road Network**, **Classified Board Use**, etc.

    - MAA does not judge **in combat the situation of undefined squares on the map**, e.g. the position of the altar in the `Taming Hut`, the `follower effect` of monsters coming out of the left side or the right side.

   So in the future, you need to try to design a set of combat logic that can cope with **all the different scenarios of a map name** (the above mentioned scenarios), and be careful of being hung up on the issue that this map operates in Emergency Mode.

2. MAA's Basic Battle Strategy -- Blocking the Blue Entry Point

    1. Ground Crews are preferentially deployed on (or around) the grid of the Blue Gate (why the grid, scroll down), orientated towards the Red Gate (auto-calculated), and are not deployed on the Red Gate

    2. Prioritise the ground before healing and ranged, in a circle from the blue gate to the perimeter

    3. It will keep deploying things that can be deployed according to the logic above (operators, summons, support items, etc.)

### Optimise basic combat strategies

1. Blue Gate Alternative

    It's obviously not smart to just pile up your operators in front of the blue door, some levels have grids where you can't get through, and the defence is obviously very efficient here

    Or if there are levels with multiple blue gates, and the MAA doesn't know which blue gate corresponds to which red gate, they may deploy randomly

    At this point you'll need to open the [map wiki](https://map.ark-nights.com/areas?coord_override=maa) while imagining the battle in your head

    Use this link, otherwise switch the `Coordinate Display` to `MAA` in `Settings`

    Then, based on your experience, find the coordinates and orientation of the points that need to be prioritised for defence, and write them into the `replacement_home` of the json.

    ```json
    {
        "stage_name": "蓄水池",        // Level name (In chinese)
        "replacement_home": [         // Entry points (blue gates replacement points),  
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
                                      //to the grid at coordinates 6,4 to the left.)
            }
        ],
        ...
    ```

2. Deployment grid blacklist

    There are points of priority for defence and points of priority for not deploying operators, such as where the fireball passes through, under the boss's feet, and some locations that are not good for output.

    At this point we introduce `blacklist_location` to blacklist the grids that we don't want MAA to deploy operators on.
    ::: info
    The grids added here will not be deployed on even if they are included in the deployment strategy later on.
    :::

    ```json
        ...
        "blacklist_location_Doc": "This is an example of usage, not that the map "Cistern" needs ban.",
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

    For example, if there is a monster at the blue door in Mizuki I.S. should we use the dice to ease the pressure of stacking monsters?

    ```json
        "not_use_dice_Doc": "When the Blue Gate operator is retreated does MAA need to use the dice. Defaults to false if empty",
        "not_use_dice": false,
    ```

### Or is it Emergency Operation? It's time to show your true while skills - customised combat strategies!

Customisation using `"deploy_plan"` and `"retreat_plan"`

The customised strategy takes precedence over the basic combat strategy, and when all the steps in the customised strategy have been attempted, the saved or fallen re-deployed operator will be deployed in accordance with the basic combat strategy.

There is no need to set up too many customised plans when there is a problem. It may be better to hand over to MAA after completing the key steps, or a combination of both.

1. Deployment of operators using groups

    ```json
    "deploy_plan": [ // Deployment logic: order from top-to-bottom, left-to-right
                     // Tries to deploy the first operator it finds, or skips it if it doesn't.
        {
            
            "groups": ["GavialAlter", "Mudrock", "GroundC", "Horn", "Vanguard"], // Looks for operators from these groups
            "location": [ 6, 4 ],                     // Deploys the first agent it finds to coordinates 6,4 and faces left.
            "direction": "left",                      // If it doesn't find it, proceed to the next deployment operation.
        },                                    
        {
            "groups": [ "Summoner" ],          
            "location": [ 6, 3 ],
            "direction": "left"
        },
        {
            "groups": [ "单奶", "群奶" ],
            "location": [ 6, 2 ],
            "direction": "down"
        }
    ]
    ```

    ::: info
    MAA flattens all deployment commands and then executes the highest priority deployment operations
    Example: deploys [ "Gavial", "Cornerstone", "GroundC"] on [6,4], deploys [ "Cornerstone", "GroundC"] on [6,3], then MAA will flatten the deployment commands to [ "Gavial", "Cornerstone", "GroundC", "Cornerstone", "GroundC"]
    If a "Gavial" operator on [6,4] is retreated during battle, the "Cornerstone" operator in hand, if avaiable, will be deployed on [6,4], instead of [6,3]
    :::

2. Deployment of operators at a point in time
    ::: tip
    Suitable for certain single-cutting operators or usage scenarios that require cannon fodder
    :::

    ```json
    "deploy_plan": [
            {
                "groups": [ "StrangeVirtue", "Assasin", "Vanguard", "OtherFloors" ],
                "location": [ 5, 3 ],
                "direction": "left",
                "condition": [ 0, 3 ]   // This operation is only performed when the number of kills is 0 - 3
            },
            {
                "groups": [ "StrangeVirtue", "Assassin", "Vanguard", "OtherFloors" ],
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

### Have a special understanding of an operator's playing style? -- Refined operation of specific operators

Please group this officer separately

When writing assignments, please consider the sequential priority of this officer in relation to existing assignments

Or you could just forget about it, and write a combat logic for this agent alone

It's also possible to use just one operator! Use MAA to clear the level (due to other imperfections in logic, the possibility is very low)

Reference examples: 1. Thorns in Phantom I.S. 2. Texas the Omertosa in Mizuki I.S. 3. Reed the Flame Shadow in Sami I.S.

## Integrated Strategy Step 3: Encounters Node logic

`resource/roguelike/theme_name/encounter/default.json` Strategy for the selection of Encounter events in the leveling mode

`resource/roguelike/theme_name/encounter/deposit.json` Strategy for the selection of Encounter events in the investment mode

### MAA currently has a method for judging Encounters

OCR recognises Encounters, but the option is to operate on a fixed position.

If the Encounter is not recognized, it will click the choice at the bottom

Generally, it only requires slight adjustment or no adjustment at all (the devs have already taken care of this)

### Optimise the priority of the Encouter options

Please refer to [prts.wiki](https://prts.wiki/w/%E9%9B%86%E6%88%90%E6%88%98%E7%95%A5) to see the effects of each Encouter's options, note that the options are not necessarily fixed.

The Encounter options can be modified to guide MAA towards special endings

```json
{
    "theme": "Sami",                              // I.S. Theme
    "stage": [                                    // Encounter event
        {
            "name": "低地市集",                    // Name of the Encounter
            "option_num": 3,                      // There are several options in total (In this case: 3)
            "choose": 3,                          // Which option should you choose first (the third one is preferred here)
                                                  // If you can't choose it, choose the escape option (basically the last one)

            "choice_require": [                   // Requirements for selecting options
                                                  // (it does not affect program operation for the time being
                                                  // only marking applicable situations for easy modification)
                {
                    "name": "Selection of herbs", // Option name
                    "ChaosLevel": {               // Immunity / Light level
                        "value": "3",             // Required number
                        "type": ">"               // Is it greater than or less than
                                                  //(here it means that the Immunity / Light level is greater than 3
                                                  // only when the option of "Selection of herbs" is activated)
                    }
                },
                {
                    "name": "Choosing good-looking fabrics",
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
                                                              // The higher the order, the higher the priority to buy.,
                                                              // However, the priority judgment is before the screening of chars and roles.
                                                              // It may happen that high-priority products are screened out and nothing is purchased.
        {
            "name": "Golden Chalice",                          // Collectible name (In this case Golden Caliche)
            "no_longer_buy": true,                             // true means don't spend money on the item after getting it
                                                               // false or empty means continue to spend money on the item
            "ignore_no_longer_buy": true,                      // True means that "no_longer_buy" is ignored when the store has the collectible
                                                               // that is, it will be bought
                                                               // False or omitted, means that the store has it.
            "effect": "每有5源石锭, 所有我方单位的攻击速度+7",    // Collectible effect (does not affect the operation, useful for convenient sorting)
            "no": 167                                          // Collectible number (can be foundo in the wiki, does not affect the opration)
        },
        
        ...
        {
            "name": "Hand of Diffusion",
            "chars": [                                         // Buy this item when you have these operators in the team
                "Passenger"                                    // (This means that if you have Passenger in your team,
                                                               // you will try to buy the Hand of Diffusion when you encounter it)
            ],
            "effect": "[Diffusion Artist], [Chain Artist], and [Blast Artist] regain 2 SP for each unit they deal damage to.",
            "no": 136                                          
        },
        ...

        {
            "name": "Folding the halberd - Breaking the cauldron",
            "roles": [                                         // Buy this collectible when you have these classes in your team
                "WARRIOR"                                      // (This means that if you have a Guard operator in your team
                                                               // you will try to buy a Halberd-Breaker when you encounter it)
            ],
            "effect": "All [Guard] Operators members have -40% Defence, but +40% Attack Power and +30% Attack Speed.",
            "no": 16                                           
        },
        ...

        {
            "name": "Miss.Christine Touch coupon",
            "promotion": 2,                                   // Purchased when there are 2 operators in the team to be promoted
            "effect": "Immediate promote two operators (Does not consume Hope)",
            "no": 15
        },
```

## Desired Logic(todo)

### Automatic Formation Logic

1. Different map formation completeness tests and skill priorities can be set for different map

2. You can avoid some difficult battles based on the available lineups

### Optimising the pathfinding algorithm

For example, you can achieve more battles in the first three levels and less battles in the later ones, so that the development will be better

### Skill Retention

The operator deployed in a certain frame, wait x seconds for the skill to turn on after the skill has been turned on, easy to align the axes; you can write Skill_hold under Deploy_plan, or you can write Skill_hold_plan

### Skills Shutdown

Useful for operators that have ammo skills

<!-- markdownlint-disable-file MD026 -->
