---
order: 1
icon: material-symbols:download
---

# 설치 및 빌드

maa-cli는 사전 컴파일된 바이너리, 패키지 관리자, 그리고 `cargo`를 이용한 소스 컴파일 등 다양한 설치 방법을 제공합니다.

## 사전 컴파일된 바이너리

maa-cli를 가장 쉽게 설치하는 방법은 원클릭 설치 스크립트를 사용하는 것입니다:

::: tabs#pre-compile

@tab:active Linux 및 macOS

```bash
curl -fsSL https://raw.githubusercontent.com/MaaAssistantArknights/maa-cli/main/install.sh | bash
```

@tab Windows (PowerShell)

```powershell
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/MaaAssistantArknights/maa-cli/main/install.ps1" -OutFile install.ps1; .\install.ps1
```

:::

이후 `maa self update`를 통해 maa-cli를 업데이트할 수 있습니다.

사용 중인 플랫폼이 위 목록에 없다면, [소스에서 직접 빌드](#소스에서-직접-빌드)를 시도해 볼 수 있습니다.

## 패키지 관리자를 통해 설치

macOS 및 지원되는 Linux 배포판 사용자는 패키지 관리자를 사용하여 maa-cli를 설치할 수 있습니다.

### macOS

Homebrew 사용자는 비공식 [tap](https://github.com/MaaAssistantArknights/homebrew-tap/)을 통해 maa-cli를 설치할 수 있습니다:

::: code-tabs

@tab:active 안정 버전

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli
```

@tab 불안정 버전/사전 출시 버전

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli-beta
```

:::

### Linux

Arch, Nix, 그리고 Linux Homebrew 사용자는 패키지 관리자를 통해 maa-cli를 설치할 수 있습니다.

#### Arch Linux

[AUR 패키지](https://aur.archlinux.org/packages/maa-cli/)를 설치할 수 있습니다:

::: code-tabs

@tab:active paru

```bash :no-line-numbers
paru -S maa-cli
```

@tab yay

```bash :no-line-numbers
yay -S maa-cli
```

:::

#### ❄️ Nix

사용자는 직접 실행할 수 있습니다:

::: code-tabs

@tab:active 안정 버전

```bash :no-line-numbers
nix run nixpkgs#maa-cli
```

@tab 나이틀리 빌드

```bash :no-line-numbers
nix run github:Cryolitia/nur-packages#maa-cli-nightly
```

:::

안정 버전은 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix)에 패키징되어 있으며, `nixpkgs`의 Rust 툴체인을 사용합니다. 나이틀리 빌드는 [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix)에 있으며, 베타 채널의 Rust 툴체인을 사용하며 GitHub Action에 의해 매일 자동으로 업데이트되고 빌드가 검증됩니다.

#### Homebrew

Linux에서 Homebrew를 사용하는 사용자는 위의 macOS 설치 방법을 참조하십시오.

#### 기타 배포판

[사전 컴파일된 바이너리](#사전-컴파일된-바이너리)를 사용하거나 [소스에서 직접 빌드](#소스에서-직접-빌드)하십시오.

관심 있는 개발자분들이 더 많은 배포판의 공식 저장소나 사용자 저장소에 maa-cli를 제출해 주시는 것을 환영합니다!

## 소스에서 직접 빌드

Rust 개발자는 `cargo`를 통해 maa-cli를 직접 빌드하고 설치할 수 있습니다:

::: code-tabs

@tab:active 안정 버전

```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --tag stable --locked
```

@tab 개발 버전

```bash :no-line-numbers
cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
```

:::

::: warning
현재 maa-cli의 최소 지원 Rust 버전(MSRV)은 1.88입니다. MSRV는 언제든지 변경될 수 있으므로, 최상의 경험을 위해 항상 최신 Rust 툴체인을 사용하는 것을 권장합니다.
:::

### 빌드 옵션

소스에서 빌드할 때 `--no-default-features`로 기본 기능을 비활성화하고 `--features`로 특정 기능을 활성화할 수 있습니다. 사용 가능한 기능은 다음과 같습니다:

- `cli_installer`: 자가 업데이트를 위한 `maa self update` 명령을 활성화합니다. 기본적으로 활성화되어 있습니다.
- `core_installer`: MaaCore 및 리소스를 설치하고 업데이트하기 위한 `maa install` 및 `maa update` 명령을 활성화합니다. 기본적으로 활성화되어 있습니다.
- `git2`: 리소스 업데이트를 위한 `libgit2` 백엔드를 제공합니다. 기본적으로 활성화되어 있습니다.

## MaaCore 및 리소스 설치

maa-cli는 명령줄 인터페이스만 제공하며, 작업을 실행하려면 MaaCore와 리소스가 필요합니다.

설치 방법과 플랫폼에 따라 작업이 달라집니다:

::: tabs#maacore

@tab:active 사전 컴파일
사전 컴파일된 바이너리를 사용하거나 직접 빌드한 사용자의 경우, maa-cli를 통해 설치 및 업데이트를 할 수 있습니다:

```bash :no-line-numbers
maa install
```

@tab Windows
Windows 플랫폼 사용자의 경우, `maa install` 명령을 실행하기 전에 관리자 권한으로 명령 프롬프트나 PowerShell에서 다음 명령을 실행하여 필수 구성 요소인 VC++ 런타임을 설치하십시오:

```bat :no-line-numbers
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
```

그런 다음 `maa install`을 실행합니다.

@tab Arch
maa-cli를 사용하여 사전 컴파일된 MaaCore를 설치할 수 있습니다:

```bash :no-line-numbers
maa install
```

또는 [AUR](https://aur.archlinux.org/packages/maa-assistant-arknights/)을 통해 maa-core를 설치할 수도 있습니다:

```bash :no-line-numbers
paru -S maa-assistant-arknights
```

또는

```bash :no-line-numbers
yay -S maa-assistant-arknights
```

@tab Nix
Nix의 maa-cli는 MaaCore에 대한 강제 종속성이 있으므로, Nix 사용자는 수동으로 MaaCore를 설치할 필요가 없으며, 해서도 안 됩니다.

:::

::: warning
`maa install`은 MAA 공식 사전 컴파일된 MaaCore를 다운로드합니다. 반면, 패키지 관리자로 설치된 MaaCore는 공식 버전과 다른 컴파일 옵션 및 종속성 버전을 사용할 수 있으며, 이로 인해 성능 및 기능에 약간의 차이가 발생할 수 있습니다.
:::
