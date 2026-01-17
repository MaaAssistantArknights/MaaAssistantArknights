---
order: 1
icon: bxs:book
---

# 통합 문서

## 인터페이스 소개

### `AsstAppendTask`

#### 인터페이스 원형

```cpp
AsstTaskId ASSTAPI AsstAppendTask(AsstHandle handle, const char* type, const char* params);
```

#### 인터페이스 설명

작업 추가

#### 반환 값

- `AsstTaskId`  
   추가 성공 시 해당 작업 ID 반환, 이후 작업 파라미터 설정에 사용 가능;  
   추가 실패 시 0 반환

#### 파라미터 설명

:::: field-group  
::: field name="handle" type="AsstHandle" required  
인스턴스 핸들  
:::  
::: field name="type" type="const char*" required  
작업 유형  
:::  
::: field name="params" type="const char*" required  
작업 파라미터, json string  
:::  
::::

##### 작업 유형 목록

- `StartUp`  
  시작 및 깨우기

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부
:::  
::: field name="client_type" type="string" required  
클라이언트 버전  
<br>
옵션: `Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::: field name="start_game_enabled" type="boolean" optional default="false"  
클라이언트 자동 실행 여부  
:::  
::: field name="account_name" type="string" optional  
계정 전환, 기본값은 전환하지 않음  
<br>
로그인된 계정으로만 전환 가능하며, 로그인 이름으로 검색하므로 입력 내용이 로그인된 모든 계정 중 유일해야 함  
<br>
공식 서버: `123****4567`인 경우 `123****4567`, `4567`, `123`, `3****4567` 입력 가능  
<br>
Bilibili 서버: `张三`인 경우 `张三`, `张`, `三` 입력 가능  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "client_type": "Official",
   "start_game_enabled": true,
   "account_name": "123****4567"
}
```

</details>

- `CloseDown`  
   게임 종료

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="client_type" type="string" required  
클라이언트 버전, 비워두면 실행하지 않음  
<br>
옵션: `Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "client_type": "Official"
}
```

</details>

- `Fight`  
   이성 사용 작전

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="stage" type="string" optional  
스테이지명, 기본값은 비어 있음(현재/지난 스테이지 인식). 실행 중 설정 불가
<br>
`"1-7"`, `"S3-2"` 등 모든 메인 스토리 스테이지 지원
<br>
스테이지명 끝에 `Normal`/`Hard`를 입력하여 일반/하드 난이도 전환 가능
<br>
섬멸 작전의 경우 반드시 `"Annihilation"` 입력
<br>
현재 진행 중인 SideStory의 뒤쪽 3개 스테이지는 반드시 전체 스테이지 번호를 입력해야 함  
:::  
::: field name="medicine" type="number" optional default="0"  
이성 회복제 최대 사용 개수  
:::  
::: field name="expiring_medicine" type="number" optional default="0"  
48시간 내 만료되는 이성 회복제 최대 사용 개수  
:::  
::: field name="stone" type="number" optional default="0"  
오리지늄 최대 사용 개수  
:::  
::: field name="times" type="number" optional default="2147483647"  
전투 횟수  
:::  
::: field name="series" type="number" optional  
연속 전투 횟수, -1~6
<br>
`-1`: 전환 비활성화
<br>
`0`: 현재 사용 가능한 최대 횟수로 자동 전환. 만약 현재 이성이 6회 미만이면 사용 가능한 최소 횟수 선택
<br>
`1~6`: 지정된 연속 전투 횟수  
:::  
::: field name="drops" type="object" optional  
지정 드랍 수량, 기본값은 지정 안 함. key는 item_id, value는 수량. key는 `resource/item_index.json` 파일 참조
<br>
예: `{ "30011": 10, "30062": 5 }`  
<br>
위 조건들은 OR 관계이므로, 어느 하나라도 도달하면 작업 중지  
:::  
::: field name="report_to_penguin" type="boolean" optional default="false"  
펭귄 물류(Penguin Stats) 데이터 전송 여부  
:::  
::: field name="penguin_id" type="string" optional  
펭귄 물류 전송 ID, 기본값 비어 있음. `report_to_penguin`이 true일 때만 유효  
:::  
::: field name="server" type="string" optional default="CN"  
서버, 드랍 인식 및 업로드에 영향
<br>
옵션: `CN` | `US` | `JP` | `KR`  
:::  
::: field name="client_type" type="string" optional  
클라이언트 버전, 기본값 비어 있음. 게임 크래시 시 재시작 후 재접속하여 계속 파밍하는 용도. 비워두면 해당 기능 비활성화
<br>
옵션: `Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::: field name="DrGrandet" type="boolean" optional default="false"  
이성 절약 모드, 오리지늄 사용 가능성이 있을 때만 유효
<br>
오리지늄 사용 확인 창에서 대기하다가, 현재 1이성이 회복되면 즉시 오리지늄을 사용  
:::  
::::  

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "stage": "1-7",
   "medicine": 1,
   "expiring_medicine": 0,
   "stone": 0,
   "times": 10,
   "series": 0,
   "drops": {
      "30011": 10
   },
   "report_to_penguin": true,
   "penguin_id": "123456",
   "report_to_yituliu": true,
   "yituliu_id": "123456",
   "server": "CN",
   "client_type": "Official",
   "DrGrandet": false
}
```

