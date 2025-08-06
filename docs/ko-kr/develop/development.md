---
order: 1
icon: iconoir:developer
---

# 개발 환경 구축

::: tip
본 페이지는 주로 PR 과정 및 MAA의 파일 포맷 요구사항을 설명합니다. MAA의 실행 로직 변경에 대한 구체적인 내용은 [프로토콜 문서](../protocol/)를 참고하세요.
:::

## Github Pull Request 진행 과정

### 프로그래밍을 잘 모르지만, json 파일과 docs 문서를 수정하고싶어요. 어떻게 해야하나요?

[웹 기반 PR 가이드](./pr-tutorial.md)를 참고하세요! (GitHub.com 웹사이트에서만 가능합니다)

### 프로그래밍을 할 줄 알지만 GitHub/C++/...에 익숙하지 않아요. 어떻게 해야 하나요?

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
    _（maadeps-download.py 파일은 프로젝트 루트에 위치）_

    ```cmd
    python maadeps-download.py
    ```

5. 개발 환경 구성

    - Visual Studio 2022 Community 설치 시 `C++ 데스크톱 개발` 및 `.NET 데스크톱 개발` 필수 선택

6. MAA.sln 파일 더블클릭 → Visual Studio에서 프로젝트 자동 로드
7. VS 설정

    - 상단 구성에서 RelWithDebInfo x64 선택 (릴리스 빌드/ARM 플랫폼 시 생략)
    - MaaWpfGui 우클릭 → 속성 → 디버그 → 네이티브 디버깅 활성화 (C++ 코어 중단점 사용 가능)

8. 이제 자유롭게 ~~개조~~ 개발 시작!
9. 주기적 커밋 (메시지 필수 작성)  
   Git 초보자는 dev 브랜치 대신 새 브랜치 생성 권장:

    ```bash
    git branch your_own_branch
    git checkout your_own_branch
    ```

    dev 브랜치 업데이트 영향에서 자유로움

10. 개발 완료 후 변경사항 원격 저장소로 푸시:

    ```bash
    git push origin dev
    ```

11. [MAA 메인 저장소](https://github.com/MaaAssistantArknights/MaaAssistantArknights)에서 Pull Request 제출 (master 대신 dev 브랜치 지정 필수)
12. 업스트림 저장소 변경사항 동기화 방법:

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

    4. 단계 7, 8, 9, 10 반복 수행

::: tip
Visual Studio 실행 시 Git 작업은 "Git 변경" 탭에서 명령어 없이 처리 가능
:::

## MAA 파일의 포매팅 요구 사항

MAA는 리포지토리의 코드 및 리소스 파일들을 아름답고 일관적으로 유지하기 위해 일련의 포매팅 도구를 사용합니다.

제출하기 전에 포맷을 지정했거나 [Pre-commit Hooks를 사용하여 자동 포매팅을 활성화](#pre-commit-hooks를-사용하여-자동-포매팅을-활성화)를 했는지 확인하세요.

현재 활성화된 포매팅 도구는 다음과 같습니다:

| 파일 유형 | 포매팅 도구 |
| --- | --- |
| C++ | [clang-format](https://clang.llvm.org/docs/ClangFormat.html) |
| Json/Yaml | [Prettier](https://prettier.io/) |
| Markdown | [markdownlint](https://github.com/DavidAnson/markdownlint-cli2) |

### Pre-commit Hooks를 사용하여 자동 포매팅을 활성화

1. Python 및 Node 환경이 컴퓨터에 설치되어 있는지 확인하세요.

2. 프로젝트 루트 디렉터리에서 다음 명령을 실행하세요.

    ```bash
    pip install pre-commit
    pre-commit install
    ```

pip 설치 후에도 Pre-commit을 실행할 수 없다면, PIP 설치 경로가 PATH에 추가되었는지 확인하세요.

이제, 매번 커밋할 때마다 포매팅 도구가 자동으로 실행되어 코드 형식이 규칙에 맞는지 확인합니다.

## Visual Studio에서 clang-format 사용 설정

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

## GitHub codespace를 사용하여 온라인으로 개발하기

GitHub codespace를 사용하여 자동으로 C++ 개발 환경을 구성하세요.

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg?color=green)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights)

그런 다음 vscode의 지침을 따르거나 [Linux 컴파일 가이드](./linux-tutorial.md)를 참고하여 GCC 12 및 CMake 프로젝트를 설정하세요.
