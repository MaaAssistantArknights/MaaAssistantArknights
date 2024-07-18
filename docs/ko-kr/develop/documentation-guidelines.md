---
order: 6
icon: jam:write-f
---

# 문서 작성 가이드

::: tip
이 문서의 목적은 문서 작성자가 주제가 제공하는 기능을 더 잘 활용하여 더 읽기 쉬운 효과를 얻는 데 도움을 주는 것입니다.
:::

우리의 문서는 [vuepress](https://github.com/vuejs/vuepress)로 작성되었으며, [vuepress-theme-hope](https://github.com/vuepress-theme-hope/vuepress-theme-hope) 테마를 사용하고 있습니다. 더 자세한 설명은 [공식 문서](https://theme-hope.vuejs.press/zh/)를 참조할 수 있으며, 여기서는 일부 일반적인 기능에 대해서만 소개하겠습니다.

## 로컬 배포

1. [pnpm](https://pnpm.io/zh/installation)을 설치하고, [Pull Request 가이드](./development.md#github-pull-request-진행-과정)을 참고해, 저장소를 로컬에 클론합니다.
2. `website` 경로에서 새로운 터미널을 열고, `pnpm i` 을 실행하여 의존성 파일을 다운로드합니다.
3. `pnpm run dev` 를 실행하여 배포를 시작합니다.

## 컨테이너

~~도커 컨테이너가 아닙니다~~

이 테마는 팁, 주석, 정보, 주의, 경고 및 세부 정보를 사용자 정의하는 컨테이너를 지원합니다. 이 기능을 사용하여 내용을 강조할 수 있습니다.

컨테이너 사용 방법:

```markdown
::: [컨테이너 타입] [컨테이너 제목(선택사항)]
내용을 입력하세요
:::
```

허용되는 컨테이너 유형 및 기본 제목은 다음과 같습니다:

- `tip` 팁
- `note` 주석
- `info` 정보
- `warning` 주의
- `danger` 경고
- `details` 세부 사항

### 컨테이너 예시

::: tip
이것은 팁 컨테이너입니다.
:::

::: note
이것은 주석 컨테이너입니다.
:::

::: info
이것은 정보 컨테이너입니다.
:::

::: warning
이것은 주의 컨테이너입니다.
:::

::: danger
이것은 위험 컨테이너입니다.
:::

::: details
이것은 세부 사항 컨테이너입니다.
:::

## 아이콘

이 테마는 아이콘을 지원합니다. 다음 위치에서 아이콘을 사용할 수 있습니다:

- 문서 제목: frontmatter에서 문서 제목 옆에 아이콘 설정

- 탐색 모음/사이드바: 탐색 모음 및 사이드바에 표시되는 아이콘 설정

- 문서 내용: 문서에서 아이콘 사용

### 문서의 아이콘 설정

문서의 [frontmatter](#frontmatter) 에서 `icon`을 사용하여 문서의 아이콘을 설정할 수 있습니다.

이 아이콘은 문서 제목 옆에 표시됩니다.

::: details 이 문서의 frontmatter 설정

```markdown
---
icon: jam:write-f
---
```

:::

### 문서에서 아이콘 사용

마크다운에서 `<HopeIcon />` 구성 요소를 사용하여 아이콘을 추가할 수 있습니다. 이 구성 요소에는 다음과 같은 속성이 있습니다:

- `icon` 아이콘 키워드 및 URL 수신. 예: `jam:write-f`, `ic:round-home` 등
- `color` css 스타일 색상 값 수신. 예: `#fff`, `red` 등 (이 옵션은 svg 아이콘에만 유효함)
- `size` css 스타일 크기 값 수신. 예: `1rem`, `2em`, `100px` 등

::: details 예시
<HopeIcon icon="ic:round-home" color="#1f1e33"/>

```markdown
<HopeIcon icon="ic:round-home" color="#1f1e33"/>
```

<HopeIcon icon="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" size="4rem" />
```markdown
<HopeIcon icon="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" size="4rem" />
```
:::

### 아이콘 키워드 가져오기

이 문서에서 사용하는 아이콘은 [iconify](https://iconify.design/)에서 제공됩니다. 원하는 아이콘을 검색하려면 제공된 [아이콘 검색 페이지](https://icon-sets.iconify.design/)에서 검색한 다음 키워드를 복사하세요.

## Frontmatter

Frontmatter는 Markdown 문서의 시작 부분에 `---`로 둘러싸인 내용이며 내부에서는 YAML 구문을 사용합니다. Frontmatter를 통해 문서의 편집 시간, 사용된 아이콘, 분류, 태그 등을 식별할 수 있습니다.

::: details 예시

```markdown
---
date: 1919-08-10
icon: jam:write-f
order: 1
---

# 문서 제목

...
```

:::

각 필드의 의미는 다음과 같습니다:

- `date` 문서의 편집 시간
- `icon` 문서 제목 옆에 표시되는 아이콘
- `order` 사이드바에서 문서의 순서
