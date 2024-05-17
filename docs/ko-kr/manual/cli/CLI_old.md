---
icon: material-symbols:terminal
---
# CLI 가이드

::: warning
해당 문서는 잠재적으로 잘못된 정보를 포함할 수 있습니다!


이 문서는 2024년 4월에 작성됐습니다! 오랜 시간이 지났다면 원본 문서를 보는 것을 권장합니다!
:::

## 기능

- TOML, YAML 또는 JSON 파일로 작업을 정의하고, `maa run <task>`을 사용해 실행합니다.
- `maa install` 및 `maa update`을 사용하여 `Maa Core` 및 리소스를 설치, 업데이트 합니다.
- `maa self update`로 업데이트합니다.

## 설치방법

### Appimage

MAA의 CLI는 Linux에서의 기본 인터페이스입니다. 최신 [Appimage](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)를 다운로드하여 CLI를 직접 사용할 수 있습니다.

### 패키지 관리자

#### macOS

[Homebrew](https://brew.sh/)를 사용하여 설치합니다:

```bash
brew install MaaAssistantArknights/tap/maa-cli
```

#### Linux

- Arch Linux 사용자는 [AUR package](https://aur.archlinux.org/packages/maa-cli/)를 설치할 수 있습니다:

    ```bash
    yay -S maa-cli
    ```

- Nix 사용자는 다음을 직접 실행할 수 있습니다:

  ```bash
  # Stable
  nix run nixpkgs#maa-cli
  ```

  ```bash
  # Nightly
  nix run github:Cryolitia/nur-packages#maa-cli-nightly
  ```

안정 버전은 nixpkgs에 패키지화된 `maa-cli`로, [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix)의 Rust 도구 체인을 사용합니다. Nightly 버전은 [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix)에 있으며, Rust 도구 체인의 베타 채널을 사용하며, 자동으로 업데이트되고 매일 Github Action으로 확인을 위해 빌드됩니다.

- Linux Brew 사용자는 다음과 같이 [Linux Brew](https://docs.brew.sh/Homebrew-on-Linux)로 설치할 수 있습니다:

   ```bash
   brew install MaaAssistantArknights/tap/maa-cli
   ```

### 사전빌드 바이너리

[`maa-cli` release page](https://github.com/MaaAssistantArknights/maa-cli/releases/latest)에서 미리 빌드된 바이너리를 다운로드하여 즐겨 사용하는 위치에 압축 해제하여 CLI를 설치할 수 있습니다. 다른 플랫폼용 파일 이름은 다음과 같습니다:

<table>
    <thead>
        <tr>
            <th>운영 체제</th>
            <th>아키텍처</th>
            <th>파일 이름</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td rowspan=2>Linux</td>
            <td>x86_64</td>
            <td>maa_cli-x86_64-unknown-linux-gnu.tar.gz</td>
        </tr>
        <tr>
            <td>aarch64</td>
            <td>maa_cli-aarch64-unknown-linux-gnu.tar.gz</td>
        </tr>
        <tr>
            <td rowspan=2>macOS</td>
            <td>x86_64</td>
            <td rowspan=2>
              maa_cli-universal-apple-darwin.zip
            </td>
        </tr>
        <tr>
            <td>aaarch64</td>
        </tr>
        <tr>
            <td rowspan=2>Windows</td>
            <td>x86_64</td>
            <td>maa_cli-x86_64-pc-windows-msvc.zip</td>
        </tr>
    </tbody>
</table>

### 소스 코드로 빌드하기

`cargo` 를 사용하여 직접 소스 코드로부터 빌드할 수도 있습니다:

```bash
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
```

### 종속성

#### MaaCore

`maa-cli`는 MaaCore의 인터페이스만 제공합니다. 작업을 실행하기 위해 `MaaCore` 및 리소스가 필요합니다. 이는 다음 명령어로 설치할 수 있습니다:

```bash
maa install
```

## 사용법

### 명령어

`maa-cli`의 주요 기능은 작업을 실행하는 것입니다. 작업은 `maa run <task>`를 사용하여 실행할 수 있습니다. 여기서 `<task>`는 작업의 이름이며, `maa list`를 사용하여 모든 사용 가능한 작업을 나열할 수 있습니다.

`maa help`를 통해 자세한 정보를 확인할 수 있습니다.

## 구성

### 구성 디렉토리

`maa-cli`의 모든 구성은 특정 구성 디렉토리에 위치합니다. 이 디렉토리는 `maa dir config`를 통해 얻을 수 있습니다. 구성 디렉토리는 환경 변수 `MAA_CONFIG_DIR`에 의해 변경할 수 있습니다. 아래 예시에서는 `$MAA_CONFIG_DIR`를 구성 디렉토리를 나타내는 변수로 사용합니다.

모든 구성 파일은 TOML, YAML 또는 JSON 형식으로 작성할 수 있습니다. 아래 예시에서는 TOML 형식과 파일 확장자 `.toml`을 사용합니다. 그러나 파일 확장자가 올바른 경우 이 세 가지 형식을 혼합하여 사용할 수 있습니다.

### 작업 정의

`maa-cli` 작업은 하나의 파일에 정의되어야 합니다. 이 파일은 `$MAA_CONFIG_DIR/tasks` 디렉토리에 위치해야 합니다.

#### 기본 구조

`maa-cli` 작업은 `MAA` 작업의 시퀀스입니다. 각 MAA 작업은 `type` 및 `params` 필드로 정의됩니다:

```toml
[[tasks]]
type = "StartUp" # maa 작업의 유형
params = { client_type = "Official", start_game_enabled = true } # 주어진 작업의 매개 변수
```

모든 사용 가능한 작업 유형 및 매개 변수에 대한 문서를 확인하려면 [MAA](../스키마/1.통합문서.md#asstappendtask) 문서를 참조하세요.

#### 작업 변형 및 조건

어떤 경우에는 조건에 따라 다른 매개 변수로 작업을 실행하고 싶을 수 있습니다. 작업에 대한 여러 변형을 정의하고 `condition` 필드를 사용하여 변형을 사용해야 할지를 결정할 수 있습니다. 예를 들어, 하루 중 다른 시간대에 다른 인프라 계획을 사용하고 싶을 수 있습니다:

```toml
[[tasks]]
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json" # 사용자 정의 인프라 플랜의 파일 이름

# 12:00:00 이전에는 계획 1을 사용하고, 12:00:00부터 18:00:00까지는 계획 2를 사용하고, 18:00:00 이후에는 계획 0을 사용합니다.
[[tasks.variants]]
condition = { type = "Time", end = "12:00:00" } # 시작이 정의되지 않으면 00:00:00이 됩니다.
params = { plan_index = 1 }

[[tasks.variants]]
condition = { type = "Time", start = "12:00:00", end = "18:00:00" }
params = { plan_index = 2 }

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" } # 종료가 정의되지 않으면 23:59:59가 됩니다.
params = { plan_index = 0 }
```

`condition` 필드는 변형이 사용되어야 하는지 여부를 결정하는 데 사용되며, 일치하는 변형의 `params` 필드는 작업의 매개 변수에 병합됩니다.

**참고:** `filename` 필드가 상대 경로인 경우 `$MAA_CONFIG_DIR/infrast`로부터 상대적입니다. 또한 사용자 정의 인프라 플랜 파일은 `maa-cli`가 아닌 `MaaCore`에서 읽힙니다. 따라서 파일의 형식은 반드시 `JSON`이어야 하며, 파일에 정의된 시간대는 해당 서브 플랜을 선택하는 데 사용되지 않습니다. 따라서 해당 시간대에 올바른 인프라 플랜을 사용하려면 작업의 매개 변수에 `plan_index` 필드를 지정해야 합니다. 이렇게 하면 적절한 시간대에 올바른 인프라 플랜이 사용됩니다.

`Time` 조건 이외에도 `DateTime`, `Weekday`, `Combined` 조건도 있습니다. `DateTime` 조건은 특정 날짜 및 시간대를 지정하는 데 사용되며, `Weekday` 조건은 주 중 일부 요일을 지정하는 데 사용됩니다. `Combined` 조건은 여러 조건의 조합을 지정하는 데 사용됩니다:

```toml
[[tasks]]
type = "Fight"

# 여름 이벤트에서 SL-8 전투
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
# 여름 이벤트가 아닌 경우 화요일, 목요일, 토요일에 CE-6 전투
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }
# 그렇지 않은 경우 1-7 전투
[[tasks.variants]]
params = { stage = "1-7" }
```

기본 전략을 사용하면 여러 변형이 일치하는 경우 첫 번째 변형만 사용됩니다. 그리고 조건이 지정되지 않은 경우 변형은 항상 일치합니다. 따라서 변형이 조건 없이 가장 뒤에 위치하도록 할 수 있습니다.

`strategy` 필드에 의해 전략을 변경할 수 있습니다:

```toml
[[tasks]]
type = "Fight"
strategy = "merge" # 또는 "first" (기본값)

# 일요일 밤에는 모든 유통기한이 지난 약을 사용합니다.
[[tasks.variants]]
params = { expiring_medicine = 1000 }
[tasks.variants.condition]
type = "Combined"
conditions = [
  { type = "Time", start = "18:00:00" },
  { type = "Weekday", weekdays = ["Sun"] },
]

# 기본적으로 1-7을 전투합니다.
[[tasks.variants]]
params = { stage = "1-7" }

# 여름 이벤트가 아닌 경우 화요일, 목요일, 토요일에 CE-6 전투
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 여름 이벤트에서 SL-8 전투
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
```

이 예제의 결과 스테이지는 이전과 동일해야하지만, 유통기한이 지난 약을 추가로 일요일 밤에 사용할 것입니다.
`merge` 전략을 사용하면 여러 변형이 일치하는 경우 모든 일치한 변형의 매개 변수가 병합됩니다. 여러 변형에 동일한 매개 변수가 있는 경우 마지막 것이 사용됩니다.

일치하는 변형이 없는 경우 작업이 실행되지 않습니다. 이는 일부 조건에서만 작업을 실행하려는 경우에 유용합니다:

```toml
# 18:00 이후에 상점 방문
[[tasks]]
type = "Mall"

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
```

#### 사용자 입력

어떤 경우에는 실행 시간에 일부 값을 입력 받고 싶을 수 있습니다. 작업 파일에 하드코딩하는 대신에. 예를 들어, 싸울 스테이지, 구매할 아이템 등을 지정할 수 있습니다. 이러한 값을 `Input` 또는 `Select` 유형으로 지정할 수 있습니다:

```toml
[[tasks]]
type = "Fight"

# 전투할 스테이지 선택
[[tasks.variants]]
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }

# 스테이지를 `Select` 유형으로 설정하고 대체 및 설명을 제공합니다.
[tasks.variants.params.stage]
alternatives = ["SL-6", "SL-7", "SL-8"] # 스테이지의 대체품, 적어도 하나의 대체품이 제공되어야 합니다.
description = "여름 이벤트에서 전투할 스테이지" # 입력의 설명, 선택 사항

# 입력이 없는 작업
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 전투할 스테이지를 입력합니다.
[[tasks.variants]]

# 스테이지를 `Input` 유형으로 설정하고 기본값과 설명을 제공합니다.
[tasks.variants.params.stage]
default = "1-7" # 스테이지의 기본값, 선택 사항 (지정하지 않으면 사용자가 빈 값으로 다시 프롬프트됩니다.)
description = "전투할 스테이지" # 입력의 설명, 선택 사항
```

`Input` 유형의 경우 사용자에게 값을 입력하라는 메시지가 표시됩니다. 기본값이 지정된 경우 사용자가 빈 값을 입력하면 기본값이 사용되고 그렇지 않으면 다시 프롬프트됩니다. `Select` 유형의 경우 사용자에게 대체품에서 값을 선택하라는 메시지가 표시됩니다 (색인별). 사용자 입력이 유효한 색인이 아닌 경우 다시 프롬프트됩니다. 프롬프트 및 입력은 `--batch` 옵션으로 비활성화할 수 있으며, 이 옵션은 일정에 따라 작업을 실행하는 데 유용합니다.

### `MaaCore` 관련 구성

MaaCore의 관련 구성은 `$MAA_CONFIG_DIR/asst.toml`에 위치합니다. 현재 사용 가능한 구성은 다음과 같습니다:

```toml
user_resource = true
resources = ["platform_diff/iOS"]

[connection]
type = "ADB"
adb_path = "adb"
device = "emulator-5554"
config = "CompatMac"

[static_options]
cpu_ocr = false
gpu_ocr = 1

[instance_options]
touch_mode = "MAATouch"
deployment_with_pause = false
adb_lite_enabled = false
kill_adb_on_exit = false
```


`user_resource` 필드는 사용자 리소스를 로드할지 여부를 지정하는 불리언 값입니다. `true`인 경우, 추가 리소스가 `$MAA_CONFIG_DIR/resource` 디렉토리에 로드됩니다(다른 모든 리소스 이후에). 이는 `--user-resource` 명령행 옵션과 동일합니다. 자세한 정보는 `maa help run`을 참조하세요.

`resources` 필드는 추가 리소스를 지정하는 데 사용되며, 리소스 디렉토리의 목록입니다(상대 경로가 주어진 경우 `$(maa dir resource)/resource` 디렉토리를 기준으로 합니다):

`connection` 섹션은 게임에 연결하는 방법을 지정하는 데 사용됩니다. 현재 `ADB` 및 `PlayTools` 두 가지 유형의 연결이 있습니다.

`ADB`를 사용하는 경우 `adb_path` 및 `device` 필드를 설정해야 합니다:

```toml
[connection]
type = "ADB"
adb_path = "adb" # adb 실행 파일의 경로
device = "emulator-5554" # Android 장치의 시리얼 번호
config = "General" # maa의 설정
```
`PlayTools`를 사용하는 경우 `address`를 설정해야 합니다. 이것은 `PlayCover`에서 설정한 `MaaTools`의 주소입니다. 자세한 내용은 [여기](./1.4-Mac용_에뮬레이터_지원.md)에서 확인할 수 있습니다:

```toml
[connection]
type = "PlayTools"
address = "localhost:1717" # MaaTools의 주소
config = "CompatMac" # 위와 동일
```

`ADB` 및 `PlayTools` 모두 `config` 필드를 공유합니다. 이는 `maa`의 `connect` 함수의 매개 변수이며, `macOS`에서는 기본값이 `CompatMac`, `Linux`에서는 `CompatPOSIXShell`, 다른 플랫폼에서는 `General`입니다. 추가 옵션 구성은 리소스 디렉토리의 `config.json`에서 찾을 수 있습니다.

`instance_options` 섹션은 `maa` [인스턴스 옵션](../스키마/1.통합문서.md#asstsetinstanceoption)을 구성하는 데 사용됩니다:

```toml
[instance_options]
touch_mode = "ADB" # 사용할 터치 모드, "ADB", "MiniTouch", "MAATouch" 또는 "MacPlayTools" (PlayCover 전용)일 수 있습니다.
deployment_with_pause = false # 배치 시 게임을 일시 정지할지 여부
adb_lite_enabled = false # adb-lite 사용 여부
kill_adb_on_exit = false # 종료 시 adb 종료 여부
```

**참고:** `PlayCover`로 게임에 연결하는 경우 `touch_mode`는 무시되고 `MacPlayTools`가 사용됩니다.

### `maa-cli` 관련 구성

`maa-cli` 관련 구성은 `$MAA_CONFIG_DIR/cli.toml`에 있어야 합니다. 현재 `core` 섹션만 포함되어 있습니다:

```toml
[core]
channel = "beta"
[core.components]
resource = false
```

`channel` 필드는 설치할 `MaaCore`의 채널을 지정하는 데 사용됩니다. `stable`, `beta`, `alpha` 중 하나가 될 수 있습니다. `MaaCore`의 구성 요소는 `components` 필드로 지정할 수 있으며, 이는 부울 값의 테이블입니다. 현재 `resource` 구성 요소만 지원됩니다.
