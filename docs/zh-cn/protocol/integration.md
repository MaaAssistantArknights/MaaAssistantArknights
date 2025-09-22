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
::: field name="handle" type="AsstHandle" required
实例句柄
:::
::: field name="type" type="const char*" required
任务类型
:::
::: field name="params" type="const char*" required
任务参数，json string
:::
::::

##### 任务类型一览

- `StartUp`  
  开始唤醒  

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="client_type" type="string" required
客户端版本。
<br>
选项：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`
:::
::: field name="start_game_enabled" type="boolean" optional default="false"
是否自动启动客户端。
:::
::: field name="account_name" type="string" optional
切换账号，默认不切换。
<br>
仅支持切换至已登录的账号，使用登录名进行查找，保证输入内容在所有已登录账号唯一即可。
<br>
官服：`123****4567`，可输入 `123****4567`、`4567`、`123`、`3****4567`
<br>
B服：`张三`，可输入 `张三`、`张`、`三`
:::
::::

- `CloseDown`  
    关闭游戏  

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="client_type" type="string" required
客户端版本，填空则不执行。
<br>
选项：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`
:::
::::

- `Fight`  
    刷理智

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="stage" type="string" optional
关卡名，默认为空，识别当前/上次的关卡。不支持运行中设置。
<br>
支持全部主线关卡，如 `"1-7"`、`"S3-2"`等。
<br>
可在关卡结尾输入`Normal`/`Hard`表示需要切换标准与磨难难度。
<br>
剿灭作战，必须输入 `"Annihilation"`。
<br>
当期 SS 活动 后三关，必须输入完整关卡编号。
:::
::: field name="medicine" type="number" optional default="0"
最大使用理智药数量。
:::
::: field name="expiring_medicine" type="number" optional default="0"
最大使用 48 小时内过期理智药数量。
:::
::: field name="stone" type="number" optional default="0"
最大吃石头数量。
:::
::: field name="times" type="number" optional default="2147483647"
战斗次数。
:::
::: field name="series" type="number" optional
连战次数, -1~6。
<br>
` -1` 为禁用切换。
<br>
`  0` 为自动切换为当前可用的最大次数, 如当前理智不够6次, 则选择最低可用次数。
<br>
`1~6` 为指定连战次数。
:::
::: field name="drops" type="object" optional
指定掉落数量，默认不指定。key 为 item_id, value 为数量。key 可参考 `resource/item_index.json` 文件。
<br>
例如: `{ "30011": 10, "30062": 5 }`
<br>
以上全部是或的关系，即任一达到即停止任务。
:::
::: field name="report_to_penguin" type="boolean" optional default="false"
是否汇报企鹅数据。
:::
::: field name="penguin_id" type="string" optional
企鹅数据汇报 id, 默认为空。仅在 `report_to_penguin` 为 true 时有效。
:::
::: field name="server" type="string" optional default="CN"
服务器，会影响掉落识别及上传。
<br>
选项：`CN` | `US` | `JP` | `KR`
:::
::: field name="client_type" type="string" optional
客户端版本，默认为空。用于游戏崩溃时重启并连回去继续刷，若为空则不启用该功能。
<br>
选项：`Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`
:::
::: field name="DrGrandet" type="boolean" optional default="false"
节省理智碎石模式，仅在可能产生碎石效果时生效。
<br>
在碎石确认界面等待，直到当前的 1 点理智恢复完成后再立刻碎石。
:::
::::

