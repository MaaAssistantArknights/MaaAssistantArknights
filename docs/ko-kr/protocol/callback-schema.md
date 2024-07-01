---
order: 2
icon: material-symbols:u-turn-left
---

# 콜백 메세지 프로토콜

::: info 정보
콜백 메시지는 버전 업데이트를 통해 빠르게 변경되고 있으며 이 문서는 오래된 것일 수 있습니다. 최신 콘텐츠는 [C# 통합 소스 코드](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/MaaWpfGui/Main/AsstProxy.cs)를 참조하세요.
:::

## 프로토타입

```cpp
typedef void(ASST_CALL* AsstCallback)(int msg, const char* details, void* custom_arg);
```

## 개요

- `int msg`  
  메시지 유형입니다.

  ```cpp
  enum class AsstMsg
  {
      /* 전역 정보 */
      InternalError = 0,          // 내부 오류
      InitFailed,                 // 초기화 실패
      ConnectionInfo,             // 연결 정보
      AllTasksCompleted,          // 모든 태스크가 완료되었는지 여부
      AsyncCallInfo,              // 비동기 호출 정보
      /* TaskChain 정보 */
      TaskChainError = 10000,     // 태스크 체인 실행/인식 오류
      TaskChainStart,             // 태스크 체인 시작
      TaskChainCompleted,         // 태스크 체인 완료
      TaskChainExtraInfo,         // 태스크 체인에 대한 추가 정보
      TaskChainStopped,           // 태스크 체인 중지
      /* SubTask 정보 */
      SubTaskError = 20000,       // 서브태스크 실행/인식 오류
      SubTaskStart,               // 서브태스크 시작
      SubTaskCompleted,           // 서브태스크 완료
      SubTaskExtraInfo,           // 서브태스크에 대한 추가 정보
      SubTaskStopped,             // 서브태스크 중지
  };
  ```

- `const char* details`  
  메시지 세부사항, JSON 형식입니다. 또한 [필드 설명](#필드-설명)도 참조하세요.
- `void* custom_arg`  
  호출자의 사용자 정의 인수로, `AsstCreateEx` 인터페이스의 `custom_arg` 인수를 전달합니다. C 언어 기반 언어에서는 이를 통해 `this` 포인터를 전달할 수 있습니다.

## 필드 설명

### InternalError

할일

### InitFailed

```json
{
    "what": string,     // 오류 유형
    "why": string,      // 오류 원인
    "details": object   // 오류 세부사항
}
```

### ConnectionInfo

```json
{
    "what": string,     // 정보 유형
    "why": string,      // 정보 원인
    "uuid": string,     // UUID (연결 실패 시 빈 문자열)
    "details": {
        "adb": string,     // AsstConnect 인터페이스의 adb_path 인수
        "address": string, // AsstConnect 인터페이스의 address 인수
        "config": string   // AsstConnect 인터페이스의 config 인수
    }
}
```

### 자주 사용되는 `what` 필드 값

- `ConnectFailed`  
  연결 실패.
- `Connected`  
  연결됨. 이제 `uuid` 필드는 비어 있습니다 (다음 단계에서 가져올 것입니다).
- `UuidGot`  
  UUID를 가져왔습니다.
- `UnsupportedResolution`  
  해상도가 지원되지 않습니다.
- `ResolutionError`  
  해상도를 가져올 수 없습니다.
- `Reconnecting`  
  연결이 끊어졌습니다 (adb/에뮬레이터 충돌), 재연결 중
- `Reconnected`  
  연결이 끊어졌습니다 (adb/에뮬레이터 충돌), 재연결 성공
- `Disconnect`  
  연결이 끊어졌습니다 (adb/에뮬레이터 충돌), 재연결 실패
- `ScreencapFailed`  
  스크린샷 실패 (adb/에뮬레이터 충돌), 재연결 실패
- `TouchModeNotAvailable`  
  터치 모드를 사용할 수 없습니다

### AsyncCallInfo

```json
{
    "uuid": string,             // UUID
    "what": string,             // 호출 유형, "Connect" | "Click" | "Screencap" | ...
    "async_call_id": int,       // 비동기 호출 ID, AsstAsyncXXX API를 호출한 결과 값입니다.
    "details": {
        "ret": bool,            // 결과
        "cost": int64,          // 경과 시간 (밀리초)
    }
}
```

### AllTasksCompleted

```json
{
    "taskchain": string,            // 마지막 태스크 체인
    "uuid": string,                 // UUID
    "finished_tasks": [             // 실행된 마지막 태스크의 ID 목록
        int,
        ...
    ]
}
```

#### 자주 사용되는 `taskchain` 필드 값

- `StartUp`  
  시작.
- `CloseDown`  
  게임 클라이언트 닫기
- `Fight`  
  전투.
- `Mall`  
  상점.
- `Recruit`  
  자동 인원 모집.
- `Infrast`  
  시설.
- `Award`  
  일일 보상 받기
- `Roguelike`  
  로그라이크.
- `Copilot`  
  JSON 파일로 자동 전투
- `SSSCopilot`  
  JSON 파일로 자동 전투 for STATIONARY SECURITY SERVICE
- `Depot`  
  창고 인식
- `OperBox`  
  오퍼레이터 박스 인식
- `ReclamationAlgorithm`  
  Reclamation Algorithm (CN 클라이언트의 새로운 모드)
- `Custom`  
  사용자 정의 태스크
- `SingleStep`  
  단일 단계 태스크
- `VideoRecognition`  
  Copilot용 비디오 인식
- `Debug`  
  디버그.

### TaskChain과 관련된 정보

```json
{
    "taskchain": string,            // 현재 Task Chain
    "taskid": int,                  // 현재 Task ID
    "uuid": string                  // UUID
}
```

### TaskChainExtraInfo

할 일

### SubTask와 관련된 정보

```json
{
    "subtask": string,             // 서브태스크 이름
    "class": string,               // 서브태스크 클래스
    "taskchain": string,           // 현재 Task Chain
    "taskid": int,                 // 현재 Task ID
    "details": object,             // 상세 정보
    "uuid": string                 // UUID
}
```

#### 자주 사용되는 `subtask` 필드 값

- `ProcessTask`  

  ```json
  // 상세 정보에 대한 예시
  {
      "task": "StartButton2",     // 태스크 이름
      "action": 512,
      "exec_times": 1,            // 실행 횟수
      "max_times": 999,           // 최대 실행 횟수
      "algorithm": 0
  }
  ```

- 기타 할 일

##### 자주 사용되는 `task` 필드 값

- `StartButton2`  
  시작 중입니다.
- `MedicineConfirm`  
  이성 회복제 사용 확인.
- `ExpiringMedicineConfirm`  
  만료가 임박한 이성 회복 아이템을 사용 확인.
- `StoneConfirm`  
  오리지늄 조각 사용 확인.
- `RecruitRefreshConfirm`  
  공개모집 새로고침 여부 확인.
- `RecruitConfirm`  
  공개모집 확인.
- `RecruitNowConfirm`  
  즉시 완료 허가증 확인.
- `ReportToPenguinStats`  
  펭귄 스텟에 보고합니다.
- `ReportToYituliu`  
  Yituliu 빅데이터에 보고합니다.
- `InfrastDormDoubleConfirmButton`  
  기반시설 더블 확인 버튼입니다. 다른 오퍼레이터와 충돌이 있을 때만 발생합니다.
- `StartExplore`  
  통합전략: 시작 중입니다.
- `StageTraderInvestConfirm`  
  통합전략: 오리지늄 각뿔 아이템 거래.
- `StageTraderInvestSystemFull`  
  통합전략: 투자 시스템이 가득 참.
- `ExitThenAbandon`  
  통합전략: 나가기 확인.
- `MissionCompletedFlag`  
  통합전략: 미션 완료.
- `MissionFailedFlag`  
  통합전략: 미션 실패.
- `StageTraderEnter`  
  통합전략: 교활한 상인 진입.
- `StageSafeHouseEnter`  
  통합전략: 안전가옥 진입.
- `StageEncounterEnter`  
  통합전략: 우연한 만남 진입.
- `StageCombatDpsEnter`  
  통합전략: 작전 진입.
- `StageEmergencyDps`  
  통합전략: 긴급 작전 진입.
- `StageDreadfulFoe`  
  통합전략: 보스 진입.
- `StartGameTask`  
  클라이언트를 시작할 수 없음 (클라이언트 유형과 호환되지 않는 구성 파일).
- 기타 할 일

### SubTaskExtraInfo

```json
{
    "taskchain": string,           // 현재 Task Chain
    "class": string,               // 서브태스크 클래스
    "what": string,                // 정보 유형
    "details": object,             // 정보 상세 내용
    "uuid": string,                // UUID
}
```

#### 자주 사용되는 `what`과 `details` 필드 값

- `StageDrops`  
  스테이지 드롭 정보

  ```json
  // 해당 details 필드 예시
  {
    "drops": [
      // 인식된 드롭 아이템
      {
        "itemId": "3301",
        "quantity": 2,
        "itemName": "技巧概要·卷1"
      },
      {
        "itemId": "3302",
        "quantity": 1,
        "itemName": "技巧概要·卷2"
      },
      {
        "itemId": "3303",
        "quantity": 2,
        "itemName": "技巧概要·卷3"
      }
    ],
    "stage": {
      // 스테이지 정보
      "stageCode": "CA-5",
      "stageId": "wk_fly_5"
    },
    "stars": 3, // 행동 종료 시 클리어 별 개수
    "stats": [
      // 해당 실행 기간 동안의 전체 드롭 아이템
      {
        "itemId": "3301",
        "itemName": "技巧概要·卷1",
        "quantity": 4,
        "addQuantity": 2 //이번에 추가된 드롭 수량
      },
      {
        "itemId": "3302",
        "itemName": "技巧概要·卷2",
        "quantity": 3,
        "addQuantity": 1
      },
      {
        "itemId": "3303",
        "itemName": "技巧概要·卷3",
        "quantity": 4,
        "addQuantity": 2
      }
    ]
  }

  ```

- `RecruitTagsDetected`  
  공개모집 태그 감지

  ```json
  // 상세 정보에 대한 예시
  {
      "tags": [
          "费用回复",
          "防护",
          "先锋干员",
          "辅助干员",
          "近战位"
      ]
  }
  ```

- `RecruitSpecialTag`  
  고급 특별 채용 감지

  ```json
  // 상세 정보에 대한 예시
  {
     "tag": "高级资深干员" // 고급 특별 채용
  }
  ```

- `RecruitResult`  
  공개모집 결과

  ```json
  // 상세 정보에 대한 예시
  {
      "tags": [                   // 모든 태그들, 반드시 5개여야 함
          "削弱",
          "减速",
          "术师干员",
          "辅助干员",
          "近战位"
      ],
      "level": 4,                 // ★ 총합
      "result": [
          {
              "tags": [
                  "削弱"
              ],
              "level": 4, // 태그의 최소 레어도
              "opers": [
                  {
                      "name": "初雪",
                      "level": 5  // 오퍼레이터의 레어도
                  },
                  {
                      "name": "陨星",
                      "level": 5
                  },
                  {
                      "name": "槐琥",
                      "level": 5
                  },
                  {
                      "name": "夜烟",
                      "level": 4
                  },
                  {
                      "name": "流星",
                      "level": 4
                  }
              ]
          },
          {
              "tags": [
                  "减速",
                  "术师干员"
              ],
              "level": 4,
              "opers": [
                  {
                      "name": "夜魔",
                      "level": 5
                  },
                  {
                      "name": "格雷伊",
                      "level": 4
                  }
              ]
          },
          {
              "tags": [
                  "削弱",
                  "术师干员"
              ],
              "level": 4,
              "opers": [
                  {
                      "name": "夜烟",
                      "level": 4
                  }
              ]
          }
      ]
  }
  ```

- `RecruitTagsRefreshed`  
  공개모집 태그 갱신 완료

  ```json
  // 상세 정보에 대한 예시
  {
      "count": 1,               // 갱신된 슬롯의 횟수
      "refresh_limit": 3        // 갱신 횟수 제한
  }
  ```

- `RecruitNoPermit`  
    모집 라이센스가 없습니다

    ```json
    // 상세 정보에 대한 예시
    {
        "continue": true,   // 계속 새로고침할지 말지
    }
    ```

- `RecruitTagsSelected`  
  공개모집 태그 선택 완료

  ```json
  // 상세 정보에 대한 예시
  {
      "tags": [
          "减速",
          "术师干员"
      ]
  }
  ```

- `RecruitSlotCompleted`  
  모집 슬롯 완료
- `RecruitError`  
  모집 인식 오류
- `EnterFacility`  
  기반시설 입장

  ```json
  // 상세 정보에 대한 예시
  {
      "facility": "Mfg",  // 시설 이름
      "index": 0          // 시설 ID
  }
  ```

- `NotEnoughStaff`  
  사용 가능한 오퍼레이터가 부족함

  ```json
  // 상세 정보에 대한 예시
  {
      "facility": "Mfg",  // 시설 이름
      "index": 0          // 시설 ID
  }
  ```

- `ProductOfFacility`  
  시설의 생산물

  ```json
  // 상세 정보에 대한 예시
  {
      "product": "Money", // 생산물
      "facility": "Mfg",  // 시설 이름
      "index": 0          // 시설 ID
  }
  ```

- `StageInfo`  
  자동 전투 스테이지 정보

  ```json
  // 상세 정보에 대한 예시
  {
      "name": string  // 스테이지 이름
  }
  ```

- `StageInfoError`  
  자동 전투 스테이지 정보 오류
- `PenguinId`  
  PenguinStats ID

  ```json
  // 상세 정보에 대한 예시
  {
      "id": string
  }
  ```

- `DepotInfo`  
  창고 인식 결과

  ```json
  // 상세 정보에 대한 예시
  // ArkPlanner 포맷만 지원합니다. 더 많은 포맷이 나중에 지원될 수 있습니다.
  "arkplanner": {
      "object": {
          "items": [
              {
                  "id": "2004",
                  "have": 4,
                  "name": "高级作战记录"
              },
              {
                  "id": "mod_unlock_token",
                  "have": 25,
                  "name": "模组数据块"
              },
              {
                  "id": "2003",
                  "have": 20,
                  "name": "中级作战记录"
              }
          ],
          "@type": "@penguin-statistics/depot"
      },
      "data": "{\"@type\":\"@penguin-statistics/depot\",\"items\":[{\"id\":\"2004\",\"have\":4,\"name\":\"高级作战记录\"},{\"id\":\"mod_unlock_token\",\"have\":25,\"name\":\"模组数据块\"},{\"id\":\"2003\",\"have\":20,\"name\":\"中级作战记录\"}]}"
  },
  "lolicon": {     // https://arkntools.app/#/material
      "object": {
          "2004" : 4,
          "mod_unlock_token": 25,
          "2003": 20


      },
      "data": "{\"2003\":20,\"2004\": 4,\"mod_unlock_token\": 25}"
  }
  // 현재는 ArkPlanner와 Lolicon (Arkntools) 포맷만 지원하며, 미래에 더 많은 웹사이트가 지원될 예정입니다.
  ```

- `OperBoxInfo`  
  오퍼레이터 보유 목록 인식 결과

  ```json
  // 상세 정보에 대한 예시
  "done": bool,       // 인식 완료 여부, false는 아직 진행 중임을 의미합니다 (진행 중인 데이터)
  "all_oper": [
      {
          "id": "char_002_amiya",
          "name": "阿米娅",
          "own": true
      },
      {
          "id": "char_003_kalts",
          "name": "凯尔希",
          "own": true
      },
      {
          "id": "char_1020_reed2",
          "name": "焰影苇草",
          "own": false
      },
  ]
  "own_opers": [
      {
          "id": "char_002_amiya", // 오퍼레이터 ID
          "name": "阿米娅", // 오퍼레이터 이름
          "own": true, // 보유 여부
          "elite": 2, // 정예화 단계 0, 1, 2
          "level": 50, // 오퍼레이터 레벨
          "potential": 6, // 오퍼레이터 잠재능력 [1, 6]
          "rarity": 5     // [1, 6]
      },
      {
          "id": "char_003_kalts",
          "name": "凯尔希",
          "own": true,
          "elite": 2,
          "level": 50,
          "potential": 1,
          "rarity": 6
      }
  ]
  ```

- `UnsupportedLevel`  
  지원되지 않는 레벨 이름