</details>

일부 소수 자원 스테이지명도 지원합니다. [통합 예시](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/tools/AutoLocalization/example/zh-cn.xaml#L260)를 참고하세요

- `Recruit`  
  공개 모집

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="refresh" type="boolean" optional default="false"  
3성 태그 새로고침 여부  
:::  
::: field name="select" type="array<number>" required  
클릭할 태그 등급  
:::  
::: field name="confirm" type="array<number>" required  
확인 클릭할 태그 등급. 공모 계산만 할 경우 빈 배열로 설정 가능  
:::  
::: field name="first_tags" type="array<string>" optional  
우선 태그, 태그 등급이 3일 때만 유효. 기본값 비어 있음
<br>
태그 등급이 3일 때, 여기에 있는 태그(존재하는 경우)를 가능한 많이 선택하며, 강제 선택이므로 "3성 태그 선택 안 함" 설정을 무시함
:::  
::: field name="extra_tags_mode" type="number" optional default="0"  
추가 태그 선택 모드
<br>
`0` - 기본 동작
<br>
`1` - 충돌 가능성 무시하고 태그 3개 선택
<br>
`2` - 충돌 가능성 무시하고 가능한 많은 고성급 태그 조합 동시 선택  
:::  
::: field name="times" type="number" optional default="0"  
모집 횟수. 공모 계산만 할 경우 0으로 설정 가능  
:::  
::: field name="set_time" type="boolean" optional default="true"  
모집 시간 설정 여부. `times`가 0일 때만 유효  
:::  
::: field name="expedite" type="boolean" optional default="false"  
즉시 완료 허가증 사용 여부  
:::  
::: field name="expedite_times" type="number" optional  
즉시 완료 사용 횟수, `expedite`가 true일 때만 유효. 기본값은 무제한(즉 `times` 상한까지)  
:::  
::: field name="skip_robot" type="boolean" optional default="true"  
로봇 태그 인식 시 건너뛸지 여부  
:::  
::: field name="recruitment_time" type="object" optional  
태그 등급(3 이상)과 대응하는 희망 모집 시간(분 단위), 기본값은 모두 540(즉 09:00:00)
<br>
예: `{ "3": 540, "4": 540 }`  
:::  
::: field name="report_to_penguin" type="boolean" optional default="false"  
펭귄 물류 데이터 전송 여부  
:::  
::: field name="penguin_id" type="string" optional  
펭귄 물류 전송 ID, 기본값 비어 있음. `report_to_penguin`이 true일 때만 유효  
:::  
::: field name="report_to_yituliu" type="boolean" optional default="false"  
Yituliu 데이터 전송 여부  
:::  
::: field name="yituliu_id" type="string" optional  
Yituliu 전송 ID, 기본값 비어 있음. `report_to_yituliu`가 true일 때만 유효  
:::  
::: field name="server" type="string" optional default="CN"  
서버, 업로드에 영향
<br>
옵션: `CN` | `US` | `JP` | `KR`  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "refresh": true,
   "select": [5, 4],
   "confirm": [4, 3],
   "first_tags": ["高级资深干员"],
   "extra_tags_mode": 1,
   "times": 4,
   "set_time": true,
   "expedite": false,
   "expedite_times": 0,
   "skip_robot": true,
   "recruitment_time": {
      "3": 540,
      "4": 540
   },
   "report_to_penguin": false,
   "penguin_id": "123456",
   "report_to_yituliu": false,
   "yituliu_id": "123456",
   "server": "CN"
}
```

</details>

- `Infrast`  
   기반시설 교대

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="mode" type="number" optional default="0"  
교대 작업 모드
<br>
`0` - `Default`: 기본 교대 모드, 단일 시설 최적해
<br>
`10000` - `Custom`: 사용자 정의 교대 모드, 사용자 설정 로드. [기반시설 스케줄링 프로토콜](./base-scheduling-schema.md) 참고
<br>
`20000` - `Rotation`: 원터치 로테이션 모드. 제어 센터, 발전소, 숙소, 사무실은 건너뛰고, 나머지 시설은 교대하지 않지만 기본 조작(드론 사용, 응접실 로직 등)은 유지  
:::  
::: field name="facility" type="array<string>" required  
교대할 시설(순서 있음). 실행 중 설정 불가
<br>
시설명: `Mfg` | `Trade` | `Power` | `Control` | `Reception` | `Office` | `Dorm` | `Processing` | `Training`  
:::  
::: field name="drones" type="string" optional default="\_NotUse"  
드론 용도. `mode = 10000`일 때 이 필드는 무효
<br>
옵션: `_NotUse` | `Money` | `SyntheticJade` | `CombatRecord` | `PureGold` | `OriginStone` | `Chip`  
:::  
::: field name="threshold" type="number" optional default="0.3"  
컨디션 임계값, 범위 [0, 1.0]
<br>
`mode = 10000`일 때 이 필드는 "autofill"에 대해서만 유효
<br>
`mode = 20000`일 때 이 필드는 무효  
:::  
::: field name="replenish" type="boolean" optional default="false"  
무역소 "오리지늄 조각" 자동 보충 여부  
:::  
::: field name="dorm_notstationed_enabled" type="boolean" optional default="false"  
작업 오퍼레이터 숙소 "미배치" 옵션 활성화 여부  
:::  
::: field name="dorm_trust_enabled" type="boolean" optional default="false"  
숙소 남은 자리에 신뢰도 미만 오퍼레이터 배치 여부  
:::  
::: field name="reception_message_board" type="boolean" optional default="true"  
응접실 게시판 크레딧 수령 여부  
:::  
::: field name="reception_clue_exchange" type="boolean" optional default="true"  
단서 교환 수행 여부  
:::  
::: field name="reception_send_clue" type="boolean" optional default="true"  
단서 보내기 여부  
:::  
::: field name="filename" type="string" required  
사용자 정의 설정 경로. 실행 중 설정 불가
<br>
<Badge type="warning" text="mode = 10000일 때만 유효" />  
:::  
::: field name="plan_index" type="number" required  
설정 내 사용할 플랜 번호. 실행 중 설정 불가
<br>
<Badge type="warning" text="mode = 10000일 때만 유효" />  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "mode": 0,
   "facility": ["Mfg", "Trade", "Reception"],
   "drones": "PureGold",
   "threshold": 0.3,
   "replenish": true,
   "dorm_notstationed_enabled": false,
   "dorm_trust_enabled": true,
   "reception_message_board": true,
   "reception_clue_exchange": true,
   "reception_send_clue": true,
   "filename": "schedules/base.json",
   "plan_index": 1
}
```

