---
order: 9
icon: material-symbols:route
---

# 이벤트 탐색

`ActivityNavigation`은 이벤트 구역으로 이동하는 독립 작업입니다. 스테이지 선택, 편성 또는 전투 시작은 수행하지 않습니다. `stage_id`(Tile-Pos 내부 ID)는 필수입니다. 선택 사항인 `client_type`을 생략하면 연결된 클라이언트를 사용하며, `is_raid`는 기본 false로 난이도 검증에만 사용됩니다. 실행 전에 게임에 연결하고 로그인해야 합니다.

`Copilot`의 `copilot_list` 모드와 `Fight`에서 `navigate_to_activity: true`로 같은 진입점을 사용할 수 있습니다(기본 false). Copilot에는 명시적인 `client_type`도 필요합니다. 단일 작전도 항목 하나의 목록을 사용하며, `filename`과 함께 활성화하면 거부됩니다. 등록된 스테이지는 작업 접수 및 각 항목 실행 시 검증됩니다. 구역 진입 후 기존 스테이지 선택, 번호 확인, 하드 모드 전환, 편성 및 전투를 계속합니다. WPF 작전 목록에는 활성화되며 단일 파일 동작은 유지됩니다.

`resource/activity_navigation.json`은 `schema_version: 1`과 `activities`를 포함합니다. 각 이벤트는 `id`, `enabled`, `clients`, `routes`, `stages`와 설명용 `name`, `sources`를 저장합니다. 각 구역은 표준 작업명 `task`와 시간대 및 초가 포함된 ISO 8601 `opens_at` / `closes_at`을 지정합니다. 내부 스테이지 ID는 `code`, `route`, `modes`(normal / raid)에 대응합니다. 시작 시각은 포함하고 종료 시각은 제외합니다. 작업 및 번호는 Tile-Pos와 일치해야 합니다. 등록된 항목이 종료, 비활성, 클라이언트 불일치 또는 미지원 난이도이면 대체 경로 없이 실패합니다. 미등록 이벤트는 기존 동작을 유지합니다. 유효한 전체 데이터가 이전 설정을 대체하고 빈 `activities`는 초기화하며 잘못된 데이터는 대체하지 않습니다.

호환 작업은 `resource/tasks/hotfix/activity_navigation/`에 배치합니다. MaaResource는 매핑 파일과 `template/StageNavigation/`을 배포 대상으로 허용하고 주 저장소 동기화 작업은 관련 변경 시 리소스 버전을 갱신해야 합니다. 새 작업 필드나 코어 기능은 코어 업그레이드가 필요합니다. `AsstGetActivityNavigationVersion()`은 지원 프로토콜(이 구현은 1)을 반환합니다. 호출 전에 심볼 존재 여부를 확인하고 구형 코어에서는 외부 Custom 탐색을 유지하세요. 콜백은 `ActivityNavigationStarted`, `ActivityNavigationCompleted`, `ActivityNavigationFailed`이며 스테이지, 구역 또는 실패 이유를 포함합니다.
