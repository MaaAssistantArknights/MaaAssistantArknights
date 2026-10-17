---
order: 9
icon: material-symbols:route
---

# 활동 탐색

기본 탐색은 클라이언트에 이미 로드된 작업을 재사용합니다. Tile-Pos 내부 ID의 표시 코드를 사용하며 `YW-1`은 `YW-OpenOcr`, `YW-EX-1`은 `YW-EX-OpenOcr`에 대응합니다. 공통 작업은 뒤로 가기가 지원되는 현재 화면에서 터미널로 돌아간 뒤 기존 OCR 입구와 챕터 작업을 실행하고, 정확한 스테이지 코드를 인식한 후 기존 선택 과정으로 넘깁니다. EX 입구가 없으면 일반 구역으로 진입하지 않고 `MissingTask`로 중지합니다.

입구와 챕터 작업은 기존 MaaRelease `resource/tasks.json` 업데이트 경로를 사용하며 코어 자체는 API를 다운로드하지 않습니다. 활동이나 구역을 추가할 때는 같은 JSON에 완전한 OCR 작업 체인을 제공하고 동시에 다운로드해야 하는 새 이미지 의존성을 피해야 합니다. 터미널 복귀는 코어의 기존 기본 이미지를 사용합니다. 템플릿 배포나 필수 이미지 누락 검사는 바꾸지 않으며 기존 개별 스테이지 템플릿 선택도 유지합니다.

독립 작업 `ActivityNavigation`은 대상 스테이지가 있는 맵으로 이동하며 편성하거나 전투를 시작하지 않습니다. `stage_id`는 필수, `client_type`과 `is_raid`는 선택 사항이며 먼저 연결하고 로그인해야 합니다. Copilot 목록과 Fight는 `navigate_to_activity: true`로 활성화합니다(API 기본값 false, WPF 목록은 활성화). Copilot은 `client_type`을 명시해야 하며 단일 작업도 한 항목의 목록을 사용합니다. `filename`과 함께 활성화하면 거부합니다. 기존 탐색이 지원하는 메인 스토리와 자원 스테이지도 터미널에 자동 복귀하며, 알 수 없는 스테이지는 현재 맵에서 기존 선택을 유지합니다. Fight의 `is_raid` 기본값은 false이며 활동 검사와 스테이지 선택 후 난이도 전환에 사용합니다.

`resource/activity_navigation.json`은 선택적 사용자 재정의입니다. 스키마 1의 활동은 `id`, `name`, `enabled`, `clients`, `sources`, `routes`, `stages`를 유지합니다. 구역에는 `task`와 시간대가 명시된 초 단위 `opens_at` / `closes_at`, 스테이지에는 `code`, `route`, `modes`(normal / raid)가 있습니다. 재정의는 자동 검색보다 우선하며 시작을 포함하고 종료를 제외하는 기간, 클라이언트, 모드를 검사합니다. 기본 OCR 작업에는 이 추가 메타데이터가 없으므로 이름에서 날짜나 클라이언트 제한을 추측하지 않습니다. 활동 시간은 기존 StageActivity API로 호출 측에서 처리합니다. 유효한 전체 설정이 이전 값을 대체하며 빈 `activities`는 재정의를 지우고 자동 검색으로 돌아갑니다. 잘못된 설정은 적용하지 않습니다.

Copilot은 수락한 경로 값을 보관하며 삭제되거나 바뀌면 중지합니다. 같은 내용의 재로드는 허용됩니다. 사용자 재정의 조회와 교체는 읽기/쓰기 잠금으로 동기화합니다. 콜백은 `ActivityNavigationStarted`, `ActivityNavigationCompleted`, `ActivityNavigationFailed`입니다. 사전 검사에는 `phase: Preflight`와 `MissingTask`, `InvalidEntryTask` 또는 사용자 정책의 `Closed`, `Disabled`, `WrongClient`, `StageCodeMismatch`, `UnsupportedMode`가 포함되며 대기 중 경로 변경은 `RouteChanged`입니다. `AsstGetActivityNavigationVersion()`은 프로토콜 2(기존 작업 자동 검색 포함)를 반환하므로 호출 전에 심볼을 확인해야 합니다. 이전 코어는 외부 Custom 탐색을 사용할 수 있습니다.
