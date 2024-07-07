---
order: 5
icon: ph:question-fill
---

# 자주 묻는 질문

## 1. macOS에서 `$HOME/.config/maa`를 설정 파일 디렉토리로 사용하려면 어떻게 해야 하나요?

Rust 라이브러리 [Directories](https://github.com/dirs-dev/directories-rs/)가 macOS에서 Apple 스타일의 디렉토리를 기본으로 사용하기 때문에, maa-cli도 기본적으로 Apple 스타일의 설정 디렉토리를 사용합니다. 하지만 명령줄 프로그램의 경우 XDG 스타일 디렉토리가 더 적합합니다. XDG 스타일 디렉토리를 사용하려면 `XDG_CONFIG_HOME` 환경 변수를 설정하면 됩니다. 예를 들어, `"export XDG_CONFIG_HOME="$HOME/.config"` 명령어를 사용하면 maa-cli가 XDG 스타일 설정 디렉토리를 사용하게 됩니다. 환경 변수를 설정하지 않고 XDG 스타일 설정 디렉토리를 사용하려면, 다음 명령어로 심볼릭 링크를 생성할 수 있습니다:

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```

## 2. 실행 중 이상한 로그가 나타납니다. 이걸 어떻게 끌 수 있나요?

MaaCore의 종속성인 `fastdeploy`가 일부 로그를 출력합니다. 따라서 maa-cli를 실행할 때 다음과 같은 로그를 볼 수 있습니다:

```plaintext
[INFO] ... /fastdeploy/runtime.cc(544)::Init Runtime initialized with Backend::ORT in Device::CPU.`
```

이 로그는 `fastdeploy`에서 출력하는 것으로, 공식적으로 사전 컴파일된 MaaCore에서는 이 로그를 끌 수 없습니다. 그러나 패키지 관리자를 통해 설치된 maa-cli를 사용하는 경우, 패키지 관리자가 제공하는 MaaCore를 설치해보세요. 패키지 관리자가 제공하는 MaaCore는 최신 버전의 `fastdeploy`를 사용하며, 기본적으로 로그가 비활성화되어 있습니다.

## 3. macOS에서 maa-cli로 실행한 PlayCover 게임 클라이언트가 전투를 대리할 수 없고 로그라이크 모드를 완료할 수 없습니다

알 수 없는 이유로, 터미널을 통해 실행한 PlayCover 게임 클라이언트는 전투를 대리할 수 없고 로그라이크 모드를 완료할 수 없습니다. 하지만 이 문제는 터미널에서 실행한 maa-cli에만 해당됩니다. 터미널에서 상호작용할 때는 게임을 수동으로 실행해야 할 수도 있습니다. cron 등의 방법으로 maa-cli를 실행하는 경우, 이 문제는 영향을 미치지 않습니다.

<!-- markdownlint-disable-file MD013 -->
