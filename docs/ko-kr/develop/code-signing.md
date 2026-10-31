---
icon: mdi:certificate-outline
order: 9
---

# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## 서명 범위와 배포 절차

Windows 안정 버전 및 베타 버전 빌드는 GitHub Actions에서 SignPath로 서명을 요청합니다. 배포용 인증서가 준비되지 않았거나 승인이 없거나 서명 검증에 실패하면 배포를 중단합니다. 이전 버전은 서명되지 않았을 수 있으므로 각 파일의 디지털 서명 속성을 확인하세요.

이 저장소와 버전이 고정된 MaaUtils 소스 하위 모듈에서 빌드한 다음 파일만 서명합니다.

- `MAA.exe`
- `MAA.dll`
- `MAA.Updater.exe`
- `MaaCore.dll`
- `MaaUtils.dll`

기존의 유효한 업스트림 서명은 유지합니다. NetBeauty가 제공하는 수정된 `hostfxr.dll` 등 일부 의존성은 서명되지 않았을 수 있으며 현재 서명 허용 목록에 포함되지 않습니다. CI는 모든 EXE 및 DLL의 서명 상태를 기록한 `MAA-signpath-audit-<arch>` 아티팩트를 업로드합니다. 서명되지 않았다는 이유만으로 이 프로젝트의 인증서로 의존성을 다시 서명하지 않습니다.

서명할 파일의 제품 버전을 통일하고 Windows PR 빌드와 서명 빌드에서 파일명, 제품명 및 제품 버전을 검증합니다. 개인 키는 SignPath의 HSM에 보관되며, Windows에 표시되는 서명 게시자는 SignPath Foundation입니다.

안정 버전 및 베타 버전 배포는 `.github/workflows/ci.yml`을 사용하며 GitHub 호스팅 실행기에서 빌드하고 패키징 전에 서명을 검증합니다. 서명된 네이티브 런타임 파일은 하위 패키지에서도 사용합니다. 모든 배포 서명에는 수동 승인이 필요합니다. 개발 버전(Nightly) 빌드는 `.github/workflows/release-nightly-ota.yml`을 사용하며 현재 SignPath 서명을 생략하므로 서명 승인 없이 기존 방식대로 자동 빌드 및 배포를 계속합니다.

첫 서명 배포 전에 저장소 Actions 변수 `SIGNPATH_RELEASE_CERTIFICATE_SHA256`을 발급된 배포용 인증서의 SHA-256 지문으로 설정하세요. 공백과 콜론이 없는 64자리 16진수를 사용하고 인증서를 교체할 때도 갱신해야 합니다. CI는 인증서 신뢰, 게시자 및 타임스탬프와 함께 이 지문을 확인합니다.

`test-signing` 정책은 신뢰되지 않는 테스트 인증서를 사용하며, 수동으로 실행하고 배포하지 않는 CI 검증에만 사용합니다. 배포용 인증서를 대신할 수 없습니다.

## 팀 역할

- 저장소 권한 그룹: [Cpp Dev](https://github.com/orgs/MaaAssistantArknights/teams/cpp-dev) / [Cs Dev](https://github.com/orgs/MaaAssistantArknights/teams/cs-dev) / [Ci Dev](https://github.com/orgs/MaaAssistantArknights/teams/ci-dev) / [Collaborators](https://github.com/orgs/MaaAssistantArknights/teams/collaborators) / [i18n Dev](https://github.com/orgs/MaaAssistantArknights/teams/i18n-dev) / [Website Dev](https://github.com/orgs/MaaAssistantArknights/teams/website-dev).
- 커미터 및 검토자: 이 저장소에 쓰기 권한이 있는 MAA 유지 관리자입니다. 다른 기여자의 변경 사항은 유지 관리자가 검토한 후 병합합니다.
- 서명 승인자: [uye / ABA2396](https://github.com/ABA2396).
- 전용 `CI builds` 사용자가 서명을 요청합니다.

유지 관리자는 SignPath와 GitHub 모두에서 다중 인증을 사용해야 합니다.

## 개인정보 보호

[사용자 약관의 개인정보 보호 및 데이터 업로드 설명](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/terms-of-service.md#42-隐私原则)을 참고하세요. Penguin Statistics와 Arknights Yituliu로 익명 데이터를 보내는 기능은 기본적으로 활성화되어 있으며 사용자가 끌 수 있습니다. Penguin Statistics 보고 ID를 입력하면 해당 ID도 보고에 포함됩니다.
