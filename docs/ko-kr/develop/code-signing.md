---
icon: mdi:certificate-outline
order: 8
---

# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## 서명 범위와 배포 절차

Windows 정식, 베타 및 나이틀리 빌드는 GitHub Actions에서 SignPath로 서명을 요청합니다. 배포용 인증서가 준비되지 않았거나 승인이 없거나 서명 검증에 실패하면 배포를 중단합니다. 이전 버전은 서명되지 않았을 수 있으므로 각 파일의 디지털 서명 속성을 확인하세요.

이 저장소와 버전이 고정된 MaaUtils 소스 하위 모듈에서 빌드한 다음 파일만 서명합니다.

- `MAA.exe`
- `MAA.dll`
- `MAA.Updater.exe`
- `MaaCore.dll`
- `MaaUtils.dll`

타사 의존성은 원본 파일과 서명을 유지합니다. 서명할 파일의 제품 버전을 통일하고 파일명과 제품명도 검증합니다. 개인 키는 SignPath의 HSM에 보관되며, Windows에 표시되는 서명 게시자는 SignPath Foundation입니다.

정식 및 베타 배포는 `.github/workflows/ci.yml`, 나이틀리 배포는 `.github/workflows/release-nightly-ota.yml`을 사용합니다. 모두 GitHub 호스팅 실행기에서 빌드하고 패키징 전에 서명을 검증합니다. 나이틀리를 포함한 모든 배포 서명에는 수동 승인이 필요합니다.

`test-signing` 정책은 신뢰되지 않는 테스트 인증서를 사용하며, 수동으로 실행하고 배포하지 않는 CI 검증에만 사용합니다. 배포용 인증서를 대신할 수 없습니다.

## 팀 역할

- 커미터 및 검토자: 이 저장소에 쓰기 권한이 있는 MAA 유지 관리자입니다. 다른 기여자의 변경 사항은 유지 관리자가 검토한 후 병합합니다.
- 서명 승인자: [uye / ABA2396](https://github.com/ABA2396).
- 전용 `CI builds` 사용자가 서명을 요청합니다.

유지 관리자는 SignPath와 GitHub 모두에서 다중 인증을 사용해야 합니다.

## 개인정보 보호

[사용자 약관의 개인정보 보호 및 데이터 업로드 설명](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/terms-of-service.md#42-隐私原则)을 참고하세요. Penguin Statistics와 Arknights Yituliu로 익명 데이터를 보내는 기능은 기본적으로 활성화되어 있으며 사용자가 끌 수 있습니다. Penguin Statistics 보고 ID를 입력하면 해당 ID도 보고에 포함됩니다.
