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
::: field handle  
@type AsstHandle
@required
인스턴스 핸들  
:::  
::: field type  
@type const char*
@required
작업 유형  
:::  
::: field params  
@type const char*
@required
작업 파라미터, json string  
:::  
::::

##### 작업 유형 목록

- `StartUp`  
  시작 및 깨우기

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부
:::  
::: field client_type  
@type string
@required
클라이언트 버전  
<br>
옵션: `Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
:::  
::: field start_game_enabled  
@type boolean
@default false
@optional
클라이언트 자동 실행 여부  
:::  
::: field account_name  
@type string
@optional
계정 전환, 기본값은 전환하지 않음  
<br>
로그인된 계정으로만 전환 가능하며, 로그인 이름으로 검색하므로 입력 내용이 로그인된 모든 계정 중 유일해야 함  
<br>
공식 서버: `123****4567`인 경우 `123****4567`, `4567`, `123`, `3****4567` 입력 가능  
<br>
Bilibili 서버: `张三`인 경우 `张三`, `张`, `三` 입력 가능  
<br>
번체 중국어 서버: 계정은 Email 형식이며(예: `ab****01@gmail.com`), 별표가 없는 평문 부분(예: `01@gmail`) 입력을 권장합니다  
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field client_type  
@type string
@required
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field stage  
@type string
@optional
스테이지명, 기본값은 비어 있음(현재/지난 스테이지 인식). 실행 중 설정 불가
<br>
`"1-7"`, `"S3-2"` 등 모든 메인 스토리 스테이지 지원
<br>
스테이지명 끝에 `-NORMAL`/`-HARD`를 추가해 난이도 전환 가능 (10-14장은 표준/재앙, 15장 이후는 일반/험지)
<br>
섬멸 작전의 경우 반드시 `"Annihilation"` 입력
<br>
현재 진행 중인 SideStory의 뒤쪽 3개 스테이지는 반드시 전체 스테이지 번호를 입력해야 함  
:::  
::: field medicine  
@type number
@default 0
@optional
이성 회복제 최대 사용 개수  
:::  
::: field medicine_expire_days  
@type number
@default 0
@optional
지정된 일수 이내에 만료되는 이성 회복제를 사용합니다. `0`은 만료 임박 이성 회복제를 사용하지 않음을 의미합니다.  
:::  
::: field expiring_medicine  
@type number
@default 0
@optional
@deprecated
v6.8.0부터 폐기됨. 대신 `medicine_expire_days`를 사용하세요.  
:::  
::: field stone  
@type number
@default 0
@optional
오리지늄 최대 사용 개수  
:::  
::: field times  
@type number
@default 2147483647
@optional
전투 횟수  
:::  
::: field series  
@type number
@optional
연속 전투 횟수, -1~10
<br>
`-1`: 전환 비활성화
<br>
`0`: 현재 사용 가능한 최대 횟수로 자동 전환. 만약 현재 이성이 최대 횟수 미만이면 사용 가능한 최소 횟수 선택
<br>
`1~10`: 지정된 연속 전투 횟수
<br>
::: info 서버 차이
입력 검증은 리소스에 `FightSeries-OldMethodFlag`가 있는지에 따라 달라집니다:
<br>

- 새 목록(중국 서버 2026/8/1 이후 주 리소스, 해당 flag 없음): `-1~10` 허용
- 이전 목록(해외 리소스에 해당 flag 있음): `-1~6`만 허용, 더 큰 값은 거부

해외 서버는 약 반년 후 따를 예정이며, 그때 상한은 리소스에 맞춰 10이 됩니다. Windows GUI의 연속 전투 드롭다운은 현재 고정으로 10까지 제공합니다. 해외에서 수동으로 7~10을 선택하면 작업 전달 시 Core에서 거부됩니다.
:::  
::: field drops  
 @type object
@optional
지정 드랍 수량, 기본값은 지정 안 함. key는 item_id, value는 수량. key는 `resource/item_index.json` 파일 참조
<br>
예: `{ "30011": 10, "30062": 5 }`  
 <br>
위 조건들은 OR 관계이므로, 어느 하나라도 도달하면 작업 중지  
 :::  
 ::: field report_to_penguin  
 @type boolean
@default false
@optional
펭귄 물류(Penguin Stats) 데이터 전송 여부  
 :::  
 ::: field penguin_id  
 @type string
@optional
펭귄 물류 전송 ID, 기본값 비어 있음. `report_to_penguin`이 true일 때만 유효  
 :::  
 ::: field server  
 @type string
@default CN
@optional
서버, 드랍 인식 및 업로드에 영향
<br>
옵션: `CN` | `US` | `JP` | `KR`  
 :::  
 ::: field client_type  
 @type string
@optional
클라이언트 버전, 기본값 비어 있음. 게임 크래시 시 재시작 후 재접속하여 계속 파밍하는 용도. 비워두면 해당 기능 비활성화
<br>
옵션: `Official` | `Bilibili` | `txwy` | `YoStarEN` | `YoStarJP` | `YoStarKR`  
 :::  
 ::: field DrGrandet  
 @type boolean
@default false
@optional
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
   "medicine_expire_days": 2,
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

일부 소수 자원 스테이지명도 지원합니다.

- `Recruit`  
  공개 모집

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field refresh  
@type boolean
@default false
@optional
3성 태그 새로고침 여부  
:::  
::: field select  
@type array<number>
@required
클릭할 태그 등급  
:::  
::: field confirm  
@type array<number>
@required
확인 클릭할 태그 등급. 공모 계산만 할 경우 빈 배열로 설정 가능  
:::  
::: field first_tags  
@type array<string>
@optional
우선 태그, 태그 등급이 3일 때만 유효. 기본값 비어 있음
<br>
태그 등급이 3일 때, 여기에 있는 태그(존재하는 경우)를 가능한 많이 선택하며, 강제 선택이므로 "3성 태그 선택 안 함" 설정을 무시함
:::  
::: field extra_tags_mode  
@type number
@default 0
@optional
추가 태그 선택 모드
<br>
`0` - 기본 동작
<br>
`1` - 충돌 가능성 무시하고 태그 3개 선택
<br>
`2` - 충돌 가능성 무시하고 가능한 많은 고성급 태그 조합 동시 선택  
:::  
::: field times  
@type number
@default 0
@optional
모집 횟수. 공모 계산만 할 경우 0으로 설정 가능  
:::  
::: field set_time  
@type boolean
@default true
@optional
모집 시간 설정 여부. `times`가 0일 때만 유효  
:::  
::: field expedite  
@type boolean
@default false
@optional
즉시 완료 허가증 사용 여부  
:::  
::: field expedite_times  
@type number
@optional
즉시 완료 사용 횟수, `expedite`가 true일 때만 유효. 기본값은 무제한(즉 `times` 상한까지)  
:::  
::: field skip_robot  
@type boolean
@default true
@optional
폐기 예정이며 구형 파라미터 호환용으로만 유지됩니다.  
<br>
`preserve_tags`가 없고 이 값이 `true`이면 `支援机械` 인식 시에만 건너뜁니다. `元素`는 더 이상 구형 1★ 태그로 취급하지 않습니다.  
:::
::: field preserve_tags  
@type array<string>
@optional
현재 공개모집 슬롯을 유지한 채 이번 모집을 건너뛸 Tag 이름 목록입니다. 기본값은 빈 배열입니다.  
<br>
지정한 Tag 중 하나라도 인식되면 MAA는 해당 슬롯을 유지하고 이번 모집을 건너뜁니다.  
:::  
::: field recruitment_time  
@type object
@optional
태그 등급(3 이상)과 대응하는 희망 모집 시간(분 단위), 기본값은 모두 540(즉 09:00:00)
<br>
예: `{ "3": 540, "4": 540 }`  
:::  
::: field report_to_penguin  
@type boolean
@default false
@optional
펭귄 물류 데이터 전송 여부  
:::  
::: field penguin_id  
@type string
@optional
펭귄 물류 전송 ID, 기본값 비어 있음. `report_to_penguin`이 true일 때만 유효  
:::  
::: field report_to_yituliu  
@type boolean
@default false
@optional
Yituliu 데이터 전송 여부  
:::  
::: field yituliu_id  
@type string
@optional
Yituliu 전송 ID, 기본값 비어 있음. `report_to_yituliu`가 true일 때만 유효  
:::  
::: field server  
@type string
@default CN
@optional
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
   "preserve_tags": ["支援机械"],
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field mode  
@type number
@default 0
@optional
교대 작업 모드
<br>
`0` - `Default`: 기본 교대 모드. 시설 내 및 시설 간을 포함한 효율적인 오퍼레이터 조합을 자동으로 계산합니다
<br>
`10000` - `Custom`: 사용자 정의 교대 모드, 사용자 설정 로드. [기반시설 스케줄링 프로토콜](./base-scheduling-schema.md) 참고
<br>
`20000` - `Rotation`: 원터치 로테이션 모드. 제어 센터, 발전소, 숙소, 사무실은 건너뛰고, 나머지 시설은 교대하지 않지만 기본 조작(드론 사용, 응접실 로직 등)은 유지  
:::  
::: field facility  
@type array<string>
@required
교대할 시설. 실행 중 설정 불가
<br>
`mode = 0`일 때 이 배열은 활성화 집합으로 취급되며, 순서와 중복 항목은 스케줄링에 영향을 주지 않습니다(교대 순서는 알고리즘이 자동으로 결정). `mode = 10000` / `20000`일 때는 배열 순서대로 처리됩니다.
<br>
시설명: `Mfg` | `Trade` | `Power` | `Control` | `Reception` | `Office` | `Dorm` | `Processing` | `Training`  
:::  
::: field drones  
@type string
@default \_NotUse
@optional
드론 용도. `mode = 10000`일 때 이 필드는 무효
<br>
옵션: `_NotUse` | `Money` | `SyntheticJade` | `CombatRecord` | `PureGold` | `OriginStone` | `Chip`  
:::  
::: field threshold  
@type number
@default 0.3
@optional
컨디션 임계값, 범위 [0, 1.0]
<br>
`mode = 10000`일 때 이 필드는 "autofill"에 대해서만 유효
<br>
`mode = 20000`일 때 이 필드는 무효  
:::  
::: field replenish  
@type boolean
@default false
@optional
무역소 "오리지늄 조각" 자동 보충 여부  
:::  
::: field dorm_notstationed_enabled  
@type boolean
@default false
@optional
작업 오퍼레이터 숙소 "미배치" 옵션 활성화 여부  
:::  
::: field dorm_trust_enabled  
@type boolean
@default false
@optional
숙소 남은 자리에 신뢰도 미만 오퍼레이터 배치 여부  
:::  
::: field fiammetta_targets  
@type array<string>
@default ["清流", "可露希尔", "但书"]
@optional
피아메타 회복 대상 목록. 교대 시 목록에서 현재 컨디션이 가장 낮은 대상 오퍼레이터가 피아메타와 함께 우선적으로 숙소에 배치됩니다. `mode = 0`일 때만 유효합니다.
<br>
옵션: `清流` | `可露希尔` | `但书` | `巫恋` | `龙舌兰` | `歌蕾蒂娅` (옵션 외 또는 중복 항목은 무시됨)  
:::  
::: field fiammetta_recovery_enabled  
@type boolean
@default false
@optional
교대 시작 시 피아메타로 회복 대상의 컨디션을 회복할지 여부입니다. 비활성화 시 교대가 숙소 준비 단계를 건너뜁니다. `mode = 0`일 때만 유효합니다.  
:::  
::: field use_pinus_sylvestris  
@type boolean
@default false
@optional
｢피누스 실베스트리스｣ 시설 간 연계 활성화 여부. `mode = 0`일 때만 유효합니다.  
:::  
::: field use_perception_information  
@type boolean
@default false
@optional
｢감지 정보｣ 시설 간 연계 활성화 여부. ｢속세의 번뇌｣보다 우선순위가 높습니다. `mode = 0`일 때만 유효합니다.  
:::  
::: field use_worldly_plight  
@type boolean
@default false
@optional
｢속세의 번뇌｣ 시설 간 연계 활성화 여부. `mode = 0`일 때만 유효합니다.  
:::  
::: field use_abyssal_hunter  
@type boolean
@default false
@optional
｢어비설 헌터｣ 시설 간 연계 활성화 여부. `mode = 0`일 때만 유효하며, ｢피누스 실베스트리스｣와 동시에 활성화하면 두 연계는 동시에 교대에 참여하지 않습니다.  
:::  
::: field reception_message_board  
@type boolean
@default true
@optional
응접실 게시판 크레딧 수령 여부  
:::  
::: field reception_clue_exchange  
@type boolean
@default true
@optional
단서 교환 수행 여부  
:::  
::: field reception_send_clue  
@type boolean
@default true
@optional
단서 보내기 여부  
:::  
::: field filename  
@type string
@required
사용자 정의 설정 경로. 실행 중 설정 불가
<br>
<Badge type="warning" text="mode = 10000일 때만 유효" />  
:::  
::: field plan_index  
@type number
@required
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field visit_friends  
@type boolean
@default true
@optional
친구 기반시설을 방문하여 크레딧 획득 여부  
:::  
::: field shopping  
@type boolean
@default true
@optional
크레딧 상점 구매 수행 여부  
:::  
::: field buy_first  
@type array<string>
@default []
@optional
우선 구매 목록. 상품명, 예: `"채용 허가증"`, `"용문폐"` 등  
:::  
::: field blacklist  
@type array<string>
@default []
@optional
구매 블랙리스트. 상품명, 예: `"즉시 완료 허가증"`, `"가구 부품"` 등  
:::  
::: field force_shopping_if_credit_full  
@type boolean
@default false
@optional
크레딧 넘침 시 블랙리스트 무시 여부  
:::  
::: field only_buy_discount  
@type boolean
@default false
@optional
할인 상품만 구매 여부, 2차 구매(목록 순서 외)에만 적용  
:::  
::: field reserve_max_credit  
@type boolean
@default false
@optional
크레딧이 300 미만일 때 구매 중지 여부, 2차 구매에만 적용  
:::  
::: field credit_fight  
@type boolean
@default false
@optional
다음 날 더 많은 크레딧을 얻기 위해 지원을 빌려 OF-1 스테이지를 1회 클리어할지 여부  
:::  
::: field formation_index  
@type number
@default 0
@optional
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field award  
@type boolean
@default true
@optional
일일/주간 임무 보상 수령  
:::  
::: field mail  
@type boolean
@default false
@optional
모든 우편 보상 수령  
:::  
::: field recruit  
@type boolean
@default false
@optional
한정 헤드헌팅 매일 무료 단차 수령  
:::  
::: field orundum  
@type boolean
@default false
@optional
합성옥 추첨(LUCKY WALL 등) 보상 수령  
:::  
::: field mining  
@type boolean
@default false
@optional
한정 채굴 허가(합성옥 채굴) 보상 수령  
:::  
::: field specialaccess  
@type boolean
@default false
@optional
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field theme  
@type string
@default Phantom
@optional
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
<br>
`BlackFlow` - 블랙 플로우  
:::  
::: field mode  
@type number
@default 0
@optional
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
<br>
`30001` - 포대기 동물 파밍, BlackFlow 테마 전용  
:::  
::: field squad  
@type string
@default 指挥分队
@optional
시작 분대명  
:::  
::: field roles  
@type string
@default 取长补短
@optional
시작 모집 조합  
:::  
::: field core_char  
@type string
@optional
시작 오퍼레이터명. 단일 오퍼레이터 **중문명**만 지원(서버 무관); 비워두거나 `""`이면 육성도에 따라 자동 선택  
:::  
::: field use_support  
@type boolean
@default false
@optional
시작 오퍼레이터를 지원 유닛으로 빌릴지 여부  
:::  
::: field use_nonfriend_support  
@type boolean
@default false
@optional
친구가 아닌 지원 유닛 사용 가능 여부. `use_support`가 true일 때만 유효  
:::  
::: field starts_count  
@type number
@default 2147483647
@optional
탐색 시작 횟수. 도달 시 자동 정지  
:::  
::: field difficulty  
@type number
@default -1
@optional
지정 난이도 등급, `-1`는 미지정. 미해금 시 현재 해금된 최고 난이도 선택  
:::  
::: field stop_at_final_boss  
@type boolean
@default false
@optional
5층 보스 노드 앞에서 정지할지 여부. **Phantom 제외** 테마에만 적용  
:::  
::: field stop_at_max_level  
@type boolean
@default false
@optional
통합 전략 만렙 도달 시 정지할지 여부  
:::  
::: field investment_enabled  
@type boolean
@default true
@optional
오리지늄각뿔 투자 여부  
:::  
::: field investments_count  
@type number
@default 2147483647
@optional
투자 횟수. 도달 시 자동 정지  
:::  
::: field stop_when_investment_full  
@type boolean
@default false
@optional
투자 한도 도달 시 자동 정지 여부  
:::  
::: field investment_with_more_score  
@type boolean
@default false
@optional
투자 후 쇼핑 시도 여부. 모드 1에만 적용  
:::  
::: field start_with_elite_two  
@type boolean
@default false
@optional
스타트 리세마라 시 2차 정예화도 함께 노릴지 여부. 모드 4에만 적용  
:::  
::: field only_start_with_elite_two  
@type boolean
@default false
@optional
다른 스타트 조건 무시하고 2차 정예화만 노릴지 여부. 모드 4이고 `start_with_elite_two`가 true일 때만 유효  
:::  
::: field refresh_trader_with_dice  
@type boolean
@default false
@optional
주사위로 상점을 새로고침하여 특수 상품 구매 시도 여부. Mizuki 테마 전용, 길잡이 비늘 파밍용  
:::  
::: field first_floor_foldartal  
@type string
@optional
1층 원견 단계에서 얻길 희망하는 암호문. Sami 테마 전용, 모드 무관; 성공 시 정지  
:::  
::: field start_foldartal_list  
@type array<string>
@default []
@optional
스타트 리세마라 시 시작 보상으로 얻길 희망하는 암호문 목록. Sami 테마 모드 4일 때만 유효
<br>
목록의 모든 암호문을 보유해야 성공으로 간주
<br>
주의: "생존지상 분대"와 함께 사용해야 함. 다른 분대는 시작 보상으로 암호문을 얻지 못함  
:::  
::: field collectible_mode_start_list  
@type object
@optional
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
<br>
`ticket`: 티켓 보상, JieGarden 테마 전용
:::  
::: field use_foldartal  
@type boolean
@optional
암호문 사용 여부. 모드 5 기본값 `false`, 기타 모드 기본값 `true`. Sami 테마 전용  
:::  
::: field check_collapsal_paradigms  
@type boolean
@optional
획득한 붕괴 패러다임 감지 여부. 모드 5 기본값 `true`, 기타 모드 기본값 `false`  
:::  
::: field double_check_collapsal_paradigms  
@type boolean
@default true
@optional
붕괴 패러다임 누락 방지 검사 수행 여부. Sami 테마이고 `check_collapsal_paradigms`가 true일 때 유효. 모드 5 기본값 `true`, 기타 모드 기본값 `false`  
:::  
::: field expected_collapsal_paradigms  
@type array<string>
@default ['目空一些', '睁眼瞎', '图像损坏', '一抹黑']
@optional
희망하는 붕괴 패러다임. Sami 테마이고 모드 5일 때 유효  
:::  
::: field monthly_squad_auto_iterate  
@type boolean
@optional
월간 소대 자동 전환 활성화 여부  
:::  
::: field monthly_squad_check_comms  
@type boolean
@optional
월간 소대 통신 완료 여부도 전환 기준으로 삼을지 여부  
:::  
::: field deep_exploration_auto_iterate  
@type boolean
@optional
심층 조사 자동 전환 활성화 여부  
:::  
::: field collectible_mode_shopping  
@type boolean
@default false
@optional
파밍 중 쇼핑 활성화 여부  
:::  
::: field collectible_mode_squad  
@type string
@optional
파밍 중 사용할 분대, 기본적으로 squad와 동기화, squad가 비었고 이 값도 없으면 지휘 분대  
:::  
::: field start_with_seed  
@type string
@optional
시드 파밍에 사용할 고정 시드, 비워 두면 비활성화
<br>
Sarkaz 테마, Investment 모드, "연금술 분대" 또는 "지원 분대"일 때만 유효  
:::  
::: field blackflow_strategy  
@type string
@optional
블랙 플로우 테마의 전략. 비워 두면 `mode`와 `investment_enabled` 값으로부터 추론됩니다.
<br>
`baby_animal` - 1층에서 일반 상점을 확인한 뒤, 2·3층을 탐색하여 秘境行商(비밀 상인)에서 씨앗을 육성합니다. `blackflow_cultivation_target`와 함께 사용해야 합니다
<br>
`investment` - 1층에서 전투 횟수가 가장 적고 예상 시간이 가장 짧은 경로로 고정 일반 상점에 도달합니다
<br>
`burn_with_investment` - 1층에서 투자를 완료한 후 최대한 빨리 3층에 도달하여 도착 즉시 재시작합니다
<br>
`burn` - 최대한 빨리 3층에 도달하여 도착 즉시 재시작합니다  
:::  
::: field blackflow_cultivation_target  
@type string
@default swaddled_cat
@optional
포대기 동물 파밍 모드의 목표. 선택 가능한 값: `swaddled_cat`(포대기의 고양이) | `swaddled_feathered_serpent`(포대기의 깃털 뱀) | `swaddled_dog`(포대기의 개) | `swaddled_cerberus`(포대기의 케르베로스). `blackflow_strategy`가 `baby_animal`일 때만 사용됩니다.  
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
   "start_with_seed": ""
}
```

