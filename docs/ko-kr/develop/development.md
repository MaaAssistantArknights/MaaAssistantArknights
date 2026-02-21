---
order: 1
icon: iconoir:developer
---

# 개발 가이드

::: tip
본 페이지는 주로 PR 과정 및 MAA의 파일 포맷 요구사항을 설명합니다. MAA의 실행 로직 변경에 대한 구체적인 내용은 [프로토콜 문서](../protocol/)를 참고하세요.
:::

::: tip
[DeepWiki에 문의하여](https://deepwiki.com/MaaAssistantArknights/MaaAssistantArknights) MAA 프로젝트의 전체적인 아키텍처를 개략적으로 이해할 수 있습니다.
:::

## 프로그래밍을 잘 모르지만, json 파일과 docs 문서를 수정하고싶어요. 어떻게 해야하나요?

[웹 기반 PR 가이드](./pr-tutorial.md)를 참고하세요! (GitHub.com 웹사이트에서만 가능합니다)

## 몇 줄의 코드만 간단하게 수정하고 싶지만 환경 설정이 너무 복잡하고, 순수 웹 편집도 불편해요. 어떻게 해야 하나요?

[GitHub Codespaces](https://github.com/codespaces) 온라인 개발 환경을 사용해보세요!

다음과 같은 다양한 개발 환경을 사전에 설정했습니다：

- 빈 환경 (순수 Linux 컨테이너) (기본값)

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2Fdevcontainer.json)

- 경량 환경, 문서 사이트 프론트엔드 개발에 적합

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F0%2Fdevcontainer.json)

