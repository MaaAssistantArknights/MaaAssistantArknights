---
order: 1
icon: bxs:book
---

# Integration

## API

### `AsstAppendTask`

#### Prototype

```cpp
TaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### Description

Appends a task.

#### Return Value

- `TaskId`  
   The task ID if the task is successfully appended, for the following configuration;
  0 if the task is not successfully appended.

#### Parameter Description

:::: field-group  
::: field handle  
@type AsstHandle
@required
Instance handle  
:::  
::: field type  
@type const char*
@required
Task type  
:::  
::: field params  
@type const char*
@required
Task parameters, json string  
:::  
::::

##### List of Task Types

- `StartUp`  
   Start-up

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field client_type  
@type string
@required
Client version.  
<br>
Options: `Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::: field start_game_enabled  
@type boolean
@default false
@optional
Whether to launch client automatically.  
:::  
::: field account_name  
@type string
@optional
Switch account, don't switch by default.  
<br>
Only supports switching to already logged-in accounts, using login name for identification, ensure the input content is unique among all logged-in accounts.  
<br>
Official server: `123****4567`, can input `123****4567`, `4567`, `123`, or `3****4567`  
<br>
Bilibili server: `Zhang San`, can input `Zhang San`, `Zhang`, or `San`  
<br>
Traditional Chinese server: Email-based, e.g. `ab****01@gmail.com`. Entering the plain-text portion without asterisks is recommended, e.g. `01@gmail`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "client_type": "Official",
   "start_game_enabled": true,
   "account_name": "123****4567"
}
```

</details>

- `CloseDown`  
   Close Game Client

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field client_type  
@type string
@required
Client version, no execution if left blank.  
<br>
Options: `Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "client_type": "Official"
}
```

</details>

- `Fight`  
   Operation

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field stage  
@type string
@optional
Stage name, by default empty, recognizes current/last stage. Editing in run-time is not supported.  
Currently supported stages for navigation include:

- All mainline stages. You can add `-NORMAL` or `-HARD` at the end of the stage to switch difficulty: Chapters 10-14 map to Standard/Adverse, while Chapters 15+ map to Normal/Raid.
- For LMD and Battle Record stages 5/6, must input `CE-6` / `LS-6`. MAA will automatically switch to stage 5 if stage 6 cannot be delegated.
- Skill Summary, Voucher, and Carbon stage 5, must input `CA-5` / `AP-5` / `SK-5`.
- All chip stages. Must input complete stage code, e.g. `PR-A-1`.
- Annihilation mode supports the following values, must use corresponding Value:
  - Current Annihilation: Annihilation
  - Chernobog: Chernobog@Annihilation
  - Lungmen Outskirts: LungmenOutskirts@Annihilation
  - Lungmen Downtown: LungmenDowntown@Annihilation
- OF-1 / OF-F3 in side stories.
- Last three stages of current SS events. Visit [API](https://api.maa.plus/MaaAssistantArknights/api/gui/StageActivityV2.json) for the list of supported stages. Requires additional loading of event stage navigation in [tasks.json](https://api.maa.plus/MaaAssistantArknights/api/resource/tasks.json) file.
- Rerun SS events. Input `SSReopen-<stage prefix>` to farm XX-1 ~ XX-9 stages at once, e.g. `SSReopen-IC`.
  :::  
  ::: field medicine  
  @type number
  @default 0
  @optional
  Maximum number of Sanity Potions used.  
  :::  
  ::: field medicine_expire_days  
  @type number
  @default 0
  @optional
  Use Sanity Potions that expire within the specified number of days. `0` means no expiring potions will be used.  
  :::  
  ::: field expiring_medicine  
  @type number
  @default 0
  @optional
  @deprecated
  Deprecated since v6.8.0, please use `medicine_expire_days` instead.  
  :::  
  ::: field stone  
  @type number
  @default 0
  @optional
  Maximum number of Originite Prime used.  
  :::  
  ::: field times  
  @type number
  @default 2147483647
  @optional
  Number of battles.  
  :::  
  ::: field series  
  @type number
  @optional
  Number of consecutive battles, -1~10.
  <br>
  `-1` to disable switching.
  <br>
  `0` to automatically switch to the current maximum available times, if current sanity is not enough for the maximum times, select the minimum available times.
  <br>
  `1~10` to specify number of consecutive battles.
  <br>
  ::: info Server Difference
  Input validation depends on whether the resource contains `FightSeries-OldMethodFlag`:
  <br>
  - New list (CN main resources after 2026/8/1, without this flag): accepts `-1~10`
  - Old list (overseas resources with this flag): accepts only `-1~6`; larger values are rejected
    <br>
    Overseas servers are expected to follow in about six months, after which the limit becomes 10 with the resource update. The Windows GUI series dropdown currently always offers up to 10; on overseas clients, manually selecting 7~10 will be rejected by Core when the task is submitted.
    :::  
    :::  
    ::: field drops  
    @type object
    @optional
    Specifying the number of drops, no specification by default. key is item_id, value is quantity. key can refer to `resource/item_index.json` file.  
    <br>
    Example: `{ "30011": 10, "30062": 5 }`  
    <br>
    All above are OR relations, i.e. task stops when any one is reached.  
    :::  
    ::: field report_to_penguin  
    @type boolean
    @default false
    @optional
    Whether to upload data to Penguin Statistics.  
    :::  
    ::: field penguin_id  
    @type string
    @optional
    Penguin Statistics report id, empty by default. Only effective when `report_to_penguin` is true.  
    :::  
    :::  
    ::: field report_to_yituliu  
    @type boolean
    @default false
    @optional
    Whether to report to YITULIU.  
    :::  
    ::: field yituliu_id  
    @type string
    @optional
    YITULIU report id, empty by default. Only effective when `report_to_yituliu` is true.  
    :::  
    ::: field server  
    @type string
    @default CN
    @optional
    Server, will affect drop recognition and upload.
    <br>
    Options: `CN` | `US` | `JP` | `KR`  
    :::  
    ::: field client_type  
    @type string
    @optional
    Client version, empty by default. Used to restart and reconnect after game crash, does not enable this feature if empty.
    <br>
    Options: `Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
    :::  
    ::: field DrGrandet  
    @type boolean
    @default false
    @optional
    Sanity-saving Originite usage mode, only effective when Originite usage may occur.
    <br>
    Wait at the Originite confirmation screen until the current 1 sanity point is restored, then immediately use Originite.  
    :::  
    ::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "stage": "1-7",
   "medicine": 1,
   "medicine_expire_days": 2,
   "stone": 0,
   "times": 10,
   "series": 0,
   "drops": {
      "30011": 10
   },
   "report_to_penguin": true,
   "penguin_id": "123456",
   "report_to_yituliu": true,
   "yituliu_id": "123456",
   "server": "CN",
   "client_type": "Official",
   "DrGrandet": false
}
```

</details>

- `Recruit`  
   Recruitment

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field refresh  
@type boolean
@default false
@optional
Whether to refresh 3★ tags.  
:::  
::: field select  
@type array<number>
@required
Tag ★ rarity to click.  
:::  
::: field confirm  
@type array<number>
@required
Tag ★ rarity for confirmation. Can be set to empty array for calculation only.  
:::  
::: field first_tags  
@type array<string>
@optional
Preferred Tags, valid only when selecting 3★ tags. Default is empty.
<br>
For 3★ recruits, MAA will try to include as many of the listed tags as possible. This is treated as a hard requirement and will override any "don't select 3★ Tags" settings.  
:::  
::: field extra_tags_mode  
@type number
@default 0
@optional
Select more tags.
<br>
`0` - default behavior
<br>
`1` - select 3 tags even if they may conflict
<br>
`2` - if possible, select more high star tag combinations even if they might conflict  
:::  
::: field times  
@type number
@default 0
@optional
Number of recruitments. Can be set to 0 for calculation only.  
:::  
::: field set_time  
@type boolean
@default true
@optional
Whether to set recruitment time limit. Only effective when `times` is 0.  
:::  
::: field expedite  
@type boolean
@default false
@optional
Whether to use Expedited Plans.  
:::  
::: field expedite_times  
@type number
@optional
Number of expedites, only effective when `expedite` is true. By default unlimited (until `times` limit is reached).  
:::  
::: field skip_robot  
@type boolean
@default true
@optional
Deprecated and kept only for backward compatibility.  
<br>
When `preserve_tags` is absent and this value is `true`, MAA skips on `支援机械` only; `元素` is no longer treated as the legacy 1★ tag.  
:::
::: field preserve_tags  
@type array<string>
@optional
List of tag names that should preserve the current recruitment slot and skip this recruitment. Default is empty.  
<br>
If any specified tag is recognized, MAA will keep that slot untouched and skip the current recruitment.  
:::  
::: field recruitment_time  
@type object
@optional
Tag ★ rarity (greater than or equal to 3) and corresponding desired recruitment time limit, in minutes, all default to 540 (i.e. 09:00:00).
<br>
Example: `{ "3": 540, "4": 540 }`  
:::  
::: field report_to_penguin  
@type boolean
@default false
@optional
Whether to report to Penguin Statistics.  
:::  
::: field penguin_id  
@type string
@optional
Penguin Statistics report id, empty by default. Only effective when `report_to_penguin` is true.  
:::  
::: field report_to_yituliu  
@type boolean
@default false
@optional
Whether to report to YITULIU data.  
:::  
::: field yituliu_id  
@type string
@optional
YITULIU report id, empty by default. Only effective when `report_to_yituliu` is true.  
:::  
::: field server  
@type string
@default CN
@optional
Server, will affect upload.
<br>
Options: `CN` | `US` | `JP` | `KR`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "refresh": true,
   "select": [5, 4],
   "confirm": [4, 3],
   "first_tags": ["高级资深干员"],
   "extra_tags_mode": 1,
   "times": 4,
   "set_time": true,
   "expedite": false,
   "expedite_times": 0,
   "preserve_tags": ["支援机械"],
   "recruitment_time": {
      "3": 540,
      "4": 540
   },
   "report_to_penguin": false,
   "penguin_id": "123456",
   "report_to_yituliu": false,
   "yituliu_id": "123456",
   "server": "CN"
}
```

