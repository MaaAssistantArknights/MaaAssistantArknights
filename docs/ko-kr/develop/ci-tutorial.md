---
order: 7
icon: devicon-plain:githubactions
---

# CI 시스템 해석

MAA는 GitHub Action을 활용하여 웹사이트 구축, 자동 리소스 업데이트, 최종 파일 빌드 및 릴리즈 등 대량의 자동화 작업을 완성했습니다. 시간이 지나면서 이러한 CI들은 점차 중첩되기 시작했고, 일부는 다른 저장소로 연결되기도 했습니다. 본 문서는 MAA의 CI 시스템을 개선하고자 하는 분들에게 간단한 소개를 제공하는 것을 목표로 합니다.

본 문서를 읽기 전에 MAA의 프로젝트 구조와 구성에 대한 기본적인 개념을 갖고 있는 것이 좋습니다.

::: tip
이 페이지에서 CI 파일명을 검색하여 원하는 부분으로 빠르게 이동할 수 있습니다.
:::

워크플로우 파일들은 모두 `.github/workflows` 하위에 저장되어 있으며, 각 파일은 기능에 따라 다음과 같은 부분으로 나눌 수 있습니다:

- [코드 테스트](#코드-테스트)
- [코드 빌드](#코드-빌드)
- [코드 보안 스캔](#코드-보안-스캔)
- [버전 릴리즈](#버전-릴리즈)
- [리소스 업데이트](#리소스-업데이트)
- [웹사이트 구축](#웹사이트-구축)
- [Issues 관리](#issues-관리)
- [Pull Requests 관리](#pull-requests-관리)
- [MirrorChyan 관련](#mirrorchyan-관련)
- [기타](#기타)

또한 [pre-commit.ci](https://pre-commit.ci/)를 통해 코드의 자동 포맷팅과 이미지 리소스의 자동 최적화를 구현했으며, PR 생성 후 자동으로 실행되므로 특별히 신경 쓸 필요가 없습니다.

## 코드 테스트

`smoke-testing.yml`

이 워크플로우는 주로 MaaCore에 대한 기본적인 검사를 담당하며, 리소스 파일 로딩, 일부 간단한 task 실행 테스트 등을 포함합니다.

테스트 케이스가 오랫동안 업데이트되지 않아서, 현재 이 워크플로우는 기본적으로 리소스 파일에 오류가 없는지, 그리고 MaaCore의 코드에 치명적인 오류(빌드에 영향을 주는 종류)가 없는지 확인하는 역할을 합니다.

## 코드 빌드

`ci.yml`

이 워크플로우는 코드의 전체 빌드 작업을 담당하며, MAA의 모든 구성 요소를 포함합니다. 빌드 결과물은 실행 가능한 MAA입니다.

필수적인 MaaCore 외에, Windows 빌드 결과물에는 MaaWpfGui가, macOS 빌드 결과물에는 MaaMacGui가, Linux 빌드 결과물에는 MaaCLI가 포함됩니다.

이 워크플로우는 새로운 Commit이나 PR이 발생할 때마다 자동으로 실행되며, 릴리즈 PR에 의해 트리거될 때는 이번 빌드 결과물이 직접 릴리즈에 사용되고 Release가 생성됩니다.

## 코드 보안 스캔

코드 보안 스캔은 CodeQL을 사용하여 코드와 워크플로우에 대한 보안 분석을 수행하며, 다음 워크플로우들이 있습니다:

`codeql-core.yml`

이 워크플로우는 MaaCore와 MaaWpfGui의 C++ 및 C# 코드에 대한 보안 분석을 담당하며, 잠재적인 보안 취약점을 탐지합니다.

이 워크플로우는 관련 소스 코드를 수정하는 PR에서 자동으로 실행되며, 매일 UTC 시간 11:45에 정기 검사를 실행합니다.

`codeql-wf.yml`

이 워크플로우는 GitHub Actions 워크플로우 파일 자체에 대한 보안 분석을 담당하며, CI/CD 프로세스의 보안을 보장합니다.

이 워크플로우는 워크플로우 파일을 수정하는 PR에서 자동으로 실행되며, 매일 UTC 시간 12:00에 정기 검사를 실행합니다.

## 버전 릴리즈

버전 릴리즈, 줄여서 릴리즈는 사용자에게 업데이트를 배포하는 필수 작업으로, 다음 워크플로우들로 구성됩니다:

- `release-nightly-ota.yml` 개발 버전 릴리즈
- `release-ota.yml` 정식 버전/공개 베타 버전 릴리즈
  - `release-preparation.yml` 정식 버전/공개 베타 버전용 changelog 생성 및 릴리즈 준비
  - `pr-auto-tag.yml` 정식 버전/공개 베타 버전용 tag 생성

::: tip
위 파일명의 ota는 Over-the-Air를 의미하며, 우리가 흔히 말하는 "증분 업데이트 패키지"입니다. 따라서 MAA의 릴리즈 과정에는 실제로 이전 버전들에 대한 OTA 패키지 빌드 단계가 포함됩니다.
:::

### 개발 버전

`release-nightly-ota.yml`

이 워크플로우는 매일 UTC 시간 22시에 자동으로 실행되어 개발 버전의 릴리즈 주기를 보장합니다. 물론 변경사항을 검증해야 할 때 수동으로 릴리즈할 수도 있습니다.

주의할 점은 개발 버전의 릴리즈는 Windows 사용자만을 대상으로 하며, MacOS와 Linux 사용자는 개발 업데이트를 받을 수 없습니다.

### 정식 버전/공개 베타 버전

이 두 채널의 릴리즈 과정은 상대적으로 복잡합니다. 릴리즈 단계를 시뮬레이션하여 각 워크플로우의 역할을 설명하겠습니다:

1. `dev`에서 `master` 브랜치로의 PR을 생성하며, 해당 PR의 이름은 `Release v******` 형식이어야 합니다
2. `release-preparation.yml`이 최근 정식 버전/공개 베타 버전부터 현재 버전까지의 changelog를 생성합니다(새로운 PR 형태로)
3. changelog를 수동으로 조정하고 간단한 설명을 추가합니다
4. PR을 병합하면 `pr-auto-tag.yml`이 트리거되어 tag를 생성하고 브랜치를 동기화합니다
5. Release 이벤트가 `release-ota.yml`을 트리거하여 master에 tag를 생성한 후 ota 패키지 빌드 및 첨부파일 업로드를 진행합니다

## 리소스 업데이트

이 부분의 워크플로우는 주로 MAA의 리소스 업데이트와 최적화를 담당하며, 구체적인 워크플로우는 다음과 같습니다:

- `res-update-game.yml` 정기적으로 실행되어 지정된 저장소에서 게임 리소스를 가져옵니다
- `sync-resource.yml` 리소스를 MaaResource 저장소에 동기화하여 리소스 업데이트에 사용합니다
- `optimize-templates.yml` 템플릿 이미지 크기를 최적화합니다

## 웹사이트 구축

`website-workflow.yml`

이 워크플로우는 주로 MAA 문서 사이트의 구축과 발행을 담당합니다.

참고로 웹사이트 발행은 릴리즈와 강하게 연결되어 있어서, 평상시 웹페이지 구성 요소를 수정할 때는 오류가 없는지 확인하기 위해 빌드만 진행하고, 릴리즈 시에만 실제로 GitHub Pages에 배포됩니다.

## Issues 관리

`issue-checker.yml`

정규 표현식 매칭을 통해 각 Issue에 태그를 달아 Issue 내용을 분류하고 표시하여 조회와 관리를 용이하게 합니다.

`issue-checkbox-checker.yml`

정규 표현식 매칭을 통해 "주의 깊게 읽지 않음"을 체크한 Issue를 자동으로 닫습니다.
"주의 깊게 읽지 않음"이 체크되지 않은 경우 모든 체크박스를 접습니다.

`stale.yml`

90일 이상 활동이 없는 Bug Issue를 검사하여 표시하고 알림을 보내며, 7일 후에도 활동이 없으면 닫습니다.

## Pull Requests 관리

`pr-checker.yml`

이 워크플로우는 PR의 Commit Message가 [관례적 커밋](https://www.conventionalcommits.org/ko/v1.0.0/)을 준수하는지, 그리고 Merge Commit을 포함하는지 검사하며, 위 조건에 해당하면 알림을 제공합니다.

## MirrorChyan 관련

MirrorChyan은 유료 업데이트 미러 서비스이며, 관련 워크플로우는 다음과 같습니다:

- `release-package-distribution.yml` 업데이트 패키지를 MirrorChyan에 동기화
- `mirrorchyan_release_note.yml` MirrorChyan용 Release Note 생성

## 기타

`markdown-checker.yml`

저장소의 모든 Markdown 파일에 유효하지 않은 링크가 포함되어 있는지 검사합니다.

`blame-ignore.yml`

Commit Message에 `blame ignore`가 포함된 커밋을 자동으로 무시하여 저장소 히스토리를 깔끔하게 유지합니다.

`cache-delete.yml`

PR 병합 후 관련 캐시를 정리하여 캐시 사용량을 절약합니다.

`update-submodules.yml`

MaaMacGui와 maa-cli 등의 서브모듈을 정기적으로 최신 버전으로 업데이트합니다. 이 워크플로우는 매일 UTC 시간 21:50에 자동으로 실행되며(매일 개발 버전 릴리즈 전), 서브모듈이 최신 상태를 유지하도록 보장합니다.