</details>

붕괴 패러다임 파밍 기능에 대한 자세한 내용은 [통합 전략 보조 프로토콜](./integrated-strategy-schema.md#탐험가의-은빛-서리-끝자락-—-붕괴-패러다임)을 참고하세요.

- `Copilot`  
   자동지휘

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field filename  
@type string
단일 작전 JSON 파일 경로, copilot_list와 택일(필수); 상대/절대 경로 모두 가능  
:::  
::: field copilot_list  
@type array`<object>`
작전 목록, filename과 택일(필수); filename과 copilot_list 동시 존재 시 copilot_list 무시; 이 파라미터 유효 시 set_params는 1회만 실행 가능
<br>
각 객체 포함:
<br>

- `filename`: 작전 JSON 파일 경로; 상대/절대 경로 모두 가능
  <br>
- `nav_name_override`: 내비게이션용 스테이지명, 선택 사항; 제공되지 않았거나 `null`인 경우 작업 파일에서 자동으로 추론합니다
- `is_raid`: 하드 모드 전환 여부, 선택 사항, 기본값 false
  :::  
  ::: field loop_times  
  @type number
  @default 1
  @optional
  반복 횟수. 단일 작전 모드(filename 지정)에서만 유효; 이 파라미터 유효 시 set_params는 1회만 실행 가능  
  :::  
  ::: field use_sanity_potion  
  @type boolean
  @default false
  @optional
  이성 부족 시 이성 회복제 사용 허용 여부  
  :::  
  ::: field formation  
  @type boolean
  @default false
  @optional
  자동 편성 수행 여부  
  :::  
  ::: field formation_index  
  @type number
  @default 0
  @optional
  자동 편성 시 사용할 편성 슬롯 번호. `formation`이 true일 때 유효
  <br>
  0–4 정수, 0은 현재 편성 선택, 1-4는 제1, 2, 3, 4 편성  
  :::  
  ::: field user_additional  
  @type array`<object>`
  @default []
  @optional
  사용자 정의 추가 오퍼레이터 목록. `formation`이 true일 때 유효
  <br>
  각 객체 포함:
  <br>
- `name`: 오퍼레이터명, 선택 사항, 기본값 "", 비워두면 무시됨
  <br>
- `skill`: 휴대 스킬, 선택 사항, 기본값 0 (게임 내 기본 스킬 선택 따름); 1–3 정수, 범위 벗어나면 게임 내 기본 스킬 선택 따름  
  :::  
  ::: field add_trust  
  @type boolean
  @default false
  @optional
  자동 편성 시 남은 자리를 신뢰도 낮은 순으로 채울지 여부. `formation`이 true일 때 유효  
  :::  
  ::: field ignore_requirements  
  @type boolean
  @default false
  @optional
  자동 편성 시 오퍼레이터 육성 요구조건 무시 여부. `formation`이 true일 때 유효  
  :::  
  ::: field support_unit_usage  
  @type number
  @default 0
  @optional
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
  ::: field support_unit_name  
  @type string
  @optional
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field filename  
@type string
@required
작전 JSON 파일 경로, 절대/상대 경로 모두 가능. 실행 중 설정 불가  
:::  
::: field loop_times  
@type number
@optional
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부.  
:::  
::: field filename  
@type string
@required
단일 작전 JSON 파일 경로, 절대/상대 경로 모두 가능. 실행 중 설정 불가. 필수, list와 택일.  
:::  
::: field list  
@type array<string>
@required
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
::: field enable  
@type boolean
@default true
@optional
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
::: field enable  
@type boolean
@default true
@optional
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field theme  
@type string
@default Tales
@optional
테마
<br>
`Fire` - _모래 속의 불_（종료）
<br>
`Tales` - _사막 이야기_
<br>
`RelaunchAnchor` - _리런치 앵커_  
:::  
::: field mode  
@type number
@default 0
@optional
모드. 테마마다 지원하는 모드가 다릅니다:
<br>
**Tales（사막 이야기）：**
<br>
`0` - 세이브 없음, 스테이지 반복으로 번영의 선물 획득。
<br>
`1` - 세이브 있음, 도구 제작으로 번영의 선물 획득。
<br>
**RelaunchAnchor（리런치 앵커）：**
<br>
`16` (`RA1`) - RA-1, 정경세작→건설→자원 납품→결산 자동 루프。
<br>
`32` (`RA15`) - RA-15, 시빌라이트 에테르나로 60킬 미션 달성。
<br>
`48` (`RA4`) - RA-4, "경영 계획"으로 얻은 적금을 사용하여 지역을 해금하고, 비샤델로 보스 처치 임무를 완료합니다。
:::  
::: field tools_to_craft  
@type array<string>
@default [&quot;荧光棒&quot;]
@optional
자동 제작 아이템, 부분 문자열 입력 권장. Tales 테마에서만 유효  
:::  
::: field increment_mode  
@type number
@default 0
@optional
클릭 유형. Tales 테마에서만 유효
<br>
`0` - 연타
<br>
`1` - 꾹 누르기
:::  
::: field num_craft_batches  
@type number
@default 16
@optional
1회 최대 제작 배치 수. Tales 테마에서만 유효  
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field task_names  
@type array<string>
@required
배열 중 첫 번째로 매칭된 작업(및 후속 next 등)을 실행. 여러 작업을 실행하려면 Custom task를 여러 번 append  
시크릿 프론트(`MiniGame@SecretFront`) 연결 형식 지원: `MiniGame@SecretFront@Begin@Ending[A-E](@이벤트명)?`, 이벤트명은 생략 가능(支援作战平台 / 游侠 / 诡影迷踪), 예: `MiniGame@SecretFront@Begin@EndingA@支援作战平台`。  
:::  
::: field params  
@type object
@optional
작업 추가 파라미터. 현재는 픽셀 아트 작업(`MiniGame@PixelPaint@Begin`)에서만 사용:

- `params.pixel_paint.groups`: 색상별 칸 좌표 목록. `color`는 팔레트 슬롯 번호(0~39, 게임 오른쪽 팔레트 순서와 동일), `points`는 `[x, y]` 칸 좌표 배열(0~23, 왼쪽 위 원점).
- `params.pixel_paint.swipe`(bool, 선택, 기본 true): 같은 색 연속 칸을 한 번의 드래그로 그려 속도를 높임. 일부 터치 방식에서는 이상 동작이 있을 수 있음.
- `params.pixel_paint.grid_delay`(int, 선택, 기본 0): 칸당 추가 대기 시간(ms). 클릭 후 대기와 드래그 시간에 모두 가산됩니다. 각 터치 방식에 기본 간격이 있어 보통 조정 불필요. 구 키 `grid_click_delay` 도 호환됩니다.

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

```json
{
   "enable": true,
   "task_names": ["MiniGame@PixelPaint@Begin"],
   "params": {
      "pixel_paint": {
         "groups": [
            { "color": 7, "points": [[0, 1], [3, 4]] }
         ]
      }
   }
}
```

</details>

- `SingleStep`  
   단일 단계 작업 (현재 전투만 지원)

:::: field-group  
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field type  
@type string
@default copilot
@required
현재 `"copilot"`만 지원  
:::  
::: field subtask  
@type string
@required
서브 작업 유형
<br>
`stage` - 스테이지명 설정, `"details": { "stage": "xxxx" }` 필요
<br>
`start` - 작전 시작, `details` 없음
<br>
`action` - 단일 작전 조작, `details`는 작전 프로토콜 중 단일 action이어야 함. 예: `"details": { "name": "수르트", "location": [ 4, 5 ], "direction": "左" }`. 상세 내용은 [자동지휘 프로토콜](./copilot-schema.md) 참고  
:::  
::: field details  
@type object
@optional
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
::: field enable  
@type boolean
@default true
@optional
본 작업 활성화 여부  
:::  
::: field filename  
@type string
@required
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
::: field handle  
@type AsstHandle
@required
인스턴스 핸들  
:::  
::: field task  
@type AsstTaskId
@required
작업 ID, `AsstAppendTask` 인터페이스 반환 값  
:::  
::: field params  
@type const char\*
@required
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
::: field key  
@type AsstStaticOptionKey
@required
Key  
:::  
::: field value  
@type const char\*
@required
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
::: field handle  
@type AsstHandle
@required
인스턴스 핸들  
:::  
::: field key  
@type AsstInstanceOptionKey
@required
Key  
:::  
::: field value  
@type const char\*
@required
Value  
:::  
::::

##### 키-값 목록

:::: field-group  
::: field Invalid  
@type number
@default 0
@optional
무효 점유. 열거값: 0  
:::  
::: field MinitouchEnabled  
@type boolean
@optional
폐기됨. 원 Minitouch 활성화 여부; "1" 켜기, "0" 끄기. 장치가 지원하지 않을 수 있음. 열거값: 1 (폐기됨)  
:::  
::: field TouchMode  
@type string
@default minitouch
@optional
터치 모드 설정. 옵션: minitouch | maatouch | adb | MaaFwAdb | MumuExtras. 기본값 minitouch. 열거값: 2  
:::  
::: field DeploymentWithPause  
@type boolean
@optional
오퍼레이터 배치 시 일시정지 여부, 자동지휘/통합 전략/보안파견에 모두 영향. 옵션: "1" 켜기, "0" 끄기. 열거값: 3  
:::  
::: field AdbLiteEnabled  
@type boolean
@optional
AdbLite 사용 여부. 옵션: "0" 끄기, "1" 켜기. 열거값: 4  
:::  
::: field KillAdbOnExit  
@type boolean
@optional
종료 시 ADB 프로세스 종료 여부. 옵션: "0" 끄기, "1" 켜기. 열거값: 5  
:::  
::: field ClientType  
@type string
@optional
클라이언트 종류(게임 채널). 대부분의 연결 설정에서는 필요하지 않습니다. `AsstConnect` / `AsstAsyncConnect` 에 전달하는 `config` 가 연결 단계에서 실행되는 명령에 `[PackageName]` 을 사용할 때만, 연결 전에 `AsstSetInstanceOption(..., ClientType, ...)` 를 호출해 설정해야 합니다. 현재 내장 설정 중에서는 `Androws` 와 `WSA` 의 `displayId` 조회만 이 값에 의존합니다. 이 옵션은 StartUp / CloseDown 등의 작업 파라미터 `client_type` 를 대체하지 않습니다. 열거값: 6  
:::  
::::
