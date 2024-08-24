---
order: 7
icon: game-icons:prisoner
---

# 保全派驻协议

::: tip
请注意 JSON 文件是不支持注释的，文本中的注释仅用于演示，请勿直接复制使用
:::

```json
{
    "type": "SSS", // 协议类型，SSS 表示保全派驻，必选，不可修改
    "stage_name": "多索雷斯在建地块", // 保全派驻地图名，必选
    "minimum_required": "v4.9.0", // 最低要求 maa 版本号，必选
    "doc": {
        // 描述，可选
        "title": "低练度高成功率作业",
        "title_color": "dark",
        "details": "对练度要求很低balabala……", // 建议在这里写上你的名字！（作者名）、参考的视频攻略链接等
        "details_color": "dark"
    },
    "buff": "自适应补给元件", // 开局导能元件选择，可选
    "equipment": [
        // 开局装备选择，横着数，可选
        // 当前版本暂未实现，只会在界面上显示一下
        "A",
        "A",
        "A",
        "A",
        "B",
        "B",
        "B",
        "B"
    ],
    "strategy": "优选策略", // 或者 自由策略，可选
    // 当前版本暂未实现，只会在界面上显示一下
    "opers": [
        // 指定干员，可选
        {
            "name": "棘刺",
            "skill": 3,
            "skill_usage": 1
        }
    ],
    "tool_men": {
        // 剩余所需各职业人数，按费用排序随便拿，可选
        // 当前版本暂未实现，只会在界面上显示一下
        "Pioneer": 13,
        "近卫": 2, // 中英文均可
        "Medic": 2
    },
    "drops": [
        // 战斗开始时和战斗中途，招募干员、获取装备优先级
        "空弦",
        "能天使", // 支持干员名、职业名
        "先锋", // 职业名中英文均可
        "Support",
        "无需增调干员", // 不招人
        "重整导能组件", // 支持装备名，全写一起.jpg
        "反制导能组件",
        "战备激活阀", // 关卡中途的可选装备，也也放这里
        "改派发讯器"
    ],
    "blacklist": [
        // 黑名单，可选。在 drops 里不会选这些人。
        // 后续版本支持编队后，编队工具人也不会选这些人
        "夜半",
        "梅尔"
    ],
    "stages": [
        {
            "stage_name": "蜂拥而上", // 单层关卡名，必选
            // 支持 name, stageId, levelId，推荐 stageId 或 levelId
            // 请勿使用 code（例如 LT-1），因为会和其他保全关卡冲突
            "strategies": [
                // 必选
                // 每次检查都会从头自上而下依次进行，并跳过执行完毕的策略
                // 若当前策略的工具人已经部署完毕
                //     若没有 core，则认为此策略执行完毕
                //     若有 core 且可以部署，则部署 core 并认为策略执行完毕
                //     若有 core 正在转费用，则等待并跳过后续策略
                // 若当前策略的工具人还未部署完毕
                //     若部署区没有所需工具人，则检查下一条策略
                //     若部署区存在所需工具人
                //         若没有能立即部署的，则等待
                //         若存在能立即部署的，则优先部署费用少的
                // 对于同一格的策略，
                //     若没有 core，则允许在待部署区没有所需工具人时（不论费用是否转好），允许继续检查同一格后续策略
                //     若有 core，则没有执行完毕时忽略同一格后续策略
                // 同一格可以写多个干员 core，非最靠后的干员 core 在其策略执行完毕后等效过牌（不计入工具人）
                // 同一格最靠后的干员 core 放置后若回到待部署区，则重置此格所有策略的执行情况，工具人也需要重新部署
                {
                    "core": "棘刺",
                    "tool_men": {
                        "Pioneer": 1, // 中英文均可
                        "Warrior": 1,
                        "Medic": 1
                    },
                    "location": [
                        10,
                        1
                    ],
                    "direction": "Left"
                },
                {
                    "core": "泥岩",
                    "tool_men": {
                        "Pioneer": 1,
                        "Warrior": 1,
                        "Medic": 1
                    },
                    "location": [
                        2,
                        8
                    ],
                    "direction": "Left"
                },
                {
                    // 不填写 core，可以用于部署辅助过牌的之类的
                    "tool_men": {
                        "Support": 100
                    },
                    "location": [
                        2,
                        8
                    ],
                    "direction": "Left"
                }
            ],
            "draw_as_possible": true, // “调配干员”按钮，是否好了就用，可选，默认 true
            "actions": [
                // 可选
                // 基本复用抄作业的逻辑，可参考 protocol/copilot-schema.md
                // 符合 action 的条件就执行 action，否则执行上面的 strategies 的逻辑
                {
                    "type": "调配干员" // 新 type，“调配干员” 按钮，点一下，在 "draw_as_possible" 为 true 时无效
                },
                {
                    "type": "CheckIfStartOver", // 新 type，检查干员在不在，不在就退出重开
                    "name": "棘刺"
                },
                {
                    "name": "桃金娘",
                    "location": [
                        4,
                        5
                    ],
                    "direction": "左"
                },
                {
                    "kills": 10,
                    "type": "撤退",
                    "name": "桃金娘"
                }
            ],
            "retry_times": 3 // 战斗失败重试次数，超过了直接放弃整局
        },
        {
            "stage_name": "见者有份"
            // ...
        }
        // 写几关打几关，比如只写到了 4，则打完 4 自动重开
    ]
}
```

## 示例文件

<https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/>
