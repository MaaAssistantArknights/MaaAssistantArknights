---
order: 6
icon: jam:write-f
---

# 문서 작성 가이드

::: tip
이 문서의 목적은 문서 작성자가 테마가 제공하는 기능을 더 잘 활용하여 더 읽기 쉬운 효과를 얻는 데 도움을 주는 것입니다.
:::

우리의 문서는 [vuepress](https://github.com/vuejs/vuepress)로 구축되었으며, [vuepress-theme-plume](https://github.com/pengzhanbo/vuepress-theme-plume) 테마를 사용하고 있습니다. 더 자세한 설명은 [공식 문서](https://theme-plume.vuejs.press/)를 참조할 수 있으며, 여기서는 일부 일반적인 기능이나 커스텀된 기능에 대해서만 소개하겠습니다.

## 로컬 배포

1. [pnpm](https://pnpm.io/installation)을 설치하고, [Pull Request 가이드](./pr-tutorial.md)를 참고해, 저장소를 로컬에 클론합니다.
2. `docs` 경로에서 새로운 터미널을 열고, `pnpm i` 를 실행하여 의존성을 설치합니다.
3. `pnpm run dev` 를 실행하여 배포를 시작합니다.

## 컨테이너와 카드

이 테마는 팁, 주석, 정보, 주의, 경고 및 세부 정보를 사용자 정의하는 컨테이너를 지원합니다. 이 기능을 사용하여 내용을 강조할 수 있습니다.

컨테이너 사용 방법:

```markdown
::: [컨테이너 타입] [컨테이너 제목(선택사항)]
내용을 입력하세요
:::
```

또는 GitHub 스타일 구문 사용:

```markdown
> [!컨테이너 타입]
> 내용을 입력하세요
```

허용되는 컨테이너 내용 및 기본 제목은 다음과 같습니다:

- `tip` 팁
- `note` 주석
- `info` 정보
- `warning` 주의
- `danger` 경고
- `details` 세부 사항
- `demo-wrapper` ==특수 컨테이너==

### 컨테이너 예시

::: tip
이것은 팁 컨테이너입니다
:::

::: note
이것은 주석 컨테이너입니다
:::

::: info
이것은 정보 컨테이너입니다
:::

::: warning
이것은 주의 컨테이너입니다
:::

::: danger
이것은 위험 컨테이너입니다
:::

::: details
이것은 세부 사항 컨테이너입니다
:::

::: demo-wrapper
이것은 매우 특별한 컨테이너입니다
:::

## 형광펜 표시

마크다운 구문을 사용하여 원하는 내용을 표시하고 강조할 수 있습니다.

사용 방법: `==표시 내용=={표시 색상(선택 사항)}` 구문을 사용하여 표시하며, 양쪽에 공백이 있어야 합니다.

**입력:**

```markdown
MaaAssistantArknights는 ==많은 돼지들== 에 의해 개발되었습니다
```

**출력:**

MaaAssistantArknights는 ==많은 돼지들== 에 의해 개발되었습니다

테마에는 다음과 같은 색상 구성표가 내장되어 있습니다:

- **default**: `==Default==` - ==Default==
- **info**: `==Info=={.info}` - ==Info=={.info}
- **note**: `==Note=={.note}` - ==Note=={.note}
- **tip**: `==Tip=={.tip}` - ==Tip=={.tip}
- **warning**: `==Warning=={.warning}` - ==Warning=={.warning}
- **danger**: `==Danger=={.danger}` - ==Danger=={.danger}
- **caution**: `==Caution=={.caution}` - ==Caution=={.caution}
- **important**: `==Important=={.important}` - ==Important=={.important}

## 숨겨진 텍스트

어떤 이유로 문서의 일부를 일시적으로 가려야 할 때, 숨겨진 텍스트 기능을 사용할 수 있습니다.

`!!숨겨야 할 내용!!{설정(선택 사항)}` 구문을 사용하여 사용할 수 있으며, 기본 효과는 다음과 같습니다.

!!왠지 모르게 맹모단(모에 백과)을 보고 있는 것 같다(취소선!!

다음과 같은 설정을 사용할 수 있습니다:

::: demo-wrapper
입력:

```markdown
+ 마스크 효과 + 마우스 호버: !!마우스 호버 시 보입니다!!{.mask .hover}
+ 마스크 효과 + 클릭: !!클릭 시 보입니다!!{.mask .click}
+ 블러 효과 + 마우스 호버: !!마우스 호버 시 보입니다!!{.blur .hover}
+ 블러 효과 + 클릭: !!클릭 시 보입니다!!{.blur .click}
```

출력:

- 마스크 효과 + 마우스 호버: !!마우스 호버 시 보입니다!!{.mask .hover}
- 마스크 효과 + 클릭: !!클릭 시 보입니다!!{.mask .click}
- 블러 효과 + 마우스 호버: !!마우스 호버 시 보입니다!!{.blur .hover}
- 블러 효과 + 클릭: !!클릭 시 보입니다!!{.blur .click}

:::

## 단계 (Steps)

단계별 튜토리얼을 작성할 때, 순서가 있는 목록은 중첩으로 인해 계층 구조가 불분명해질 수 있습니다. 이럴 때 `steps` 컨테이너가 최고의 선택입니다.

이 컨테이너는 일반적인 컨테이너와 달리 시작과 끝을 표시하기 위해 4개의 콜론을 사용합니다.

입력:

````markdown
:::: steps
1. 단계 1

   ```ts
   console.log('Hello World!')
   ```

2. 단계 2

   여기는 단계 2의 관련 내용입니다

3. 단계 3

   ::: tip
   팁 컨테이너
   :::

4. 종료
::::
````

출력:

:::: steps

1. 단계 1

   ```ts
   console.log('Hello World!')
   ```

2. 단계 2

   여기는 단계 2의 관련 내용입니다

3. 단계 3

   ::: tip
   팁 컨테이너
   :::

4. 종료

::::

## 스마트 이미지 컨테이너

우리는 테마가 제공하는 기능을 기반으로 이미지 컨테이너를 포장했습니다. 이 컨테이너는 라이트/다크 테마에 따라 해당 이미지를 자동으로 표시하며, 자동 레이아웃을 지원합니다.

마크다운 본문에서 `<ImageGrid>` 컴포넌트를 사용하여 이 메서드를 호출할 수 있으며, 구체적인 구문과 효과는 다음과 같습니다.

::: demo-wrapper

구문:

```markdown
<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  },
  {
    light: 'images/zh-cn/readme/3-light.png',
    dark: 'images/zh-cn/readme/3-dark.png'
  },
  {
    light: 'images/zh-cn/readme/4-light.png',
    dark: 'images/zh-cn/readme/4-dark.png'
  }
]" />
```

렌더링 효과:

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  }
]" />

:::

## 필드 컨테이너

이 구문은 다소 복잡하므로 [공식 문서](https://theme-plume.vuejs.press/guide/markdown/field/)를 참고하여 사용하세요.

효과는 다음과 같습니다.

:::: field-group
::: field name="theme" type="ThemeConfig" required default="{ base: '/' }"
테마 설정
:::

::: field name="enabled" type="boolean" optional default="true"
활성화 여부
:::

::: field name="callback" type="(...args: any[]) => void" optional default="() => {}"
<Badge type="tip" text="v1.0.0 추가"  />
콜백 함수
:::

::: field name="other" type="string" deprecated
<Badge type="danger" text="v0.9.0 폐기"  />
폐기된 속성
:::
::::

## 아이콘

이 테마는 아이콘을 지원합니다. 다음 위치에서 아이콘을 사용할 수 있습니다:

- 문서 제목: frontmatter에서 문서 제목 옆에 아이콘 설정.

- 내비게이션 바/사이드바: 내비게이션 바와 사이드바에 표시되는 아이콘 설정.

- 문서 내용: 문서 내에서 아이콘 사용.

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

마크다운에서 `<Icon />` 컴포넌트를 사용하여 아이콘을 추가할 수 있습니다. 이 컴포넌트에는 다음과 같은 속성이 있습니다:

- `icon` 아이콘 키워드 및 URL을 받습니다. 예: `jam:write-f`, `ic:round-home` 등
- `color` css 스타일 색상 값을 받습니다. 예: `#fff`, `red` 등 (이 옵션은 svg 아이콘에만 유효함)
- `size` css 스타일 크기 값을 받습니다. 예: `1rem`, `2em`, `100px` 등

::: demo-wrapper 예시

입력:

```markdown
- home - <Icon name="material-symbols:home" color="currentColor" size="1em" />
- vscode - <Icon name="skill-icons:vscode-dark" size="2em" />
- twitter - <Icon name="skill-icons:twitter" size="2em" />
```

출력:

- home - <Icon name="material-symbols:home" color="currentColor" size="1em" />
- vscode - <Icon name="skill-icons:vscode-dark" size="2em" />
- twitter - <Icon name="skill-icons:twitter" size="2em" />

:::

### 아이콘 키워드 획득

이 문서에서 사용하는 아이콘은 [iconify](https://iconify.design/)에서 가져옵니다. 제공되는 [아이콘 검색 인터페이스](https://icon-sets.iconify.design/)에서 원하는 아이콘을 검색한 후 키워드를 복사할 수 있습니다.

## Frontmatter

Frontmatter는 마크다운 문서 맨 처음에 `---`로 감싸진 내용으로, 내부적으로 yml 문법을 사용합니다. Frontmatter를 통해 문서의 편집 시간, 사용하는 아이콘, 분류, 태그 등을 식별할 수 있습니다.

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

- `date`: 문서의 편집 시간
- `icon`: 문서 제목 옆의 아이콘
- `order`: 사이드바에서 문서의 정렬 순서
