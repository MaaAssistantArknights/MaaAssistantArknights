---
order: 7
icon: game-icons:prisoner
---


# 보안파견 스키마

::: tip
주의: JSON 형식은 주석을 지원하지 않으므로 아래의 예시에서 주석은 제거해주시기 바랍니다.
:::

```json
{
    "type": "SSS",                         // 프로토콜 유형, SSS는 보안파견 의미함, 필수, 변경 불가
    "stage_name": "多索雷斯在建地块",       // 구역 위치의 이름(도솔레스 건설부지), 필수
    "minimum_required": "v4.9.0",          // MAA의 최소 요구 버전, 필수
    "doc": {                               // 설명, 선택 사항
        "title": "低练度高成功率作业",
        "title_color": "dark",
        "details": "L对练度要求很低balabala……", // 여기에 작성자 명이나, 참고 영상등의 정보를 적는게 좋습니다!
        "details_color": "dark"
    },
    "buff": "自适应补给元件",               // 자유롭게 선택 가능합니다
    "equipment": [                         // 전술 장비 선택, 가로로 카운트, 선택 사항
                                           // 현재 버전에서는 구현되지 않으며, 인터페이스에만 표시됨
        "A", "A", "A", "A",
        "B", "B", "B", "B"
    ],
    "strategy": "优选策略",                 // "Optimized strategy"(최적화 전략) 또는 "Free strategy"(자유 전략), 선택 사항
                                           // 현재 버전에서는 구현되지 않으며, 인터페이스에만 표시됨
    "opers": [                             // 지정된 오퍼레이터, 선택 사항
        {
            "name": "棘刺",
            "skill": 3,
            "skill_usage": 1
        }
    ],
    "tool_men": {                          // 필요한 각 직의 남은 인원 수, 코스트별로 정렬, 선택 사항
                                           // 현재 버전에서는 구현되지 않으며, 인터페이스에만 표시됨
        "Pioneer": 13,
        "Guards": 2,
        "Medic": 2
    },
    "drops": [                             // 우선 순위로 플레이 시작 및 전투 중에 오퍼레이터 및 장비 획득
        "空弦",
        "能天使",
        "先锋",
        "Support",
        "无需增调干员",                     // 아무도 영입하지 않음
        "重整导能组件",                     // 장비 이름을 지원, 함께 씀
        "反制导能组件",
        "战备激活阀",                       // 레벨 중간의 선택적 장비도 여기에 포함
        "改派发讯器"
    ],
    "blacklist": [ // 블랙리스트, 선택 사항. 이들은 드롭에서 선택되지 않을 것입니다.
                   // 이후 버전에서는 배치 도구의 사람들도 이들을 선택하지 않습니다.
        "夜半",
        "梅尔"
    ],
    "stages": [
        {
            "stage_name": "蜂拥而上",       // 단일 레벨 스테이지 이름, 필수
                                           // 이름, stageId, levelId, 추천 stageId 또는 levelId를 지원합니다.
                                           // 기타 보존 스테이지와 충돌하는 코드 (예: LT-1)는 사용하지 말아주세요.


            "strategies": [                // 필수
                                         // 순서대로 각 객체의 tool_men에 따라 배치될 것입니다.
                                         // 현재 핸드에 tool_men이 없는 경우 다음 객체를 배치합니다.
                {
                    "core": "棘刺",
                    "tool_men": {
                        "Pioneer": 1,     // 영어 또는 중국어 이름 모두 허용됩니다.
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
                    // "core"를 제공하지 않으면, 통과를 보조하는 tool_men을 배치하는 데 사용될 수 있습니다.
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
            "draw_as_possible": true,   // "오퍼레이터 배치" 버튼. 가능할 때 사용할지 여부. 선택 사항, 기본값은 true입니다.
            "actions": [                // 선택 사항
                                        // 숙제 복사의 논리를 재사용합니다.
                                        // 조건이 해당 액션을 충족하면 액션을 실행합니다. 그렇지 않으면 위의 전략들이 실행됩니다.
                {
                    "type": "调配干员"   // 새로운 타입. "오퍼레이터 배치" 버튼을 클릭합니다. "draw_as_possible"가 true일 때는 무효화됩니다.
                },
                {
                    "type": "CheckIfStartOver", // 새로운 타입. 오퍼레이터가 사용 가능한지 확인합니다. 사용 불가능하면 종료하고 재시작합니다.
                    "name": "棘刺"
                },
                {
                    "name": "桃金娘",
                    "location": [
                        4,
                        5
                    ],
                    "direction": "Left"
                },
                {
                    "kills": 10,
                    "type": "撤退",
                    "name": "桃金娘"
                }
            ],
            "retry_times": 3 // 전투 실패 시 재시도 횟수. 이 제한을 초과하면 해당 스테이지를 포기합니다.
        },
        {
            "stage_name": "见者有份"
            // ...
        }
        // 하고 싶은만큼의 스테이지를 작성합니다. 예를 들어, 4개만 작성하면 스테이지 4개를 완료한 후 자동으로 재시작합니다.
    ]
}
```

## 예시

<https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/>
