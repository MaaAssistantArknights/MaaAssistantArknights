---
order: 2
icon: material-symbols:u-turn-left
---

# 콜백 메시지 프로토콜

::: info 정보
콜백 메시지는 버전 업데이트를 통해 빠르게 변경되고 있으며 이 문서는 오래된 것일 수 있습니다. 최신 콘텐츠는 [C# 통합 소스 코드](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/src/MaaWpfGui/Main/AsstProxy.cs)를 참조하세요.
:::

## 콜백 함수 프로토타입

```cpp
typedef void(ASST_CALL* AsstApiCallback)(AsstMsgId msg, const char* details_json, void* custom_arg);
```

여기서 `AsstMsgId`는 `int32_t`의 별칭입니다.

## 파라미터 개요

- `AsstMsgId msg`  
  메시지 유형

  ```cpp
  enum class AsstMsg
  {
      /* Global Info */
      InternalError     = 0,           // 내부 오류
      InitFailed        = 1,           // 초기화 실패
      ConnectionInfo    = 2,           // 연결 정보
      AllTasksCompleted = 3,           // 모든 작업 완료
      AsyncCallInfo     = 4,           // 비동기 호출 정보
      Destroyed         = 5,           // 인스턴스 소멸됨

      /* TaskChain Info */
      TaskChainError     = 10000,      // 작업 체인 실행/인식 오류
      TaskChainStart     = 10001,      // 작업 체인 시작
      TaskChainCompleted = 10002,      // 작업 체인 완료
      TaskChainExtraInfo = 10003,      // 작업 체인 추가 정보
      TaskChainStopped   = 10004,      // 작업 체인 수동 중지

      /* SubTask Info */
      SubTaskError      = 20000,       // 서브 작업 실행/인식 오류
      SubTaskStart      = 20001,       // 서브 작업 시작
      SubTaskCompleted  = 20002,       // 서브 작업 완료
      SubTaskExtraInfo  = 20003,       // 서브 작업 추가 정보
      SubTaskStopped    = 20004,       // 서브 작업 수동 중지

      /* Web Request */
      ReportRequest     = 30000,       // 보고 요청
  };
  ```

- `const char* details`  
  메시지 세부사항, JSON 문자열, [필드 설명](#필드-설명) 참조
- `void* custom_arg`  
  호출자의 사용자 정의 인수로, `AsstCreateEx` 인터페이스의 `custom_arg` 인수가 그대로 전달됩니다. C 계열 언어에서는 이를 통해 `this` 포인터를 전달할 수 있습니다.

## 필드 설명

### InternalError

`details` 필드는 비어 있습니다.

### InitFailed

:::: field-group
::: field what
@type string
@required
오류 유형
:::  
::: field why
@type string
@required
오류 원인
:::  
::: field details
@type object
@required
오류 상세
:::  
::::

### ConnectionInfo

:::: field-group
::: field what
@type string
@required
정보 유형
:::
::: field why
@type string
@required
정보 원인
:::
::: field uuid
@type string
장치 고유 코드(UUID) (연결 실패 시 비어 있음)
:::
::: field details
@type object
@required
연결 상세 정보. 구조는 다음과 같습니다:

- `adb` (string, required): `AsstConnect` 인터페이스의 `adb_path` 파라미터.
- `address` (string, required): `AsstConnect` 인터페이스의 `address` 파라미터.
- `config` (string, required): `AsstConnect` 인터페이스의 `config` 파라미터.

:::
::::

### 자주 사용되는 `What` 필드

- `ConnectFailed`  
  연결 실패
- `Connected`  
  연결됨, 이때 `uuid` 필드는 비어 있음 (다음 단계에서 획득)
- `UuidGot`  
  장치 고유 코드(UUID) 획득
- `UnsupportedResolution`  
  지원하지 않는 해상도
- `ResolutionError`  
  해상도 획득 오류
- `Reconnecting`  
  연결 끊김 (adb / 에뮬레이터 충돌), 재연결 중
- `Reconnected`  
  연결 끊김 (adb / 에뮬레이터 충돌), 재연결 성공
- `Disconnect`  
  연결 끊김 (adb / 에뮬레이터 충돌), 재시도 실패
- `ScreencapFailed`  
  스크린샷 실패 (adb / 에뮬레이터 충돌), 재시도 실패
- `TouchModeNotAvailable`  
  지원하지 않는 터치 모드
- `MuMuExtrasInputStatus`  
  MuMu 터치 강화의 실제 적용 상태, `details` 구조:
  - `available` (boolean, required): 적용되었는지 여부.
  - `deferred` (boolean, required): 아직 판정되지 않았는지 여부 (연결 시 게임이 렌더링되지 않아 렌더링 시작 후 자동으로 다시 판정됨).
- `ResolutionGot`  
  해상도를 획득함
- `ResolutionChanged`  
  실행 중 해상도가 변경되어 연결이 무효화되고 현재 작업이 중단됨, `details` 구조:
  - `width` (number, required): 현재 너비.
  - `height` (number, required): 현재 높이.
- `FastestWayToScreencap`  
  가장 빠른 스크린샷 방식을 찾음, `details` 구조:
  - `method` (string, required): 가장 빠른 스크린샷 방식.
  - `cost` (number, required): 소요 시간 (밀리초).
  - `alternatives` (array`<object>`, required): 모든 후보 방식과 소요 시간.

- `ScreencapCost`  
  스크린샷 소요 시간 통계 (10회마다 보고), `details` 구조:
  - `min` (number, required): 최소 소요 시간 (밀리초).
  - `max` (number, required): 최대 소요 시간 (밀리초).
  - `avg` (number, required): 평균 소요 시간 (밀리초).
  - `fault_times` (number): 실패 횟수 (실패가 있는 경우에만 존재).

- `EmulatorFPS`  
  에뮬레이터 주사율 (1분마다 검사), `details` 구조:
  - `fps` (number, required): 에뮬레이터/시스템 주사율 (FPS).
  - `refresh_period_ns` (number, required): 프레임 주기 (나노초).

### AsyncCallInfo

:::: field-group
::: field uuid
@type string
@required
장치 고유 코드
:::
::: field what
@type string
@required
콜백 유형, 예: `Connect` | `AttachWindow` | `Click` | `Screencap` 등
:::
::: field async_call_id
@type number
@required
비동기 요청 id, 즉 `AsstAsyncXXX` 호출 시 반환값
:::
::: field details
@type object
@required
비동기 호출 상세. 구조는 다음과 같습니다:

- `ret` (boolean, required): 실제 호출의 반환값.
- `cost` (number, required): 소요 시간, 단위 밀리초.
- `error` (string): 처리되지 않은 예외의 유형 (처리되지 않은 예외로 호출이 실패한 경우에만 존재).

:::
::::

### AllTasksCompleted

:::: field-group
::: field taskchain
@type string
@required
마지막 작업 체인
:::
::: field uuid
@type string
@required
장치 고유 코드
:::
::: field finished_tasks
@type array<number>
@required
이미 실행된 작업 id 목록
:::
::::

#### 자주 사용되는 `taskchain` 필드

- `StartUp`  
  시작 (깨우기)
- `CloseDown`  
  게임 종료
- `Fight`  
  이성 소모 작전
- `Mall`  
  크레딧 상점 및 쇼핑
- `Recruit`  
  자동 공개모집
- `Infrast`  
  기반시설 교대
- `Award`  
  일일 보상 수령
- `Roguelike`  
  통합 전략 반복
- `Copilot`  
  자동지휘
- `SSSCopilot`  
  보안 파견 자동지휘
- `Depot`  
  창고 인식
- `OperBox`  
  오퍼레이터 인식
- `Reclamation`  
  생존 연산
- `Custom`  
  사용자 정의 작업
- `SingleStep`  
  단일 단계 작업
- `VideoRecognition`  
  비디오 인식 작업
- `Debug`  
  디버그

### TaskChain 관련 메시지

:::: field-group
::: field taskchain
@type string
@required
현재 작업 체인
:::
::: field taskid
@type number
@required
현재 작업 TaskId
:::
::: field uuid
@type string
@required
장치 고유 코드
:::
::::

### TaskChainExtraInfo

기본적으로 위의 공통 필드만 포함됩니다. 통합 전략(쉐이 테마)에서 경로 계획 결과 피할 수 없는 전투가 너무 많다고 판단되어 능동적으로 재시작하는 경우, 메시지에 추가로 다음이 포함됩니다:

- `what` (string, required): `RoutingRestart`로 고정.
- `why` (string, required): `TooManyBattlesAhead`로 고정.
- `node_cost` (number, required): 계획된 다음 노드의 비용.

### SubTask 관련 메시지

:::: field-group
::: field subtask
@type string
@required
서브 작업명
:::
::: field class
@type string
@required
서브 작업 심볼명
:::
::: field taskchain
@type string
@required
현재 작업 체인
:::
::: field taskid
@type number
@required
현재 작업 TaskId
:::
::: field details
@type object
@required
상세 정보
:::
::: field uuid
@type string
@required
장치 고유 코드
:::
::::

#### 자주 사용되는 `subtask` 필드

- `ProcessTask`  
  `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field task
  @type string
  @required
  작업명
  :::
  ::: field action
  @type string
  @required
  동작 이름, 예: `ClickSelf` | `DoNothing` | `Swipe`
  :::
  ::: field exec_times
  @type number
  @required
  실행된 횟수
  :::
  ::: field max_times
  @type number
  @required
  최대 실행 횟수
  :::
  ::: field algorithm
  @type string
  @required
  인식 알고리즘 이름, 예: `MatchTemplate` | `OcrDetect` | `FeatureMatch` | `JustReturn`
  :::
  ::::

- Todo 기타

##### 자주 사용되는 `task` 필드

- `StartButton2`  
  작전 시작
- `MedicineConfirm`  
  이성 회복제 사용 확인
- `StoneConfirm`  
  오리지늄 사용 확인
- `RecruitRefreshConfirm`  
  공개모집 태그 갱신 확인
- `RecruitConfirm`  
  공개모집 채용 확인
- `RecruitNowConfirm`  
  공개모집 즉시 완료 허가증 사용 확인
- `InfrastDormDoubleConfirmButton`  
  기반시설의 2차 확인 버튼, 배치할 오퍼레이터가 이미 다른 시설에 근무 중인 경우에만 나타남(MAA가 자동으로 클릭), 사용자에게 알림 필요
- `StartExplore`  
  통합 전략 탐험 시작
- `StageTraderInvestConfirm`  
  통합 전략 각뿔 투자 확인
- `StageTraderInvestSystemFull`  
  통합 전략 투자 한도 도달
- `ExitThenAbandon`  
  통합 전략 이번 탐험 포기
- `MissionCompletedFlag`  
  통합 전략 전투 완료
- `MissionFailedFlag`  
  통합 전략 전투 실패
- `StageTraderEnter`  
  통합 전략 노드: 교활한 상인
- `StageSafeHouseEnter`  
  통합 전략 노드: 안전가옥
- `StageEncounterEnter`  
  통합 전략 노드: 우연한 만남/고성의 선물
- `StageCombatOpsEnter`  
  통합 전략 노드: 일반 작전
- `StageEmergencyOps`  
  통합 전략 노드: 긴급 작전
- `StageDreadfulFoe`  
  통합 전략 노드: 험난한 길
- Todo 기타

### SubTaskExtraInfo

:::: field-group
::: field taskchain
@type string
@required
현재 작업 체인
:::
::: field class
@type string
@required
서브 작업 유형
:::
::: field what
@type string
@required
정보 유형
:::
::: field details
@type object
@required
정보 상세
:::
::: field uuid
@type string
@required
장치 고유 코드
:::
::::

#### 자주 사용되는 `what` 및 `details` 필드

- `StageDrops`  
  노드 재료 드롭 정보. `details` 필드 구조는 다음과 같습니다:
  - `drops` (array, required): 이번에 식별된 드롭 재료, 배열의 각 항목:
    - `itemId` (string, required): 재료 ID
    - `quantity` (number, required): 드롭 수량
    - `itemName` (string, required): 재료 명칭
  - `stage` (object, required): 노드 정보:
    - `stageCode` (string, required): 노드 코드
    - `stageId` (string, required): 노드 ID
  - `stars` (number, required): 작전 종료 등급(별)
  - `stats` (array, required): 이번 실행 기간 동안의 총 재료 드롭, 배열의 각 항목:
    - `itemId` (string, required): 재료 ID
    - `itemName` (string, required): 재료 명칭
    - `quantity` (number, required): 총 수량
    - `addQuantity` (number, required): 이번에 추가된 드롭 수량

- `RecruitTagsDetected`  
  공개모집 태그를 식별했습니다. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field tags
  @type array<string>
  @required
  식별된 태그 목록
  :::
  ::::

- `RecruitSpecialTag`  
  공개모집 특수 태그를 식별했습니다. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field tag
  @type string
  @required
  특수 태그 명칭, 예: `고급 특별 채용`
  :::
  ::::

- `RecruitPreservedTag`  
  공개모집에서 보류 대상으로 설정된 태그를 식별했습니다. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field tag
  @type string
  @required
  보류를 트리거한 태그 명칭, 예: `지원 기계`
  :::
  ::::

- `RecruitResult`  
  공개모집 식별 결과. `details` 필드 구조는 다음과 같습니다:
  - `tags` (array, required): 식별된 모든 태그, 현재 5개로 고정
  - `level` (number, required): 조합의 최고 등급(별)
  - `result` (array, required): 구체적인 조합 결과, 배열의 각 항목:
    - `tags` (array, required): 조합에 참여한 태그
    - `level` (number, required): 이 태그 조합의 등급
    - `opers` (array, required): 모집 가능한 오퍼레이터, 배열의 각 항목:
      - `name` (string, required): 오퍼레이터 명칭
      - `level` (number, required): 오퍼레이터 등급

- `RecruitTagsRefreshed`  
  공개모집 태그를 갱신했습니다. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field count
  @type number
  @required
  현재 슬롯 갱신 횟수
  :::
  ::: field refresh_limit
  @type number
  @required
  현재 슬롯 갱신 횟수 상한
  :::
  ::::

- `RecruitNoPermit`  
  모집 허가증이 없습니다. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field continue
  @type boolean
  @required
  계속 갱신할지 여부
  :::
  ::::

- `RecruitTagsSelected`  
  공개모집 태그를 선택했습니다. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field tags
  @type array<string>
  @required
  선택한 태그 목록
  :::
  ::::

- `RecruitError`  
  공개모집 식별 오류. `details` 필드는 비어 있으며, 현재 슬롯의 갱신 횟수가 상한에 도달하여 발생한 경우 `refresh_limit`(슬롯 갱신 횟수 상한)가 포함됩니다

- `EnterFacility`  
  기반시설 시설에 진입했습니다. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field facility
  @type string
  @required
  시설명
  :::
  ::: field index
  @type number
  @required
  시설 순서(인덱스)
  :::
  ::::

- `NotEnoughStaff`  
  기반시설 가용 오퍼레이터 부족. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field facility
  @type string
  @required
  시설명
  :::
  ::: field index
  @type number
  @required
  시설 순서
  :::
  ::::

- `ProductOfFacility`  
  기반시설 생산물. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field product
  @type string
  @required
  생산물명
  :::
  ::: field facility
  @type string
  @required
  시설명
  :::
  ::: field index
  @type number
  @required
  시설 순서
  :::
  ::::

- `StageInfo`  
  자동 작전 노드 정보. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field name
  @type string
  @required
  노드명
  :::
  ::::

- `StageInfoError`  
  자동 작전 노드 식별 오류. `details` 필드는 비어 있습니다

- `DepotInfo`  
  창고 인식 결과. `details` 필드 구조는 다음과 같습니다:
  - `done` (boolean, required): 인식 완료 여부, `false`는 아직 인식 중임(진행 중 데이터)을 의미
  - `data` (string, required): JSON 문자열, 형식은 `{"아이템ID": 수량, ...}`, 예: `{"2001":18000,"31043":317}`

- `OperBoxInfo`  
  오퍼레이터 보관함 인식 결과. `details` 필드 구조는 다음과 같습니다:
  - `done` (boolean, required): 인식 완료 여부, `false`는 아직 인식 중임(진행 중 데이터)을 의미
  - `all_opers` (array, required): 전체 오퍼레이터 목록, 배열의 각 항목:
    - `id` (string, required): 오퍼레이터 ID
    - `name` (string, required): 오퍼레이터 명칭
    - `own` (boolean, required): 보유 여부
    - `rarity` (number, required): 오퍼레이터 레어도 [1, 6]
  - `own_opers` (array, required): 보유 중인 오퍼레이터 상세 정보 목록, 배열의 각 항목:
    - `id` (string, required): 오퍼레이터 ID
    - `name` (string, required): 오퍼레이터 명칭
    - `own` (boolean, required): 보유 여부
    - `elite` (number, required): 정예화 단계 [0, 2]
    - `level` (number, required): 오퍼레이터 레벨
    - `potential` (number, required): 오퍼레이터 잠재 [1, 6]
    - `rarity` (number, required): 오퍼레이터 레어도 [1, 6]

- `UnsupportedLevel`  
  자동지휘, 지원하지 않는 노드명. `details` 필드 내용은 다음과 같습니다:

  :::: field-group
  ::: field level
  @type string
  @required
  지원하지 않는 노드명
  :::
  ::::

### ReportRequest

이 필드는 주로 코어 모듈이 UI 계층에 네트워크 요청 정보를 전달하는 데 사용되며, UI가 구체적인 HTTP 보고(Report) 작업을 수행합니다

:::: field-group
::: field url
@type string
@required
요청 전체 URL, 예: `https://penguin-stats.io/PenguinStats/api/v2/report`
:::
::: field headers
@type object
@required
요청 헤더 키-값 쌍 (`Content-Type` 미포함, UI 계층에서 자동 추가)
:::
::: field body
@type string
@required
요청 본문 내용 (일반적으로 JSON 형식의 문자열)
:::
::: field subtask
@type string
@required
서브 작업명, 구체적인 상보 임무 식별, 예: `ReportToPenguinStats`, `ReportToYituliu`
:::
::::
