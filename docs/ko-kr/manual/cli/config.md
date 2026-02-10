---
order: 3
icon: material-symbols:settings
---

# 설정

## 설정 경로

maa-cli 설정 파일은 특정 설정 경로에 위치하며, `maa dir config` 명령어를 통해 설정 경로를 확인할 수 있습니다. 설정 경로는 환경 변수 `MAA_CONFIG_DIR`을 통해 변경할 수 있습니다. 아래 예시에서는 설정 경로를 `$MAA_CONFIG_DIR`로 표시하겠습니다.

모든 설정 파일은 TOML, YAML 또는 JSON 형식을 사용할 수 있으며, 아래 예시에서는 TOML 형식을 사용하고 파일 확장자는 `.toml`을 사용하겠습니다. 하지만 파일 확장자가 올바르면 이 세 가지 형식을 혼합해서 사용할 수 있습니다.

또한 일부 작업은 `filename`을 매개변수로 받아들이며, 상대 경로를 사용할 경우 설정 경로의 해당 하위 경로를 기준으로 합니다. 예를 들어, 사용자 정의 기반시설 계획 파일의 상대 경로는 `$MAA_CONFIG_DIR/infrast`를 기준으로 해야 하고, 보안 파견 작업 파일은 `$MAA_CONFIG_DIR/ssscopilot`를 기준으로 해야 합니다.

## 사용자 정의 작업

각 사용자 정의 작업은 개별 파일로, `$MAA_CONFIG_DIR/tasks` 경로에 위치해야 합니다.

### 기본 구조

작업 파일은 여러 하위 작업을 포함하며, 각 하위 작업은 MAA 작업으로 다음과 같은 옵션을 포함합니다:

```toml
[[tasks]]
name = "게임 시작" # 작업 이름, 선택 사항, 기본값은 작업 유형
type = "StartUp" # 작업 유형
params = { client_type = "Official", start_game_enabled = true } # 해당 작업의 매개변수
```

구체적인 작업 유형과 매개변수는 [MAA 통합 문서][task-types]에서 찾을 수 있습니다. 주의할 점은 현재 maa-cli는 매개변수 이름과 값이 올바른지 확인하지 않으며, 실행 중 오류가 발생하지 않는 한 MaaCore가 실행 시 오류를 감지하지 않습니다.

### 작업 조건

조건에 따라 다른 매개변수로 작업을 실행하려면 여러 작업 변형을 정의할 수 있습니다:

```toml
[[tasks]]
name = "기반시설 교대"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json" # 사용자 정의 기반시설 계획 파일 이름은 `$MAA_CONFIG_DIR/infrast`에 있어야 합니다.

# 18:00:00부터 다음 날 04:00:00까지 계획 0, 12:00:00 이전에는 계획 1, 그 이후에는 계획 2 사용
[[tasks.variants]]
condition = { type = "Time", start = "18:00:00", end = "04:00:00" } # 종료 시간이 시작 시간보다 작으면 종료 시간은 다음 날로 간주됩니다.
params = { plan_index = 0 }

[[tasks.variants]]
condition = { type = "Time", end = "12:00:00" } # 시작 시간이 생략되면 현재 시간이 종료 시간보다 작을 때 이 조건이 일치합니다.
params = { plan_index = 1 }

[[tasks.variants]]
condition = { type = "Time", start = "12:00:00" } # 종료 시간이 생략되면 현재 시간이 시작 시간보다 클 때 이 조건이 일치합니다.
params = { plan_index = 2 }
```

`condition` 필드는 어떤 변형이 사용될지를 결정하는 데 사용되며, 일치하는 변형의 `params` 필드는 작업의 매개변수에 합쳐집니다.

**주의**: 사용자 정의 기반시설 계획 파일이 상대 경로를 사용하는 경우, `$MAA_CONFIG_DIR/infrast`를 기준으로 해야 합니다. 또한, 기반시설 파일은 maa-cli가 아닌 MaaCore가 읽기 때문에 해당 파일 형식은 반드시 JSON이어야 합니다. maa-cli는 기반시설 파일을 읽지 않으며, 파일에 정의된 시간대에 따라 적절한 서브 계획을 선택하지 않습니다. 따라서 `condition` 필드를 통해 적절한 시간대에 올바른 기반시설 계획의 `plan_index` 매개변수를 지정해야 합니다. 이렇게 하면 적절한 시간대에 올바른 기반시설 계획이 사용될 수 있습니다.

