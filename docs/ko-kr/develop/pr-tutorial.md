---
order: 3
icon: mingcute:git-pull-request-fill
---

# 웹 기반 PR 가이드

소소한 GitHub Pull Request 사용 가이드 (\*´▽｀)ノノ

::: warning
이 가이드는 많은 개념을 단순화하여 더 많은 사람들이 실제로 사용할 수 있도록 설계되었습니다. 때로는 꽤 불안불안한 ~~하지만 간단한~~ 조치들과 약간 부정확한 설명이 있을 수 있으므로 전문가 분들의 이해를 부탁드립니다.
만약 Git 사용 경험이 있고 프로그래밍 기초 지식을 가지고 계시다면 ~~그럼 당신은 왜 보고 있나요🔨~~, 약간 더 발전된 가이드인 [GitHub Pull Request 진행과정](./development.md#완전한-환경-설정-windows)을 확인해주세요.
:::

## 기본 개념 및 용어 설명

이 장에서는 다소 지루한 내용을 다루고 있습니다. 관심 없으면 바로 실제 조작 부분으로 건너뛸 수 있으며, 이해되지 않는 부분은 나중에 다시 찾아보세요.

### Repository (저장소)

repo로도 알려져 있으며 코드 및 다른 리소스 파일을 저장하는 곳입니다.

👇 현재 이 웹 페이지 및 그 내용 전체가 MAA의 저장소입니다 (일반적으로 MAA의 기본 저장소로 불립니다).

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/repository-light.png',
    dark: 'images/zh-cn/pr-tutorial/repository-dark.png'
  }
]" />

### Fork (복사)

단어 그대로 MAA의 코드를 복사하고 나서 수정 등의 작업을 수행할 수 있습니다. 원본을 망치지 않도록 조심하세요.
하지만 일반적으로 '복제'라고 하면 Ctrl+C/V를 생각하기 쉬우므로, 보통 영단어 그대로 'Fork'라고 부릅니다. 예를 들어 "코드를 fork해간다"라고 합니다.

복사한 것은 `MAA (1)` (복사본)입니다.
원래 저장소와 구별하기 위해 일반적으로 원래 MAA 저장소를 "원본 저장소", "원격 저장소", "상위 저장소"로 지칭합니다.
각 사람이 자신의 복사본을 만들 수 있기 때문에 복사본을 "개인 저장소"로 지칭합니다.

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/fork-light.png',
    dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
  }
]" />

### Pull Request

PR로도 알려져 있으며, "풀 리퀘스트"라는 용어는 너무 길고 타이핑 하기 싫어서 ~~글자 수가 많고 너무 힘들어~~ 그래서 모두 간단하게 "PR"이라고 부릅니다.
앞서 fork(복사)한 개인 저장소에서 수정한 내용을 주 저장소에 제공하려면 PR을 엽니다.

물론 "요청"이니까 승인이 필요합니다. MAA 팀의 분들이 당신의 변경 사항에 대해 몇 가지 의견을 제시할 수 있습니다. 물론 우리의 의견이 모두 옳은 것은 아니며, 합리적으로 논의합니다~

👇 아래는 현재 전문가들이 제시한 PR을 검토 대기 중인 상태입니다.

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/pull-request-light.png',
    dark: 'images/zh-cn/pr-tutorial/pull-request-dark.png'
  }
]" />

### Conflict (충돌)

가정해 봅시다. 원 저장소에는 A 파일이 있으며 내용이 111입니다. 당신은 이를 fork하여 내용을 222로 변경했지만, PR을 제출하기 직전에 누군가가 fork하여 PR을 제출하고 A 파일을 333으로 변경했습니다.
이 경우 둘 다 A 파일을 수정했으며 수정 사항이 다르므로 누구의 것을 사용할지 결정해야 합니다. 이것이 충돌입니다.
충돌 해결은 꽤 복잡하지만 여기서는 개념을 설명하기 위해만 언급하며 실제로 발생할 때 무슨 일이 일어나는지 이해할 수 있도록 하겠습니다. 해결 방법에 대해서는 현재 설명하지 않겠습니다.

## 웹 기반 PR 작업 전체 프로세스

1. 먼저 MAA 주 저장소로 이동하여 코드를 fork합니다.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/fork-light.png',
       dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
     }
   ]" />

2. 그런 다음 Create Fork를 클릭합니다.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-2-dark.png'
     }
   ]" />

3. 이제 개인 저장소로 이동하면 제목이 "YourName/MaaAssistantArknights"이고 아래에 한 줄짜리 작은 글씨로 "forked from MaaAssistantArknights/MaaAssistantArknights" (MAA 주 저장소에서 복제됨)이라고 표시됩니다.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-3-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-3-dark.png'
     }
   ]" />

4. 수정하려는 파일을 찾아 "Go to file"을 클릭하여 전역 검색을 수행하거나 아래 폴더에서 직접 찾을 수 있습니다. (파일의 위치를 알고 있다면)

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-4-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-4-dark.png'
     }
   ]" />

5. 파일을 열면 파일 오른쪽 상단의 ✏️을 클릭하여 편집합니다.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-5-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-5-dark.png'
     }
   ]" />

