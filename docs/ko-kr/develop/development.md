---
order: 1
icon: iconoir:developer
---

# 개발 환경 구축

## Github Pull Request 진행 과정

### 프로그래밍을 잘 모르지만, json 파일과 docs 문서를 수정하고싶어요. 어떻게 해야하나요?

[웹 기반 PR 가이드](./pr-tutorial.md)를 참고하세요! (GitHub.com 웹사이트에서만 가능합니다)

### 프로그래밍을 할 줄 알지만 GitHub/C++/...에 익숙하지 않아요. 어떻게 해야 하나요?

1. 이전에 `fork`한 적이 있다면 먼저 본인의 저장소의 `Settings`로 이동하여 가장 아래로 스크롤하여 삭제합니다.
2. [MAA 메인 레포](https://github.com/MaaAssistantArknights/MaaAssistantArknights)를 열고 `Fork`를 클릭한 다음 `Create fork`를 클릭합니다.
3. 로컬에서 레포(dev 브랜치)를 클론합니다.

   ```bash
   git clone --recurse-submodules <당신의 저장소의 git 링크> -b dev
   ```

4. Visual Studio와 같은 --recurse-submodules 매개변수가 포함되지 않은 Git GUI를 사용 중이라면, 클론한 후에 `git submodule update --init` 명령을 실행하여 서브모듈을 가져와야 합니다.

   **Python 환경이 필요합니다. Python 설치 방법은 스스로 찾아보세요.**  
   _（maadeps-download.py 파일은 프로젝트 루트에 있습니다）_

   ```cmd
   python maadeps-download.py
   ```

5. 개발 환경을 설정합니다.

   - `Visual Studio 2022 community`를 다운로드 및 설치하고 설치 중에 `C++ 기반 데스크톱 개발` 및 `.NET 데스크톱 개발`을 선택합니다.

6. `MAA.sln` 파일을 더블 클릭하여 엽니다. Visual Studio는 프로젝트를 자동으로 로드합니다.。
7. VS 설정

   - VS 상단의 구성을 `RelWithDebInfo x64`로 선택합니다. (릴리스 패키지 또는 ARM 플랫폼을 빌드하는 경우, 이 단계를 무시하세요)
   - MaaWpfGui를 마우스 오른쪽 버튼으로 클릭하여 속성을 선택하고 `디버그` - `로컬 디버깅 활성화`를 선택합니다. (이렇게 하면 C++ 코어 부분에 중단점을 설정할 수 있습니다.)

8. 이제 마음껏 개발을 시작할 수 있습니다.
9. 개발 중에 정기적으로 커밋을 하고 메시지를 작성하는 것을 잊지 마세요.
   Git 사용에 익숙하지 않다면 변경 사항을 직접 `dev` 브랜치에 제출하는 대신 새 브랜치를 만들어 작업하고 커밋하는 것이 좋습니다.

   ```bash
   git branch your_own_branch
   git checkout your_own_branch
   ```

   이렇게 하면 새 브랜치에서 변경 사항이 발생하며 `dev` 브랜치의 업데이트에 영향을 받지 않습니다.

10. 개발이 완료되면 수정한 로컬 브랜치(dev를 기준으로 함)를 원격(fork한 저장소)에 푸시합니다.

    ```bash
    git push origin dev
    ```

11. [MAA 메인 레포](https://github.com/MaaAssistantArknights/MaaAssistantArknights)를 엽니다. Pull request를 제출하고 관리자의 승인을 기다립니다. dev 브랜치에서 수정했는지 확인하세요.
12. MAA 원본 저장소에 변경 사항이 발생할 때마다 (다른 사람이 한 경우), 이러한 변경 사항을 로컬 브랜치에 동기화해야 할 수 있습니다.

    1. MAA 원본 저장소를 연결합니다.

       ```bash
       git remote add upstream https://github.com/MaaAssistantArknights/MaaAssistantArknights.git
       ```

    2. 업데이트를 가져옵니다.

       ```bash
       git fetch upstream
       ```

    3. 리베이스(권장) 또는 변경 사항을 병합합니다.

       ```bash
       git rebase upstream/dev # 리베이스
       ```

       또는

       ```bash
       git merge # 병합
       ```

    4. 7, 8, 9, 10 단계를 반복합니다.

::: tip
Visual Studio 2022를 열면 관련된 git 작업을 명령 줄 도구 대신 vs 자체 "Git 변경사항"을 사용하여 수행할 수 있습니다.
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

이제, 매번 커밋할 때마다 포매팅 도구가 자동으로 실행되어 코드 형식이 규칙에 맞는지 확인합니다.

## Visual Studio에서 clang-format 사용 설정

1. clang-format 17 또는 그 이상 버전을 설치합니다.

    ```bash
    python -m pip install clang-format
    ```

2. Everything 등의 도구를 사용하여 clang-format.exe의 설치 위치를 찾습니다. 참고로 Anaconda를 사용하는 경우 clang-format.exe는 YourAnacondaPath/Scripts/clang-format.exe에 설치됩니다.

3. Visual Studio에서 `도구`-`옵션`을 검색하여 `clang-format`을 클릭합니다.
4. `ClangFormat 지원 활성화`를 클릭하고 아래의 `사용자 정의 clang-format.exe 파일 사용`을 선택한 다음 2단계에서 찾은 `clang-format.exe`를 선택합니다.

![Visual Studio에서 clang-format 사용 설정](/image/zh-cn/development-enable-vs-clang-format.png)

이제 Visual Studio에서 C++20 구문을 지원하는 clang-format을 사용할 수 있습니다!

또한 프로젝트 루트에서 `tools\ClangFormatter\clang-formatter.py`를 실행하여 직접 clang-format을 호출하여 포맷팅할 수도 있습니다.

- `python tools\ClangFormatter\clang-formatter.py --clang-format=PATH\TO\YOUR\clang-format.exe --input=src\MaaCore`

## GitHub codespace를 사용하여 온라인으로 개발하기

GitHub codespace를 사용하여 자동으로 C++ 개발 환경을 구성하세요.

[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg?color=green)](https://codespaces.new/MaaAssistantArknights/MaaAssistantArknights)

그런 다음 vscode의 지침을 따르거나 [Linux 컴파일 가이드](./linux-tutorial.md)를 참고하여 GCC 12 및 CMake 프로젝트를 설정하세요.