`Time` 조건 외에도 `DateTime`, `Weekday`, `DayMod` 조건이 있습니다. `DateTime` 조건은 특정 시간대를 지정하는 데 사용되고, `Weekday` 조건은 일주일 중 특정 요일을 지정하는 데 사용됩니다. `DayMod`는 사용자 정의 주기의 특정 일을 지정하는 데 사용됩니다.

```toml
[[tasks]]
type = "Fight"

# 여름 이벤트 기간 동안 SL-8 파밍
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }

# 여름 이벤트 기간 외의 화요일, 목요일, 토요일에는 CE-6 파밍
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"], timezone = "Official"}
params = { stage = "CE-6" }

# 그 외 시간에는 1-7 파밍
[[tasks.variants]]
params = { stage = "1-7" }
```

위와 같은 모든 시간 관련 조건에는 `timezone` 매개변수를 통해 시간대를 지정할 수 있습니다. 이 매개변수 값은 숫자일 수 있으며, UTC의 오프셋을 나타냅니다. 만약 사용자의 시간대가 동부 8구역이라면 `timezone = 8`로 지정할 수 있습니다. 이 매개변수는 클라이언트 유형일 수도 있으며, 예를 들어 `timezone = "Official"`로 지정하면 공식 서버의 시간을 사용하여 판단하게 됩니다. **주의**: 공식 서버의 시간대는 동부 8구역이 아닌 동부 4구역입니다. 게임에서 하루의 시작 시간은 00:00:00이 아닌 04:00:00이기 때문입니다. 시간대를 지정하지 않으면 사용자의 로컬 시간대를 사용하게 됩니다.

위의 특정 조건 외에도, `OnSideStory`라는 조건이 있으며, 이 조건을 활성화하면 maa-cli는 해당 리소스를 읽어 현재 이벤트가 진행 중인지 판단합니다. 만약 진행 중인 이벤트가 있으면 해당 변형이 일치하게 됩니다. 예를 들어, 여름 이벤트 기간 동안 SL-8 파밍을 위한 조건은 `{ type = "OnSideStory", client = "Official" }`로 간단히 할 수 있습니다. 여기서 `client` 매개변수는 사용 중인 클라이언트를 나타내며, 각 클라이언트의 이벤트 시간은 다릅니다. 공식 서버나 b 서버를 사용하는 경우 이 매개변수를 생략할 수 있습니다. 이 조건을 통해 매번 이벤트가 업데이트될 때마다 이벤트의 열림 시간을 수동으로 수정할 필요 없이 해당 이벤트의 스테이지만 업데이트하면 됩니다.

위의 기본 조건 외에도 `{ type = "And", conditions = [...] }`, `{ type = "Or", conditions = [...] }`, `{ type = "Not", condition = ... }`를 사용하여 조건을 논리적으로 결합할 수 있습니다.
여러 날에 걸친 기반시설 교대를 원하는 사용자는 `DayMod`와 `Time`을 조합하여 여러 날에 걸친 교대를 설정할 수 있습니다. 예를 들어, 매 이틀마다 6번 교대를 원한다면 다음과 같이 작성할 수 있습니다:

