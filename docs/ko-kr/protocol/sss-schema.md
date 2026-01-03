---
order: 7
icon: game-icons:prisoner
---

# 보안 파견 프로토콜

::: tip
주의: JSON 파일은 주석을 지원하지 않으므로, 텍스트 내의 주석은 예시용입니다. 직접 복사하여 사용하지 마세요.
:::

```json5
{
    "type": "SSS",                         // 프로토콜 유형, SSS는 보안파견을 의미. 필수, 변경 불가
    "stage_name": "多索雷斯在建地块",     // 보안 파견 맵 이름, 필수
    "minimum_required": "v4.9.0",          // MAA 최소 요구 버전, 필수
    "doc": {                               // 설명, 선택 사항
        "title": "저스펙 고승률 공략",
        "title_color": "dark",
        "details": "스펙 요구량 매우 낮음...", // 여기에 작성자명이나 참고 영상 링크 등을 적으면 좋습니다.
        "details_color": "dark"
    },
    "buff": "自适应补给元件",             // 시작 전술 장비 선택, 선택 사항
    "equipment": [                         // 시작 장비 선택, 가로 방향 순서, 선택 사항
                                           // 현재 버전 미구현, UI에만 표시됨
        "A", "A", "A", "A",
        "B", "B", "B", "B"
    ],
    "strategy": "优选策略",                // "우선 전략"(Optimized) 또는 "자유 전략"(Free), 선택 사항
                                           // 현재 버전 미구현, UI에만 표시됨
    "opers": [                             // 지정 오퍼레이터, 선택 사항
        {
            "name": "棘刺",
            "skill": 3,
            "skill_usage": 1
        }
    ],
    "tool_men": {                          // 필요한 직군별 나머지 인원 수, 코스트 순 정렬, 선택 사항
                                           // 현재 버전 미구현, UI에만 표시됨
        "Pioneer": 13,
        "Guards": 2,
        "Medic": 2
    },
    "drops": [                             // 전투 시작 및 도중 오퍼레이터/장비 획득 우선순위
        "空弦",
        "能天使",
        "先锋",
        "Support",
        "无需增调干员",             // 아무도 영입하지 않음
        "重整导能组件",                 // 장비명 지원
        "反制导能组件",
        "战备激活阀",                   // 층 중간 선택 장비도 여기에 포함
        "改派发讯器"
    ],
    "blacklist": [ // 블랙리스트, 선택 사항. 드랍 선택 시 제외됨.
                   // 추후 버전에서 편성 시에도 이 오퍼레이터들은 제외됨.
        "夜半",
        "梅尔"
    ],
    "stages": [
        {
            "stage_name": "蜂拥而上",           // 단일 층 스테이지명, 필수
                                           // 이름, stageId, levelId 지원.
                                           // 다른 보안 파견 맵과 겹칠 수 있는 코드(예: LT-1)는 사용 지양.
            "strategies": [                // 필수
                                           // 위에서부터 순서대로 조건을 확인하여 실행.
                                           // 완료된 전략은 건너뜀.
                {
                    "core": "棘刺",
                    "tool_men": {
                        "Pioneer": 1,      // 영문/중문 가능
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
                    // "core"가 없으면 패순환용 오퍼레이터(tool_men) 배치 용도로 사용 가능
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
            "draw_as_possible": true,   // "오퍼레이터 배치"(패순환) 버튼. 쿨 돌면 바로 누를지 여부. 선택 사항, 기본값 true.
            "actions": [                // 선택 사항
                                        // 자동지휘 로직 재사용
                                        // action 조건 만족 시 action 실행, 아니면 위 strategies 실행
                {
                    "type": "调配干员"   // 신규 타입. "오퍼레이터 배치" 버튼 클릭. "draw_as_possible"이 true면 무효.
                },
                {
                    "type": "CheckIfStartOver", // 신규 타입. 오퍼레이터 존재 확인. 없으면 재시도.
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
                    "type": "Retreat", // 철수
                    "name": "桃金娘"
                }
            ],
            "retry_times": 3 // 전투 실패 시 재시도 횟수. 초과 시 해당 판 포기.
        },
        {
            "stage_name": "人人为我"
            // ...
        }
        // 원하는 층수만큼 작성. 예를 들어 4층까지만 쓰면 4층 깨고 자동 재시도.
    ]
}
```

## 예시 파일

<https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/>
