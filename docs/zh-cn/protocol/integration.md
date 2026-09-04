---
order: 1
icon: bxs:book
---

# 集成文档

## 接口介绍

### `AsstAppendTask`

#### 接口原型

```cpp
AsstTaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### 接口说明

添加任务

#### 返回值

- `AsstTaskId`  
   若添加成功，返回该任务 ID, 可用于后续设置任务参数；  
   若添加失败，返回 0

#### 参数说明

:::: field-group  
::: field handle  
@type AsstHandle
@required
实例句柄  
:::  
::: field type  
@type const char*
@required
任务类型  
:::  
::: field params  
@type const char*
@required
任务参数，json string  
:::  
::::

##### 任务类型一览

- `StartUp`  
  开始唤醒

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field client_type  
@type string
@required
客户端版本。  
<br>
选项：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::: field start_game_enabled  
@type boolean
@default false
@optional
是否自动启动客户端。  
:::  
::: field account_name  
@type string
@optional
切换账号，默认不切换。  
<br>
仅支持切换至已登录的账号，使用登录名进行查找，保证输入内容在所有已登录账号唯一即可。  
<br>
官服：`123****4567`，可输入 `123****4567`、`4567`、`123`、`3****4567`  
<br>
B服：`张三`，可输入 `张三`、`张`、`三`  
<br>
繁中服：账号为 Email，如 `ab****01@gmail.com`，建议填不含星号的明文片段，如 `01@gmail`  
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
   关闭游戏

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field client_type  
@type string
@required
客户端版本，填空则不执行。  
<br>
选项：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
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
   理智作战

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field stage  
@type string
@optional
关卡名，默认为空，识别当前/上次的关卡。不支持运行中设置。  
目前支持导航的关卡有：

- 全部主线关卡。可在关卡末尾添加 `-NORMAL` 或 `-HARD` 切换难度：10-14 章对应标准/磨难，15 章及以后对应常规/险地。
- 龙门币、作战记录的 5 / 6 关，但必须输入 `CE-6` / `LS-6`。MAA 会在第六关无法代理的情况下自动切换至第五关。
- 技能书、采购凭证、碳本第 5 关，必须输入 `CA-5` / `AP-5` / `SK-5`。
- 所有芯片本。必须输入完整关卡编号，如 `PR-A-1`。
- 剿灭模式支持以下传入值，必须使用对应的 Value：
  - 当期剿灭：Annihilation
  - 切尔诺伯格：Chernobog@Annihilation
  - 龙门外环：LungmenOutskirts@Annihilation
  - 龙门市区：LungmenDowntown@Annihilation
- 别传中的 OF-1 / OF-F3。
- 当期 SS 活动 后三关。可访问 [API](https://api.maa.plus/MaaAssistantArknights/api/gui/StageActivityV2.json) 获取支持的关卡列表。需额外加载 [tasks.json](https://api.maa.plus/MaaAssistantArknights/api/resource/tasks.json) 文件中的活动关卡导航。
- 复刻的 SS 活动。输入 `SSReopen-<关卡前缀>` ，可一次性刷完 XX-1 ~ XX-9 关，如 `SSReopen-IC`。
  :::  
  ::: field medicine  
  @type number
  @default 0
  @optional
  最大使用理智药数量。  
  :::  
  ::: field medicine_expire_days  
  @type number
  @default 0
  @optional
  使用过期时间在指定天数内的理智药，0 表示不使用过期理智药。  
  :::  
  ::: field expiring_medicine  
  @type number
  @default 0
  @optional
  @deprecated
  已弃用，自 v6.8.0 起请使用 `medicine_expire_days` 代替。  
  :::  
  ::: field stone  
  @type number
  @default 0
  @optional
  最大吃石头数量。  
  :::  
  ::: field times  
  @type number
  @default 2147483647
  @optional
  战斗次数。  
  :::  
  ::: field series  
  @type number
  @default 1
  @optional
  代理倍率, -1~10。
  <br>
  `-1` 为禁用切换。
  <br>
  `0` 为自动切换为当前可用的最大倍率, 如当前理智不够最大倍率, 则选择最低可用倍率。
  <br>
  `1~10` 为指定代理倍率。
  <br>
  ::: info 服务器差异
  输入校验取决于资源是否存在 `FightSeries-OldMethodFlag`：
  <br>
  - 新列表（国服 2026/8/1 后主资源，无该 flag）：接受 `-1~10`
  - 旧列表（外服资源带该 flag）：仅接受 `-1~6`，更大值会被拒绝

  外服预计约半年后跟进，届时上限随资源变为 10。Windows GUI 的代理倍率下拉目前固定提供到 10；外服若手动选择 7~10，任务下发时会被 Core 拒绝。
  :::  
  ::: field drops  
  @type object
  @optional
  指定掉落数量，默认不指定。key 为 item_id, value 为数量。key 可参考 `resource/item_index.json` 文件。  
  <br>
  例如: `{ "30011": 10, "30062": 5 }`  
  <br>
  以上全部是或的关系，即任一达到即停止任务。  
  :::  
  ::: field report_to_penguin  
  @type boolean
  @default false
  @optional
  是否汇报企鹅数据。  
  :::  
  ::: field penguin_id  
  @type string
  @optional
  企鹅数据汇报 id, 默认为空。仅在 `report_to_penguin` 为 true 时有效。  
  :::  
  ::: field report_to_yituliu  
  @type boolean
  @default false
  @optional
  是否汇报一图流。  
  :::  
  ::: field yituliu_id  
  @type string
  @optional
  一图流汇报 id, 默认为空。仅在 `report_to_yituliu` 为 true 时有效。  
  :::  
  ::: field server  
  @type string
  @default CN
  @optional
  服务器，会影响掉落识别及上传。
  <br>
  选项：`CN` | `US` | `JP` | `KR`  
  :::  
  ::: field client_type  
  @type string
  @optional
  客户端版本，默认为空。用于游戏崩溃时重启并连回去继续刷，若为空则不启用该功能。
  <br>
  选项：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
  :::  
  ::: field DrGrandet  
  @type boolean
  @default false
  @optional
  节省理智碎石模式，仅在可能产生碎石效果时生效。
  <br>
  在碎石确认界面等待，直到当前的 1 点理智恢复完成后再立刻碎石。  
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
  公开招募

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field refresh  
@type boolean
@default false
@optional
是否刷新三星 Tags。  
:::  
::: field select  
@type array<number>
@required
会去点击标签的 Tag 等级。  
:::  
::: field confirm  
@type array<number>
@required
会去点击确认的 Tag 等级。若仅公招计算，可设置为空数组。  
:::  
::: field first_tags  
@type array<string>
@optional
首选 Tags，仅在 Tag 等级为 3 时有效。默认为空。
<br>
当 Tag 等级为 3 时，会尽可能多地选择这里的 Tags（如果有），而且是强制选择，也就是会忽略所有“让 3 星 Tag 不被选择”的设置。  
:::  
::: field extra_tags_mode  
@type number
@default 0
@optional
选择更多的 Tags。
<br>
`0` - 默认行为
<br>
`1` - 选 3 个 Tags, 即使可能冲突
<br>
`2` - 如果可能, 同时选择更多的高星 Tag 组合, 即使可能冲突  
:::  
::: field times  
@type number
@default 0
@optional
招募多少次。若仅公招计算，可设置为 0。  
:::  
::: field set_time  
@type boolean
@default true
@optional
是否设置招募时限。仅在 `times` 为 0 时生效。  
:::  
::: field expedite  
@type boolean
@default false
@optional
是否使用加急许可。  
:::  
::: field expedite_times  
@type number
@optional
加急次数，仅在 `expedite` 为 true 时有效。当前版本已不生效，加急不受次数限制，直至 `times` 达到上限。  
:::  
::: field skip_robot  
@type boolean
@default true
@optional
已废弃，仅用于兼容旧参数。  
<br>
当未提供 `preserve_tags` 且该值为 `true` 时，会在识别到 `支援机械` 时跳过；`元素` 不再视为旧版 1 星词条。  
:::
::: field preserve_tags  
@type array<string>
@optional
需要保留并跳过当前公招槽位的 Tag 名称列表。默认为空。  
<br>
当识别到任一指定 Tag 时，MAA 会保留该槽位并跳过本次招募。  
:::  
::: field recruitment_time  
@type object
@optional
Tag 等级（大于等于 3）和对应的希望招募时限，单位为分钟，默认值都为 540（即 09:00:00）。
<br>
例如: `{ "3": 540, "4": 540 }`  
:::  
::: field report_to_penguin  
@type boolean
@default false
@optional
是否汇报企鹅数据。  
:::  
::: field penguin_id  
@type string
@optional
企鹅数据汇报 id, 默认为空。仅在 `report_to_penguin` 为 true 时有效。  
:::  
::: field report_to_yituliu  
@type boolean
@default false
@optional
是否汇报一图流数据。  
:::  
::: field yituliu_id  
@type string
@optional
一图流汇报 id, 默认为空。仅在 `report_to_yituliu` 为 true 时有效。  
:::  
::: field server  
@type string
@default CN
@optional
服务器，会影响上传。
<br>
选项：`CN` | `US` | `JP` | `KR`  
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
   基建换班

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field mode  
@type number
@default 0
@optional
换班工作模式。
<br>
`0` - `Default`: 默认换班模式，自动计算效率较高的设施内及跨设施干员组合。
<br>
`10000` - `Custom`: 自定义换班模式，读取用户配置，可参考 [基建排班协议](./base-scheduling-schema.md)。
<br>
`20000` - `Rotation`: 一键轮换模式，会跳过控制中枢、发电站、宿舍以及办公室，其余设施不进行换班但保留基本操作（如使用无人机、会客室逻辑）。  
:::  
::: field facility  
@type array<string>
@required
要换班的设施。不支持运行中设置。
<br>
`mode = 0` 时该数组为启用集合，顺序与重复项不参与调度（换班顺序由算法统一安排）；`mode = 10000` / `20000` 时按数组顺序执行。
<br>
设施名：`Mfg` | `Trade` | `Power` | `Control` | `Reception` | `Office` | `Dorm` | `Processing` | `Training`  
:::  
::: field drones  
@type string
@default \_NotUse
@optional
无人机用途。`mode = 10000` 时该字段无效。
<br>
选项：`_NotUse` | `Money` | `SyntheticJade` | `CombatRecord` | `PureGold` | `OriginStone` | `Chip`  
:::  
::: field threshold  
@type number
@default 0.3
@optional
工作心情阈值，取值范围 [0, 1.0]。
<br>
`mode = 10000` 时该字段仅针对 "autofill" 有效。
<br>
`mode = 20000` 时该字段无效。  
:::  
::: field replenish  
@type boolean
@default false
@optional
贸易站“源石碎片”是否自动补货。  
:::  
::: field dorm_notstationed_enabled  
@type boolean
@default false
@optional
是否启用宿舍“未进驻”选项。  
:::  
::: field dorm_trust_enabled  
@type boolean
@default false
@optional
是否将宿舍剩余位置填入信赖未满干员。  
:::  
::: field fiammetta_targets  
@type array<string>
@default ["清流", "可露希尔", "但书"]
@optional
菲亚梅塔恢复目标名单，换班开始时会将名单中当前心情最低的干员与菲亚梅塔一同进驻宿舍互换心情。仅 `mode = 0` 且 `fiammetta_recovery_enabled` 为 true 时生效。
<br>
选项：`清流` | `可露希尔` | `但书` | `巫恋` | `龙舌兰` | `歌蕾蒂娅`（不在选项内或重复的条目会被忽略）  
:::  
::: field fiammetta_recovery_enabled  
@type boolean
@default false
@optional
是否在换班开始时使用菲亚梅塔为恢复目标恢复心情；关闭时换班将跳过宿舍准备步骤。仅 `mode = 0` 时生效。  
:::  
::: field use_pinus_sylvestris  
@type boolean
@default false
@optional
是否启用 ｢红松骑士团｣ 跨设施组合。仅 `mode = 0` 时生效。  
:::  
::: field use_perception_information  
@type boolean
@default false
@optional
是否启用 ｢感知信息｣ 跨设施组合，优先度高于 ｢人间烟火｣。仅 `mode = 0` 时生效。  
:::  
::: field use_worldly_plight  
@type boolean
@default false
@optional
是否启用 ｢人间烟火｣ 跨设施组合。仅 `mode = 0` 时生效。  
:::  
::: field use_abyssal_hunter  
@type boolean
@default false
@optional
是否启用 ｢深海猎人｣ 跨设施组合。仅 `mode = 0` 时生效，与 ｢红松骑士团｣ 同时启用时两者不会同时参与排班。  
:::  
::: field reception_message_board  
@type boolean
@default true
@optional
是否领取会客室信息板信用。  
:::  
::: field reception_clue_exchange  
@type boolean
@default true
@optional
是否进行线索交流。  
:::  
::: field reception_send_clue  
@type boolean
@default true
@optional
是否赠送线索。  
:::  
::: field filename  
@type string
@required
自定义配置路径。不支持运行中设置。
<br>
<Badge type="warning" text="仅在 mode = 10000 时生效" />  
:::  
::: field plan_index  
@type number
@required
使用配置中的方案序号。不支持运行中设置。
<br>
<Badge type="warning" text="仅在 mode = 10000 时生效" />  
:::  
::: field continue_training  
@type boolean
@default false
@optional
训练室是否继续未完成的专精训练。  
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
   领取信用及商店购物。  
   会先有序的按 `buy_first` 购买一遍，再从左到右并避开 `blacklist` 购买第二遍，在信用溢出时则会无视黑名单从左到右购买第三遍直到不再溢出

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field visit_friends  
@type boolean
@default true
@optional
是否访问好友基建以获得信用。  
:::  
::: field shopping  
@type boolean
@default true
@optional
是否购物。  
:::  
::: field buy_first  
@type array<string>
@default []
@optional
优先购买列表。商品名，如 `"招聘许可"`、`"龙门币"` 等。  
:::  
::: field blacklist  
@type array<string>
@default []
@optional
购物黑名单列表。商品名，如 `"加急许可"`、`"家具零件"` 等。  
:::  
::: field force_shopping_if_credit_full  
@type boolean
@default false
@optional
是否在信用溢出时无视黑名单。  
:::  
::: field only_buy_discount  
@type boolean
@default false
@optional
是否只购买折扣物品，只作用于第二轮购买。  
:::  
::: field reserve_max_credit  
@type boolean
@default false
@optional
是否在信用点低于 300 时停止购买，只作用于第二轮购买。  
:::  
::: field credit_fight  
@type boolean
@default false
@optional
是否借助战打一局 OF-1 关卡以便在第二天获得更多信用。  
:::  
::: field formation_index  
@type number
@default 0
@optional
打 OF-1 时所使用的编队栏位的编号。
<br>
为 0–4 的整数，其中 0 表示选择当前编队，1-4 分别表示第一、二、三、四编队。  
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
   领取各种奖励

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field award  
@type boolean
@default true
@optional
领取每日/每周任务奖励。  
:::  
::: field mail  
@type boolean
@default false
@optional
领取所有邮件奖励。  
:::  
::: field recruit  
@type boolean
@default false
@optional
领取限定池子赠送的每日免费单抽。  
:::  
::: field orundum  
@type boolean
@default false
@optional
领取幸运墙的合成玉奖励。  
:::  
::: field mining  
@type boolean
@default false
@optional
领取限时开采许可的合成玉奖励。  
:::  
::: field specialaccess  
@type boolean
@default false
@optional
领取五周年赠送的月卡奖励。  
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
   无限刷肉鸽

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field theme  
@type string
@default Phantom
@optional
主题。
<br>
`Phantom` - 傀影与猩红血钻
<br>
`Mizuki` - 水月与深蓝之树
<br>
`Sami` - 探索者的银凇止境
<br>
`Sarkaz` - 萨卡兹的无终奇语
<br>
`JieGarden` - 岁的界园志异
<br>
`BlackFlow` - 黑流树海  
:::  
::: field mode  
@type number
@default 0
@optional
模式。
<br>
`0` - 刷分/奖励点数，尽可能稳定地打更多层数。
<br>
`1` - 刷源石锭，第一层投资完就退出。
<br>
`2` - <Badge type="danger" text="已移除" /> 原兼顾模式 0 与 1，当前版本传入会被拒绝。
<br>
`3` - <Badge type="danger" text="未开放" /> 传入会被拒绝。
<br>
`4` - 凹开局，先在 0 难度下到达第三层后重开，再到指定难度下凹开局奖励，若不为热水壶或希望则回到 0 难度下重新来过；若在 Phantom 主题下则不切换难度，仅在当前难度下尝试到达第三层、重开、凹开局。
<br>
`5` - 刷坍缩范式；仅适用于 Sami 主题；通过战斗漏怪等方式加快坍缩值积累，若遇到的第一个的坍缩范式在 `expected_collapsal_paradigms` 列表中则停止任务，否则重开。
<br>
`6` - 刷月度小队蚊子腿，除了针对模式的适配以外和模式 0 相同。
<br>
`7` - 刷深入调查蚊子腿，除了针对模式的适配以外和模式 0 相同。
<br>
`10001` - 快速通过第一层；仅适用于 Sarkaz 主题。
<br>
`20001` - 刷常乐节点，第一层进洞，找不到需要的节点就重开；仅适用于 JieGarden 主题，需配合 `find_playTime_target`。
<br>
`30001` - 刷襁褓动物；仅适用于 BlackFlow 主题。
:::  
::: field squad  
@type string
@default 指挥分队
@optional
开局分队名。  
:::  
::: field roles  
@type string
@default 取长补短
@optional
开局职业组。  
:::  
::: field core_char  
@type string
@optional
开局干员名。仅支持单个干员**中文名**，无论区服；若留空或设置为空字符串 `""` 则根据练度自动选择。  
:::  
::: field use_support  
@type boolean
@default false
@optional
开局干员是否为助战干员。  
:::  
::: field use_nonfriend_support  
@type boolean
@default false
@optional
是否可以是非好友助战干员。仅在 `use_support` 为 true 时有效。  
:::  
::: field starts_count  
@type number
@default 2147483647
@optional
开始探索的次数。达到后自动停止任务。  
:::  
::: field difficulty  
@type number
@default -1
@optional
指定难度等级，`-1` 表示不指定难度。若指定难度未解锁，则会选择当前已解锁的最高难度。  
:::  
::: field stop_at_final_boss  
@type boolean
@default false
@optional
是否在第 5 层险路恶敌节点前停止任务。仅适用于**除 Phantom 以外**的主题。  
:::  
::: field stop_at_max_level  
@type boolean
@default false
@optional
是否在肉鸽等级刷满后停止任务。  
:::  
::: field investment_enabled  
@type boolean
@default true
@optional
是否投资源石锭。  
:::  
::: field investments_count  
@type number
@default 2147483647
@optional
投资源石锭的次数。达到后自动停止任务。  
:::  
::: field stop_when_investment_full  
@type boolean
@default false
@optional
是否在投资到达上限后自动停止任务。  
:::  
::: field investment_with_more_score  
@type boolean
@default false
@optional
是否在投资后尝试购物。仅适用于模式 1。  
:::  
::: field start_with_elite_two  
@type boolean
@default false
@optional
是否在凹开局的同时凹干员精二直升。仅适用于模式 4。  
:::  
::: field only_start_with_elite_two  
@type boolean
@default false
@optional
是否只凹开局干员精二直升而忽视其他开局条件。仅在模式为 4 且 `start_with_elite_two` 为 true 时有效。  
:::  
::: field refresh_trader_with_dice  
@type boolean
@default false
@optional
是否用骰子刷新商店购买特殊商品。仅适用于 Mizuki 主题，用于刷指路鳞。  
:::  
::: field first_floor_foldartal  
@type string
@optional
希望在第一层远见阶段得到的密文版。仅适用于 Sami 主题，不限模式；若成功凹到则停止任务。  
:::  
::: field start_foldartal_list  
@type array<string>
@default []
@optional
凹开局时希望在开局奖励阶段得到的密文板。仅在主题为 Sami 且模式为 4 时有效。
<br>
仅当开局拥有列表中所有的密文板时才算凹开局成功。
<br>
注意，此参数须与 “生活至上分队” 同时使用，其他分队在开局奖励阶段不会获得密文板。  
:::  
::: field collectible_mode_start_list  
@type object
@optional
凹开局时期望的奖励，默认全为 false。仅在模式为 4 时有效。
<br>
`hot_water`: 热水壶奖励，常用于触发烧水机制（通用）。
<br>
`shield`: 护盾奖励，约等于额外生命值（通用）。
<br>
`ingot`: 源石锭奖励（通用）。
<br>
`hope`: 希望奖励（通用，注意：JieGarden 主题下无 hope 奖励）。
<br>
`random`: 随机奖励选项：游戏中指“消耗所有源石锭换一个随机收藏品”（通用）。
<br>
`key`: 钥匙奖励，仅在 Mizuki 主题时有效。
<br>
`dice`: 骰子奖励，仅在 Mizuki 主题时有效。
<br>
`ideas`: 2 构想奖励，仅在 Sarkaz 主题时有效。
<br>
`ticket`: 票券奖励，仅在 JieGarden 主题时有效。
:::  
::: field use_foldartal  
@type boolean
@optional
是否使用密文板。模式 5 下默认值 `false`，其他模式下默认值 `true`。仅适用于 Sami 主题。  
:::  
::: field check_collapsal_paradigms  
@type boolean
@optional
是否检测获取的坍缩范式。模式 5 下默认值 `true`，其他模式下默认值 `false`。  
:::  
::: field double_check_collapsal_paradigms  
@type boolean
@default true
@optional
是否执行坍缩范式检测防漏措施。仅在主题为 Sami 且 `check_collapsal_paradigms` 为 true 时有效。模式 5 下默认值 `true`，其他模式下默认值 `false`。  
:::  
::: field expected_collapsal_paradigms  
@type array<string>
@default ['目空一些', '睁眼瞎', '图像损坏', '一抹黑']
@optional
希望触发的坍缩范式。仅在主题为 Sami 且模式为 5 时有效。  
:::  
::: field monthly_squad_auto_iterate  
@type boolean
@optional
是否启动月度小队自动切换。  
:::  
::: field monthly_squad_check_comms  
@type boolean
@optional
是否将月度小队通信也作为切换依据。  
:::  
::: field deep_exploration_auto_iterate  
@type boolean
@optional
是否启动深入调查自动切换。  
:::  
::: field collectible_mode_shopping  
@type boolean
@default false
@optional
烧水是否启用购物。  
:::  
::: field collectible_mode_squad  
@type string
@optional
烧水时使用的分队, 默认与squad同步, 当squad为空字符串且未指定collectible_mode_squad值时为指挥分队。  
:::  
::: field start_with_seed  
@type string
@optional
使用种子刷钱时填入固定种子，留空则不启用。
<br>
仅在 Sarkaz 主题，Investment 模式，“点刺成锭分队” or “后勤分队” 时生效。  
:::  
::: field blackflow_strategy  
@type string
@optional
黑流树海主题的策略；留空时按 `mode` 与 `investment_enabled` 推断。
<br>
`baby_animal` - 第一层检查普通商店，第二、三层探索并进入秘境行商培育种子，需配合 `blackflow_cultivation_target`
<br>
`investment` - 第一层以战斗次数最少、预计时间最短的完整路线抵达固定普通商店
<br>
`burn_with_investment` - 第一层完成投资后尽快抵达第三层，到达即重开
<br>
`burn` - 尽快抵达第三层，到达即重开  
:::  
::: field blackflow_cultivation_target  
@type string
@default swaddled_cat
@optional
刷襁褓动物模式的目标。可选值：`swaddled_cat` | `swaddled_feathered_serpent` | `swaddled_dog` | `swaddled_cerberus`；仅在 `blackflow_strategy` 为 `baby_animal` 时使用。  
:::  
::: field find_playTime_target  
@type number
@optional
刷常乐节点模式的目标常乐节点。`1` - 令（掷地有声）；`2` - 黍（种因得果）；`3` - 年（三缺一）。仅在主题为 JieGarden 且模式为 20001 时使用，该模式下必填；不填或其他值会导致任务参数设置失败。  
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
   "collectible_mode_squad": "",
   "start_with_seed": ""
}
```