6. 수정하세요! (리소스 파일과 같은 경우 먼저 컴퓨터에있는 MAA 폴더에서 수정 사항을 테스트 한 다음 웹 페이지로 복사하여 잘못된 수정을 방지하세요)
7. 수정이 완료되면 오른쪽 상단의 👇 이 버튼을 클릭하여 커밋 페이지를 열고, 무엇을 수정했는지 작성합니다.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-1-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-1-dark.png'
     }
   ]" />

   우리는 간단한 커밋 제목 [관례적 커밋](https://www.conventionalcommits.org/ko/v1.0.0/)이 있으니 가능하면 지켜주세요. 물론 정말 이해가 안 된다면 일단 아무렇게나 써도 됩니다.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-2-dark.png'
     }
   ]" />

8. 두 번째 파일을 수정해야하나요? 수정 후 오류가 발생하여 다시 수정하고 싶으신가요? 전혀 문제 없습니다! 4-7 단계를 반복하세요!
9. 모두 완료되면 PR을 제출하세요! Code를 클릭하여 **개인 저장소** 메인 페이지로 돌아갑니다.  
   Compare & Pull Request 버튼이 있으면 바로 클릭하세요!  
   없어도 걱정하지 마세요. 아래의 Contribute 버튼을 클릭한 다음 Open Pull Request를 클릭하면 됩니다.

   <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-9-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-9-dark.png'
      }
    ]" />

10. 이제 주 저장소로 이동하여 제출할 PR을 확인합니다.
    아래 그림에서 화살표가 있는 곳은 오른쪽에 개인 이름/MAA의 dev 브랜치를 주 저장소/MAA의 dev 브랜치에 병합하는 것입니다.

    <ImageGrid :imageList="[
       {
         light: 'images/zh-cn/pr-tutorial/pr-10-1-light.png',
         dark: 'images/zh-cn/pr-tutorial/pr-10-1-dark.png'
       }
     ]" />

    그런 다음 제목, 수정한 내용 등을 작성하고 확인을 클릭합니다.
    PR 제목도 [관례적 커밋](https://www.conventionalcommits.org/ko/v1.0.0/)을 지켜주세요. 물론 여전히 이해가 안 된다면 일단 아무렇게나 써도 됩니다.

    <ImageGrid :imageList="[
       {
         light: 'images/zh-cn/pr-tutorial/pr-10-2-light.png',
         dark: 'images/zh-cn/pr-tutorial/pr-10-2-dark.png'
       }
     ]" />

11. MAA 팀원들의 검토를 기다립니다! 물론 그들도 의견을 제시 할 수 있습니다.
    👇 예를 들어(오로지 오락을 위한 것이니 진지하게 받아들이지 마세요)

    <ImageGrid :imageList="[
       {
         light: 'images/zh-cn/pr-tutorial/pr-11-light.png',
         dark: 'images/zh-cn/pr-tutorial/pr-11-dark.png'
       }
     ]" />

12. 만약 팀원들이 몇 가지 작은 문제를 수정하라고 한다면, **개인 저장소**로 돌아가서 4-7 단계를 반복하세요!  
    1-2 단계(다시 fork)와 9-10 단계(다시 Pull Request)는 필요 없습니다. 현재 Pull Request는 여전히 검토 대기 중이며, 이후 수정 사항은 이 Pull Request에 직접 반영됩니다.  
    👇 예를 들어 맨 아래에 재수정한 내용이 추가된 것을 볼 수 있습니다.

    <ImageGrid :imageList="[
       {
         light: 'images/zh-cn/pr-tutorial/pr-12-light.png',
         dark: 'images/zh-cn/pr-tutorial/pr-12-dark.png'
       }
     ]" />

13. MAA 팀원들의 승인을 기다리면 모두 완료됩니다! 수정한 내용이 이미 MAA 메인 저장소에 들어갔습니다!

14. 다음에 다른 PR을 제출하려면 먼저 개인 저장소 메인 페이지로 돌아가, `Sync fork`를 클릭하여 당신의 저장소를 주 저장소와 동기화하세요.

    여기서 주의할 점은, 빨간색 `Discard 1 commit`이 있다면 빨간색을 클릭하고, 없다면 녹색 `Update branch`를 클릭하세요.

    그 다음 4-10 단계를 반복하여 다시 수정할 수 있습니다.

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-14-1-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-14-1-dark.png'
      },
      {
        light: 'images/zh-cn/pr-tutorial/pr-14-2-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-14-2-dark.png'
      }
    ]" />

**신규 버전이 출시**되면 당신의 GitHub 프로필 사진이 자동으로 기여자 목록에 추가됩니다. 모든 분들의 자비로운 공헌에 감사드립니다!  
~~왜 다들 오타쿠야, 아 나도 그렇지, 그럼 괜찮아~~

::: tip 기여자
개발/테스트에 참여한 모든 분들에게 감사드립니다. 여러분의 도움으로 MAA가 점점 더 좋아지고 있습니다! (\*´▽ ｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=105&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
:::