</details>

- `Mall`  
   크레딧 수령 및 상점 구매  
   먼저 `buy_first` 목록 순서대로 구매하고, 그 다음 `blacklist`를 제외하고 왼쪽에서 오른쪽으로 구매하며, 크레딧이 넘칠 경우 블랙리스트를 무시하고 넘치지 않을 때까지 구매

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="visit_friends" type="boolean" optional default="true"  
친구 기반시설을 방문하여 크레딧 획득 여부  
:::  
::: field name="shopping" type="boolean" optional default="true"  
크레딧 상점 구매 수행 여부  
:::  
::: field name="buy_first" type="array<string>" optional default="[]"  
우선 구매 목록. 상품명, 예: `"채용 허가증"`, `"용문폐"` 등  
:::  
::: field name="blacklist" type="array<string>" optional default="[]"  
구매 블랙리스트. 상품명, 예: `"즉시 완료 허가증"`, `"가구 부품"` 등  
:::  
::: field name="force_shopping_if_credit_full" type="boolean" optional default="false"  
크레딧 넘침 시 블랙리스트 무시 여부  
:::  
::: field name="only_buy_discount" type="boolean" optional default="false"  
할인 상품만 구매 여부, 2차 구매(목록 순서 외)에만 적용  
:::  
::: field name="reserve_max_credit" type="boolean" optional default="false"  
크레딧이 300 미만일 때 구매 중지 여부, 2차 구매에만 적용  
:::  
::: field name="credit_fight" type="boolean" optional default="false"  
다음 날 더 많은 크레딧을 얻기 위해 지원을 빌려 OF-1 스테이지를 1회 클리어할지 여부  
:::  
::: field name="formation_index" type="number" optional default="0"  
OF-1 플레이 시 사용할 편성 슬롯 번호
<br>
0~4의 정수, 0은 현재 편성, 1~4는 제1~4 편성  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "visit_friends": true,
   "shopping": true,
   "buy_first": ["招聘许可", "龙门币"],
   "blacklist": ["家具零件"],
   "force_shopping_if_credit_full": false,
   "only_buy_discount": true,
   "reserve_max_credit": false,
   "credit_fight": false,
   "formation_index": 0
}
```

</details>

- `Award`  
   각종 보상 수령

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="award" type="boolean" optional default="true"  
일일/주간 임무 보상 수령  
:::  
::: field name="mail" type="boolean" optional default="false"  
모든 우편 보상 수령  
:::  
::: field name="recruit" type="boolean" optional default="false"  
한정 헤드헌팅 매일 무료 단차 수령  
:::  
::: field name="orundum" type="boolean" optional default="false"  
합성옥 추첨(LUCKY WALL 등) 보상 수령  
:::  
::: field name="mining" type="boolean" optional default="false"  
한정 채굴 허가(합성옥 채굴) 보상 수령  
:::  
::: field name="specialaccess" type="boolean" optional default="false"  
5주년 등 이벤트 월정액 보상 수령  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "award": true,
   "mail": true,
   "recruit": true,
   "orundum": false,
   "mining": true,
   "specialaccess": false
}
```