</details>

刷坍缩范式功能具体请参考 [肉鸽辅助协议](./integrated-strategy-schema.md#萨米肉鸽——坍缩范式)

- `Copilot`  
   自动抄作业

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field filename  
@type string
单一作业 JSON 文件的路径，与 copilot_list 二选一（必填）；相对路径与绝对路径均可。  
:::  
::: field copilot_list  
@type array`<object>`
作业列表，与 filename 二选一（必填）；当 filename 与 copilot_list 同时存在时，忽视 copilot_list；此参数生效时仅可执行 set_params 一次。
<br>
每个对象包含：
<br>

- `filename`: 作业 JSON 文件的路径；相对路径与绝对路径均可
  <br>
- `nav_name_override`: 导航用关卡名，可选；未提供或为 `null` 时自动从作业文件推导
  <br>
- `is_raid`: 是否切换为突袭模式，可选，默认值 false
  :::  
  ::: field loop_times  
  @type number
  @default 1
  @optional
  循环次数。仅在单一作业模式下（即指定 filename 时）有效；此参数生效时仅可执行 set_params 一次。  
  :::  
  ::: field use_sanity_potion  
  @type boolean
  @default false
  @optional
  是否允许在剩余理智不足时使用理智药。  
  :::  
  ::: field formation  
  @type boolean
  @default false
  @optional
  是否进行自动编队。  
  :::  
  ::: field formation_index  
  @type number
  @default 0
  @optional
  自动编队所使用的编队栏位的编号。仅在 formation 为 true 时有效。
  <br>
  为 0–4 的整数，其中 0 表示选择当前编队，1-4 分别表示第一、二、三、四编队。  
  :::  
  ::: field user_additional  
  @type array`<object>`
  @default []
  @optional
  自定义追加干员列表。仅在 formation 为 true 时有效。
  <br>
  每个对象包含：
  <br>
- `name`: 干员名，可选，默认值 ""，若留空则忽视此干员
  <br>
- `skill`: 需要携带的技能，可选，默认值 0，即遵从游戏内默认的技能选择；为 1–3 的整数，若不在此范围内也遵从游戏内默认的技能选择  
  :::  
  ::: field add_trust  
  @type boolean
  @default false
  @optional
  是否在自动编队时以信赖值升序自动填充空余栏位。仅在 formation 为 true 时有效。  
  :::  
  ::: field ignore_requirements  
  @type boolean
  @default false
  @optional
  是否在自动编队时忽视干员属性要求。仅在 formation 为 true 时有效。  
  :::  
  ::: field support_unit_usage  
  @type number
  @default 0
  @optional
  助战干员的使用模式。为 0–3 的整数。仅在 formation 为 true 时有效。
  <br>
  `0` - 表示不使用助战干员
  <br>
  `1` - 如果有且仅有一名缺失干员则尝试寻找助战干员补齐编队，如果无缺失干员则不使用助战干员
  <br>
  `2` - 如果有且仅有一名缺失干员则尝试寻找助战干员补齐编队，如果无缺失干员则使用指定助战干员  
  <br>
  `3` - 如果有且仅有一名缺失干员则尝试寻找助战干员补齐编队，如果无缺失干员则使用随机助战干员  
  :::  
  ::: field support_unit_name  
  @type string
  @optional
  指定助战干员名。仅在 support_unit_usage 为 2 时有效。  
  :::  
  ::::

作业 JSON 请参考 [战斗流程协议](./copilot-schema.md)

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
   自动抄保全作业

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field filename  
@type string
@required
作业 JSON 的文件路径，绝对、相对路径均可。不支持运行期设置。  
:::  
::: field loop_times  
@type number
@optional
循环执行次数。  
:::  
::::  
保全作业 JSON 请参考 [保全派驻协议](./sss-schema.md)

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
  自动抄悖论模拟作业

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field filename  
@type string
@required
单个作业 JSON 的文件路径，绝对、相对路径均可。不支持运行期设置。必选，与 list 二选一。  
:::  
::: field list  
@type array`<object>` | array`<string>`
@required
作业列表，不支持运行期设置。必选，与 filename 二选一。
<br>
数组元素支持两种形式：对象形式包含 `id`（作业标识，会原样透传至 `CopilotListLoadTaskFileSuccess` 回调）与 `filename`（作业 JSON 文件的路径，绝对、相对路径均可）；也可直接使用作业路径字符串。  
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
   仓库识别

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
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
   干员 box 识别

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
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
   生息演算

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field theme  
@type string
@default Tales
@optional
主题。
<br>
`Fire` - _沙中之火_（已关闭）
<br>
`Tales` - _沙洲遗闻_
<br>
`RelaunchAnchor` - _重启锚点_  
:::  
::: field mode  
@type number
@default 0
@optional
模式。不同主题支持的模式不同：
<br>
**Tales（沙洲遗闻）：**
<br>
`0` - 无存档，通过进出关卡刷生息点数。
<br>
`1` - 有存档，通过组装支援道具刷生息点数。
<br>
**RelaunchAnchor（重启锚点）：**
<br>
`16` (`RA1`) - RA-1，自动执行精耕细作、建设、交付资源、结算循环。
<br>
`32` (`RA15`) - RA-15，用圣聆初雪完成 60 杀任务。
<br>
`48` (`RA4`) - RA-4，使用筹划经营策略给予的赤金解锁区域，使用维什戴尔完成击杀 boss 任务。
:::  
::: field tools_to_craft  
@type array<string>
@default []
@optional
自动制造的物品，建议填写子串，留空则不制造。仅 Tales 主题的有存档模式（mode = 1）有效。  
:::  
::: field clear_store  
@type boolean
@default false
@optional
任务完成后是否购买（清空）商店商品。仅 Tales 主题的无存档模式（mode = 0）有效。  
:::  
::: field increment_mode  
@type number
@default 0
@optional
点击类型。仅 Tales 主题有效。
<br>
`0` - 连点
<br>
`1` - 长按  
:::  
::: field num_craft_batches  
@type number
@default 16
@optional
单次最大制造轮数。仅 Tales 主题有效。  
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
   自定义任务

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field task_names  
@type array<string>
@required
执行数组中第一个匹配上的任务（及后续 next 等）。若想执行多个任务，可多次 append Custom task。  
支持秘密前线（`MiniGame@SecretFront`）拼接形式：`MiniGame@SecretFront@Begin@Ending[A-E](@事件名)?`，事件名可选（支援作战平台 / 游侠 / 诡影迷踪），例如 `MiniGame@SecretFront@Begin@EndingA@支援作战平台`。  
:::  
::: field params  
@type object
@optional
任务附加参数。目前仅像素画任务（`MiniGame@PixelPaint@Begin`）使用：

- `params.pixel_paint.groups`：按色分组点列。`color` 为色板序号（0~39，与游戏右侧色板顺序一致），`points` 为 `[x, y]` 格子坐标数组（0~23，左上为原点）。
- `params.pixel_paint.swipe`（bool，可选，默认 true）：同色同行连续格用拖动一次画完，更快但部分触控模式可能异常。
- `params.pixel_paint.grid_delay`（int，可选，默认 0）：每格额外等待（ms）。点击后 sleep，拖动时长按格累加。各触控方式自带基础间隔，一般无需调整。兼容旧键 `grid_click_delay`。

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
   单步任务（目前仅支持战斗）

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field type  
@type string
@default copilot
@required
目前仅支持 `"copilot"`。  
:::  
::: field subtype  
@type string
@required
子任务类型。
<br>
`stage` - 设置关卡名，需要 `"details": { "stage_name": "xxxx" }`。
<br>
`start` - 开始作战，无 `details`。
<br>
`action` - 单步作战操作，`details` 需为作战协议中的单个 action，例如：`"details": { "name": "史尔特尔", "location": [ 4, 5 ], "direction": "左" }`，详情参考 [战斗流程协议](./copilot-schema.md)。  
:::  
::: field details  
@type object
@optional
子任务的详细参数。  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "type": "copilot",
   "subtype": "stage",
   "details": {
      "stage_name": "1-7"
   }
}
```

</details>

- `VideoRecognition`  
  视频识别，目前仅支持作业（作战）视频

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
是否启用本任务。  
:::  
::: field filename  
@type string
@required
视频的文件路径，绝对、相对路径均可。不支持运行期设置。  
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

#### 接口原型

```cpp
AsstBool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);
```

#### 接口说明

设置任务参数

#### 返回值

- `AsstBool`  
   返回是否设置成功

#### 参数说明

:::: field-group  
::: field handle  
@type AsstHandle
@required
实例句柄  
:::  
::: field id  
@type AsstTaskId
@required
任务 ID, `AsstAppendTask` 接口的返回值  
:::  
::: field params  
@type const char\*
@required
任务参数，json string，与 `AsstAppendTask` 接口相同。  
未标注“不支持运行中设置”的字段都支持实时修改；否则若当前任务正在运行，会忽略对应的字段  
:::  
::::

### `AsstSetStaticOption`

#### 接口原型

```cpp
AsstBool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### 接口说明

