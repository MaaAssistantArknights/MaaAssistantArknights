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
    "stage_name": "多索雷斯在建地块",       // 보안 파견 맵 이름, 필수
    "minimum_required": "v4.9.0",          // MAA 최소 요구 버전, 필수
    "doc": {                               // 설명, 선택 사항
        "title": "低练度高成功率作业",
        "title_color": "dark",
        "details": "对练度要求很低balabala……", // 여기에 작성자명이나 참고 영상 링크 등을 적으면 좋습니다.
        "details_color": "dark"
    },
    "buff": "自适应补给元件",               // 시작 전술 장비 선택, 선택 사항
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
        "近卫": 2,                         // 중/영문 모두 가능
        "Medic": 2
    },
    "drops": [                             // 전투 시작 및 도중 오퍼레이터/장비 획득 우선순위
        "空弦",
        "能天使",                          // 오퍼레이터명, 직군명 지원
        "先锋",                            // 직군명은 중/영문 모두 가능
        "Support",
        "无需增调干员",                    // 아무도 영입하지 않음
        "重整导能组件",                    // 장비명 지원, 함께 작성 가능
        "反制导能组件",
        "战备激活阀",                      // 층 중간 선택 장비도 여기에 포함
        "改派发讯器"
    ],
    "blacklist": [                         // 블랙리스트, 선택 사항. drops에서 선택되지 않음.
                                           // 추후 버전에서 편성 시에도 이 오퍼레이터들은 제외됨.
        "夜半",
        "梅尔"
    ],
    "stages": [
        {
            "stage_name": "蜂拥而上",      // 단일 층 스테이지명, 필수
                                           // name, stageId, levelId 지원. stageId 또는 levelId 권장.
                                           // 다른 보안 파견 맵과 겹칠 수 있는 code(예: LT-1)는 사용하지 마세요.
            "strategies": [                // 필수
                // 매 확인마다 위에서부터 순서대로 진행하며, 실행 완료된 전략은 건너뜀.
                // 현재 전략의 tool_men 배치가 완료된 경우:
                //     core가 없으면 해당 전략 실행 완료로 간주
                //     core가 있고 배치 가능하면 core를 배치하고 전략 실행 완료로 간주
                //     core가 코스트 전환 중이면 대기하고 후속 전략 건너뜀
                // 현재 전략의 tool_men 배치가 아직 완료되지 않은 경우:
                //     대기 구역에 필요한 tool_men이 없으면 다음 전략 확인
                //     대기 구역에 필요한 tool_men이 있으면:
                //         즉시 배치 가능한 유닛이 없으면 대기
                //         즉시 배치 가능한 유닛이 있으면 코스트 적은 순으로 우선 배치
                // 같은 칸의 전략에 대해:
                //     core가 없으면 대기 구역에 필요 유닛 없을 때(코스트 무관) 같은 칸 후속 전략 계속 확인 허용
                //     core가 있으면 실행 완료 전까지 같은 칸 후속 전략 무시
                // 같은 칸에 여러 core 오퍼레이터를 작성 가능. 맨 마지막이 아닌 core 오퍼레이터는 전략 완료 후 패스와 동등하게 취급됨(tool_men 미포함)
                {
                    "core": "棘刺",
                    "tool_men": {
                        "Pioneer": 1,      // 중/영문 모두 가능
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
                    "direction": "左"
                },
                {
                    "kills": 10,
                    "type": "撤退",
                    "name": "桃金娘"
                }
            ],
            "retry_times": 3 // 전투 실패 시 재시도 횟수. 초과 시 해당 판 포기.
        },
        {
            "stage_name": "见者有份"
            // ...
        }
        // 원하는 층수만큼 작성. 예를 들어 4층까지만 쓰면 4층 깨고 자동 재시도.
    ]
}
```

## 예시 파일

<https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/resource/copilot/>