</details>

- `Infrast`  
   Infrastructure shifting

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field mode  
@type number
@default 0
@optional
Shift mode. Editing in run-time is not supported.
<br>
`0` - `Default`: Default shift mode, single facility optimal solution.
<br>
`10000` - `Custom`: Custom shift mode, reads user configuration, see [Base Scheduling Schema](./base-scheduling-schema.md).
<br>
`20000` - `Rotation`: One-key rotation mode, skips control center, power station, dormitory and office, other facilities do not change shifts but retain basic operations (such as using drones, reception room logic).  
:::  
::: field facility  
@type array<string>
@required
Facilities for shifting (ordered). Editing in run-time is not supported.
<br>
Facility name: `Mfg` | `Trade` | `Power` | `Control` | `Reception` | `Office` | `Dorm` | `Processing` | `Training`  
:::  
::: field drones  
@type string
@default \_NotUse
@optional
Usage of drones. This field is ignored when `mode = 10000`.
<br>
Options: `_NotUse` | `Money` | `SyntheticJade` | `CombatRecord` | `PureGold` | `OriginStone` | `Chip`  
:::  
::: field threshold  
@type number
@default 0.3
@optional
Morale threshold, range [0, 1.0].
<br>
When `mode = 10000`, this field is only effective for "autofill".
<br>
This field is ignored when `mode = 20000`.  
:::  
::: field replenish  
@type boolean
@default false
@optional
Whether to replenish Originium Shard in trading post.  
:::  
::: field dorm_notstationed_enabled  
@type boolean
@default false
@optional
Whether to enable "Not Stationed in Dorm" option.  
:::  
::: field dorm_trust_enabled  
@type boolean
@default false
@optional
Whether to fill dormitory with operators not at max trust.  
:::  
::: field reception_message_board  
@type boolean
@default true
@optional
Whether to collect credits from reception room message board.  
:::  
::: field reception_clue_exchange  
@type boolean
@default true
@optional
Whether to perform clue exchange.  
:::  
::: field reception_send_clue  
@type boolean
@default true
@optional
Whether to send clues.  
:::  
::: field filename  
@type string
@required
Custom config path. Editing in run-time is not supported.
<br>
<Badge type="warning" text="Only effective when mode = 10000" />  
:::  
::: field plan_index  
@type number
@required
Plan index number in the configuration. Editing in run-time is not supported.
<br>
<Badge type="warning" text="Only effective when mode = 10000" />  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "mode": 0,
   "facility": ["Mfg", "Trade", "Reception"],
   "drones": "PureGold",
   "threshold": 0.3,
   "replenish": true,
   "dorm_notstationed_enabled": false,
   "dorm_trust_enabled": true,
   "reception_message_board": true,
   "reception_clue_exchange": true,
   "reception_send_clue": true,
   "filename": "schedules/base.json",
   "plan_index": 1
}
```

</details>

- `Mall`  
   Collecting Credits and auto-purchasing.  
   Will buy items in order following `buy_first` list, buy other items from left to right ignoring items in `blacklist`, and buy other items from left to right ignoring the `blacklist` while credit overflows.

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field visit_friends  
@type boolean
@default true
@optional
Whether to visit friends' base to obtain Credits.  
:::  
::: field shopping  
@type boolean
@default true
@optional
Whether to buy items from the store.  
:::  
::: field buy_first  
@type array<string>
@default []
@optional
Items to be purchased with priority. Item name, e.g. `"招聘许可"` (Recruitment Permit), `"龙门币"` (LMD), etc.  
:::  
::: field blacklist  
@type array<string>
@default []
@optional
Blacklist. Item name, e.g. `"加急许可"` (Expedited Plan), `"家具零件"` (Furniture Part), etc.  
:::  
::: field force_shopping_if_credit_full  
@type boolean
@default false
@optional
Whether to ignore the Blacklist if credit overflows.  
:::  
::: field only_buy_discount  
@type boolean
@default false
@optional
Whether to purchase only discounted items, applicable only on the second round of purchases.  
:::  
::: field reserve_max_credit  
@type boolean
@default false
@optional
Whether to stop purchasing when credit points fall below 300, applicable only on the second round of purchases.  
:::  
::: field credit_fight  
@type boolean
@default false
@optional
Whether to run one battle of OF-1 to gain more Credits the next day.  
:::  
::: field formation_index  
@type number
@default 0
@optional
Formation slot index used for the OF-1 battle.
<br>
Integer between 0–4, where 0 = current squad, 1–4 = first, second, third, fourth squad.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "visit_friends": true,
   "shopping": true,
   "buy_first": ["招聘许可", "龙门币"],
   "blacklist": ["家具零件"],
   "force_shopping_if_credit_full": false,
   "only_buy_discount": true,
   "reserve_max_credit": false,
   "credit_fight": false,
   "formation_index": 0
}
```