```toml
[[tasks]]
name = "기반시설 교대 (2일 6교대)"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json"

# 첫 번째 교대, 첫날 4:00:00 - 12:00:00
[[tasks.variants]]
params = { plan_index = 0 }
[tasks.variants.condition]
type = "And"
conditions = [
    # 여기서 divisor는 주기를, remainder는 오프셋을 지정합니다.
    # 오프셋은 num_days_since_ce % divisor와 같습니다.
    # 여기서 num_days_since_ce는 공원 이후의 일 수이며, 0001-01-01이 첫날입니다.
    # 현재 오프셋은 `maa remainder <divisor>` 명령어를 통해 확인할 수 있습니다.
    # 예를 들어, 2024-01-27은 738,912일이며, 738912 % 2 = 0입니다.
    # 이 경우 오프셋은 0이며, 이 조건이 일치합니다.
    { type = "DayMod", divisor = 2, remainder = 0 },
    { type = "Time", start = "04:00:00", end = "12:00:00" },
]

# 두 번째 교대, 첫날 12:00:00 - 20:00:00
[[tasks.variants]]
params = { plan_index = 1 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 0 },
  { type = "Time", start = "12:00:00", end = "20:00:00" },
]

# 세 번째 교대, 첫날 20:00:00 - 다음날 4:00:00
[[tasks.variants]]
params = { plan_index = 2 }
[tasks.variants.condition]
# 여기서는 반드시 Or 조건을 사용해야 하며, Time { start = "20:00:00", end = "04:00:00" }를 직접 사용하면 안 됩니다.
# 이 경우 다음날 00:00:00 - 04:00:00이 일치하지 않습니다.
# 물론 교대 시간을 조정하여 하루를 넘기지 않는 것이 더 좋습니다. 이 예시는 단지 설명을 위한 것입니다.
type = "Or"
conditions = [
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 0 },
     { type = "Time", start = "20:00:00" },
  ] },
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 1 },
     { type = "Time", end = "04:00:00" },
  ] },
]

# 네 번째 교대, 둘째 날 4:00:00 - 12:00:00
[[tasks.variants]]
params = { plan_index = 3 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 1 },
  { type = "Time", start = "04:00:00", end = "12:00:00" },
]

# 다섯 번째 교대, 둘째 날 12:00:00 - 20:00:00
[[tasks.variants]]
params = { plan_index = 4 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 1 },
  { type = "Time", start = "12:00:00", end = "20:00:00" },
]

# 여섯 번째 교대, 둘째 날 20:00:00 - 셋째 날(새로운 첫날) 4:00:00
[[tasks.variants]]
params = { plan_index = 5 }
[tasks.variants.condition]
type = "Or"
conditions = [
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 1 },
     { type = "Time", start = "20:00:00" },
  ] },
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 0 },
     { type = "Time", end = "04:00:00" },
  ] },
]

```

기본 전략에서는 여러 변형이 일치하면 첫 번째 변형이 사용됩니다. 조건이 주어지지 않으면 변형은 항상 일치하므로 조건이 없는 변형을 마지막에 두어 기본값으로 사용할 수 있습니다.

`strategy` 필드를 사용하여 매칭 전략을 변경할 수 있습니다:

```toml
[[tasks]]
type = "Fight"
strategy = "merge" # 또는 "first" (기본값)

# 일요일 저녁에는 모든 만료 임박한 약물을 사용
[[tasks.variants]]
params = { expiring_medicine = 1000 }

[tasks.variants.condition]
type = "And"
conditions = [
  { type = "Time", start = "18:00:00" },
  { type = "Weekday", weekdays = ["Sun"] },
]

# 기본 1-7 파밍
[[tasks.variants]]
params = { stage = "1-7" }

# 화요일, 목요일, 토요일에는 CE-6 파밍
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 여름 이벤트 기간에는 SL-8 파밍
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
```

이 예시와 위의 예시는 동일한 스테이지를 파밍하지만, 일요일 저녁에는 모든 만료 임박한 약물을 사용합니다. `merge` 전략에서는 여러 변형이 일치하면 후속 변형의 매개변수가 앞선 변형의 매개변수에 합쳐집니다. 여러 변형에 동일한 매개변수가 있으면 후속 변형의 매개변수가 앞선 변형의 매개변수를 덮어씁니다.

어떤 변형도 일치하지 않으면 작업이 실행되지 않으며, 이는 특정 조건에서만 하위 작업을 실행하는 데 사용할 수 있습니다:

```toml
# 18:00:00 이후에만 크레딧 상점 관련 작업 수행
[[tasks]]
type = "Mall"

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
```

### 사용자 입력

일부 작업에서는 실행 시 매개변수를 입력해야 할 수도 있습니다. 예를 들어, 스테이지 이름을 입력할 수 있습니다. 해당 매개변수를 `Input` 또는 `Select` 유형으로 설정할 수 있습니다:

