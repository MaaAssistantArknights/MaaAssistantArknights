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
    `Sarkaz/` for Sarkaz's I.S.
    - `autopilot/` combat json for each level
      - `level_name_in_chinese.json` combat logic of the level
    - `encounter/` encounter nodes logic
      - `default.json` levelling mode
      - `deposit.json` investing mode
    - `recruitment.json` operators recruitment logic
    - `shopping.json` trade store purchasing logic

- Specifically, in `Sami/`
  - `foldartal.json` represents the Mysterious Foldartal usage logic
  - `collapsal_paradigms.json` represents the types of Collapsal Paradigms
  - `autopilot/level_name_collapse.json` level combat logic (Collapsal Paradigms farming mode)
  - `encounter/collapse.json` encounter logic for Collapsal Paradigms farming mode

## Integrated Strategy Step 1: Operator Recruitment

`resource/roguelike/theme_name/recruitment.json` describes the logic of the operator recruitment
There are helper tools in `tools/RoguelikeRecruitmentTool` and `tools/RoguelikeOperSearch` to help view and write

```json5
{
    "theme": "Phantom",              //I.S. Theme (In this case Phantom)
    "priority": [                    //Group's priority, in order
             ...
        ],
    "team_complete_condition": [     //Conditions for squad completeness
             ...
        ]
}
```

### Operator classification

Split operators in different ***groups*** according to your understanding of the game (Group, related conceptual references [Combat Flow Protocol](./copilot-schema.md))

::: info Note

1. Operators and summons within the same group must be deployed in the same way (i.e. both melee or ranged), and **should have similar attack range**

2. Allows the same operator or summon to be sorted into different groups, depending on its usage.

3. Please do not change the name of an existing group, as this may cause previous versions of the task to be unavailable when MAA is updated!

4. Please try not to add new groups, instead try to implement new operators added to the task, into existing groups according to the usage
:::

::: tip
By default, only E1 Level 55 operators will be recruited
:::