</details>

- `Award`  
   Collecting various rewards

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field award  
@type boolean
@default true
@optional
Collect daily/weekly task rewards.  
:::  
::: field mail  
@type boolean
@default false
@optional
Collect all mail rewards.  
:::  
::: field recruit  
@type boolean
@default false
@optional
Collect daily free pulls from limited banners.  
:::  
::: field orundum  
@type boolean
@default false
@optional
Collect Orundum from lucky drop wall.  
:::  
::: field mining  
@type boolean
@default false
@optional
Collect Orundum from limited mining licenses.  
:::  
::: field specialaccess  
@type boolean
@default false
@optional
Collect monthly card rewards from 5th anniversary.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "award": true,
   "mail": true,
   "recruit": true,
   "orundum": false,
   "mining": true,
   "specialaccess": false
}
```

</details>

- `Roguelike`  
   Integrated Strategies

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field theme  
@type string
@default Phantom
@optional
Theme.
<br>
`Phantom` - Phantom & Crimson Solitaire
<br>
`Mizuki` - Mizuki & Caerula Arbor
<br>
`Sami` - Expeditioner's Jǫklumarkar
<br>
`Sarkaz` - Tales of the Unfathomable
<br>
`JieGarden` - Sui's Garden of Grotesqueries  
:::  
::: field mode  
@type number
@default 0
@optional
Mode.
<br>
`0` - Score farming/reward points, aiming to consistently reach higher levels.
<br>
`1` - Originium Ingot farming, exit after investing in the first layer.
<br>
`2` - <Badge type="danger" text="Deprecated" /> Balances modes 0 and 1; continues until the next investment, exits otherwise.
<br>
`3` - Under development...
<br>
`4` - Opening reset; first reaches the third layer at difficulty 0, then restarts and switches to the specified difficulty to reset the opening reward. If not the desired item, restart at difficulty 0; in the Phantom theme, retry only in the current difficulty.
<br>
`5` - Collapsal Paradigm farming; only for the Sami theme; accelerates collapsal buildup via missed enemies; if the first collapsal paradigm encountered is in the `expected_collapsal_paradigms` list, stops the task; otherwise, restarts.
<br>
`6` - Monthly squad rewards farming, same as mode 0 except for specific mode adaptations.
<br>
`7` - Deep Dive rewards farming, same as mode 0 except for specific mode adaptations.
:::  
::: field squad  
@type string
@default 指挥分队
@optional
Starting squad name.  
:::  
::: field roles  
@type string
@default 取长补短
@optional
Starting role group.  
:::  
::: field core_char  
@type string
@optional
Starting operator name. Supports only single operator **Chinese name**, regardless of server; leave empty or set to `""` to auto-select based on level.  
:::  
::: field use_support  
@type boolean
@default false
@optional
Whether the starting operator is a support operator.  
:::  
::: field use_nonfriend_support  
@type boolean
@default false
@optional
Whether non-friend support operators are allowed. Only effective when `use_support` is true.  
:::  
::: field starts_count  
@type number
@default 2147483647
@optional
Number of times to start exploration. Stops automatically upon reaching limit.  
:::  
::: field difficulty  
@type number
@default 0
@optional
Specified difficulty level. Selects the highest unlocked difficulty if the desired one is not unlocked.  
:::  
::: field stop_at_final_boss  
@type boolean
@default false
@optional
Whether to stop before the level 5 final boss node. Only applicable to themes **excluding Phantom**.  
:::  
::: field stop_at_max_level  
@type boolean
@default false
@optional
Whether to stop if max level for roguelike has been achieved.  
:::  
::: field investment_enabled  
@type boolean
@default true
@optional
Whether to invest Originium Ingots.  
:::  
::: field investments_count  
@type number
@default 2147483647
@optional
Number of Originium Ingot investments. Stops automatically upon reaching limit.  
:::  
::: field stop_when_investment_full  
@type boolean
@default false
@optional
Whether to stop automatically when investment limit is reached.  
:::  
::: field investment_with_more_score  
@type boolean
@default false
@optional
Whether to try shopping after investment. Only applicable to mode 1.  
:::  
::: field start_with_elite_two  
@type boolean
@default false
@optional
Whether to reset for an Elite 2 operator at start. Only applicable to mode 4.  
:::  
::: field only_start_with_elite_two  
@type boolean
@default false
@optional
Whether to reset only for Elite 2 operator while ignoring other starting conditions. Only effective when mode is 4 and `start_with_elite_two` is true.  
:::  
::: field refresh_trader_with_dice  
@type boolean
@default false
@optional
Whether to refresh the shop with dice for special items. Only applicable to the Mizuki theme, used to refresh Scale of Past.  
:::  
::: field first_floor_foldartal  
@type string
@optional
Desired Foldartal to acquire in the first floor foresight phase. Only applicable to the Sami theme, any mode; task stops once obtained successfully.  
:::  
::: field start_foldartal_list  
@type array<string>
@default []
@optional
Desired Foldartals for the starting reward phase during opening reset. Effective only for Sami theme and mode 4.
<br>
Reset is successful only when all Foldartals in the list are present in the opening rewards.
<br>
Note: This parameter must be used with the "生活至上分队" (Life-Sustaining Squad) as other squads do not obtain Foldartals in the opening reward phase.  
:::  
::: field collectible_mode_start_list  
@type object
@optional
Desired starting rewards, all false by default. Only valid in mode 4.
<br>
`hot_water`: Hot Water reward, typically used to trigger boiling mechanism (universal).
<br>
`shield`: Shield reward, equivalent to extra HP (universal).
<br>
`ingot`: Originium Ingot reward (universal).
<br>
`hope`: Hope reward (universal, note: not available in JieGarden theme).
<br>
`random`: Random reward option: consumes all Ingots for a random collectible (universal).
<br>
`key`: Key reward, only available in Mizuki theme.
<br>
`dice`: Dice reward, only available in Mizuki theme.
<br>
`ideas`: 2 Ideas reward, only available in Sarkaz theme.
<br>
`ticket`: Coupon reward, only available in JieGarden theme.
:::  
::: field use_foldartal  
@type boolean
@optional
Whether to use Foldartals. Default is `false` in mode 5 and `true` in other modes. Only applicable to the Sami theme.  
:::  
::: field check_collapsal_paradigms  
@type boolean
@optional
Whether to check obtained Collapsal Paradigms. Default is `true` in mode 5 and `false` in other modes.  
:::  
::: field double_check_collapsal_paradigms  
@type boolean
@default true
@optional
Whether to perform additional checks to prevent missed Collapsal Paradigms. Only effective when theme is Sami and `check_collapsal_paradigms` is true. Default is `true` in mode 5 and `false` in other modes.  
:::  
::: field expected_collapsal_paradigms  
@type array<string>
@default ['目空一些', '睁眼瞎', '图像损坏', '一抹黑']
@optional
Desired Collapsal Paradigms to trigger. Only effective when theme is Sami and mode is 5.  
:::  
::: field monthly_squad_auto_iterate  
@type boolean
@optional
Whether to enable automatic monthly squad rotation.  
:::  
::: field monthly_squad_check_comms  
@type boolean
@optional
Whether to also check monthly squad communications as rotation criteria.  
:::  
::: field deep_exploration_auto_iterate  
@type boolean
@optional
Whether to enable automatic deep exploration rotation.  
:::  
::: field collectible_mode_shopping  
@type boolean
@default false
@optional
Whether to enable shopping in hot water mode.  
:::  
::: field collectible_mode_squad  
@type string
@optional
Squad to use in hot water mode, default synced with squad, when squad is empty and collectible_mode_squad not specified, uses 指挥分队.  
:::  
::: field start_with_seed  
@type boolean
@default false
@optional
Use seed for money farming.
<br>
Only possible to be true in Sarkaz theme, Investment mode, with "点刺成锭分队" (Point-Stab Ingot Squad) or "后勤分队" (Logistics Squad).
<br>
Uses fixed seed.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "theme": "Sami",
   "mode": 5,
   "squad": "指挥分队",
   "roles": "取长补短",
   "core_char": "塑心",
   "use_support": false,
   "use_nonfriend_support": false,
   "starts_count": 3,
   "difficulty": 8,
   "stop_at_final_boss": false,
   "stop_at_max_level": false,
   "investment_enabled": true,
   "investments_count": 2,
   "stop_when_investment_full": false,
   "investment_with_more_score": false,
   "start_with_elite_two": false,
   "only_start_with_elite_two": false,
   "refresh_trader_with_dice": false,
   "first_floor_foldartal": "",
   "start_foldartal_list": [],
   "collectible_mode_start_list": {
      "hot_water": true,
      "shield": false,
      "ingot": false,
      "hope": true,
      "random": false,
      "key": false,
      "dice": false,
      "ideas": false
   },
   "use_foldartal": true,
   "check_collapsal_paradigms": true,
   "double_check_collapsal_paradigms": true,
   "expected_collapsal_paradigms": ["目空一些", "睁眼瞎"],
   "monthly_squad_auto_iterate": false,
   "monthly_squad_check_comms": false,
   "deep_exploration_auto_iterate": false,
   "collectible_mode_shopping": false,
   "collectible_mode_squad": "指挥分队",
   "start_with_seed": false
}
```