另支持少部分资源关卡名请参考[集成示例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/tools/AutoLocalization/example/zh-cn.xaml#L260)

- `Recruit`  
  公开招募

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="refresh" type="boolean" optional default="false"
是否刷新三星 Tags。
:::
::: field name="select" type="array<number>" required
会去点击标签的 Tag 等级。
:::
::: field name="confirm" type="array<number>" required
会去点击确认的 Tag 等级。若仅公招计算，可设置为空数组。
:::
::: field name="first_tags" type="array<string>" optional
首选 Tags，仅在 Tag 等级为 3 时有效。默认为空。
<br>
当 Tag 等级为 3 时，会尽可能多地选择这里的 Tags（如果有），而且是强制选择，也就是会忽略所有“让 3 星 Tag 不被选择”的设置。
:::
::: field name="extra_tags_mode" type="number" optional default="0"
选择更多的 Tags。
<br>
`0` - 默认行为
<br>
`1` - 选 3 个 Tags, 即使可能冲突
<br>
`2` - 如果可能, 同时选择更多的高星 Tag 组合, 即使可能冲突
:::
::: field name="times" type="number" optional default="0"
招募多少次。若仅公招计算，可设置为 0。
:::
::: field name="set_time" type="boolean" optional default="true"
是否设置招募时限。仅在 `times` 为 0 时生效。
:::
::: field name="expedite" type="boolean" optional default="false"
是否使用加急许可。
:::
::: field name="expedite_times" type="number" optional
加急次数，仅在 `expedite` 为 true 时有效。默认无限使用（直到 `times` 达到上限）。
:::
::: field name="skip_robot" type="boolean" optional default="true"
是否在识别到小车词条时跳过。
:::
::: field name="recruitment_time" type="object" optional
Tag 等级（大于等于 3）和对应的希望招募时限，单位为分钟，默认值都为 540（即 09:00:00）。
<br>
例如: `{ "3": 540, "4": 540 }`
:::
::: field name="report_to_penguin" type="boolean" optional default="false"
是否汇报企鹅数据。
:::
::: field name="penguin_id" type="string" optional
企鹅数据汇报 id, 默认为空。仅在 `report_to_penguin` 为 true 时有效。
:::
::: field name="report_to_yituliu" type="boolean" optional default="false"
是否汇报一图流数据。
:::
::: field name="yituliu_id" type="string" optional
一图流汇报 id, 默认为空。仅在 `report_to_yituliu` 为 true 时有效。
:::
::: field name="server" type="string" optional default="CN"
服务器，会影响上传。
<br>
选项：`CN` | `US` | `JP` | `KR`
:::
::::

- `Infrast`  
    基建换班

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="mode" type="number" optional default="0"
换班工作模式。
<br>
`0` - `Default`: 默认换班模式，单设施最优解。
<br>
`10000` - `Custom`: 自定义换班模式，读取用户配置，可参考 [基建排班协议](./base-scheduling-schema.md)。
<br>
`20000` - `Rotation`: 一键轮换模式，会跳过控制中枢、发电站、宿舍以及办公室，其余设施不进行换班但保留基本操作（如使用无人机、会客室逻辑）。
:::
::: field name="facility" type="array<string>" required
要换班的设施（有序）。不支持运行中设置。
<br>
设施名：`Mfg` | `Trade` | `Power` | `Control` | `Reception` | `Office` | `Dorm` | `Processing` | `Training`
:::
::: field name="drones" type="string" optional default="_NotUse"
无人机用途。`mode = 10000` 时该字段无效。
<br>
选项：`_NotUse` | `Money` | `SyntheticJade` | `CombatRecord` | `PureGold` | `OriginStone` | `Chip`
:::
::: field name="threshold" type="number" optional default="0.3"
工作心情阈值，取值范围 [0, 1.0]。
<br>
`mode = 10000` 时该字段仅针对 "autofill" 有效。
<br>
`mode = 20000` 时该字段无效。
:::
::: field name="replenish" type="boolean" optional default="false"
贸易站“源石碎片”是否自动补货。
:::
::: field name="dorm_notstationed_enabled" type="boolean" optional default="false"
是否启用宿舍“未进驻”选项。
:::
::: field name="dorm_trust_enabled" type="boolean" optional default="false"
是否将宿舍剩余位置填入信赖未满干员。
:::
::: field name="reception_message_board" type="boolean" optional default="true"
是否领取会客室信息板信用。
:::
::: field name="reception_clue_exchange" type="boolean" optional default="true"
是否进行线索交流。
:::
::: field name="filename" type="string" required
自定义配置路径。不支持运行中设置。
<br>
<Badge type="warning" text="仅在 mode = 10000 时生效" />
:::
::: field name="plan_index" type="number" required
使用配置中的方案序号。不支持运行中设置。
<br>
<Badge type="warning" text="仅在 mode = 10000 时生效" />
:::
::::

- `Mall`  
    领取信用及商店购物。  
    会先有序的按 `buy_first` 购买一遍，再从左到右并避开 `blacklist` 购买第二遍，在信用溢出时则会无视黑名单从左到右购买第三遍直到不再溢出

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="visit_friends" type="boolean" optional default="true"
是否访问好友基建以获得信用。
:::
::: field name="shopping" type="boolean" optional default="true"
是否购物。
:::
::: field name="buy_first" type="array<string>" optional default="[]"
优先购买列表。商品名，如 `"招聘许可"`、`"龙门币"` 等。
:::
::: field name="blacklist" type="array<string>" optional default="[]"
购物黑名单列表。商品名，如 `"加急许可"`、`"家具零件"` 等。
:::
::: field name="force_shopping_if_credit_full" type="boolean" optional default="false"
是否在信用溢出时无视黑名单。
:::
::: field name="only_buy_discount" type="boolean" optional default="false"
是否只购买折扣物品，只作用于第二轮购买。
:::
::: field name="reserve_max_credit" type="boolean" optional default="false"
是否在信用点低于 300 时停止购买，只作用于第二轮购买。
:::
::: field name="credit_fight" type="boolean" optional default="false"
是否借助战打一局 OF-1 关卡以便在第二天获得更多信用。
:::
::: field name="formation_index" type="number" optional default="0"
打 OF-1 时所使用的编队栏位的编号。
<br>
为 0–4 的整数，其中 0 表示选择当前编队，1-4 分别表示第一、二、三、四编队。
:::
::::

- `Award`  
    领取各种奖励

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="award" type="boolean" optional default="true"
领取每日/每周任务奖励。
:::
::: field name="mail" type="boolean" optional default="false"
领取所有邮件奖励。
:::
::: field name="recruit" type="boolean" optional default="false"
领取限定池子赠送的每日免费单抽。
:::
::: field name="orundum" type="boolean" optional default="false"
领取幸运墙的合成玉奖励。
:::
::: field name="mining" type="boolean" optional default="false"
领取限时开采许可的合成玉奖励。
:::
::: field name="specialaccess" type="boolean" optional default="false"
领取五周年赠送的月卡奖励。
:::
::::

- `Roguelike`  
    无限刷肉鸽

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="theme" type="string" optional default="Phantom"
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
`JieGarden` - 界园
:::
::: field name="mode" type="number" optional default="0"
模式。
<br>
`0` - 刷分/奖励点数，尽可能稳定地打更多层数。
<br>
`1` - 刷源石锭，第一层投资完就退出。
<br>
`2` - <Badge type="danger" text="已弃用" /> 兼顾模式 0 与 1，投资过后再退出，没有投资就继续往后打。
<br>
`3` - 开发中...
<br>
`4` - 凹开局，先在 0 难度下到达第三层后重开，再到指定难度下凹开局奖励，若不为热水壶或希望则回到 0 难度下重新来过；若在 Phantom 主题下则不切换难度，仅在当前难度下尝试到达第三层、重开、凹开局。
<br>
`5` - 刷坍缩范式；仅适用于 Sami 主题；通过战斗漏怪等方式加快坍缩值积累，若遇到的第一个的坍缩范式在 `expected_collapsal_paradigms` 列表中则停止任务，否则重开。
<br>
`6` - 刷月度小队蚊子腿，除了针对模式的适配以外和模式0相同。
<br>
`7` - 刷深入调查蚊子腿，除了针对模式的适配以外和模式0相同。
:::
::: field name="squad" type="string" optional default="指挥分队"
开局分队名。
:::
::: field name="roles" type="string" optional default="取长补短"
开局职业组。
:::
::: field name="core_char" type="string" optional
开局干员名。仅支持单个干员**中文名**，无论区服；若留空或设置为空字符串 `""` 则根据练度自动选择。
:::
::: field name="use_support" type="boolean" optional default="false"
开局干员是否为助战干员。
:::
::: field name="use_nonfriend_support" type="boolean" optional default="false"
是否可以是非好友助战干员。仅在 `use_support` 为 true 时有效。
:::
::: field name="starts_count" type="number" optional default="2147483647"
开始探索的次数。达到后自动停止任务。
:::
::: field name="difficulty" type="number" optional default="0"
指定难度等级。仅适用于**除 Phantom 以外**的主题。若未解锁难度，则会选择当前已解锁的最高难度。
:::
::: field name="stop_at_final_boss" type="boolean" optional default="false"
是否在第 5 层险路恶敌节点前停止任务。仅适用于**除 Phantom 以外**的主题。
:::
::: field name="stop_at_max_level" type="boolean" optional default="false"
是否在肉鸽等级刷满后停止任务。
:::
::: field name="investment_enabled" type="boolean" optional default="true"
是否投资源石锭。
:::
::: field name="investments_count" type="number" optional default="2147483647"
投资源石锭的次数。达到后自动停止任务。
:::
::: field name="stop_when_investment_full" type="boolean" optional default="false"
是否在投资到达上限后自动停止任务。
:::
::: field name="investment_with_more_score" type="boolean" optional default="false"
是否在投资后尝试购物。仅适用于模式 1。
:::
::: field name="start_with_elite_two" type="boolean" optional default="false"
是否在凹开局的同时凹干员精二直升。仅适用于模式 4。
:::
::: field name="only_start_with_elite_two" type="boolean" optional default="false"
是否只凹开局干员精二直升而忽视其他开局条件。仅在模式为 4 且 `start_with_elite_two` 为 true 时有效。
:::
::: field name="refresh_trader_with_dice" type="boolean" optional default="false"
是否用骰子刷新商店购买特殊商品。仅适用于 Mizuki 主题，用于刷指路鳞。
:::
::: field name="first_floor_foldartal" type="string" optional
希望在第一层远见阶段得到的密文版。仅适用于 Sami 主题，不限模式；若成功凹到则停止任务。
:::
::: field name="start_foldartal_list" type="array<string>" optional default="[]"
凹开局时希望在开局奖励阶段得到的密文板。仅在主题为 Sami 且模式为 4 时有效。
<br>
仅当开局拥有列表中所有的密文板时才算凹开局成功。
<br>
注意，此参数须与 “生活至上分队” 同时使用，其他分队在开局奖励阶段不会获得密文板。
:::
::: field name="collectible_mode_start_list" type="object" optional
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
:::
::: field name="use_foldartal" type="boolean" optional
是否使用密文板。模式 5 下默认值 `false`，其他模式下默认值 `true`。仅适用于 Sami 主题。
:::
::: field name="check_collapsal_paradigms" type="boolean" optional
是否检测获取的坍缩范式。模式 5 下默认值 `true`，其他模式下默认值 `false`。
:::
::: field name="double_check_collapsal_paradigms" type="boolean" optional default="true"
是否执行坍缩范式检测防漏措施。仅在主题为 Sami 且 `check_collapsal_paradigms` 为 true 时有效。模式 5 下默认值 `true`，其他模式下默认值 `false`。
:::
::: field name="expected_collapsal_paradigms" type="array<string>" optional default="['目空一些', '睁眼瞎', '图像损坏', '一抹黑']"
希望触发的坍缩范式。仅在主题为 Sami 且模式为 5 时有效。
:::
::: field name="monthly_squad_auto_iterate" type="boolean" optional
是否启动月度小队自动切换。
:::
::: field name="monthly_squad_check_comms" type="boolean" optional
是否将月度小队通信也作为切换依据。
:::
::: field name="deep_exploration_auto_iterate" type="boolean" optional
是否启动深入调查自动切换。
:::
::: field name="collectible_mode_shopping" type="boolean" optional default="false"
烧水是否启用购物。
:::
::: field name="collectible_mode_squad" type="string" optional
烧水时使用的分队, 默认与squad同步, 当squad为空字符串且未指定collectible_mode_squad值时为指挥分队。
:::
::: field name="start_with_seed" type="boolean" optional default="false"
使用种子刷钱。
<br>
仅在 Sarkaz 主题，Investment 模式，“点刺成锭分队” or “后勤分队” 时可能为 true。
<br>
使用固定种子。
:::
::::

刷坍缩范式功能具体请参考 [肉鸽辅助协议](./integrated-strategy-schema.md#萨米肉鸽——坍缩范式)

- `Copilot`  
    自动抄作业

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="filename" type="string" required
作业 JSON 的文件路径，绝对、相对路径均可。不支持运行期设置。
:::
::: field name="formation" type="boolean" optional default="false"
是否进行 “快捷编队”。不支持运行期设置。
:::
::::

作业 JSON 请参考 [战斗流程协议](./copilot-schema.md)

- `SSSCopilot`  
    自动抄保全作业

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="filename" type="string" required
作业 JSON 的文件路径，绝对、相对路径均可。不支持运行期设置。
:::
::: field name="loop_times" type="number" optional
循环执行次数。
:::
::::

保全作业 JSON 请参考 [保全派驻协议](./sss-schema.md)

- `Depot`  
    仓库识别

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::::

- `OperBox`  
    干员 box 识别

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::::

- `Reclamation`  
    生息演算

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="theme" type="string" optional default="Fire"
主题。
<br>
`Fire` - *沙中之火*
<br>
`Tales` - *沙洲遗闻*
:::
::: field name="mode" type="number" optional default="0"
模式。
<br>
`0` - 刷分与建造点，进入战斗直接退出。
<br>
`1` - 沙中之火：刷赤金，联络员买水后基地锻造；沙洲遗闻：自动制造物品并读档刷货币。
:::
::: field name="tools_to_craft" type="array<string>" optional default="['荧光棒']"
自动制造的物品，建议填写子串。
:::
::: field name="increment_mode" type="number" optional default="0"
点击类型。
<br>
`0` - 连点
<br>
`1` - 长按
:::
::: field name="num_craft_batches" type="number" optional default="16"
单次最大制造轮数。
:::
::::

- `Custom`  
    自定义任务

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="task_names" type="array<string>" required
执行数组中第一个匹配上的任务（及后续 next 等）。若想执行多个任务，可多次 append Custom task。
:::
::::

- `SingleStep`  
    单步任务（目前仅支持战斗）

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="type" type="string" required default="copilot"
目前仅支持 `"copilot"`。
:::
::: field name="subtask" type="string" required
子任务类型。
<br>
`stage` - 设置关卡名，需要 `"details": { "stage": "xxxx" }`。
<br>
`start` - 开始作战，无 `details`。
<br>
`action` - 单步作战操作，`details` 需为作战协议中的单个 action，例如：`"details": { "name": "史尔特尔", "location": [ 4, 5 ], "direction": "左" }`，详情参考 [战斗流程协议](./copilot-schema.md)。
:::
::: field name="details" type="object" optional
子任务的详细参数。
:::
::::

- `VideoRecognition`  
  视频识别，目前仅支持作业（作战）视频

:::: field-group
::: field name="enable" type="boolean" optional default="true"
是否启用本任务。
:::
::: field name="filename" type="string" required
视频的文件路径，绝对、相对路径均可。不支持运行期设置。
:::
::::

### `AsstSetTaskParams`

#### 接口原型

```cpp
bool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);
```

#### 接口说明

设置任务参数

#### 返回值

- `bool`  
    返回是否设置成功

#### 参数说明

:::: field-group
::: field name="handle" type="AsstHandle" required
实例句柄
:::
::: field name="task" type="AsstTaskId" required
任务 ID, `AsstAppendTask` 接口的返回值
:::
::: field name="params" type="const char*" required
任务参数，json string，与 `AsstAppendTask` 接口相同。
未标注“不支持运行中设置”的字段都支持实时修改；否则若当前任务正在运行，会忽略对应的字段
:::
::::

### `AsstSetStaticOption`

#### 接口原型

```cpp
bool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### 接口说明

设置进程级参数

#### 返回值

- `bool`  
    返回是否设置成功

#### 参数说明

:::: field-group
::: field name="key" type="AsstStaticOptionKey" required
键
:::
::: field name="value" type="const char*" required
值
:::
::::

##### 键值一览

暂无

### `AsstSetInstanceOption`

#### 接口原型

```cpp
bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### 接口说明

设置实例级参数

#### 返回值

- `bool`  
    返回是否设置成功

#### 参数说明

:::: field-group
::: field name="handle" type="AsstHandle" required
实例句柄
:::
::: field name="key" type="AsstInstanceOptionKey" required
键
:::
::: field name="value" type="const char*" required
值
:::
::::

##### 键值一览

```cpp
    enum InstanceOptionKey
    {
        Invalid = 0,
        // 已弃用 // MinitouchEnabled = 1,   // 是否启用 minitouch
                                // 开了也不代表就一定能用，有可能设备不支持等
                                // "1" 开，"0" 关
        TouchMode = 2,          // 触控模式设置，默认 minitouch
                                // minitouch | maatouch | adb
        DeploymentWithPause = 3,    // 是否暂停下干员，同时影响抄作业、肉鸽、保全
                                    // "1" | "0"
        AdbLiteEnabled = 4,     // 是否使用 AdbLite， "0" | "1"
        KillAdbOnExit = 5,       // 退出时是否杀掉 Adb 进程， "0" | "1"
    };
```