</details>

- `Roguelike`  
   통합 전략 무한 반복

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="theme" type="string" optional default="Phantom"  
테마
<br>
`Phantom` - 팬텀 & 크림슨 솔리테어
<br>
`Mizuki` - 미즈키 & 카이룰라 아버
<br>
`Sami` - 탐험가의 은빛 서리 끝자락
<br>
`Sarkaz` - 살카즈의 영겁 기담
<br>
`JieGarden` - 쉐이의 기이한 계원  
:::  
::: field name="mode" type="number" optional default="0"  
모드
<br>
`0` - 점수/보상 포인트 파밍, 가능한 안정적으로 많은 층수 도달
<br>
`1` - 오리지늄 각뿔 파밍, 1층 투자 후 퇴각
<br>
`2` - <Badge type="danger" text="폐기됨" /> 모드 0과 1 겸용, 투자 후 퇴각, 투자할 게 없으면 계속 진행
<br>
`3` - 개발 중..
<br>
`4` - 스타트 리세마라, 난이도 0으로 3층 도달 후 재시작, 지정 난이도에서 스타트 보상 확인. 전기주전자나 희망이 아니면 난이도 0으로 돌아가 반복; Phantom 테마는 난이도 전환 없이 현재 난이도에서 반복
<br>
`5` - 붕괴 패러다임 파밍; Sami 테마 전용; 전투 중 적을 흘려 붕괴치를 빠르게 누적, 첫 붕괴 패러다임이 `expected_collapsal_paradigms` 목록에 있으면 정지, 아니면 재시작
<br>
`6` - 월간 소대 보상 파밍, 모드 적응 외엔 모드 0과 동일
<br>
`7` - 심층 조사 보상 파밍, 모드 적응 외엔 모드 0과 동일  
:::  
::: field name="squad" type="string" optional default="指挥分队"  
시작 분대명  
:::  
::: field name="roles" type="string" optional default="取长补短"  
시작 모집 조합  
:::  
::: field name="core_char" type="string" optional  
시작 오퍼레이터명. 단일 오퍼레이터 **중문명**만 지원(서버 무관); 비워두거나 `""`이면 육성도에 따라 자동 선택  
:::  
::: field name="use_support" type="boolean" optional default="false"  
시작 오퍼레이터를 지원 유닛으로 빌릴지 여부  
:::  
::: field name="use_nonfriend_support" type="boolean" optional default="false"  
친구가 아닌 지원 유닛 사용 가능 여부. `use_support`가 true일 때만 유효  
:::  
::: field name="starts_count" type="number" optional default="2147483647"  
탐색 시작 횟수. 도달 시 자동 정지  
:::  
::: field name="difficulty" type="number" optional default="0"  
지정 난이도 등급. 미해금 시 현재 해금된 최고 난이도 선택  
:::  
::: field name="stop_at_final_boss" type="boolean" optional default="false"  
5층 보스 노드 앞에서 정지할지 여부. **Phantom 제외** 테마에만 적용  
:::  
::: field name="stop_at_max_level" type="boolean" optional default="false"  
통합 전략 만렙 도달 시 정지할지 여부  
:::  
::: field name="investment_enabled" type="boolean" optional default="true"  
오리지늄각뿔 투자 여부  
:::  
::: field name="investments_count" type="number" optional default="2147483647"  
투자 횟수. 도달 시 자동 정지  
:::  
::: field name="stop_when_investment_full" type="boolean" optional default="false"  
투자 한도 도달 시 자동 정지 여부  
:::  
::: field name="investment_with_more_score" type="boolean" optional default="false"  
투자 후 쇼핑 시도 여부. 모드 1에만 적용  
:::  
::: field name="start_with_elite_two" type="boolean" optional default="false"  
스타트 리세마라 시 2차 정예화도 함께 노릴지 여부. 모드 4에만 적용  
:::  
::: field name="only_start_with_elite_two" type="boolean" optional default="false"  
다른 스타트 조건 무시하고 2차 정예화만 노릴지 여부. 모드 4이고 `start_with_elite_two`가 true일 때만 유효  
:::  
::: field name="refresh_trader_with_dice" type="boolean" optional default="false"  
주사위로 상점을 새로고침하여 특수 상품 구매 시도 여부. Mizuki 테마 전용, 길잡이 비늘 파밍용  
:::  
::: field name="first_floor_foldartal" type="string" optional  
1층 원견 단계에서 얻길 희망하는 암호문. Sami 테마 전용, 모드 무관; 성공 시 정지  
:::  
::: field name="start_foldartal_list" type="array<string>" optional default="[]"  
스타트 리세마라 시 시작 보상으로 얻길 희망하는 암호문 목록. Sami 테마 모드 4일 때만 유효
<br>
목록의 모든 암호문을 보유해야 성공으로 간주
<br>
주의: "생존지상 분대"와 함께 사용해야 함. 다른 분대는 시작 보상으로 암호문을 얻지 못함  
:::  
::: field name="collectible_mode_start_list" type="object" optional  
스타트 리세마라 시 희망 보상, 기본값은 모두 false. 모드 4일 때만 유효
<br>
`hot_water`: 전기주전자 보상, 파밍 매커니즘 트리거용 (공통)
<br>
`shield`: 보호막 값 보상, 추가 체력 (공통)
<br>
`ingot`: 오리지늄 각뿔 보상 (공통)
<br>
`hope`: 희망 보상 (공통, 주의: JieGarden 테마는 hope 보상 없음)
<br>
`random`: 랜덤 보상 옵션: 게임 내 "모든 각뿔을 소모하여 랜덤 소장품 1개 획득" (공통)
<br>
`key`: 열쇠 보상, Mizuki 테마 전용
<br>
`dice`: 주사위 보상, Mizuki 테마 전용
<br>
`ideas`: 구상 2개 보상, Sarkaz 테마 전용  
:::  
::: field name="use_foldartal" type="boolean" optional  
암호문 사용 여부. 모드 5 기본값 `false`, 기타 모드 기본값 `true`. Sami 테마 전용  
:::  
::: field name="check_collapsal_paradigms" type="boolean" optional  
획득한 붕괴 패러다임 감지 여부. 모드 5 기본값 `true`, 기타 모드 기본값 `false`  
:::  
::: field name="double_check_collapsal_paradigms" type="boolean" optional default="true"  
붕괴 패러다임 누락 방지 검사 수행 여부. Sami 테마이고 `check_collapsal_paradigms`가 true일 때 유효. 모드 5 기본값 `true`, 기타 모드 기본값 `false`  
:::  
::: field name="expected_collapsal_paradigms" type="array<string>" optional default="['目空一些', '睁眼瞎', '图像损坏', '一抹黑']"  
희망하는 붕괴 패러다임. Sami 테마이고 모드 5일 때 유효  
:::  
::: field name="monthly_squad_auto_iterate" type="boolean" optional  
월간 소대 자동 전환 활성화 여부  
:::  
::: field name="monthly_squad_check_comms" type="boolean" optional  
월간 소대 통신 완료 여부도 전환 기준으로 삼을지 여부  
:::  
::: field name="deep_exploration_auto_iterate" type="boolean" optional  
심층 조사 자동 전환 활성화 여부  
:::  
::: field name="collectible_mode_shopping" type="boolean" optional default="false"  
파밍 중 쇼핑 활성화 여부  
:::  
::: field name="collectible_mode_squad" type="string" optional  
파밍 중 사용할 분대, 기본적으로 squad와 동기화, squad가 비었고 이 값도 없으면 지휘 분대  
:::  
::: field name="start_with_seed" type="boolean" optional default="false"  
시드를 사용하여 각뿔 파밍
<br>
Sarkaz 테마, Investment 모드, "연금술 분대" 또는 "지원 분대"일 때만 true 가능
<br>
고정 시드 사용  
:::  
::::   

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "theme": "Sami",
   "mode": 5,
   "squad": "指挥分队",
   "roles": "取长补短",
   "core_char": "塑心",
   "use_support": false,
   "use_nonfriend_support": false,
   "starts_count": 3,
   "difficulty": 8,
   "stop_at_final_boss": false,
   "stop_at_max_level": false,
   "investment_enabled": true,
   "investments_count": 2,
   "stop_when_investment_full": false,
   "investment_with_more_score": false,
   "start_with_elite_two": false,
   "only_start_with_elite_two": false,
   "refresh_trader_with_dice": false,
   "first_floor_foldartal": "",
   "start_foldartal_list": [],
   "collectible_mode_start_list": {
      "hot_water": true,
      "shield": false,
      "ingot": false,
      "hope": true,
      "random": false,
      "key": false,
      "dice": false,
      "ideas": false
   },
   "use_foldartal": true,
   "check_collapsal_paradigms": true,
   "double_check_collapsal_paradigms": true,
   "expected_collapsal_paradigms": ["目空一些", "睁眼瞎"],
   "monthly_squad_auto_iterate": false,
   "monthly_squad_check_comms": false,
   "deep_exploration_auto_iterate": false,
   "collectible_mode_shopping": false,
   "collectible_mode_squad": "",
   "start_with_seed": false
}
```

</details>

붕괴 패러다임 파밍 기능에 대한 자세한 내용은 [통합 전략 보조 프로토콜](./integrated-strategy-schema.md#탐험가의-은빛-서리-끝자락-—-붕괴-패러다임)을 참고하세요.

- `Copilot`  
   자동지휘

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="filename" type="string"  
단일 작전 JSON 파일 경로, copilot_list와 택일(필수); 상대/절대 경로 모두 가능  
:::  
::: field name="copilot_list" type="array<object>"  
작전 목록, filename과 택일(필수); filename과 copilot_list 동시 존재 시 copilot_list 무시; 이 파라미터 유효 시 set_params는 1회만 실행 가능
<br>
각 객체 포함:
<br>

- `filename`: 작전 JSON 파일 경로; 상대/절대 경로 모두 가능
  <br>
- `stage_name`: 스테이지명, 구체적인 건 [PRTS.Map](https://map.ark-nights.com) 참고
  <br>
- `is_raid`: 하드 모드 전환 여부, 선택 사항, 기본값 false
  :::  
  ::: field name="loop_times" type="number" optional default="1"  
  반복 횟수. 단일 작전 모드(filename 지정)에서만 유효; 이 파라미터 유효 시 set_params는 1회만 실행 가능  
  :::  
  ::: field name="use_sanity_potion" type="boolean" optional default="false"  
  이성 부족 시 이성 회복제 사용 허용 여부  
  :::  
  ::: field name="formation" type="boolean" optional default="false"  
  자동 편성 수행 여부  
  :::  
  ::: field name="formation_index" type="number" optional default="0"  
  자동 편성 시 사용할 편성 슬롯 번호. `formation`이 true일 때 유효
  <br>
  0–4 정수, 0은 현재 편성 선택, 1-4는 제1, 2, 3, 4 편성  
  :::  
  ::: field name="user_additional" type="array<object>" optional default="[]"  
  사용자 정의 추가 오퍼레이터 목록. `formation`이 true일 때 유효
  <br>
  각 객체 포함:
  <br>
- `name`: 오퍼레이터명, 선택 사항, 기본값 "", 비워두면 무시됨
  <br>
- `skill`: 휴대 스킬, 선택 사항, 기본값 1; 1–3 정수, 범위 벗어나면 게임 내 기본 스킬 선택 따름  
  :::  
  ::: field name="add_trust" type="boolean" optional default="false"  
  자동 편성 시 남은 자리를 신뢰도 낮은 순으로 채울지 여부. `formation`이 true일 때 유효  
  :::  
  ::: field name="ignore_requirements" type="boolean" optional default="false"  
  자동 편성 시 오퍼레이터 육성 요구조건 무시 여부. `formation`이 true일 때 유효  
  :::  
  ::: field name="support_unit_usage" type="number" optional default="0"  
  지원 유닛 사용 모드. 0–3 정수. `formation`이 true일 때 유효
  <br>
  `0` - 지원 유닛 사용 안 함
  <br>
  `1` - 결원이 딱 1명일 때만 지원 유닛으로 보충, 결원 없으면 안 씀
  <br>
  `2` - 결원이 딱 1명일 때 지원 유닛 보충 시도, 결원 없으면 지정된 지원 유닛 사용  
  <br>
  `3` - 결원이 딱 1명일 때 지원 유닛 보충 시도, 결원 없으면 랜덤 지원 유닛 사용  
  :::  
  ::: field name="support_unit_name" type="string" optional default=""  
  지정 지원 유닛명. `support_unit_usage`가 2일 때 유효  
  :::  
  ::::

작전 JSON은 [자동지휘 프로토콜](./copilot-schema.md)을 참고하세요.

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "copilot/1-7.json",
   "loop_times": 2,
   "use_sanity_potion": false,
   "formation": true,
   "formation_index": 1,
   "user_additional": [
      {
         "name": "能天使",
         "skill": 3
      }
   ],
   "add_trust": true,
   "ignore_requirements": false,
   "support_unit_usage": 2,
   "support_unit_name": "艾雅法拉"
}
```