</details>

For specific information about the Collapsal Paradigm farming feature, please refer to [Integrated Strategy Schema](./integrated-strategy-schema.md#sami-integrated-strategy-collapsal-paradigms)

- `Copilot`  
   Auto combat feature

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field filename  
@type string
Path to a single job JSON file, mutually exclusive with copilot_list (required, choose one); both relative and absolute paths are supported.  
:::  
::: field copilot_list  
@type array`<object>`
List of jobs, mutually exclusive with filename (required, choose one); when both filename and copilot_list are present, copilot_list will be ignored; set_params can only be executed once when this parameter is in effect.
<br>
Each object contains:
<br>

- `filename`: Path to the job JSON file; both relative and absolute paths are supported
  <br>
- `nav_name_override`: Navigation stage name, optional; if omitted or `null`, automatically inferred from the task file
  <br>
- `is_raid`: Whether to switch to Challenge Mode (Raid), optional, default false
  :::  
  ::: field loop_times  
  @type number
  @default 1
  @optional
  Number of loops. Effective only in single job mode (i.e., when filename is specified); set_params can only be executed once when this parameter is in effect.  
  :::  
  ::: field use_sanity_potion  
  @type boolean
  @default false
  @optional
  Whether to use sanity potions when sanity is insufficient.  
  :::  
  ::: field formation  
  @type boolean
  @default false
  @optional
  Whether to enable auto formation.  
  :::  
  ::: field formation_index  
  @type number
  @default 0
  @optional
  The index of the formation slot to use in auto formation. Only effective when formation is true.
  <br>
  An integer between 0–4: 0 means the current formation, 1–4 refer to the 1st–4th formations.  
  :::  
  ::: field user_additional  
  @type array`<object>`
  @default []
  @optional
  Custom additional operators list. Only effective when formation is true.
  <br>
  Each object contains:
  <br>
- `name`: Operator name, optional, default "", if left empty this operator will be ignored
  <br>
- `skill`: Skill to bring, optional, default 1; must be an integer between 1–3; otherwise, follows the in-game default
  :::  
  ::: field add_trust  
  @type boolean
  @default false
  @optional
  Whether to auto-fill empty slots by ascending trust value during auto formation. Only effective when formation is true.  
  :::  
  ::: field ignore_requirements  
  @type boolean
  @default false
  @optional
  Whether to ignore operator attribute requirements during auto formation. Only effective when formation is true.  
  :::  
  ::: field support_unit_usage  
  @type number
  @default 0
  @optional
  Support operator usage mode. Integer between 0–3. Only effective when formation is true.
  <br>
  `0` - Do not use support operators
  <br>
  `1` - Use support operator only if exactly one operator is missing; otherwise, do not use support
  <br>
  `2` - Use support operator if one is missing; otherwise, use the specified support operator
  <br>
  `3` - Use support operator if one is missing; otherwise, use a random support operator
  :::  
  ::: field support_unit_name  
  @type string
  @optional
  Specified support operator name. Only effective when support_unit_usage is 2.  
  :::  
  ::::

For more details about auto-combat JSON, please refer to [Combat Operation Protocol](./copilot-schema.md)

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "copilot/1-7.json",
   "loop_times": 2,
   "use_sanity_potion": false,
   "formation": true,
   "formation_index": 1,
   "user_additional": [
      {
         "name": "能天使",
         "skill": 3
      }
   ],
   "add_trust": true,
   "ignore_requirements": false,
   "support_unit_usage": 2,
   "support_unit_name": "艾雅法拉"
}
```

</details>

- `SSSCopilot`  
   Auto combat feature for Stationary Security Service

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field filename  
@type string
@required
Filename and path of the task JSON, supporting absolute/relative paths. Editing in run-time is not supported.  
:::  
::: field loop_times  
@type number
@optional
Number of times to loop execution.  
:::  
::::

For more details about Stationary Security Service JSON, please refer to [SSS Schema](./sss-schema.md)

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "sss/plan.json",
   "loop_times": 1
}
```