```toml
[[tasks]]
type = "Fight"

# 스테이지 선택
[[tasks.variants]]
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
[tasks.variants.params.stage]
# 선택 가능한 스테이지, 적어도 하나의 선택 값을 제공해야 합니다.
# 선택 값은 값일 수도 있고, 값과 설명을 포함하는 테이블일 수도 있습니다.
alternatives = [
    "SL-7", # "1. SL-7"로 표시됨
    { value = "SL-8", desc = "경맹광물" } # "2. SL-8 (경맹광물)"로 표시됨
]
default_index = 1 # 기본값의 인덱스, 1부터 시작, 설정하지 않으면 빈 값 입력 시 다시 입력하라는 메시지가 표시됨
description = "여름 이벤트에서 싸울 스테이지" # 설명, 선택 사항
allow_custom = true # 사용자 정의 값을 입력할 수 있는지 여부, 기본값은 false, 허용하면 정수가 아닌 값이 사용자 정의 값으로 간주됨

# 입력 불필요
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 스테이지 입력
[[tasks.variants]]
[tasks.variants.params.stage]
default = "1-7" # 기본 스테이지, 선택 사항 (기본값이 없으면 빈 값 입력 시 다시 입력하라는 메시지가 표시됨)
description = "싸울 스테이지" # 설명, 선택 사항

# 입력한 스테이지가 1-7일 때 이성 회복제 수량 입력 필요
[tasks.variants.params.medicine]
# 매개변수는 조건부 매개변수로 설정할 수 있으며, 조건이 충족될 때만 입력이 필요합니다.
# conditions 필드는 테이블로, 키는 동일 레벨의 다른 매개변수 이름이고 값은 예상 값입니다.
# 여기서 조건은 stage가 1-7인 경우입니다. 여러 조건이 있는 경우 모든 조건이 충족되어야 합니다.
conditions = { stage = "1-7" }
default = 1000
description = "사용할 이성 회복제"
```

`Input` 유형의 경우, 작업 실행 시 값을 입력하라는 메시지가 표시됩니다. 빈 값을 입력하면 기본값이 있으면 기본값이 사용되며, 없으면 다시 입력하라는 메시지가 표시됩니다. `Select` 유형의 경우, 작업 실행 시 인덱스 또는 사용자 정의 값을 입력하라는 메시지가 표시됩니다(허용할 경우). 빈 값을 입력하면 기본값이 있으면 기본값이 사용되며, 없으면 다시 입력하라는 메시지가 표시됩니다.

`--batch` 옵션은 작업 실행 시 모든 입력을 건너뛰고 기본값을 사용하도록 할 수 있습니다. 입력에 기본값이 없으면 오류가 발생합니다.

## MaaCore 관련 설정

MaaCore 관련 설정은 `$MAA_CONFIG_DIR/profiles` 경로에 있어야 합니다. 이 경로의 각 파일은 설정 파일이며, `-p` 또는 `--profile` 옵션을 통해 설정 파일 이름을 지정할 수 있습니다. 지정하지 않으면 `default` 설정 파일을 읽으려고 시도합니다.

현재 지원되는 설정 필드는 다음과 같습니다:

```toml
[connection]
preset = "MuMuPro"
adb_path = "adb"
device = "emulator-5554"
config = "CompatMac"

[resource]
global_resource = "YoStarEN"
platform_diff_resource = "iOS"
user_resource = true

[static_options]
cpu_ocr = false
gpu_ocr = 1

[instance_options]
touch_mode = "MaaTouch"
deployment_with_pause = false
adb_lite_enabled = false
kill_adb_on_exit = false
```

### 연결 설정

`[connection]` 관련 필드는 MaaCore가 게임에 연결하는 매개변수를 지정하는 데 사용됩니다:

```toml
[connection]
adb_path = "adb" # adb 실행 파일 경로, 기본값은 "adb", 이는 adb 실행 파일이 환경 변수 PATH에 포함된다는 의미
address = "emulator-5554" # 연결 주소, 예: "emulator-5554" 또는 "127.0.0.1:5555"
config = "General" # 연결 설정, 일반적으로 변경할 필요 없음
```

`adb_path`는 `adb` 실행 파일의 경로입니다. 경로를 지정하거나 환경 변수 `PATH`에 추가하여 MaaCore가 찾을 수 있도록 할 수 있습니다. 대부분의 에뮬레이터는 자체적으로 `adb`를 포함하고 있어 별도의 설치가 필요 없습니다. 그렇지 않으면 직접 `adb`를 설치해야 합니다. `address`는 `adb`의 연결 주소입니다. 에뮬레이터의 경우 `127.0.0.1:[포트 번호]`를 사용할 수 있습니다. 일반적인 에뮬레이터 포트 번호는 [자주 묻는 질문][emulator-ports]을 참고하세요. `address`를 지정하지 않으면 `adb devices` 명령어를 통해 연결된 장치를 검색하며, 여러 장치가 연결된 경우 첫 번째 장치를 사용합니다. 장치를 찾을 수 없으면 `emulator-5554`에 연결을 시도합니다. `config`는 플랫폼 및 에뮬레이터 관련 설정을 지정하는 데 사용됩니다. Linux에서는 기본값이 `CompatPOSIXShell`, macOS에서는 `CompatMac`, Windows에서는 `General`입니다. 추가 설정은 리소스 폴더의 `config.json` 파일에서 찾을 수 있습니다.