</details>

- `SSSCopilot`  
   보안 파견 자동지휘

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="filename" type="string" required  
작전 JSON 파일 경로, 절대/상대 경로 모두 가능. 실행 중 설정 불가  
:::  
::: field name="loop_times" type="number" optional  
반복 실행 횟수  
:::  
::::  
보안 파견 작전 JSON은 [보안 파견 프로토콜](./sss-schema.md)을 참고하세요.

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "sss/plan.json",
   "loop_times": 1
}
```

</details>

- `ParadoxCopilot`
  패러독스 시뮬레이션 작전 계획 자동 수행

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부.  
:::  
::: field name="filename" type="string" required  
단일 작전 JSON 파일 경로, 절대/상대 경로 모두 가능. 실행 중 설정 불가. 필수, list와 택일.  
:::  
::: field name="list" type="array<string>" required  
작전 JSON 목록, 절대/상대 경로 모두 가능. 실행 중 설정 불가. 필수, filename과 택일.  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "paradox/exusiai.json",
   "list": []
}
```

</details>

- `Depot`  
   창고 인식

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true
}
```

</details>

- `OperBox`  
   오퍼레이터 인식

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true
}
```

</details>

- `Reclamation`  
   생존 연산

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="theme" type="string" optional default="Fire"  
테마
<br>
`Fire` - _모래 속의 불_
<br>
`Tales` - _사막 이야기_  
:::  
::: field name="mode" type="number" optional default="0"  
모드
<br>
`0` - 점수 및 건설 포인트 파밍, 전투 진입 후 바로 포기
<br>
`1` - 모래 속의 불: 적금 파밍, 연락원에게 물 구매 후 기지에서 주조; 사막 이야기: 자동 아이템 제작 및 로드 반복으로 화폐 파밍  
:::  
::: field name="tools_to_craft" type="array<string>" optional default="[&quot;荧光棒&quot;]"  
자동 제작 아이템, 부분 문자열 입력 권장  
:::  
::: field name="increment_mode" type="number" optional default="0"  
클릭 유형
<br>
`0` - 연타
<br>
`1` - 꾹 누르기
:::  
::: field name="num_craft_batches" type="number" optional default="16"  
1회 최대 제작 배치 수  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "theme": "Fire",
   "mode": 1,
   "tools_to_craft": ["荧光棒", "发电机"],
   "increment_mode": 0,
   "num_craft_batches": 12
}
```

</details>

- `Custom`  
   사용자 정의 작업

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="task_names" type="array<string>" required  
배열 중 첫 번째로 매칭된 작업(및 후속 next 등)을 실행. 여러 작업을 실행하려면 Custom task를 여러 번 append  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "task_names": ["StartUp", "Infrast", "Fight"]
}
```