</details>

- `ParadoxCopilot`
  Automatically run Paradox Simulation operations

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field filename  
@type string
@required
File path of a single operation JSON, supports absolute/relative paths. Runtime editing not supported. Mutually exclusive with list (required, choose one).  
:::  
::: field list  
@type array<string>
@required
List of operation JSON files, supports absolute/relative paths. Runtime editing not supported. Mutually exclusive with filename (required, choose one).  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "paradox/exusiai.json",
   "list": []
}
```

</details>

- `Depot`
  Depot recognition

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true
}
```

</details>

- `OperBox`  
   Operator box recognition

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true
}
```

</details>

- `Reclamation`  
   Reclamation Algorithm

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field theme  
@type string
@default Tales
@optional
Theme.
<br>
`Fire` - _Fire Within the Sand_ (Closed)
<br>
`Tales` - _Tales Within the Sand_
<br>
`RelaunchAnchor` - _Relaunch Anchor_
:::  
::: field mode  
@type number
@default 0
@optional
Mode. Supported modes vary by theme:
<br>
**Tales:**
<br>
`0` - No save, farm prosperity points by entering and exiting stages.
<br>
`1` - With save, farm currency by crafting support items.
<br>
**RelaunchAnchor:**
<br>
`16` (`RA1`) - RA-1, automatically execute intensive farming, construction, resource delivery, and settlement loop.
<br>
`32` (`RA15`) - RA-15, complete the 60-kill mission with Civilight Eterna.
<br>
`48` (`RA4`) - RA-4, Use the Gold from Strategy Planning Management to unlock areas, and use Wis'adel to complete the boss elimination mission.
:::  
::: field tools_to_craft  
@type array<string>
@default [&quot;荧光棒&quot;]
@optional
Automatically crafted items. Suggested to fill in the substring. Only effective for Tales theme.  
:::  
::: field increment_mode  
@type number
@default 0
@optional
Click type. Only effective for Tales theme.
<br>
`0` - Rapid Click
<br>
`1` - Long Press
:::  
::: field num_craft_batches  
@type number
@default 16
@optional
Maximum number of craft batches per session. Only effective for Tales theme.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "theme": "Fire",
   "mode": 1,
   "tools_to_craft": ["荧光棒", "发电机"],
   "increment_mode": 0,
   "num_craft_batches": 12
}
```

