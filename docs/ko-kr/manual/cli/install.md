---
order: 2
icon: material-symbols:download
---

# 설치 및 빌드

maa-cli는 패키지 관리자, 사전 컴파일된 바이너리 파일, `cargo`를 통한 자체 컴파일 설치 등 다양한 설치 방법을 제공합니다.

## 패키지 관리자를 통한 설치

macOS 및 지원되는 Linux 배포판 사용자에게는 패키지 관리자를 통해 maa-cli를 설치하는 것을 권장합니다.

### macOS

Homebrew 사용자는 비공식 [tap](https://github.com/MaaAssistantArknights/homebrew-tap/)을 통해 maa-cli를 설치할 수 있습니다:

- 안정 버전:

  ```bash
  brew install MaaAssistantArknights/tap/maa-cli
  ```

- 불안정 베타 버전:

  ```bash
  brew install MaaAssistantArknights/tap/maa-cli-beta
  ```

### Linux

- Arch Linux 사용자는 [AUR 패캐지](https://aur.archlinux.org/packages/maa-cli/)를 설치할 수 있습니다:

  ```bash
  yay -S maa-cli
  ```

- ❄️ Nix 사용자는 다음 명령어를 실행할 수 있습니다:

  ```bash
  # 안정 버전
  nix run nixpkgs#maa-cli
  ```

  ```bash
  # 매일 최신화 버전
  nix run github:Cryolitia/nur-packages#maa-cli-nightly
  ```

  안정 버전은 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-cli/package.nix)에 포함되어 있으며, `nixpkgs`의 Rust 도구 체인을 사용합니다. 매일 빌드는 [NUR](https://github.com/Cryolitia/nur-packages/blob/master/pkgs/maa-assistant-arknights/maa-cli.nix)에 위치하며, Beta 채널의 Rust 도구 체인을 사용하고 GitHub Action을 통해 매일 자동으로 업데이트 및 빌드 검증을 수행합니다.

- Linux에서 Homebrew를 사용하는 경우 위의 macOS 설치 방법을 참조하세요.

## 사전 컴파일된 바이너리 파일

지원되지 않는 시스템을 사용하거나 패키지 관리자를 사용하지 않으려는 경우, 아래 링크를 통해 해당 플랫폼의 사전 컴파일된 바이너리 파일을 다운로드하고, 압축을 풀어 실행 파일을 `PATH`에 추가하여 사용할 수 있습니다.

- [macOS](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-v0.4.5-universal-apple-darwin.zip)
- [Linux x86_64 (x64, amd64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-v0.4.5-x86_64-unknown-linux-gnu.tar.gz)
- [Linux aarch64 (arm64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-v0.4.5-aarch64-unknown-linux-gnu.tar.gz)
- [Windows x86_64 (x64, amd64)](https://github.com/MaaAssistantArknights/maa-cli/releases/latest/download/maa_cli-v0.4.5-x86_64-pc-windows-msvc.zip)

위의 목록에 없는 플랫폼의 경우, 직접 컴파일하여 설치할 수 있습니다(아래 참조).

## 컴파일 설치

Rust 개발자는 `cargo`를 통해 maa-cli를 직접 컴파일하여 설치할 수 있습니다:

- 안정 버전:

  ```bash
  cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --tag stable --locked
  ```

- 개발 버전:

  ```bash
  cargo install --git https://github.com/MaaAssistantArknights/maa-cli.git --bin maa --locked
  ```

### 컴파일 옵션

소스에서 컴파일할 때 `--no-default-features`를 통해 기본 기능을 비활성화하고 `--features`를 통해 특정 기능을 활성화할 수 있습니다. 현재 사용 가능한 기능은 다음과 같습니다:

- `cli_installer`: `maa self update` 명령어를 활성화하여 자체 업데이트를 가능하게 합니다. 이 기능은 기본적으로 활성화되어 있습니다.
- `core_installer`: `maa install` 및 `maa update` 명령어를 활성화하여 MaaCore 및 리소스를 설치하고 업데이트할 수 있습니다. 이 기능은 기본적으로 활성화되어 있습니다.
- `git2`: `libgit2` 리소스 업데이트 백엔드를 제공합니다. 이 기능은 기본적으로 활성화되어 있습니다.
- `vendored-openssl`: 시스템의 `openssl` 라이브러리를 사용하는 대신 자체적으로 `openssl` 라이브러리를 컴파일합니다. 이 기능은 기본적으로 비활성화되어 있으며, 시스템에 `openssl` 라이브러리가 없거나 버전이 너무 낮을 때 활성화합니다.

## MaaCore 및 리소스 설치

maa-cli는 명령줄 인터페이스만 제공하며, 작업을 실행하려면 MaaCore와 리소스가 필요합니다. maa-cli를 설치한 후, 다음 명령어를 통해 MaaCore와 리소스를 설치할 수 있습니다:

```bash
maa install
```

패키지 관리자를 통해 설치한 사용자는 패키지 관리자를 통해 MaaCore를 설치할 수 있습니다:

- Homebrew：

  ```bash
  brew install MaaAssistantArknights/tap/maa-core
  ```

- Arch Linux：

  ```bash
  yay -S maa-assistant-arknights
  ```

- Nix：

  ```bash
  nix-env -iA nixpkgs.maa-assistant-arknights
  ```

**주의**: 패키지 관리자를 통해 maa-cli를 설치한 경우에만 패키지 관리자를 통해 MaaCore를 설치할 수 있습니다. 그렇지 않으면 `maa install` 명령어를 사용해야 합니다. 또한, `maa install` 명령어는 공식적으로 사전 컴파일된 MaaCore를 다운로드합니다. 패키지 관리자를 통해 설치된 MaaCore는 컴파일 옵션과 종속성 버전이 공식 사전 컴파일 버전과 다를 수 있습니다. 이는 maa-cli의 사용에 영향을 미치지 않지만, MaaCore의 기능과 성능에 차이가 있을 수 있습니다. 예를 들어, 패키지 관리자를 통해 설치된 MaaCore는 최신 버전의 `fastdeploy`를 사용하고, 공식 사전 컴파일된 MaaCore는 구버전의 `fastdeploy`를 사용합니다. 최신 버전의 `fastdeploy`에서는 로그를 숨길 수 있어 불필요한 로그 출력을 줄일 수 있습니다.