- 전체 환경, MAA Core 관련 개발에 적합 (사용 권장하지 않음, 로컬 개발 권장, 관련 환경을 완전히 설정, 다음 섹션 참조)

  [![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights?devcontainer_path=.devcontainer%2F1%2Fdevcontainer.json)

## 완전한 환경 설정 (Windows)

1. 이전에 fork 한 기록이 있다면 저장소 Settings 맨 아래에서 삭제
2. [MAA 메인 저장소](https://github.com/MaaAssistantArknights/MaaAssistantArknights)에서 Fork → Create fork 클릭
3. 본인 저장소의 dev 브랜치를 서브모듈 포함 클론:

   ```bash
   git clone --recurse-submodules <저장소 git 링크> -b dev
   ```

   ::: warning
   Visual Studio 등 --recurse-submodules 미지원 Git GUI 사용 시, 클론 후 다음 실행:

   ```bash
   git submodule update --init
   ```

   :::

4. 사전 빌드된 외부 라이브러리 다운로드

   **Python 환경 필요 (별도 설치 필요)**

   ```cmd
   python tools/maadeps-download.py
   ```

5. 개발 환경 구성
   - `CMake` 다운로드 및 설치
   - Visual Studio 2026 Community 설치 시 `C++ 데스크톱 개발` 및 `.NET 데스크톱 개발` 필수 선택

6. cmake 프로젝트 구성 실행

   ```cmd
   cmake --preset windows-x64
   ```

7. `build/MAA.slnx` 파일을 더블 클릭하여 엽니다. Visual Studio가 자동으로 전체 프로젝트를 로드합니다.
8. VS 설정
   - 상단 구성에서 `Debug` `x64` 선택
   - `MaaWpfGui` 우클릭 → 시작 프로젝트로 설정
   - F5 키를 눌러 실행

   ::: tip
   Win32Controller(Windows 창 제어) 관련 기능을 디버깅하려면 [MaaFramework Releases](https://github.com/MaaXYZ/MaaFramework/releases)에서 해당 플랫폼 압축 파일을 다운로드하고, `bin` 디렉토리의 `MaaWin32ControlUnit.dll`을 MAA DLL과 같은 디렉토리(예: `build/bin/Debug`)에 배치해야 합니다. 자동 다운로드 스크립트 PR 환영!
   :::

9. 이제 자유롭게 ~~개조~~ 개발 시작!
10. 주기적 커밋 (메시지 필수 작성)  
    Git 초보자는 dev 브랜치 대신 새 브랜치 생성 권장:

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    dev 브랜치 업데이트 영향에서 자유로움

11. 개발 완료 후 변경사항 원격 저장소로 푸시:

    ```bash
    git push origin dev
    ```

12. [MAA 메인 저장소](https://github.com/MaaAssistantArknights/MaaAssistantArknights)에서 Pull Request 제출 (master 대신 dev 브랜치 지정 필수)
13. 업스트림 저장소 변경사항 동기화 방법:
    1. 업스트림 저장소 추가:

       ```bash
       git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
       ```

    2. 변경사항 가져오기:

       ```bash
       git fetch upstream
       ```

    3. 리베이스(권장) 또는 병합:

       ```bash
       git rebase upstream/dev
       ```

       또는

       ```bash
       git merge
       ```

    4. 단계 8, 9, 10, 11 반복 수행

::: tip
Visual Studio 실행 시 Git 작업은 "Git 변경" 탭에서 명령어 없이 처리 가능
:::

## VSCode로 개발하기 (선택 사항)

::: warning
**Visual Studio를 사용한 개발을 권장합니다.** MAA 프로젝트는 주로 Visual Studio를 기반으로 구축되며, 위의 완전한 환경 설정 과정이 모든 개발 요구를 충족하여 최상의 즉시 사용 가능한 경험을 제공합니다. VS Code 워크플로는 VS Code + CMake + clangd에 이미 익숙한 개발자를 위한 대안으로만 제공되며, 설정 난이도가 상대적으로 높습니다.
:::

VSCode를 선호한다면 CMake, clangd 등의 확장을 사용해 코드 완성, 탐색, 디버깅을 할 수 있습니다. 위 1–6단계(클론, 의존성, CMake 설정)를 완료한 후 다음 단계로 설정하세요.

### 추천 확장

VS Code Marketplace에서 설치:

| 확장                                                                                                | 용도                                                      |
| --------------------------------------------------------------------------------------------------- | --------------------------------------------------------- |
| [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)            | CMake 설정, 빌드, 디버깅 통합                             |
| [clangd](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd) | C++ IntelliSense, 코드 탐색, 진단 (LSP 기반)              |
| [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)                     | C++ 프로그램 디버깅 (CMake Tools 또는 launch.json과 연동) |

::: tip
clangd 사용 시 C/C++ 확장의 IntelliSense를 비활성화(`C_Cpp.intelliSenseEngine`을 `disabled`로 설정)하는 것을 권장합니다. 충돌을 피하기 위함입니다.
:::

### 설정 단계

1. VSCode에서 프로젝트 루트 열기
2. **CMake Tools**: 상태 표시줄에서 Configure Preset(예: `windows-x64`, `linux-x64`) 선택 후 Build Preset으로 빌드 실행
3. **clangd**: Linux/macOS에서는 프리셋에서 `CMAKE_EXPORT_COMPILE_COMMANDS`가 활성화되어 clangd가 `build/compile_commands.json`을 자동 사용합니다. Windows에서 clangd의 코드 완성 및 탐색 기능을 사용하려면 먼저 `compile_commands.json`을 생성해야 합니다:

   ::: warning Windows에서 clangd 설정 안내
   - VS Installer에서 **C++용 Clang 컴파일러 (Windows)**(clang-cl)를 선택하여 설치
   - `windows-x64-clang` 프리셋으로 전환 후 Configure를 한 번 실행하면 `build/`에 `compile_commands.json`이 생성되며, 이후 clangd가 동작합니다
   - **해당 프리셋은 clang-cl을 사용하므로 MSVC가 아니어서 실제 빌드 산출물을 생성할 수 없습니다**. 실제 빌드 시에는 `windows-x64`로 다시 전환해야 합니다
   - clangd는 clang-cl의 컴파일 정보로 분석하므로, 일부 코드(예: MSVC 전용 확장)에서 오류가 표시될 수 있으나 무시해도 되며 실제 MSVC 빌드에는 영향을 주지 않습니다

   **명령줄에서 프리셋 전환 예시** (프로젝트 루트에서 실행):

   ```cmd
   rem compile_commands.json 생성 (Configure만, 빌드 없음)
   cmake --preset windows-x64-clang

   rem MSVC로 전환하여 실제 빌드 수행
   cmake --preset windows-x64
   cmake --build --preset windows-x64-RelWithDebInfo
   ```

   :::

4. **디버깅**: 프로젝트에 `.vscode/launch.json`이 포함되어 MaaWpfGui 또는 Debug Demo를 바로 실행 가능

### 빌드 및 디버깅 단축키

- **빌드**: `Ctrl+Shift+B` 또는 CMake Tools 상태 표시줄
- **디버깅**: F5 또는 Run and Debug 패널에서 구성 선택

## MAA 파일의 포매팅 요구 사항

MAA는 리포지토리의 코드 및 리소스 파일들을 아름답고 일관적으로 유지하기 위해 일련의 포매팅 도구를 사용합니다.

제출하기 전에 포맷을 지정했거나 [Pre-commit Hooks를 사용하여 자동 포매팅을 활성화](#pre-commit-hooks를-사용하여-자동-포매팅을-활성화)를 했는지 확인하세요.

현재 활성화된 포매팅 도구는 다음과 같습니다:

| 파일 유형 | 포매팅 도구                                                     |
| --------- | --------------------------------------------------------------- |
| C++       | [clang-format](https://clang.llvm.org/docs/ClangFormat.html)    |
| JSON/YAML | [Prettier](https://prettier.io/)                                |
| Markdown  | [markdownlint](https://github.com/DavidAnson/markdownlint-cli2) |

### Pre-commit Hooks를 사용하여 자동 포매팅을 활성화

1. Python 및 Node 환경이 컴퓨터에 설치되어 있는지 확인하세요.

2. 프로젝트 루트 디렉터리에서 다음 명령을 실행하세요.

   ```bash
   pip install pre-commit
   pre-commit install
   ```

pip 설치 후에도 Pre-commit을 실행할 수 없다면, PIP 설치 경로가 PATH에 추가되었는지 확인하세요.

이제, 매번 커밋할 때마다 포매팅 도구가 자동으로 실행되어 코드 형식이 규칙에 맞는지 확인합니다.

### Visual Studio에서 clang-format 사용 설정

1. clang-format 20.1.0 또는 그 이상 버전을 설치합니다.

   ```bash
   python -m pip install clang-format
   ```

2. Everything 등의 도구를 사용하여 clang-format.exe의 설치 위치를 찾습니다. 참고로 Anaconda를 사용하는 경우 clang-format.exe는 YourAnacondaPath/Scripts/clang-format.exe에 설치됩니다.

3. Visual Studio에서 `도구`-`옵션`을 검색하여 `clang-format`을 클릭합니다.
4. `ClangFormat 지원 활성화`를 클릭하고 아래의 `사용자 정의 clang-format.exe 파일 사용`을 선택한 다음 2단계에서 찾은 `clang-format.exe`를 선택합니다.

![Visual Studio에서 clang-format 사용 설정](/images/zh-cn/development-enable-vs-clang-format.png)

이제 Visual Studio에서 C++20 구문을 지원하는 clang-format을 사용할 수 있습니다!

또한 프로젝트 루트에서 `tools\ClangFormatter\clang-formatter.py`를 실행하여 직접 clang-format을 호출하여 포맷팅할 수도 있습니다.

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`