</details>

- `Custom`  
   Custom Task

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field task_names  
@type array<string>
@required
Execute the task on the first match in the array (and subsequent next, etc.). If you want to perform multiple tasks, you can append Custom task multiple times.  
Supports the Secret Front (`MiniGame@SecretFront`) concatenated form: `MiniGame@SecretFront@Begin@Ending[A-E](@event)?`, where event is optional (支援作战平台 / 游侠 / 诡影迷踪), e.g. `MiniGame@SecretFront@Begin@EndingA@支援作战平台`.  
:::  
::: field params  
@type object
@optional
Additional task parameters. Currently only used by the pixel paint task (`MiniGame@PixelPaint@Begin`):

- `params.pixel_paint.groups`: color-grouped cell list. `color` is the palette slot index (0~39, same order as the in-game right-side palette), `points` is an array of `[x, y]` grid coordinates (0~23, origin at top-left).
- `params.pixel_paint.swipe` (bool, optional, default true): consecutive cells of the same color are drawn in a single drag for speed; some touch modes may behave abnormally.
- `params.pixel_paint.grid_delay` (int, optional, default 0): extra per-cell wait (ms). Applied after clicks and added to drag duration. Each touch mode already has its own base interval; usually no need to adjust. Legacy key `grid_click_delay` is still accepted.

:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "task_names": ["StartUp", "Infrast", "Fight"]
}
```

```json
{
   "enable": true,
   "task_names": ["MiniGame@PixelPaint@Begin"],
   "params": {
      "pixel_paint": {
         "groups": [
            { "color": 7, "points": [[0, 1], [3, 4]] }
         ]
      }
   }
}
```

</details>

- `SingleStep`  
   Single-step task (currently only supports copilot)

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field type  
@type string
@default copilot
@required
Currently only supports `"copilot"`.  
:::  
::: field subtask  
@type string
@required
Subtask type.
<br>
`stage` - Set stage name, requires `"details": { "stage": "xxxx" }`.
<br>
`start` - Start mission, without details.
<br>
`action` - Single battle action, details is single action in Copilot, e.g.: `"details": { "name": "史尔特尔", "location": [ 4, 5 ], "direction": "左" }`, see [Combat Operation Protocol](./copilot-schema.md) for details.
:::  
::: field details  
@type object
@optional
Detailed parameters for the subtask.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "type": "copilot",
   "subtask": "stage",
   "details": {
      "stage": "1-7"
   }
}
```

</details>

- `VideoRecognition`  
   Video recognition, currently only supports operation (combat) video

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
Whether to enable this task.  
:::  
::: field filename  
@type string
@required
Video file path, supporting absolute/relative paths. Editing in run-time is not supported.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "videos/copilot.mp4"
}
```

</details>

### `AsstSetTaskParams`

#### Prototype

```cpp
bool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);
```

#### Description

Set task parameters

#### Return Value

- `bool`  
   Whether the parameters are successfully set.

#### Parameter Description

:::: field-group  
::: field handle  
@type AsstHandle
@required
Instance handle  
:::  
::: field task  
@type AsstTaskId
@required
Task ID, the return value of `AsstAppendTask`  
:::  
::: field params  
@type const char\*
@required
Task parameter in JSON, same as `AsstAppendTask`.  
For those fields that do not mention "Editing in run-time is not supported" can be changed during run-time. Otherwise these changes will be ignored when the task is running.  
:::  
::::

### `AsstSetStaticOption`

#### Prototype

```cpp
bool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### Description