일부 일반적인 에뮬레이터의 경우 `preset`을 사용하여 사전 설정된 구성을 사용할 수 있습니다:

```toml
[connection]
preset = "MuMuPro" # MuMuPro 사전 설정 연결 구성 사용
adb_path = "/path/to/adb" # 필요 시 사전 설정된 adb 경로를 재정의할 수 있으며, 대부분의 경우 필요하지 않습니다.
address = "127.0.0.1:7777" # 필요 시 사전 설정된 주소를 재정의할 수 있습니다.
```

현재 `MuMuPro` 한 가지 에뮬레이터만 사전 설정이 있으며, 다른 일반적인 에뮬레이터의 사전 설정을 추가하고 싶다면 issue나 PR을 제출하세요.

#### 특수 프리셋

현재 `PlayCover (macOS)`, `Waydroid (Linux)` 두 가지 특수 프리셋이 사전 구성되어 있습니다.

- `PlayCover`는 macOS에서 `PlayCover`를 통해 네이티브로 실행되는 게임 클라이언트에 연결하는 데 사용됩니다. 이 경우 `adb_path`를 지정할 필요가 없으며, `address`는 `adb` 연결 주소가 아닌 `PlayTools`의 주소입니다. 자세한 내용은 [PlayCover 지원 문서][playcover-doc]를 참고하세요.

- `Waydroid`는 Linux에서 `Waydroid`를 통해 네이티브로 실행되는 게임 클라이언트에 연결하는 데 사용됩니다. 이 경우에도 `adb_path`를 지정해야 합니다. 자세한 내용은 [Waydroid 지원 문서][waydroid-doc]를 참고하세요.

### 리소스 설정

`[resource]` 관련 필드는 MaaCore가 로드하는 리소스를 지정하는 데 사용됩니다:

```toml
[resource]
global_resource = "YoStarEN" # 비 중국어 버전의 리소스
platform_diff_resource = "iOS" # 비 안드로이드 버전의 리소스
user_resource = true # 사용자 정의 리소스를 로드할지 여부
```

비 중국어 게임 클라이언트를 사용할 때, MaaCore는 기본적으로 간체 중국어 리소스를 로드하기 때문에 `global_resource` 필드를 지정하여 비중문 버전의 리소스를 로드해야 합니다. iOS 버전의 게임 클라이언트를 사용할 때는 `platform_diff_resource` 필드를 지정하여 iOS 버전의 리소스를 로드해야 합니다. 이 두 필드는 선택 사항이며, 리소스를 로드할 필요가 없는 경우 두 필드를 비워둘 수 있습니다. 또한, `startup` 작업에서 `client_type` 필드를 지정한 경우 `global_resource`는 해당 클라이언트의 리소스로 설정되며, `PlayTools` 연결을 사용하는 경우 `platform_diff_resource`는 iOS로 설정됩니다. 마지막으로 사용자 정의 리소스를 로드하려는 경우 `user_resource` 필드를 `true`로 설정해야 합니다.

### 정적 옵션

`[static_options]` 관련 필드는 MaaCore 정적 옵션을 지정하는 데 사용됩니다:

```toml
[static_options]
cpu_ocr = false # CPU OCR을 사용할지 여부, 기본값은 CPU OCR 사용
gpu_ocr = 1 # GPU OCR을 사용할 때 사용하는 GPU ID, 이 값이 비어 있으면 CPU OCR을 사용
```

### 인스턴스 옵션

`[instance_options]` 관련 필드는 MaaCore 인스턴스 옵션을 지정하는 데 사용됩니다:

```toml
[instance_options]
touch_mode = "ADB" # 사용할 터치 모드, 가능한 값은 "ADB", "MiniTouch", "MaaTouch", "MacPlayTools"
deployment_with_pause = false # 배포 시 게임을 일시 중지할지 여부
adb_lite_enabled = false # adb-lite를 사용할지 여부
kill_adb_on_exit = false # 종료 시 adb를 종료할지 여부
```

`touch_mode`의 `MacPlayTools` 옵션은 연결 방식 `PlayTools`와 연동됩니다. `PlayTools`로 연결할 때 `touch_mode`는 강제로 `MacPlayTools`로 설정됩니다.

## CLI 관련 설정

CLI 관련 설정은 `$MAA_CONFIG_DIR/cli.toml`에 있어야 합니다. 현재 포함된 설정은 다음과 같습니다:

```toml
# MaaCore 설치 및 업데이트 관련 설정
[core]
channel = "Stable" # 업데이트 채널, 가능한 값은 "Alpha", "Beta", "Stable", 기본값은 "Stable"
test_time = 0    # 미러 속도를 테스트할 시간, 0은 테스트하지 않음을 의미, 기본값은 3
# MaaCore 최신 버전을 조회하는 API 주소, 비워두면 기본 주소를 사용
api_url = "https://github.com/MaaAssistantArknights/MaaRelease/raw/main/MaaAssistantArknights/api/version/"

# MaaCore의 관련 구성 요소를 설치할지 여부를 설정, 분리 설치는 버전 불일치 문제를 야기할 수 있어 권장되지 않음. 이 옵션은 미래 버전에서 제거될 수 있음.
[core.components]
library = true  # MaaCore 라이브러리를 설치할지 여부, 기본값은 true
resource = true # MaaCore 리소스를 설치할지 여부, 기본값은 true

# CLI 업데이트 관련 설정
[cli]
channel = "Stable" # 업데이트 채널, 가능한 값은 "Alpha", "Beta", "Stable", 기본값은 "Stable"
# maa-cli 최신 버전을 조회하는 API 주소, 비워두면 기본 주소를 사용
api_url = "https://github.com/MaaAssistantArknights/maa-cli/raw/version/"
# 사전 컴파일된 바이너리 파일의 다운로드 주소, 비워두면 기본 주소를 사용
download_url = "https://github.com/MaaAssistantArknights/maa-cli/releases/download/"

# maa-cli의 관련 구성 요소를 설치할지 여부를 설정
[cli.components]
binary = true # maa-cli 바이너리 파일을 설치할지 여부, 기본값은 true

# 리소스 핫 업데이트 관련 설정
[resource]
auto_update = true  # 각 작업 실행 시 리소스를 자동 업데이트할지 여부, 기본값은 false
warn_on_update_failure = true # 업데이트 실패 시 오류를 바로 보고하지 않고 경고를 발행할지 여부
backend = "libgit2" # 리소스 핫 업데이트 백엔드, 가능한 값은 "git" 또는 "libgit2", 기본값은 "git"

# 리소스 핫 업데이트 원격 저장소 관련 설정
[resource.remote]
branch = "main" # 원격 저장소의 브랜치, 기본값은 "main"
# 원격 리소스 저장소의 URL, 비워두면 기본 URL 사용
# GitHub 저장소는 HTTPS와 SSH 두 가지 프로토콜을 지원하며, HTTPS 프로토콜 사용을 권장합니다. 별도의 설정이 필요 없기 때문입니다.
url = "https://github.com/MaaAssistantArknights/MaaResource.git"
# url = "git@github.com:MaaAssistantArknights/MaaResource.git"
# SSH 프로토콜을 사용해야 하는 경우, SSH 키를 제공해야 합니다. 가장 간단한 방법은 키의 경로를 제공하는 것입니다.
ssh_key = "~/.ssh/id_ed25519" # ssh 키의 경로
# maa는 기본적으로 암호화되지 않은 키를 사용합니다. 키가 암호로 보호된 경우, 키를 복호화할 암호를 제공해야 합니다.
# 주의: libgit2 백엔드를 사용할 때만 maa가 암호를 libgit2에 전달합니다.
# git 백엔드를 사용할 때는 git이 직접 암호를 요청합니다.
# git 백엔드를 사용하고 키가 암호로 보호된 경우, ssh-agent를 사용하여 키를 관리하세요.
passphrase = "password"       # ssh 키의 암호
# 그러나 설정 파일에 평문 암호를 저장하는 것은 안전하지 않으므로, 이를 피하는 몇 가지 방법이 있습니다.
# 1. `passphrase`를 true로 설정하면, maa-cli가 매번 암호를 입력하라고 요청합니다.
# 이 방법은 안전하지만 번거롭고 batch 모드에서는 사용할 수 없습니다.
# passphrase = true
# 2. `passphrase`를 환경 변수 이름으로 설정하면, maa-cli는 해당 환경 변수를 암호로 사용합니다.
# 이 방법은 평문 암호보다 안전하지만, 환경 변수는 모든 프로그램에서 접근할 수 있으므로 여전히 위험이 있습니다.
# passphrase = { env = "MAA_SSH_PASSPHRASE" }
# 3. `passphrase`를 명령어로 설정하면, maa-cli는 해당 명령어를 실행하여 암호를 가져옵니다.
# 암호 관리자를 사용하여 암호를 관리한다면, 이 방법이 가장 안전하고 편리할 수 있습니다.
# passphrase = { cmd = ["pass", "show", "ssh/id_ed25519"] }
# 4. ssh-agent를 사용하여 키를 관리, **권장**
# ssh-agent는 키를 메모리에 저장하므로 매번 암호를 입력할 필요가 없습니다.
# 주의: ssh-agent가 시작되어 있고 키가 추가되어 있어야 하며, SSH_AUTH_SOCK 환경 변수가 설정되어 있어야 합니다.
# use_ssh_agent = true # ssh-agent를 사용하여 인증, true로 설정하면 ssh_key와 passphrase 필드가 무시됩니다.
```