```json5
{
    "theme": "Phantom",
    "priority": [                     // Groups, in order
        {
            "name": "棘刺",           // Group name (In this case Thorns)
            "opers": [                // Group contains operators, ordered, representing the deployment priority
                                      // (e.g. if you need to deploy the group's operators, test for Thorns, deploy Thorns if it's available, test for Horn if Thorns is not available)
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

    Take Sami's I.S. as an example: operators are mainly divided into:

    | Group | Main Consideration | Main Classes | Example Operators |  
    | :--- | :--- | :--- | :--- |
    | ***Ground block*** | Field control and clearing | Defender, Guard | Healing Defenders, Cornerstone units, Plume, Mountain, M3, Ling and Himeko's summons, Spot, Defender Reserve Operators |
    | ***Ground single-block/Executioners*** | Solo elite enemies | Executor Specialists | Surtr, Eunectes, Kirara R Yato, M3, Projekt Red |
    | ***Ranged C*** | Sustained and boss DPS | Sniper, Caster | Wishslash, Logos, Ch'en the Holungday, Ebenholz |
    | ***Ranged DPS*** | Anti-air and sustained DPS | Sniper, Caster | Archetto, Exusiai, Kroos, Steward |
    | ***Fast-sniper*** | Physical DPS, standard range | Sniper | Aak, Exusiai, Fartooth, Kroos |
    | ***Caster*** | Arts damage, standard range | Single-target Caster | Eyjafjalla, Logos, Steward |
    | ***Supporter*** | Can hit 1 tile behind | Supporter | Pozyomka, Suzuran, Fartooth, Orchid |
    | ***Sniper*** | Long-range Ranged | Artilleryman, Siege | Wishslash, Toddifons, Rosa |
    | ***Medic*** | Healing capability | Medic, Supporter | Kal'tsit, Skadi the Corrupting Heart, Hibiscus, Ansel |
    | ***Single-target Medic*** | Attack range depth>=4 | Medic | Kal'tsit, Lumen, Purestream Eyjafjalla, Hibiscus, Ansel |
    | ***AoE Medic*** | Attack range depth<4, can heal behind | Medic, Supporter | Nightingale, Ptilopsis, Skadi the Corrupting Heart |
    | ***DP recovery*** | DP recovery | Vanguard | Myrtle, Ines, Fang, Vanilla |
    | ***Ground spikes*** | No blocking, provides DPS or slow in front of defenders | Trapper | Specter the Unchained, Ascalon, Manticore |
    | ***Ground ranged*** | Ground operators with extended range, can output behind shields | Instructor, Lord | Horn, Pallas, Thorns, SilverAsh |
    | ***Lord*** | Ground attack range depth>4, can attack air | Fortress, Lord | Thorns, SilverAsh, Chongyue |
    | ***Phalanx Caster*** | Short attack range, has some tanking ability | Phalanx Caster | Lin, Carnelian |
    | ***Fodder*** | Absorb damage, redeploy | Specialist, Summon | M3, Projekt Red, Myrtle, Reserve Operators |
    | ***Big Dragon*** | Tank in front of defenders, placed close for combining | Summon | Ling's dragons, Deepcolor Jessica's shield |
    | ***Supply Station*** | Speed up skill rotation for main DPS | Summon | Support Supply Station, Artificer operator summons |
    | ***Drones*** | Healing summons that ignore high/low ground | Summon | Skadi's Seaborn, Silence's Drone |
    | ***Support Trap*** | Ground-deployable explosives | Summon | Support Mist Generator, Support Rumble Generator |
    | ***Roadblock*** | Don't take deployment slots but attract aggro or block | Summon | Bird Cage, obstacles |
    | ***Other Ground*** | Ground operators not preferred for priority use | Push/Pull, Block-1 Vanguard, Swordmaster | Bagpipe, Croissant |
    | ***Ranged reserve/Other Ranged*** | Ranged operators not preferred for priority use | AoE Caster, Chain Caster, Tactician | Orchid, Reserve Operators |

    ::: tip
    Ground blocking mainly considers operator's overall defense capability (sometimes killing everything is also a strong defense), including Ground ranged and Lord groups

    Medic mainly considers overall healing capability, including Single-target and AoE Medic, need to separately use Single-target Medic (heal 4 tiles) or AoE Medic (heal behind) when considering attack range coverage

    Ranged DPS only considers output capability, mainly mixed sorting of Sniper and Caster classes, need to use Fast-sniper, Caster, Supporter, Phalanx Caster separately when considering output type, attack range and other limitations

    For trappers and similar summons that are numerous, don't put them in Support Trap, let MAA deploy them automatically works better
    :::

2. Groups requiring special handling

    In addition to those general groups, sometimes we need more custom tweaks for some operators or types such as:

    | Group | Operators | Main Features |
    | :--- | :--- | :--- |
    | Wishslash | Wishslash | High-ground output, priority deployment can reduce pressure |
    | Thorns | Thorns, Horn | Long-range melee output, Phantom I.S. has some maps with particularly suitable positions |
    | Summoners | Kal'tsit, Ling, Himeko | Melee blocking, some maps need deployment prioritization, summons can be used as blocks or fodder |
    | Agent | Cantabile, Ines | Has DP recovery, deals DPS and is single block |
    | Skadi Alter | Skadi the Corrupting Heart | Low pressure healing is adequate, but range is special, some maps have optimal positions |
    | Lumen | Lumen | Commonly used as an opening operator in Sami I.S., provides healing and DPS, some maps have optimal positions |
    | Młynar | Młynar, SilverAsh | Ground operators with large range decision making output, can be deployed for bosses |
    | Surtr | Surtr | Since E2 always carries the 3rd skill, her field control ability is almost zero and deployment priority is extremely low |
    | Dice | Dice | In Mizuki I.S. the dice needs to be operated separately |

::: info Note
Currently unidentified ground operators are automatically grouped into the second-to-last group (Other Ground), and unidentified ranged operators are grouped into the last group (Other Ranged)
:::

::: tip
Newly implemented operators need to be manually added to each I.S.'s `recruitment.json`, and the developer may not remember to make adaptations
If you find this, you can either create an issue to remind the developer or submit a PR directly
:::

### Preset lineup—Squad completeness detection

In a team that you expect to clear or reach high floors, which operators form your basic core lineup? Which ones are indispensable? And how many do you need?

::: info Note
The current script's recruiting logic is to only recruit 0 hope (currently only three-stars) and key (can be understood as whitelisted) operators before the lineup meets the completeness threshold, saving hope for high-star key operators
(The current implementation is to manually mark high-strength operators and three-stars as key operators, and only recruit key operators during recruitment) (TODO: Identify 0 hope operators)

So you need to set an appropriate completeness total. It's recommended that all the operators needed (the basic core lineup, generally 1 core, 2 ground, 1-2 healers, 1 ranged) should add up to 4-8 operators.

After the team meets the lineup completeness test, each time a recruitment permit is obtained, recruitment will be based on the operator's score and E2 priority, so to avoid wasting hope, operators you don't want to recruit can be set without a score or set below the score of three-star/reserve operators in the same class.
(For operators that are must-haves versus absolutely don't want, you can specially assign extremely high and extremely low scores, see `Mizuki` in `Sami`)

During combat formation, it will first read and store the default sorting of the operator selection interface (from top to bottom, left to right) in order.
Based on this order, operators meeting the lineup readiness that appear first will be moved to the beginning while maintaining their relative order, until the lineup readiness is fully met or there are no operators meeting the lineup readiness.
Then reserve operators will be moved to the end, forming a new order, and selection is made according to the new order.
In particular, since six-star temporary recruitment operators are at the very beginning by default during formation, they are easily included in the actual formation.
(TODO: Exclude E1 temporary recruited operators that cannot be used)

For beginner accounts: If 10 recruitments have not satisfied half of the key operators in the team, the completeness test will be abandoned, and operators will be used according to their scores, so beginner accounts without trained three-stars may end up with only two or three six-stars and a bunch of reserve operators.
:::

```json5
{
    "theme": "Phantom",              //I.S. Theme
    "priority": [                    //Groups
             ...
        ],
    "team_complete_condition": [     //Squad completeness detection
        {                            //A strategy group (condition)
            "groups": [              //Which group operators are needed
                "高台输出"            //(This indicates a need for a Ranged DPS group operator)
            ],
            "threshold": 1           //How many of these operators are needed
        },
        {                            //There can be multiple strategy groups
            "groups": [
                "棘刺",              //(A minimum of 2 operators are required from Thorns, Ground block, Ground single-block, Fodder)
                "地面阻挡",
                "地面单切",
                "炮灰"
            ],
            "threshold": 2
        },
        {
            "groups": [
                "奶"                 //(This indicates a need for a Medic group operator)
            ],
            "threshold": 1 
        }
        ...
        ]
}
```

::: caution
When an operator appears in multiple operator groups, it is only counted once
For example: Operator `Thorns` may appear in both the `Thorns` and `Ground block` groups, within this strategy group, operator `Thorns` is only counted once

But each strategy group counts separately
For example: Operator `Lumen` may appear in both the `Ranged DPS` and `Medic` groups, in this case both strategy groups can count operator `Lumen`
:::

### Adjusting operator recruitment parameters

1. The order of operators within a group represents the deployment priority when using that group

2. Temporary recruited operators will have 600 points added to their original score

3. Random promotion operators' scores are calculated by adding recruitment score + E2 priority score

4. The meaning of each field in the group of operators and script-related logic

    ```json5
    {
        "theme": "Phantom",
        "priority": [
            "name": "地面阻挡",                                // Group name (This is Ground block group)
            "doc": "标准线为1档(清杂能力或者站场能力比山强)>山>2档(阻挡>2,可自回)>斑点,站场能力小于斑点放到单切或者炮灰组",
                                                              // The "doc" field is for documentation in the JSON, does not affect program execution
            "opers": [                                        // Which operators to include, ordered, represents deployment priority
                {
                    "name": "百炼嘉维尔",                      // Operator name (This is Blemishine, in 1st position, meaning when deploying Ground block, check for Blemishine first)
                    "skill": 3,                               // Which skill to use (using skill 3 in this example)
                    "skill_usage": 2,                         // Skill usage mode, refer to Combat Flow Protocol, different in that default is 1 if not specified
                                                              // (0 for not auto use, 1 for use when ready, 2 for use x times (x set via "skill_times"), 3 not yet supported)
                    "skill_times": 2,                         // Number of skill uses, default is 1, effective when "skill_usage" is 2
                    "alternate_skill": 2,                     // Alternate skill when the specified skill is unavailable, usually needed for 6-star operators not E2 but using skill 3 when E2 (use skill 2 when skill 3 unavailable)
                    "alternate_skill_usage": 1,               // Alternate skill usage mode (This field is not yet implemented)
                    "alternate_skill_times": 1,               // Alternate skill usage times (This field is not yet implemented)
                    "recruit_priority": 900,                  // Recruitment priority, higher number means higher priority, 900+ means must recruit, below 400 means priority lower than promoting some key operators
                                                              // Temporary recruited operators automatically get +600 priority
                    "promote_priority": 600,                  // Promotion priority, higher number means higher priority, 900+ means promote with hope, below 400 means priority lower than recruiting normal three-star operators
                                                              // Tip: When you lower recruitment priority or don't specify it, and raise promotion priority, you're effectively raising E2 priority for temporarily recruited operators
                    "is_key": true,                           // true for key operators, false or omitted for non-key operators. When squad completeness is not met, only key operators and 0 hope operators will be recruited, saving hope
                    "is_start": true,                         // true for starting operators, false or omitted for non-starting operators. When no start operators are in team, only recruit start operators and 0 hope operators
                                                              // Linked to user interface starting operators, and user manually filled operators will be forcibly set as start operators
                    "auto_retreat": 0,                        // Auto retreat after X seconds of deployment, integer, effective when > 0, mainly for specialist operators and throwers, since I.S. usually runs at 2x speed, recommend setting to skill duration/2
                    "recruit_priority_when_team_full": 850,   // No need to set separately, when squad completeness is met, recruitment priority defaults to recruit priority-100
                    "promote_priority_when_team_full": 850,   // No need to set separately, when squad completeness is met, promotion priority defaults to promotion priority+300
                    "recruit_priority_offsets": [             // Adjust recruitment priority based on current lineup
                        {
                            "groups": [                       // Which groups need to meet conditions, counts by group, these groups should not have duplicate operators or they will be counted multiple times
                                "凯尔希",
                                "地面阻挡",
                                "棘刺"
                            ],
                            "is_less": false,                 // Is condition greater than or less than, false is greater than or equal to, true is less than or equal to, default is false if not specified
                            "threshold": 2,                   // Number that meets conditions, default is 0 if not specified
                            "offset": -300                    // Adjustment to recruitment priority after meeting condition, default is 0 if not specified
                                                              // (This means when there are 2 or more operators from Kal'tsit, Ground block, Thorns groups, Blemishine's recruitment priority is reduced by 300)
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

    ::: info Note
    The `recruit_priority_offsets` `group` should not have duplicate operators

    After setting `auto_retreat`, generally no need to set `retreat_plan` for it in the combat plan
    :::

5. Add new groups and operators based on your understanding

    After adding a new group, you can copy operators from existing groups, reference the scores given by experts, and modify based on that

## Integrated Strategy Step 2—Combat Logic

`resource/roguelike/theme_name/autopilot/level_name.json` describes the combat strategy for each level

### MAA Integrated Strategy generic combat logic—Source of high blood pressure for players

(Takes effect when no combat logic file exists for the current level name)

1. Basic combat operations based on tile types on the map

    - MAA will perform basic combat operations based on whether the tiles on the map are blue gates or red gates, high ground or ground, deployable or not

    - MAA only decides which strategy to use based on map name or ID, it does not judge map **normal**, **emergency**, **road networks**, **foldartal usage** or other variations

    - MAA does not judge **uncertain tiles during combat**, such as the altar position in `Beast Training Room`, or whether `Conformity Effect` spawns enemies from the left or right

    So later, you need to design a set of combat logic that can deal with **all different situations** for a map name (mentioned above), be careful not to be exposed on issues for high blood pressure operations (laugh)

2. MAA's generic combat strategy—Block blue gates

    1. Ground operators will preferentially deploy on blue gate tiles (why on the tiles, see below) or around them, facing the red gate (automatically calculated)

    2. Priority to deploy ground operators, then deploy medics and ranged operators, deploying in circles from the blue gate outward

    3. Will continuously deploy according to the logic above for deployable items (operators, summons, support items, etc.)

### Optimizing basic combat strategy

1. Blue gate alternative

    Simply stacking operators at blue gates is clearly not smart, some maps have tiles that are one-man chokepoints which are clearly more efficient for defense

    Or some maps have multiple blue gates and MAA doesn't know which blue gate corresponds to which red gate, leading to random deployment

    In these cases, you need to open the [Map Wiki](https://map.ark-nights.com/areas?coord_override=MAA) and command the battle in your mind while looking at the map

    First change the `Coordinate Display` to `MAA` in the `Settings`

    Then find the coordinates and direction of priority defense points based on your experience (usually facing the direction enemies come from), and write them in the `"replacement_home"` in the JSON

    ```json5
     {
         "stage_name": "蓄水池", // Level name
         "replacement_home": [ // Important defense points (blue gate alternatives), at least 1 must be filled
             {
                 "location": [ // Tile coordinates, obtained from map wiki
                     6,
                     4
                 ],
                 "direction_Doc1": "Priority direction, but doesn't mean it will absolutely be this direction (algorithm decides)",
                 "direction_Doc2": "Default is none if not filled, meaning no recommended direction, algorithm decides completely",
                 "direction_Doc3": "none / left / right / up / down / 无 / 上 / 下 / 左 / 右",
                 "direction": "left" // (This means preferentially deploy operators at coordinates 6,4 facing left, auto-deployed operators around it will also try to face left)
             }
         ],
    ```

    ::: tip
    Blue gate alternatives will take effect when all `deploy_plan` is completed but there are still operators in the deployment area, following the same logic as the generic combat strategy
    :::

2. Deployment tile blacklist

    Just as there are priority defense points, there are tiles we want to avoid deploying operators on, such as where fireballs pass through, under the boss, positions with poor DPS potential

    In these cases we introduce `"blacklist_location"` to add tiles we don't want operators to be deployed on to a blacklist

    ::: info Note
    Tiles added here cannot be deployed on even if they are specified in the deployment strategy later
    :::

    ```json5
        ...
        "blacklist_location_Doc": "This is a usage example, not saying the Reservoir map needs to ban these two points",
        "blacklist_location": [  // Positions where the program is forbidden from deploying
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

3. Other map strategies

    For example, in Mizuki I.S. if enemies reach the blue gate, should dice be used to relieve stacked enemy pressure

    ```json5
        "not_use_dice_Doc": "Whether dice should be used when operators at blue gate retreat, default is false if not specified",
        "not_use_dice": false,
    ```

### Still high blood pressure? Time to show your true skill—Custom combat strategy!

Use `"deploy_plan"` and `"retreat_plan"` to implement customized operations

Custom strategy takes precedence over generic combat strategy. After all steps in the custom strategy have been attempted, remaining operators or redeploying operators will be continuously deployed according to the generic strategy

Sometimes you don't need to set too many custom strategies, completing key steps and then letting MAA take over may work better as a combination

1. Deploy operators from various groups

    ```json5
    "deploy_plan": [ // Deployment logic, searching from top to bottom, left to right, and attempting to deploy the first operator found, skipping if none found
        {
            "groups": [ "百嘉", "基石", "地面C", "号角", "挡人先锋" ],//This step searches for operators from these groups
            "location": [ 6, 4 ],             //Search through Blemishine group, Cornerstone group, Ground C group, etc.
            "direction": "left"               //Deploy the first operator found at coordinates 6,4 facing left
        },                                    //If none found, proceed to the next deployment operation
        {
            "groups": [ "召唤" ],
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

    ::: info Note
    MAA will flatten all deployment instructions and execute the highest priority deployment operation
    Example: Deploy ["Blemishine", "Cornerstone", "Ground C"] at [6,4], deploy ["Cornerstone", "Ground C"] at [6,3], then MAA will flatten deployment instructions into ["Blemishine", "Cornerstone", "Ground C", "Cornerstone", "Ground C"] with coordinates
    If during battle, "Blemishine" group operator at position [6,4] falls, and there's a deployable "Cornerstone" group operator, it will be preferentially placed at [6,4] not [6,3]

    This means from a macro perspective, after executing each deployment action, it will check executable strategies from the beginning (current step's position has no already placed operator, and deployment area has operators belonging to this step's group)
    :::

    ::: tip
    Some common group usages:

    1. In many tasks, the main defensive point combination is ["Ground block", "Executioners", "Other Ground"], meaning when the main tanking operator dies, it will try to use Executioners to stall for cooldown; when this point has high survival pressure, consider using ["Defender", "Ground block", "Executioners", "Fodder", "Other Ground"]; for positions behind shields, prioritize ["Ground ranged", "Ground block", "Executioners", "Other Ground"]; for purely attracting aggro or sacrificing, use ["Fodder", "Roadblock", "Other Ground"]

    2. High ground position combinations commonly use ["Ranged DPS", "Other Ranged"], if you want any high ground operator to be placed, use ["Ranged DPS", "Sniper", "Supporter", "Phalanx Caster", "Other Ranged"]

    3. Some ground positions are suitable for both SilverAsh-type operators and trappers ["Młynar", "Ground spikes"]
    :::

2. Deploy operators at specific kill counts
    ::: tip
    Suitable for certain single-target operators or scenarios requiring fodder
    :::

    ```json5
    "deploy_plan": [
            {
                "groups": [ "异德", "刺客", "挡人先锋", "其他地面" ],
                "location": [ 5, 3 ],
                "direction": "left",
                "condition": [ 0, 3 ]       // This operation only occurs when kill count is 0-3
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

3. Retreat operators at specific kill counts
    ::: tip
    Sometimes fodder is too strong and holds the line or you need to reorganize positions, retreat!

    Deployment and retreat commands for the same position should not have overlapping condition numbers, otherwise it will instantly place and remove operators
    :::

    ```json5
    "retreat_plan": [  // Retreat targets at specific times
            {
                "location": [ 4, 1 ],
                "condition": [ 7, 8 ] // When kill count is 7-8, retreat the operator at position [4,1], skip if none
            }
        ]
    ```

4. Activate skills at specific kill counts (to do)

5. Some other fields (not recommended to use)

    ```json5
        "role_order_Doc": "Operator type deployment order, unspecified parts are completed with Guard, Vanguard, Medic, Defender, Sniper, Caster, Supporter, Specialist, Summon order, input in English",
        "role_order": [ // Not recommended, please configure deploy_plan field
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
        "force_air_defense_when_deploy_blocking_num_Doc": "When there are 10000 blocking units on the field, start forcing deployment of 1 anti-air unit (filling or not filling doesn't affect normal deployment logic), during this time don't ban deploying medical units (default is false if not specified)",
        "force_air_defense_when_deploy_blocking_num": { // Not recommended, please configure deploy_plan field
            "melee_num": 10000,
            "air_defense_num": 1,
            "ban_medic": false
        },
        "force_deploy_direction_Doc": "These points have forced deployment direction for certain classes",
        "force_deploy_direction": [ // Not recommended, please configure deploy_plan field
            {
                "location": [
                    1,
                    1
                ],
                "role_Doc": "Classes listed apply forced direction",
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

::: info Note
When MAA cannot find custom combat strategy for the current level, it will automatically execute the generic combat strategy

When MAA fails to recognize the current level name, it will not execute any combat logic
:::

### Have special understanding of certain operator playstyles? —Fine-tuned operations for specific operators

Please place this operator in a separate group

When writing tasks, consider the order priority of this operator with existing tasks

You can also not think too much about it and write a separate combat logic for this operator

Using just one operator is also possible! Use MAA for single-operator clear attempts (due to other logic imperfections, the probability is very low)

Reference examples: 1.Thorns in Phantom I.S., 2.Eunectes in Mizuki I.S., 3.Lumen/Wishslash in Sami I.S., 4.Wishslash in Sarkaz I.S.

## Integrated Strategy Step 3—Encounter node logic

`resource/roguelike/theme_name/encounter/default.json` describes the encounter event selection strategy in levelling mode

`resource/roguelike/theme_name/encounter/deposit.json` describes the encounter event selection strategy in ingot farming mode and starting mode

### MAA's current judgment method for encounters

OCR recognizes the encounter event name, but options are fixed position operations

If event name recognition fails, it will click the option type icon (if any), or click the bottom option

A common reason for getting stuck is when the option type icon exists but the option is not selectable (TODO)

When improving I.S. logic, usually only minor adjustments or no adjustments are needed (the experts have done a good job)

### Optimizing encounter option priority

Please check the effects of various event options on [prts.wiki](https://prts.wiki/w/%E9%9B%86%E6%88%90%E6%88%98%E7%95%A5), note that options may not be fixed

You can modify the encounter event options yourself to guide MAA towards certain special endings

```json5
{
    "theme": "Sami",                              //I.S. theme
    "stage": [                                    //Encounter events
        {
            "name": "低地市集",                    //Encounter event name
            "option_num": 3,                      //Total number of options (3 in this case)
            "choose": 3,                          //Preferred option number (prefers 3rd option here), if not selectable will choose escape option (usually the last one)
            "choices": [                          //Option selection requirements (currently doesn't affect program execution, just annotates applicable scenarios for modification convenience)
                {
                    "name": "选择碎草药",          //Option name
                    "ChaosLevel": {               //Anti-interference/Light level
                        "value": "3",             //Required number
                        "type": ">"               //Greater than or less than (this means select herbs when anti-interference/light is greater than 3)
                    }
                },
                {
                    "name": "选择好看的织物",
                    "ChaosLevel": {
                        "value": "3",
                        "type": ">"
                    }
                },
                ...
```

### Dynamically adjust certain option priorities based on team composition (TODO)

## Integrated Strategy Step 4—Store collectible priority

`resource/roguelike/theme_name/shopping.json` describes the strategy for purchasing collectibles in shops (and selecting collectibles after combat?)

```json5
{
    "theme": "Phantom",                                       //I.S. theme name (Phantom here)
    "priority": [                                             //Priority, ordered, sequence is priority, higher ones are purchased first
                                                              //But priority judgment is before chars, roles filtering, may result in high priority items being filtered out and nothing being bought
        {
            "name": "金酒之杯",                                //Collectible name (Gold Cup here)
            "no_longer_buy": true,                             //true means stop buying collectibles after getting this, false or omit means continue buying collectibles
            "ignore_no_longer_buy": true,                      //true means ignore "no_longer_buy" when shop has this item, meaning still buy it, false or omit means after getting items with "no_longer_buy" will not purchase this collectible
            "effect": "每有5源石锭，所有我方单位的攻击速度+7",    //Collectible effect (doesn't affect program execution, notes effect for sorting convenience)
            "no": 167                                          //Collectible number, can check wiki (doesn't affect program execution, notes for sorting convenience)
        },

        ...
        {
            "name": "扩散之手",
            "chars": [                                         //Buy this collectible when these operators are in team
                "异客"                                         //(This means buy Diffusion Hand when Mizuki is in team)
            ],
            "effect": "【扩散术师】、【链术师】和【轰击术师】每对一个单位造成伤害就回复2点技力值",
            "no": 136
        },
        ...

        {
            "name": "折戟-破釜沉舟",
            "roles": [                                         //Buy this collectible when these classes are in team
                "WARRIOR"                                      //(This means buy Broken Halberd-Desperate when Guard operators are in team)
            ],
            "effect": "所有【近卫】干员的防御力-40%，但攻击力+40%，攻击速度+30",
            "no": 16
        },
        ...

        {
            "name": "Miss.Christine摸摸券",
            "promotion": 2,                                   //Buy when 2 operators need promotion (right?)
            "effect": "立即进阶两个干员（不消耗希望）",
            "no": 15
        },
        ...

        {
            "name": "警戒篱木",
            "effect": "坍缩值-2，目标生命上限+2",
            "No": 198,
            "decrease_collapse": true                          // true means getting this collectible will decrease collapse value. When mode is 5 will not purchase
        },
        ...

    "others":                                                  // Collectibles MAA won't buy, like ending collectibles and crane
        {
            "name": "无人起重机"
        },
```

## Integrated Strategy Special Mechanisms

### Sami I.S.—Foldartal

`resource/roguelike/Sami/foldartal.json` describes the Sami Mysterious Foldartal strategy

```json5
{
    "theme": "Sami",                                         //I.S. theme name (Sami here)
    "groups": [                                              //Usage and application grouping
        {
            "usage": "SkipBattle",                           //Usage (Skip battle, for cash farming and heating water mode combat nodes, use boards to skip battles to save time)
            "doc": "跳过战斗,刷钱和烧开水模式",
            "pairs": [                                       //Board pairs (when corresponding node is encountered, will check if there's a matching board pair as set below, if so, use all available, if not enter directly)
                {                                            //(Scar can only link with Void)
                    "up": [                                  //Upper board
                        "伤痕"
                    ],
                    "down": [                                //Lower board
                        "空无"
                    ]
                },
                {                                            //(Will search in order: "Exile"+"Surprise", "Exile"+"Doubt", "Exile"+..., "Hunter"+"Surprise", "Hunter"+"Doubt", "Hunter"+..., ...)
                    "up": [
                        "黜人",
                        "猎手",
                        ...
                    ],
                    "down": [
                        "惊讶",
                        "疑惑",
                        ...
                    ]
                }
            ]
        },
        {
            "usage": "Boss",                                 //(This indicates board pairs to use at boss nodes)
            "doc": "有的用全用了",
            ...
        }
    ],
    "foldartal": [                                          //Foldartal effect notes (just for reference to check board effects, doesn't affect program execution)
        {
            "name": "布局",                                  //Foldartal type (upper or lower board)
            "foldartal": [
                {
                    "name": "黜人",                           //Foldartal name
                    "effect": "选择所有右侧邻近的战斗节点"     //Foldartal effect
                },

```

### Sami I.S.—Collapsal Paradigms

When `check_collapsal_paradigms` is `true`, MAA will check Collapsal Paradigms in two different ways:

- Click the top middle of the screen on the level selection screen to expand the collapse status bar, hereafter called Panel Check;
- Observe whether there is a Collapsal Paradigms notification on the right side of the screen, hereafter called Banner Check.

There are many ways to get collapse value, we've considered the following situations:

1. Collapse value increases after battle due to imperfect combat, performs Banner Check.
2. Collapse value changes after battle due to collectible acquisition, performs Banner Check.
3. Collapse value changes during encounter nodes due to option selection, performs Banner Check.
4. Collapse value changes at mysterious merchant nodes due to collectible purchase, performs Banner Check.
5. Collapse value decreases due to using Foldartal, performs Banner Check.
6. Collapse value increases due to entering a new floor, performs Panel Check.
7. If a Banner Check discovers Collapsal Paradigms fading, since we don't know if two levels can fade at once (even if possible, probability is extremely low), an additional Panel Check will be triggered the next time returning to level selection screen.
8. When `double_check_collapsal_paradigms` is `true`, every time returning to level selection screen will trigger an additional Panel Check to verify whether previous collapse paradigms were missed or over-recorded.

Below is an example task configuration for farming hidden Collapsal Paradigms:

```json5
{
    "theme": "Sami",
    "mode": 5,
    "investment_enabled": false,
    "squad": "远程战术分队",
    "roles": "稳扎稳打",
    "core_char": "维什戴尔",
    "expected_collapsal_paradigms": ["目空一些", "睁眼瞎", "图像损坏", "一抹黑"]
}
```

When `mode` is `5`:

- Preferentially uses combat strategy with `stage_name` as `level_name_collapse`, e.g. `resource/roguelike/Sami/autopilot/事不过四_collapse.json`;
- Uses encounter event selection strategy described in `resource/roguelike/Sami/encounter/collapse.json`,
- Will not purchase collectibles with `decrease_collapse` as `true`.

When `mode` is not `5` but `check_collapsal_paradigms` is `true`, will still detect Collapsal Paradigms and stop the task when encountering paradigms in the `expected_collapsal_paradigms` list, but will not restart when encountering other paradigms.

When farming hidden Collapsal Paradigms, N10 difficulty is recommended, with suggested teams:

- Wishslash + Spot + Steward;
- Lumen + Orchid + Popukar;
- Toddifons + Spot + Steward.

## Desired Logic Improvements (TODO)

### Automatic squad formation logic

1. Set different map squad completeness detection and skill priorities for different maps

2. Avoid high difficulty combat based on current lineup

### ~~Optimize pathfinding algorithm~~ (Initially implemented)

For instance, implement more combat in first three floors, fewer later, for better development

Like recognizing connections between nodes to determine optimal paths

### Skill holding

For operators deployed at certain locations, wait X seconds after skill is ready before using it, for better timing; can write Skill_hold under Deploy_plan, or write Skill_hold_plan

### Skill disabling

Useful for ammo-type operators

<!-- markdownlint-disable-file MD026 -->
