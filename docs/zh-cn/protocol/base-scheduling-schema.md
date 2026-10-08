---
order: 6
icon: material-symbols:view-quilt-rounded
---

# 基建排班协议

`resource/custom_infrast/*.json` 的使用方法及各字段说明

::: tip
请注意 JSON 文件是不支持注释的，文本中的注释仅用于演示，请勿直接复制使用
:::

[可视化排班表生成工具](https://ark.yituliu.cn/tools/scheduleV2)

[可视化排班表自动生成工具](https://ark.yituliu.cn/tools/scheduleV3)

[自动生成基建排班表工具](https://riic.autos/)

## 完整字段一览

```json
{
    "title": "小号的换班方案", // 作业名，可选
    "description": "哈哈哈哈", // 作业描述，可选
    "plans": [
        {
            "name": "早班", // 计划名，可选
            "description": "lol", // 计划描述，可选
            "description_post": "", // 计划执行完时显示的描述，可选
            "period": [
                // 换班时间段，可选
                // 若当前时间在该区间内，则自动选择该计划（整个 json 文件中可能包含多个计划）
                // 如果该字段不存在，则每次换班结束后，自动切换为下一个计划
                // core 不处理该字段，若您使用接口集成 maa，请自行实现该逻辑
                [
                    "22:00", // 要求格式 hh:mm，目前只是简单的比较数字大小，如果要跨天请仿照该示例中写法
                    "23:59"
                ],
                [
                    "00:00",
                    "06:00"
                ]
            ],
            "duration": 360, // 工作持续时长（分钟），保留字段，目前无作用。以后可能到时间了弹窗提醒该换班了，或者直接自动换了
            "Fiammetta": {
                // “菲亚梅塔” 为哪位干员使用，可选，不填写则不使用
                "enable": true, // 是否使用“菲亚梅塔”，可选，默认 true
                "target": "巫恋", // 目标干员，使用 OCR 进行，需要传入对应客户端语言的干员名
                "order": "pre" // 在整个换班前使用，还是换完班才用，可选，取值范围 "pre" / "post"，默认 "pre"
            },
            "drones": {
                // 无人机使用，可选，不填写则不使用无人机
                "enable": true, // 是否使用无人机，可选，默认 true
                "room": "trading", // 为哪个类型房间使用，取值范围 "trading" / "manufacture"
                "index": 1, // 为第几个该类型房间使用，对应左边 tab 栏序号，取值范围 [1, 5]
                "rule": "all", // 使用规则，保留字段，目前无作用。以后可能拿来支持插拔等操作
                "order": "pre" // 在换干员前使用还是在换完才用，可选，取值范围 "pre" / "post"，默认 "pre"
            },
            "groups": [
                // 对于 "control" / "manufacture" / "trading"，可以设置干员编组
                {
                    "name": "古+银",
                    "operators": ["古米", "银灰", "梅"]
                },
                {
                    "name": "清流",
                    "operators": ["清流", "森蚺", "温蒂"]
                }
            ],
            "rooms": {
                // 房间信息，必选
                // 取值范围 "control" / "manufacture" / "trading" / "power" / "meeting" / "hire" / "dormitory" / "processing" / "training" / "recycling"
                // training / recycling 的 A/B 分组为草案，当前 MAA 自定义排班尚未支持
                // 缺少某个则该设施使用默认算法进行换班。
                // 若想不对某个房间换班请使用 skip 字段，或直接在软件 任务设置 - 基建换班 - 常规设置 中取消改设施的勾选
                "control": [
                {
                    "operators": [
                        "夕", // 使用 OCR 进行，需要传入对应客户端语言的干员名
                        "令",
                        "凯尔希",
                        "阿米娅",
                        "玛恩纳"
                    ]
                }
                ],
                "manufacture": [
                {
                    "operators": ["芬", "稀音", "克洛丝"],
                    "sort": false // 是否排序（按照上面 operators 的顺序），可选，默认 false
                    // 例子：当使用稀音、帕拉斯、巫恋、等干员且 "sort": false，干员顺序可能会被打乱，导致暖机效果丢失。
                    //     使用 "sort": true，可以避免这个问题
                },
                {
                    "skip": true // 是否跳过当前房间（数组序号对应），可选，默认 false
                    // 若为 true，其他字段均可为空。仅跳过换干员操作，其他如使用无人机、线索交流等仍会正常进行
                },
                {
                    "operators": ["Castle-3"],
                    "autofill": true, // 使用原先的算法，自动填充剩下的位置，可选，默认 false
                    // 若 operators 为空，则该房间完整的使用原先算法进行排班
                    // 若 operators 不为空，将仅考虑单干员效率，而不考虑整个组合效率
                    // 注意可能和后面自定义的干员产生冲突，比如把后面需要的干员拿到这里用了，请谨慎使用，或将 autofill 的房间顺序放到最后
                    "product": "Battle Record" // 当前制造产物，可选。
                    // 若识别到当前设施与作业中设置的产物不符合，界面会弹个红色字样提示，以后可能有更多作用
                    // 取值范围： "Battle Record" | "Pure Gold" |  "Dualchip" | "Originium Shard" | "LMD" | "Orundum"
                },
                {
                    "operators": ["多萝西"],
                    "candidates": [
                        // 备选干员，可选。这里面的有谁用谁，选满为止
                        // 与 autofill=true 不兼容，即该数组不为空时，autofill 需要为 false
                        "星源",
                        "白面鸮",
                        "赫默"
                    ]
                },
                {
                    "use_operator_groups": true, // 设置为 true 以使用 groups 中的干员编组，默认为 false
                    "operators": [
                        // 启用后, operators 中的名字将被解释为编组名
                        "古+银", // 将按照心情阈值以及设置顺序选择编组
                        "清流" // 如 古+银 组中有干员心情低于阈值，将使用 清流 组
                    ]
                }
                ],
                "meeting": [
                    {
                        "autofill": true // 这个房间内整个 autofill
                    }
                ],
                "training": [ // 训练室
                    {
                        // 人数上限、缺省与空数组含义及与原有房间字段、干员编组的关系待定
                        "operatorsA": ["阿米娅"], // 协助训练的干员名称数组（A 位），可选；名称要求同 operators
                        "operatorsB": ["杜宾"] // 接受训练的干员名称数组（B 位），可选；名称要求同 operators
                    }
                ],
                "recycling": [ // 回收站，键名暂定
                    {
                        "operatorsA": ["芬"], // 首个解锁位置的干员名称数组（A 位），可选；名称要求同 operators
                        "operatorsB": ["克洛丝"] // 第二个解锁位置的干员名称数组（B 位），可选；名称要求同 operators
                    }
                ]
            }
        },
        {
        "name": "晚班"
        // ...
        }
    ]
}
```

## 举例

[243 极限效率，一天三换](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master-v2/resource/custom_infrast/243_layout_3_times_a_day.json)

[153 极限效率，一天三换](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master-v2/resource/custom_infrast/153_layout_3_times_a_day.json)

## 排班表扩充协议

MAA 基础排班协议仅包括干员班次信息，排班表扩充协议在 MAA 排班协议的基础上，补充可选的基建布局 `layout`、干员信息 `operators` 和来源与版本信息 `metadata`，使 MAA、[**明日方舟一图流-排班表自动生成工具**](https://ark.yituliu.cn/tools/scheduleV3)、[**明日方舟一图流-排班表收益计算器**](https://ark.yituliu.cn/tools/maa-schedule-calculator)**等第三方应用可以**共用一份文件，实现数据双向互通。扩展字段均为可选内容，可根据第三方应用实际需求进行调整。

| 顶层字段    | 作用                                                                   |
| ----------- | ---------------------------------------------------------------------- |
| `layout`    | 记录基建布局，包括设施类型、等级和位置                                 |
| `operators` | 记录干员精英阶段和等级信息                                             |
| `metadata`  | 记录协议版本、生成工具及模块版本等数据，也包括第三方工具所需的额外数据 |

### 扩充字段一览

示例需与基础协议的 `plans` 等字段合并使用，练度及版本号仅用于演示；实际 JSON 需去掉注释。

```jsonc
{
    // 三个字段均为顶层可选字段，可分别提供，不影响 MAA 基础排班执行
    // 工具导入、编辑和导出时应保留扩充信息
    "layout": [ // 完整的已建造布局，可选；缺省表示未提供布局
        // 未列出的设施视为未建造；plans[].rooms 中省略的设施沿用基础协议默认规则
        // type：设施类型；level：设施等级；position：实际位置
        { "type": "control", "level": 5 },
        // 同类型房间顺序对应 plans[].rooms 中的顺序及无人机 index 编号
        { "type": "trading", "level": 3, "position": "B101" },
        { "type": "trading", "level": 3, "position": "B102" },
        { "type": "trading", "level": 1, "position": "B103" },
        { "type": "manufacture", "level": 3, "position": "B201" },
        { "type": "manufacture", "level": 3, "position": "B202" },
        { "type": "manufacture", "level": 2, "position": "B203" },
        { "type": "manufacture", "level": 2, "position": "B301" },
        { "type": "power", "level": 3, "position": "B302" },
        { "type": "power", "level": 3, "position": "B303" },
        { "type": "meeting", "level": 3 },
        { "type": "processing", "level": 3, "position": "B105" },
        { "type": "hire", "level": 3, "position": "B205" },
        { "type": "training", "level": 3, "position": "B305" },
        { "type": "dormitory", "level": 1, "position": "B104" },
        { "type": "dormitory", "level": 1, "position": "B204" },
        { "type": "dormitory", "level": 1, "position": "B304" },
        { "type": "dormitory", "level": 1, "position": "B404" }
    ],
    "operators": { // 干员练度，可选；以游戏干员 ID 为键
        // 不替代房间内的人员安排
        "char_002_amiya": {
            "name": "阿米娅", // 干员名称，可选
            "elite": 2, // 精英阶段：0、1、2
            "level": 80 // 当前精英阶段下的等级
        },
        "char_124_kroos": {
            "name": "克洛丝",
            "elite": 1,
            "level": 55
        }
    },
    "metadata": { // 来源与版本等附加信息，可选
        "extensionVersion": "1.0", // 整套扩充协议的版本字符串，可选；缺省按无版本格式处理
        "generator": { // 原始生成工具信息
            "id": "yituliu-riic-schedule-generator",
            "name": "明日方舟一图流-排班表自动生成工具",
            "url": "https://ark.yituliu.cn/tools/scheduleV3"
        },
        "moduleVersions": { // 生成时的模块版本；模块标识由生成工具定义，结合 generator.id 解读
            "layout": "v20260924.2314", // 布局模块
            "data": "v20260924.2314", // 数据模块
            "team": "v20260928.1118", // 班组模块
            "assembler": "v20260822.2233", // 组装器
            "yield": "v20261002.1334", // 收益计算器
            "recommendation": "v20260928.1102" // 调试信息
        }
    }
}
```