**주의사항**：

- MaaCore의 업데이트 채널에서 `Alpha`는 Windows에서만 사용할 수 있습니다.
- CLI의 기본 API 링크와 다운로드 링크는 GitHub 링크이므로 국내에서는 문제가 있을 수 있습니다. `api_url`과 `download_url`을 설정하여 미러를 사용할 수 있습니다.
- 리소스 핫 업데이트를 시작해도 MaaCore 리소스를 설치해야 합니다. 리소스 핫 업데이트는 모든 리소스 파일을 포함하지 않고 일부 업데이트 가능한 리소스 파일만 포함합니다. 기본 리소스 파일은 여전히 설치해야 합니다.
- 리소스 핫 업데이트는 Git을 통해 원격 저장소를 가져옵니다. 백엔드를 `git`으로 설정한 경우 `Git` 명령어 도구가 사용 가능해야 합니다.
- SSH 프로토콜을 사용하여 원격 저장소를 가져오려면 `ssh_key` 필드를 설정해야 합니다. 이 필드는 SSH 개인 키를 가리키는 경로여야 합니다.
- 원격 저장소의 `url` 설정은 최초 리소스 설치에만 유효합니다. 원격 저장소 주소를 변경하려면 Git 명령어 도구를 통해 수동으로 변경하거나 해당 저장소를 삭제해야 합니다. 저장소 위치는 `maa dir hot-update` 명령어로 확인할 수 있습니다.

## 참고 설정

- [예시 설정][example-config]
- [사용자 설정][wangl-cc-dotfiles]

## JSON Schema

[`schemas` 경로][schema-dir]에서 maa-cli의 JSON Schema 파일을 찾을 수 있으며, 이 파일을 사용하여 설정 파일을 검증하거나 편집기에서 자동 완성을 받을 수 있습니다.

- 사용자 정의 작업 파일의 JSON Schema 파일은 [`task.schema.json`][task-schema]입니다.
- MaaCore 설정의 JSON Schema 파일은 [`asst.schema.json`][asst-schema]입니다.
- CLI 설정의 JSON Schema 파일은 [`cli.schema.json`][cli-schema]입니다.

[task-types]: ../../protocol/integration.md#작업-유형-목록
[emulator-ports]: ../../manual/connection.md#포트-번호-입력
[playcover-doc]: ../../manual/device/macos.md#✅-playcover-제일-부드럽습니다-🚀
[waydroid-doc]: ../../manual/device/linux.md#✅-waydroid
[example-config]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/config_examples
[wangl-cc-dotfiles]: https://github.com/wangl-cc/dotfiles/tree/main/home/dot_config/maa
[schema-dir]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/
[task-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/task.schema.json
[asst-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/asst.schema.json
[cli-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/cli.schema.json