Set process-level parameters

#### Return Value

- `bool`  
   Is the setup successful

#### Parameter Description

:::: field-group  
::: field key  
@type AsstStaticOptionKey
@required
key  
:::  
::: field value  
@type const char\*
@required
value  
:::  
::::

##### List of Key and value

None currently

### `AsstSetInstanceOption`

#### Prototype

```cpp
bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### Description

Set instance-level parameters

#### Return Value

- `bool`  
   Is the setup successful

#### Parameter Description

:::: field-group  
::: field handle  
@type AsstHandle
@required
handle  
:::  
::: field key  
@type AsstInstanceOptionKey
@required
key  
:::  
::: field value  
@type const char\*
@required
value  
:::  
::::

##### List of Key and value

:::: field-group  
::: field Invalid  
@type number
@default 0
@optional
Invalid placeholder. Enum value: 0.  
:::  
::: field MinitouchEnabled  
@type boolean
@optional
Deprecated. Originally for enabling Minitouch; "1" - on, "0" - off. Note that the device may not support it. Enum value: 1 (deprecated).  
:::  
::: field TouchMode  
@type string
@default minitouch
@optional
Touch mode setting. Options: minitouch | maatouch | adb | MaaFwAdb. Default minitouch. Enum value: 2.  
:::  
::: field DeploymentWithPause  
@type boolean
@optional
Whether to pause when deploying operators (affects IS, Copilot and Stationary Security Service). Options: "1" | "0". Enum value: 3.  
:::  
::: field AdbLiteEnabled  
@type boolean
@optional
Whether to enable AdbLite or not. Options: "0" | "1". Enum value: 4.  
:::  
::: field KillAdbOnExit  
@type boolean
@optional
Release Adb on exit. Options: "0" | "1". Enum value: 5.  
:::  
::: field ClientType  
@type string
@optional
Client channel. Most connection configs do not need this option. Set it before `AsstConnect` / `AsstAsyncConnect` only when the selected `config` uses `[PackageName]` in commands executed during connect. In the built-in configs, only `Androws` and `WSA` currently require it for `displayId` lookup. This does not replace the `client_type` task parameter used by StartUp / CloseDown tasks. Enum value: 6.  
:::  
::::
