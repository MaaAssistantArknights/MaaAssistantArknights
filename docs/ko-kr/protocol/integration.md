---
order: 1
icon: bxs:book
---

# 통합문서

## API

### `AsstAppendTask`

#### 프로토타입

```cpp
TaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### 설명

작업을 추가합니다.

#### 반환 값

- `TaskId`  
  작업이 성공적으로 추가되면 작업 ID가 반환됩니다.
  그렇지 않으면 0이 반환됩니다.

#### 매개변수 설명

- `AsstHandle handle`  
  인스턴스 핸들
- `const char* type`  
  작업 유형
- `const char* params`  
  JSON 형식의 작업 매개변수

##### 작업 유형 목록

- `StartUp`  
  게임 시작

```json
// 해당 작업 매개변수
{
    "enable": bool,              // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "client_type": string,       // 클라이언트 버전, 선택 사항, 기본값은 빈 문자열
                                 // 옵션: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "start_game_enabled": bool   // 클라이언트를 자동으로 실행할지 여부, 선택 사항, 기본값은 false
}
```

- `CloseDown`  
  게임 종료

```json
// 해당 작업 매개변수
{
    "enable": bool,              // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "client_type": string,       // 클라이언트 버전, 선택 사항, 기본값은 빈 문자열
                                 // 옵션: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
}
```

- `Fight`  
  작전

```json
// 해당 작업 매개변수
{
    "enable": bool,             // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "stage": string,            // 스테이지 이름, 선택 사항, 기본적으로 현재/마지막 스테이지로 설정됩니다. 실행 중에 편집할 수 없습니다.
                                // "1-7", "S3-2" 등 모든 메인스토리 스테이지를 지원합니다.
                                // 레벨 끝에서 노말/하드를 입력하여 노말과 하드 난이도를 전환할 수 있습니다.
                                // "CE-6" 등 일부 특수 스테이지를 지원합니다. (C# 통합 예시 참조)
    "medicine": int,            // 최대 사용 가능한 이성 회복제 수, 선택 사항, 기본값은 0
    "expiring_medicine": int,   // 48시간 이내에 만료되는 이성 회복제 수, 선택 사항, 기본값은 0
    "stone": int,               // 최대 사용 가능한 오리지늄의 수, 선택 사항, 기본값은 0
    "times": int,               // 최대 반복 횟수, 선택 사항, 기본값은 무한대입니다.
    "series": int,              // 연전 횟수, 선택사항, 1~6
    "drops": {                  // 드랍 수량을 지정합니다. 선택 사항, 기본적으로 지정되지 않습니다.
        "30011": int,           // 키: 아이템 ID; 값: 아이템 수량. 키는 resource/item_index.json을 참조합니다.
        "30062": int            // OR 조합
    },
    /* 아이템은 OR 연산자와 결합되며, 즉 어떤 조건을 만족하면 작업이 중지됩니다. */

    "report_to_penguin": bool,  // Pengiun Stats에 데이터를 업로드할지 여부, 선택 사항, 기본값은 false
    "penguin_id": string,       // Pengiun Stats ID, 선택 사항, 기본값은 빈 문자열입니다. `report_to_penguin`이 `true`인 경우에만 사용할 수 있습니다.
    "server": string,           // 서버, 선택 사항, 기본값은 "CN"입니다. 재료 드랍 인식과 업로드에 영향을 미칩니다.
                                // 옵션: "CN" | "US" | "JP" | "KR"
    "client_type": string,      // 클라이언트 버전, 선택 사항, 기본값은 빈 문자열입니다. 게임이 충돌한 후 다시 연결하는 데 사용됩니다. 빈 값은 이 기능을 비활성화합니다.
                                // 옵션: "Official" | "Bilibili" | "txwy" | "YoStarEN" | "YoStarJP" | "YoStarKR"
    "DrGrandet": bool,          // 오리지늄을 사용하여 "이성을 절약하는(매우 중요)" 옵션, 선택 사항, 기본값은 false입니다. 오리지늄을 사용해야하는 경우에만 유효합니다.
                                // 이성 1 포인트가 복구되면 오리지늄을 사용하여 확정화면에서 대기한 다음 즉시 오리지늄을 사용합니다.
}
```

특수 스테이지의 일부를 지원합니다. 자세한 내용은 [자동 현지화 예제](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/tools/AutoLocalization/example/ko-kr.xaml#L233)를 참고하세요.

- `Recruit`  
  공개모집

```json
// 해당 작업 매개변수
{
    "enable": bool,             // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "refresh": bool,            // 3★ 태그를 새로 고칠지 여부, 선택 사항, 기본값은 false
    "select": [                 // 클릭할 태그 레벨, 필수
        int,
        ...
    ],
    "confirm": [                // 확인용 태그 레벨, 필수. 계산용으로만. 빈 배열로 설정할 수 있습니다.
        int,
        ...
    ],
    "first_tags": [             // 기본 태그, 태그 레벨이 3인 경우에만 유효합니다. 선택 사항, 기본값은 비어 있습니다.
        string,                 // 태그 레벨이 3이면 여기에 있는 태그(있는 경우)가 최대한 많이 선택됩니다.
        ...                     // 또한 강제 선택이므로 '3등급 태그 선택 해제' 설정은 모두 무시됩니다.
    ],
    "extra_tags_mode": int,
    "times": int,               // 고용 횟수, 선택 사항, 기본값은 0입니다. 계산용으로만 0으로 설정할 수 있습니다.
    "set_time": bool,           // 시간을 9시간으로 설정할지 여부, `times`가 0인 경우에만 사용 가능한 옵션입니다, 선택 사항, 기본값은 true입니다.
    "expedite": bool,           // 즉시 완료 허가증을 사용할지 여부, 선택 사항, 기본값은 false입니다.
    "expedite_times": int,      // 즉시 완료 허가증을 사용할 횟수, `expedite`가 `true`일 때만 사용 가능한 옵션입니다, 선택 사항, 기본적으로 `times`가 한도에 도달할 때까지 무한대입니다.
    "skip_robot": bool,         // 로봇 태그가 인식될 때 건너뛸지 여부, 선택 사항, 기본값은 건너뛰기입니다.
    "recruitment_time": {       // 태그 레벨과 지속 시간(분)을 설정합니다, 선택 사항, 기본값은 540(9시간)입니다.
        "3": int,
        "4": int,
        ...
    },
    "report_to_penguin": bool,  // Penguin Stats에 보고할지 여부, 선택 사항, 기본값은 false입니다.
    "penguin_id": string,       // Penguin Stats 사용자 ID, 선택 사항, 기본값은 빈 문자열입니다. `report_to_penguin`이 `true`인 경우에만 사용할 수 있습니다.
    "report_to_yituliu": bool,  // YITULIU에 보고할지 여부, 선택 사항, 기본값은 false입니다.
    "yituliu_id": string,       // YITULIU 사용자 ID, 선택 사항, 기본값은 빈 문자열입니다. `report_to_yituliu`이 `true`인 경우에만 사용할 수 있습니다.
    "server": string,           // 서버, 선택 사항, 기본값은 "CN"입니다. 업로드에 영향을 미칩니다.
}
```

- `Infrast`  
  기반시설 교대

```json
{
    "enable": bool,             // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "mode": int,            // 전환 모드, 선택 사항입니다. 실행 중에는 편집할 수 없습니다.
                            // 0 - 기본값, 자동 전환
                            // 10000 - 사용자 정의 모드, 인프라 스키마 문서를 참조하세요.
    "facility": [           // 전환할 시설, 필수입니다. 실행 중에 편집할 수 없습니다.
        string,             // 시설 이름: "Mfg" | "Trade" | "Power" | "Control" | "Reception" | "Office" | "Dorm"
        ...
    ],
    "drones": string,       // 드론 사용, 선택 사항, 기본값은 "_NotUse"입니다.
                            // "_NotUse"、"Money"、"SyntheticJade"、"CombatRecord"、"PureGold"、"OriginStone"、"Chip"
    "threshold": float,     // 의욕 임계값, 범위 [0, 1.0], 선택 사항, 기본값은 0.3입니다.
    "replenish": bool,      // 오리지늄 조각을 보충할지 여부, 선택 사항, 기본값은 false입니다.
    "dorm_notstationed_enabled": bool, // "비어 있지 않음" 옵션을 활성화할지 여부, 기본값은 false입니다.
    "dorm_trust_enabled": bool, // 숙소에서 신뢰도작을 할지에 대한 여부, 기본값은 false입니다.

    /* 다음은 mode == 10000일 때만 유효합니다 */
    "filename": string,     // 사용자 정의 구성 json 파일 경로
    "plan_index": int,      // 사용자 정의 구성 플랜 인덱스
}
```

- `Mall`  
  크레딧 수집 및 자동 구매
  "구매 우선 항목" 목록에 따라 순서대로 아이템을 구매하며 "무시 항목"에 있는 항목을 무시하고 크레딧이 넘칠 때까지 다른 항목을 순서대로 구매합니다.

```json
// 해당 작업 매개변수
{
    "enable": bool,         // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "shopping": bool,       // 상점에서 아이템을 구매할지 여부, 선택 사항, 기본값은 false입니다. 실행 중에 편집할 수 없습니다.
    "buy_first": [          // 우선적으로 구매할 아이템 목록, 선택 사항입니다. 실행 중에 편집할 수 없습니다.
        string,             // 아이템 이름, 예: "招聘许可" (모집 허가증), "龙门币" (용문폐) 등
        ...
    ],
    "blacklist": [          // 블랙리스트, 선택 사항입니다. 실행 중에 편집할 수 없습니다.
        string,             // 아이템 이름, 예: "加急许可" (즉시 완료 허가증), "家具零件" (가구 부품) 등
        ...
    ],
    "force_shopping_if_credit_full": bool // 크레딧이 넘친다면 블랙리스트를 무시할지 여부, 기본값은 true입니다.
    "only_buy_discount": bool // 크레딧 포인트가 300 미만으로 떨어질 때 구매를 중단할지 여부입니다. 기본적으로 두 번째 구매 시에만 적용되며 기본값은 false입니다.
    "reserve_max_credit": boll // 크레딧 포인트가 300 미만으로 떨어질 때 구매를 중단할지 여부. 기본적으로 두 번째 구매 시에만 적용되며 기본값은 false입니다.
}
```

- `Award`  
  임무 보상 수령

```json
// 해당 작업 매개변수
{
    "enable": bool          // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
}
```

- `Roguelike`  
  통합전략

```json
{
    "enable": bool,         // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "theme": string,        // 테마 이름, 선택 사항, 기본값은 "Phantom"입니다.
                            // Phantom - 팬텀 & 크림손 솔리테어
                            // Mizuki  - 미즈키 & 카이룰라 아버
    "mode": int,            // 모드, 선택 사항, 기본값은 0입니다.
                            // 0 - 레벨 우선, 안정적인 전략으로 최대한 플레이
                            // 1 - 오리지늄각뿔 우선, 미래를 위한 투자 및 2층 도달 즉시 탐험 중도 포기
                            // 2 - 둘 다, 투자를 했을 경우 종료, 못했을 경우 최대한 진행
    "starts_count": int,    // 시작 횟수, 선택 사항, 기본값은 INT_MAX입니다.
    "investment_enabled": bool, // 기본값은 true입니다.

    "investments_count": int,
                            // 투자 횟수, 선택 사항, 기본값은 INT_MAX입니다.
    "stop_when_investment_full": bool,
                            // 투자가 가득 찬 경우 작업을 중지할지 여부, 선택 사항, 기본값은 false입니다.
    "squad": string,        // "강습 전술 분대"와 같은 부대 이름, 선택 사항, 기본값은 "지휘 분대"입니다.
    "roles": string,        // 역할, 선택 사항
    "core_char": string,    // 오퍼레이터 이름, 선택 사항입니다. 레벨에 따라 자동으로 선택합니다.
    "use_support": bool,    // "core_char"이 지원 유닛인지 여부, 선택 사항, 기본값은 false입니다.
    "use_nonfriend_support": bool,  // 지원 유닛이 친구가 아닌 사람의 지원 유닛인지 여부, 선택 사항, 기본값은 false입니다. use_support=true일 때 유효합니다.
    "refresh_trader_with_dice": bool  // 주사위로 거래상인을 새로 고침하여 특별 아이템을 구매할지 여부, 선택 사항, 기본값은 false입니다.
}
```

- `Copilot`  
  Copilot 기반 자동 전투 기능

```json
{
    "enable": bool,             // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "filename": string,         // 작업 JSON의 파일명과 경로, 절대/상대 경로를 지원합니다. 실행 중에 편집할 수 없습니다.
    "formation": bool           // "빠른 배치"을 할지 여부(ESC키 PAUSE 배치), 선택 사항, 기본값은 false입니다. 실행 중에 편집할 수 없습니다.
}
```

더 많은 자세한 정보는 [전투 스키마](./copilot-schema.md)를 참조하세요.

- `SSSCopilot`  
  보안파견용 AI 동반(Copilot 기반) 자동 전투 기능

```json
{
    "enable": bool,             // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
    "filename": string,         // 작업 JSON의 파일명과 경로, 절대/상대 경로를 지원합니다. 실행 중에 편집할 수 없습니다.
    "loop_times": int
}
```

더 많은 자세한 정보는 [보안파견 스키마](./sss-schema.md)를 참조하세요.

- `Depot`
  창고 인식

```json
// 해당 작업 매개변수
{
    "enable": bool          // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
}
```

- `OperBox`  
  오퍼레이터 보유 목록 인식 결과

```json
// 해당 작업 매개변수
{
    "enable": bool          // 이 작업을 활성화할지 여부, 선택 사항, 기본값은 true
}
```

- `ReclamationAlgorithm`  
  생명연산

```json
{
    "enable": bool,
    "theme": int,           // 테마, 선택사항, 기본값은 1입니다.
                            // 0 - *모래 속의 불꽃*
                            // 1 - *모래 안의 이야기*
    "mode": int,            // 모드,선택사항, 기본값은 0
                            // 0 - 점수를 획득하고 건설 점수를 얻어 전투에 진입한 후 바로 나가기
                            // 1 - *모래 속의 불꽃*: 황금을 얻기 위해 오퍼레이터가 물을 사고 기지를 강화합니다.
                            //     *모래 안의 이야기*: 모래 안의 이야기: 자동으로 아이템을 제작하고 로드하여 통화를 획득
    "product": string       // 자동으로 제작되는 아이템, 선택 사항, 기본값은 형광봉
                            // 부분 문자열을 채우는 것이 좋습니다
}
```

- `Custom`  

  사용자 정의 태스크

```json
{
    "enable": bool,
    "task_names": [     // 배열에서 첫 번째 일치하는 항목부터 실행 (이후 next 등 순차적으로 진행)
                        // 여러 태스크를 수행하려면 Custom 태스크를 여러 번 추가할 수 있습니다.
        string,
        ...
    ]
}
```

- `SingleStep`  

  단일 단계 태스크 (현재로서는 Copilot만 지원)

```json
{
    "enable": bool,
    "type": string,     // 현재로서는 "copilot"만 지원합니다.
    "subtask": string,  // "stage" | "start" | "action"
                        // "stage"는 단계 이름 설정 (예: "details": { "stage": "xxxx" })
                        // "start"는 미션 시작으로, 추가적인 세부사항 없이 시작합니다.
                        // "action"은 단일 전투 동작으로, 세부사항은 Copilot의 단일 동작입니다.
                        //           예시: "details": { "name": "史尔特尔", "location": [ 4, 5 ], "direction": "左" },자세한 내용은 전투 스키마 문서 참조
    "details": {
        ...
    }
}
```

- `VideoRecognition`  

  비디오 인식, 현재로서는 전투 비디오만 지원합니다.

```json
{
    "enable": bool,
    "filename": string,
}
```

### `AsstSetTaskParams`

#### 프로토타입

```cpp
bool ASSTAPI AsstSetTaskParams(AsstHandle handle, TaskId id, const char* params);
```

#### 설명

태스크 매개변수 설정

#### 반환 값

- `bool`  
  매개변수가 성공적으로 설정되었는지 여부입니다.

#### 매개변수 설명

- `AsstHandle handle`  
  인스턴스 핸들
- `TaskId task`  
  태스크 ID, `AsstAppendTask`의 반환 값입니다.
- `const char* params`  
  JSON 형식의 태스크 매개변수, `AsstAppendTask`와 동일합니다.
  "Editing in run-time is not supported"으로 표시되지 않은 필드는 실행 중에 변경할 수 있습니다. 그렇지 않으면 태스크가 실행 중일 때 이러한 변경 사항은 무시됩니다.

### `AsstSetStaticOption`

#### 프로토타입

```cpp
bool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### 설명

