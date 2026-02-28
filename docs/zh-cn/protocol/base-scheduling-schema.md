---
order: 6
icon: material-symbols:view-quilt-rounded
---

# 基建排班协议

`resource/custom_infrast/*.json` 的使用方法及各字段说明

::: tip
请注意 JSON 文件是不支持注释的，文本中的注释仅用于演示，请勿直接复制使用
:::

[可视化排班生成工具](https://ark.yituliu.cn/tools/schedule)

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
                // 取值范围 "control" / "manufacture" / "trading" / "power" / "meeting" / "hire" / "dormitory" / "processing"
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
                    "blacklist": ["Lancet-2"], // 将指定干员排除出当前房间autofill的范围，可选
                    // 若被指定的干员信赖未满，仍然可能会在填入信赖未满干员时被填入宿舍
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

[243 极限效率，一天三换](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/243_layout_3_times_a_day.json)

[153 极限效率，一天三换](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/custom_infrast/153_layout_3_times_a_day.json)