设置进程级参数

#### 返回值

- `AsstBool`  
   返回是否设置成功

#### 参数说明

:::: field-group  
::: field key  
@type AsstStaticOptionKey
@required
键  
:::  
::: field value  
@type const char\*
@required
值  
:::  
::::

##### 键值一览

:::: field-group  
::: field Invalid  
@type number
@default 0
@optional
无效占位。枚举值：0。  
:::  
::: field CpuOCR  
@type boolean
@optional
使用 CPU 进行 OCR。值不参与解析。资源加载后不支持切换。枚举值：1。  
:::  
::: field GpuOCR  
@type string
@optional
使用 GPU 进行 OCR。值为 GPU 设备序号（整数），Windows 上也可传 `luid:<十六进制 LUID>`。资源加载后不支持切换。枚举值：2。  
:::  
::::

### `AsstSetInstanceOption`

#### 接口原型

```cpp
AsstBool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### 接口说明

设置实例级参数

#### 返回值

- `AsstBool`  
   返回是否设置成功

#### 参数说明

:::: field-group  
::: field handle  
@type AsstHandle
@required
实例句柄  
:::  
::: field key  
@type AsstInstanceOptionKey
@required
键  
:::  
::: field value  
@type const char\*
@required
值  
:::  
::::

##### 键值一览

:::: field-group  
::: field Invalid  
@type number
@default 0
@optional
无效占位。枚举值：0。  
:::  
::: field MinitouchEnabled  
@type boolean
@optional
已弃用。原为是否启用 Minitouch；"1" 开，"0" 关。注意设备可能不支持。枚举值：1（已弃用）。  
:::  
::: field TouchMode  
@type string
@default minitouch
@optional
触控模式设置。可选值：minitouch | maatouch | adb | MacPlayTools | MaaFwAdb | MumuExtras | MaaFwLinux。默认 minitouch。枚举值：2。  
:::  
::: field DeploymentWithPause  
@type boolean
@optional
是否暂停下干员，同时影响抄作业、肉鸽、保全。可用值："1" 或 "0"。枚举值：3。  
:::  
::: field AdbLiteEnabled  
@type boolean
@optional
是否使用 AdbLite。可用值："0" 或 "1"。枚举值：4。  
:::  
::: field KillAdbOnExit  
@type boolean
@optional
退出时是否杀掉 Adb 进程。可用值："0" 或 "1"。枚举值：5。  
:::  
::: field ClientType  
@type string
@optional
客户端类型（游戏渠道）。大多数连接配置不需要设置。仅当传给 `AsstConnect` / `AsstAsyncConnect` 的 `config` 在连接阶段命令里使用 `[PackageName]` 时，才需要在连接前调用 `AsstSetInstanceOption(..., ClientType, ...)`。当前内置配置仅 `Androws` 和 `WSA` 的 `displayId` 查询依赖该值。该选项不替代 StartUp / CloseDown 等任务参数里的 `client_type`。枚举值：6。  
:::  
::::
