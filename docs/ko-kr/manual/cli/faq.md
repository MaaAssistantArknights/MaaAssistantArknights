---
order: 4
icon: ph:question-fill
---

# 자주 묻는 질문

## 1. macOS에서 `$HOME/.config/maa`를 설정 파일 디렉토리로 사용하려면 어떻게 해야 하나요?

Rust 라이브러리 [Directories](https://github.com/dirs-dev/directories-rs/)가 macOS에서 Apple 스타일의 디렉토리를 기본으로 사용하기 때문에, maa-cli도 기본적으로 Apple 스타일의 설정 디렉토리를 사용합니다. 하지만 명령줄 프로그램의 경우 XDG 스타일 디렉토리가 더 적합합니다. XDG 스타일 디렉토리를 사용하려면 `XDG_CONFIG_HOME` 환경 변수를 설정하면 됩니다. 예를 들어, `"export XDG_CONFIG_HOME="$HOME/.config"` 명령어를 사용하면 maa-cli가 XDG 스타일 설정 디렉토리를 사용하게 됩니다. 환경 변수를 설정하지 않고 XDG 스타일 설정 디렉토리를 사용하려면, 다음 명령어로 심볼릭 링크를 생성할 수 있습니다:

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```