프로세스 수준의 매개변수 설정

#### 반환 값

- `bool`  
  설정이 성공적인지 여부입니다.

#### 매개변수 설명

- `AsstStaticOptionKey key`  
  키
- `const char* value`  
  값

##### 키와 값 목록

없음

### `AsstSetInstanceOption`

#### 프로토타입

```cpp
bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### 설명

인스턴스 수준의 매개변수 설정

#### 반환 값

- `bool`  
  설정이 성공적인지 여부입니다.

#### 매개변수 설명

- `AsstHandle handle`  
  핸들
- `AsstInstanceOptionKey key`  
  키
- `const char* value`  
  값

##### 키와 값 목록

```json
    enum InstanceOptionKey
    {
        Invalid = 0,
        // Deprecated // MinitouchEnabled = 1,   // minitouch를 사용할 수 있는지 여부
                                // minitouch를 사용할 수 없는 경우에는 활성화되지 않습니다.
                                // "1" - 켬, "0" - 끔
        TouchMode = 2,          // 터치 모드, 기본적으로 minitouch
                                // minitouch | maatouch | adb
        DeploymentWithPause = 3,    // 일시정지 중에 배치 (로그라이크, Copilot 및 보안파견)
                                    // "1" | "0"
        AdbLiteEnabled = 4,     // AdbLite를 활성화할지 여부, "0" | "1"
        KillAdbOnExit = 5,       // 종료 시 Adb 해제, "0" | "1"
    };
```