</details>

- `SingleStep`  
   단일 단계 작업 (현재 전투만 지원)

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="type" type="string" required default="copilot"  
현재 `"copilot"`만 지원  
:::  
::: field name="subtask" type="string" required  
서브 작업 유형
<br>
`stage` - 스테이지명 설정, `"details": { "stage": "xxxx" }` 필요
<br>
`start` - 작전 시작, `details` 없음
<br>
`action` - 단일 작전 조작, `details`는 작전 프로토콜 중 단일 action이어야 함. 예: `"details": { "name": "수르트", "location": [ 4, 5 ], "direction": "左" }`. 상세 내용은 [자동지휘 프로토콜](./copilot-schema.md) 참고  
:::  
::: field name="details" type="object" optional  
서브 작업 상세 파라미터  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "type": "copilot",
   "subtask": "stage",
   "details": {
      "stage": "1-7"
   }
}
```

</details>

- `VideoRecognition`  
  영상 인식, 현재 작전(전투) 영상만 지원

:::: field-group  
::: field name="enable" type="boolean" optional default="true"  
본 작업 활성화 여부  
:::  
::: field name="filename" type="string" required  
영상 파일 경로, 절대/상대 경로 모두 가능. 실행 중 설정 불가  
:::  
::::

<details>
<summary>Example</summary>

```json
{
   "enable": true,
   "filename": "videos/copilot.mp4"
}
```

</details>

### `AsstSetTaskParams`

#### 인터페이스 원형

```cpp
bool ASSTAPI AsstSetTaskParams(AsstHandle handle, AsstTaskId id, const char* params);
```

#### 인터페이스 설명

작업 파라미터 설정

#### 반환 값

- `bool`  
   설정 성공 여부 반환

#### 파라미터 설명

:::: field-group  
::: field name="handle" type="AsstHandle" required  
인스턴스 핸들  
:::  
::: field name="task" type="AsstTaskId" required  
작업 ID, `AsstAppendTask` 인터페이스 반환 값  
:::  
::: field name="params" type="const char\*" required  
작업 파라미터, json string, `AsstAppendTask` 인터페이스와 동일  
"실행 중 설정 불가"로 표시되지 않은 필드는 실시간 수정 가능; 표시된 경우 현재 작업이 실행 중이면 해당 필드 무시  
:::  
::::

### `AsstSetStaticOption`

#### 인터페이스 원형

```cpp
bool ASSTAPI AsstSetStaticOption(AsstStaticOptionKey key, const char* value);
```

#### 인터페이스 설명

프로세스 레벨 파라미터 설정

#### 반환 값

- `bool`  
   설정 성공 여부 반환

#### 파라미터 설명

:::: field-group  
::: field name="key" type="AsstStaticOptionKey" required  
Key  
:::  
::: field name="value" type="const char\*" required  
Value  
:::  
::::

##### 키-값 목록

없음

### `AsstSetInstanceOption`

#### 인터페이스 원형

```cpp
bool ASSTAPI AsstSetInstanceOption(AsstHandle handle, AsstInstanceOptionKey key, const char* value);
```

#### 인터페이스 설명

인스턴스 레벨 파라미터 설정

#### 반환 값

- `bool`  
   설정 성공 여부 반환

#### 파라미터 설명

:::: field-group  
::: field name="handle" type="AsstHandle" required  
인스턴스 핸들  
:::  
::: field name="key" type="AsstInstanceOptionKey" required  
Key  
:::  
::: field name="value" type="const char\*" required  
Value  
:::  
::::

##### 키-값 목록

:::: field-group  
::: field name="Invalid" type="number" optional default="0"  
무효 점유. 열거값: 0  
:::  
::: field name="MinitouchEnabled" type="boolean" optional  
폐기됨. 원 minitouch 활성화 여부; "1" 켜기, "0" 끄기. 장치가 지원하지 않을 수 있음. 열거값: 1 (폐기됨)  
:::  
::: field name="TouchMode" type="string" optional default="minitouch"  
터치 모드 설정. 옵션: minitouch | maatouch | adb. 기본값 minitouch. 열거값: 2  
:::  
::: field name="DeploymentWithPause" type="boolean" optional  
오퍼레이터 배치 시 일시정지 여부, 자동지휘/통합 전략/보안파견에 모두 영향. 옵션: "1" 켜기, "0" 끄기. 열거값: 3  
:::  
::: field name="AdbLiteEnabled" type="boolean" optional  
AdbLite 사용 여부. 옵션: "0" 끄기, "1" 켜기. 열거값: 4  
:::  
::: field name="KillAdbOnExit" type="boolean" optional  
종료 시 ADB 프로세스 종료 여부. 옵션: "0" 끄기, "1" 켜기. 열거값: 5  
:::  
::::
