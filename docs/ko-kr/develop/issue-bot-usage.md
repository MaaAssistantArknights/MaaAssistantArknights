---
order: 4
icon: bxs:bot
---

# Issue Bot 사용 방법

Issue Bot의 동작은 [issue-checker](https://github.com/zzyyyl/issue-checker)로 이루어지며, 설정 파일은 [issue-checker.yaml](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yaml) 입니다.

::: warning
Commitizen 규격으로 커밋하지 않는 경우, 병합 요청(Merge Request)가 `ambiguous(애매모호한)`로 표시됨에 주의해주세요.
:::

## 기능

### 자동 알림

- 이슈와 풀 리퀘스트에 라벨을 추가합니다. 예: `module`, `Client`, `ambiguous`, `translation required` 등
  Issue Bot은 키워드를 기반으로 카테고리를 추가합니다.
  키워드에 대해서는 [설정 파일](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yaml)을 참조해주세요.
- MAA 공개 팀 멤버의 이슈와 풀 리퀘스트에 `MAA Team` 라벨을 추가합니다.

#### 이슈와 이에 대한 코멘트

- `incomplete(미완료)` 라벨을 추가하고, 사용자에게 파일이 제대로 업로드되지 않았음을 알리는 코멘트를 추가합니다.

#### 풀 리퀘스트

Issue Bot은 풀 리퀘스트의 제목을 확인합니다. 아래 키워드로 시작하지 않는 제목은 `ambiguous(애매모호한)`로 표시됩니다.

- `build`, `chore`, `ci`, `doc`, `docs`, `feat`, `fix`, `perf`, `refactor`, `rfc`, `style`, `test`
- `Merge`, `merge`, `Revert`, `revert`

### 수동으로 트리거하기

아래 명령어 목록 대신 키워드로 Issue Bot을 트리거하는 것이 좋습니다. 단, 이로 인해 **Issue Bot이 혼동될 수 있는 작업을 수행할 수 있음에 주의**해야 합니다.

#### 이슈와 풀 리퀘스트

- `Remove {LABEL_NAME}`은 라벨을 삭제합니다.
- `Remove labels`는 모든 라벨을 삭제합니다.
- `Skip {LABEL_NAME}`은 라벨을 건너뜁니다.
- `Skip labels`는 모든 라벨을 건너뜁니다.

#### 이슈 코멘트와 풀 리퀘스트 코멘트

- `Skip {LABEL_NAME}`은 특정 유형의 라벨이 추가되지 않도록 합니다.
- `Skip labels`은 모든 라벨이 추가되지 않도록 합니다.
- `fixed(수정함)` 라벨을 추가하려면 다음 명령어 중 하나를 사용할 수 있습니다:`<sup>`1 `</sup>`
  - `https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH} fixed`
  - `fixed by https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH}`
  - `{VERSION} fixed`
  - `fixed by {VERSION}`
- `Duplicate of #{ISSUE_NUMBER}`는 이슈에 `duplicate(중복)` 라벨을 추가합니다.
- **코멘트가 Issue Bot을 혼동시킬 수 있는 경우 `skip` 작업을 추가할 수 있습니다.**

::: info 참고
<sup>1</sup> 여기서 `COMMIT_HASH`는 40자 길이의 전체 해시값입니다.
:::

#### Push

푸시(push)에 포함된 모든 커밋에 대해:

- 커밋 메시지에 다음 중 하나의 키워드가 포함되어 있다면, `fixed(수정됨)` 라벨이 추가됩니다:
  - `fix #{ISSUE_NUMBER}`
  - `close #{ISSUE_NUMBER}`
  - `fix https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
  - `close https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
