---
order: 1
icon: material-symbols:download
---

# 설치 및 빌드

maa-cli는 패키지 관리자, 사전 컴파일된 바이너리 파일, `cargo`를 통한 자체 컴파일 설치 등 다양한 설치 방법을 제공합니다.

## 패키지 관리자를 통한 설치

macOS 및 지원되는 Linux 배포판 사용자에게는 패키지 관리자를 통해 maa-cli를 설치하는 것을 권장합니다.

### macOS

Homebrew 사용자는 비공식 [tap](https://github.com/MaaAssistantArknights/homebrew-tap/)을 통해 maa-cli를 설치할 수 있습니다:

::: code-tabs

@tab:active 안정 버전

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli
```

@tab 불안정 버전/사전 릴리스 버전

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-cli-beta
```

:::

### Linux

현재 Arch와 Nix 사용자를 위한 패키지 관리자 기반 배포를 지원합니다.

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

다음 명령어를 직접 실행할 수 있습니다:

::: code-tabs

@tab:active 안정 버전

```bash :no-line-numbers
nix run nixpkgs#maa-cli
```

@tab 매일 빌드

```bash :no-line-numbers
nix run github:Cryolitia/nur-packages#maa-cli-nightly
```

:::

안정 버전은 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix)에 포함되어 있으며, `nixpkgs`의 Rust 도구 체인을 사용합니다. 매일 빌드는 [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix)에 위치하며, Beta 채널의 Rust 도구 체인을 사용하고 GitHub Action을 통해 매일 자동으로 업데이트 및 빌드 검증을 수행합니다.

#### 기타 배포판

Linux에서 Homebrew를 사용하는 경우 위의 macOS 설치 방법을 참조하세요.

그렇지 않으면 [사전 컴파일된 바이너리 파일](#사전-컴파일된-바이너리-파일)을 사용하거나 [직접 컴파일 설치](#컴파일-설치)를 하세요.

관심 있는 개발자분들이 maa-cli를 더 많은 배포판의 공식 저장소나 사용자 저장소에 제출해 주시면 감사하겠습니다!

## 사전 컴파일된 바이너리 파일

지원되지 않는 시스템을 사용하거나 패키지 관리자를 사용하지 않으려는 경우, 아래 링크를 통해 해당 플랫폼의 사전 컴파일된 바이너리 파일을 다운로드하고, 압축을 풀어 실행 파일을 `PATH`에 추가하여 사용할 수 있습니다.

::: tabs#pre-compile

@tab:active macOS
[다운로드](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-universal-apple-darwin.zip)

@tab Linux
아키텍처를 선택하세요

- [x64/x86_64/amd64](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-x86_64-unknown-linux-gnu.tar.gz)
- [aarch64/arm64](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-aarch64-unknown-linux-gnu.tar.gz)

@tab Windows
아키텍처를 선택하세요

- [x64/x86_64/amd64](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-x86_64-pc-windows-msvc.zip)
- [aarch64/arm64](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-aarch64-pc-windows-msvc.zip)

:::

위의 목록에 없는 플랫폼의 경우, 직접 컴파일하여 설치할 수 있습니다(아래 참조).

## 컴파일 설치

Rust 개발자는 `cargo`를 통해 maa-cli를 직접 컴파일하여 설치할 수 있습니다:

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

### 컴파일 옵션

소스에서 컴파일할 때 `--no-default-features`를 통해 기본 기능을 비활성화하고 `--features`를 통해 특정 기능을 활성화할 수 있습니다. 현재 사용 가능한 기능은 다음과 같습니다:

- `cli_installer`: `maa self update` 명령어를 활성화하여 자체 업데이트를 가능하게 합니다. 이 기능은 기본적으로 활성화되어 있습니다.
- `core_installer`: `maa install` 및 `maa update` 명령어를 활성화하여 MaaCore 및 리소스를 설치하고 업데이트할 수 있습니다. 이 기능은 기본적으로 활성화되어 있습니다.
- `git2`: `libgit2` 리소스 업데이트 백엔드를 제공합니다. 이 기능은 기본적으로 활성화되어 있습니다.
- `vendored-openssl`: 시스템의 `openssl` 라이브러리를 사용하는 대신 자체적으로 `openssl` 라이브러리를 컴파일합니다. 이 기능은 기본적으로 비활성화되어 있으며, 시스템에 `openssl` 라이브러리가 없거나 버전이 너무 낮을 때 활성화합니다.

## MaaCore 및 리소스 설치

::: warning
패키지 관리자를 통해 maa-cli를 설치한 경우에만 패키지 관리자를 통해 MaaCore를 설치할 수 있습니다. 그렇지 않으면 `maa install` 명령어를 사용해야 합니다.  
또한, `maa install`로 다운로드하는 것은 공식적으로 사전 컴파일된 MaaCore이며, 패키지 관리자를 통해 설치된 MaaCore는 공식 사전 컴파일 버전과 다른 컴파일 옵션 및 종속성 버전을 사용할 수 있어 성능과 기능에 약간의 차이가 있을 수 있습니다.
:::

maa-cli는 명령줄 인터페이스만 제공하며, 작업을 실행하려면 MaaCore와 리소스가 필요합니다. maa-cli를 설치한 후, 다음 명령어를 통해 MaaCore와 리소스를 설치할 수 있습니다:

```bash :no-line-numbers
maa install
```

플랫폼에 따라 작업이 다릅니다.

::: tabs#maacore

@tab Windows
Windows 사용자의 경우, `maa install` 명령어를 실행하기 전에 관리자 권한으로 명령 프롬프트 또는 PowerShell에서 다음 명령어를 실행하여 필수 도구인 VC++을 설치하세요.

```bat :no-line-numbers
winget install "Microsoft.VCRedist.2015+.x64" --override "/repair /passive /norestart" --uninstall-previous --accept-package-agreements --force
```

그런 다음 `maa install`을 실행하세요.

@tab Homebrew
Homebrew를 통해 직접 maa-core를 설치할 수 있습니다.

```bash :no-line-numbers
brew install MaaAssistantArknights/tap/maa-core
```

@tab Arch
[AUR](https://aur.archlinux.org/packages/maa-assistant-arknights/)을 통해 maa-core를 설치할 수 있습니다.
::: code-tabs

@tab:active paru

```bash :no-line-numbers
paru -S maa-assistant-arknights
```

@tab yay

```bash :no-line-numbers
yay -S maa-assistant-arknights
```

:::

@tab Nix
Nix의 maa-cli는 MaaCore에 대한 필수 종속성이 있습니다. 따라서 Nix 사용자는 MaaCore를 수동으로 설치할 필요가 없으며, 설치해서도 안 됩니다.

:::
