---
order: 3
icon: mingcute:git-pull-request-fill
---

# 웹 기반 PR 가이드

::: warning
이 가이드는 많은 개념을 단순화하여 더 많은 사람들이 실제로 사용할 수 있도록 설계되었습니다. 때로는 꽤 불안불안한 ~~하지만 간단한~~ 조치들과 약간 부정확한 설명이 있을 수 있으므로 전문가 분들의 이해를 부탁드립니다.
만약 Git 사용 경험이 있고 프로그래밍 기초를 가지고 계시다면 ~~그럼 당신은 왜 보고 있나요🔨~~, 약간 더 발전된 가이드인 [Github Pull Request 진행과정](./development.md#github-pull-request-진행-과정)을 확인해주세요.
:::

소소한 GitHUb Pull Request 사용 가이드 (\*´▽｀)ノノ

## 기본 개념 및 용어 설명

 장에서는 다소 지루한 내용을 다루고 있습니다. 관심 없으면 바로 실제 조작 부분으로 건너뛸 수 있으며, 이해되지 않는 부분은 나중에 다시 찾아보세요.

### Repository (저장소)

repo로도 알려져 있으며 코드 및 다른 리소스 파일을 저장하는 곳입니다.

👇 현재 이 웹 페이지 및 그 내용 전체가 MAA의 저장소입니다 (일반적으로 MAA의 기본 저장소로 불립니다).

```component Image
{
  "imageList": [
    {
      "light": "image/zh-cn/pr-tutorial/repository-light.png",
      "dark": "image/zh-cn/pr-tutorial/repository-dark.png"
    }
  ]
}
```

### Fork (복사)

단어 그대로 MAA의 코드를 복사하고 나서 수정 등의 작업을 수행할 수 있습니다. 원본을 망치지 않도록 조심하세요.
그러나 중국어로 "복사"라는 용어를 사용할 때는 일반적으로 "복사"라는 용어를 생각합니다.또한 "fork"라는 명확한 번역이 없으므로 일반적으로 영어를 사용합니다. 예를 들어 "코드를 fork하여 복사합니다."

복사한 것은 `MAA (1)` (bushi)입니다.
원래 저장소와 구별하기 위해 일반적으로 원래 MAA 저장소를 "원본 저장소", "원격 저장소", "상위 저장소"로 지칭합니다.
각 사람이 자신의 복사본을 만들 수 있기 때문에 복사본을 "개인 저장소"로 지칭합니다.

```component Image
{
  "imageList": [
    {
      "light": "image/zh-cn/pr-tutorial/fork-light.png",
      "dark": "image/zh-cn/pr-tutorial/fork-dark.png"
    }
  ]
}
```

### Pull Request

PR로도 알려져 있으며, "풀 리퀘스트"라는 용어는 너무 길고 타이핑 하기 싫어서 ~~글자 수가 많고 너무 힘들어~~ 그래서 모두 간단하게 "PR"이라고 부릅니다.
앞서 fork(복사)한 개인 저장소에서 수정한 내용을 주 저장소에 제공하려면 PR을 엽니다.

물론 "요청"이니까 승인이 필요합니다. MAA 팀의 분들이 당신의 변경 사항에 대해 몇 가지 의견을 제시할 수 있습니다. 물론 우리의 의견이 모두 옳은 것은 아니며, 합리적으로 논의합니다~

👇  아래는 현재 전문가들이 제시한 PR을 대기 중인 상태입니다.

```component Image
{
  "imageList": [
    {
      "light": "image/zh-cn/pr-tutorial/pull-request-light.png",
      "dark": "image/zh-cn/pr-tutorial/pull-request-dark.png"
    }
  ]
}
```

### Conflict (충돌)

가정해 봅시다. 원 저장소에는 A 파일이 있으며 내용이 111입니다. 당신은 이를 fork하여 내용을 222로 변경했지만, PR을 제출하기 직전에 누군가가 fork하여 PR을 제출하고 A 파일을 333으로 변경했습니다.
이 경우 둘 다 A 파일을 수정했으며 수정 사항이 다르므로 누구의 것을 사용할지 결정해야 합니다. 이것이 충돌입니다.
충돌 해결은 꽤 복잡하지만 여기서는 개념을 설명하기 위해만 언급하며 실제로 발생할 때 무슨 일이 일어나는지 이해할 수 있도록 하겠습니다. 해결 방법에 대해서는 현재 설명하지 않겠습니다.

## 웹 기반 PR 작업 전체 프로세스

1. 먼저 MAA 주 저장소로 이동하여 코드를 fork합니다.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/fork-light.png",
          "dark": "image/zh-cn/pr-tutorial/fork-dark.png"
        }
      ]
    }
    ```

2. "마스터 브랜치만" 옵션을 해제한 다음 Fork를 클릭합니다.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-2-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-2-dark.png"
        }
      ]
    }
    ```

3. 이제 개인 저장소로 이동하면 제목이 "YourName/MaaAssistantArknights"이고 아래에 한 줄짜리 작은 글씨로 "forked from MaaAssistantArknights/MaaAssistantArknights" (MAA 주 저장소에서 복제됨)이라고 표시됩니다.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-3-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-3-dark.png"
        }
      ]
    }
    ```

4. 수정하려는 파일을 찾아 "Go to file"을 클릭하여 전역 검색을 수행하거나 아래 폴더에서 직접 찾을 수 있습니다. (파일의 위치를 알고 있다면)

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-4-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-4-dark.png"
        }
      ]
    }
    ```

5. 파일을 열면 파일 오른쪽 상단의 ✏️을 클릭하여 편집합니다.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-5-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-5-dark.png"
        }
      ]
    }
    ```

6. 수정하세요! (리소스 파일과 같은 경우 먼저 컴퓨터에있는 MAA 폴더에서 수정 사항을 테스트 한 다음 웹 페이지로 복사하여 잘못된 수정을 방지하세요)
7. 수정이 완료되면 맨 아래로 스크롤하여 수정 내용을 적습니다.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-7-1-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-7-1-dark.png"
        }
      ]
    }
    ```

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-7-2-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-7-2-dark.png"
        }
      ]
    }
    ```

8. 두 번째 파일을 수정해야합니까? 수정 후 오류가 발생하여 수정을 다시하고 싶습니까? 전혀 문제가되지 않습니다! 5-8을 반복하세요!
9. 모두 완료되면 PR을 제출하세요! Pull Request 탭을 클릭합니다.
   Compare & Pull Request 버튼이 있으면 좋지만 없어도 걱정하지 마세요. 아래의 New Pull Request를 클릭하면 됩니다. (11단계 참조)

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-9-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-9-dark.png"
        }
      ]
    }
    ```

10. 이제 주 저장소로 이동하여 제출할 PR을 확인합니다.
   아래 그림에서 화살표가 있는 곳은 오른쪽에 개인 이름/MAA의 dev 브랜치를 주 저장소/MAA의 dev 브랜치에 병합하는 것입니다.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-10-1-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-10-1-dark.png"
        }
      ]
    }
    ```

    그런 다음 제목, 수정한 내용 등을 작성하고 확인을 클릭합니다.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-10-2-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-10-2-dark.png"
        }
      ]
    }
    ```

11. MAA 팀의 전문가들의 검토를 기다립니다! 물론 그들도 의견을 제시 할 수 있습니다.
   👇 예를 들어(오로지 오락을 위한 것이니 진지하게 받아들이지 마세요)

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-11-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-11-dark.png"
        }
      ]
    }
    ```

12. 만약 전문가들이 몇 가지 작은 문제를 수정하라고 말한다면 개인 저장소로 돌아가 이전의 dev 브랜치로 이동하여 3-9 단계를 반복하세요!
   당신의 현재 PR은 여전히 검토 대기 중이므로 나중에 수정 사항이 이 PR에 직접 반영됩니다.
   👇 예를 들어 아래에서 "재수정 데모"라는 새로운 내용이 추가된 것을 볼 수 있습니다.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-12-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-12-dark.png"
        }
      ]
    }
    ```

13. 전문가들의 승인을 기다리면 모두 완료됩니다! **버전이 출시**되면 당신의 GitHub 프로필 사진이 자동으로 기여자 목록에 추가됩니다. 모든 분들의 자비로운 공헌에 감사드립니다!  
    ~~왜 모든 사람들이 이중 인격이야, 오 나도 그렇지, 그럼 괜찮아~~

    ::: tip 기여자
    개발/테스트에 참여한 모든 친구들에게 감사드립니다. 여러분의 도움으로 MAA가 점점 더 좋아지고 있습니다! (\*´▽ ｀)ノノ

    [![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=114514&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
    :::

14. 다음에 다른 PR을 제출하려면 먼저 dev 브랜치로 전환한 다음 아래 그림과 같이 진행하세요.

    ::: warning
    이 조작은 개인 저장소를 주 저장소와 완전히 동일한 상태로 강제로 동기화합니다. 이것은 충돌을 해결하는 가장 간단하고 효과적인 방법입니다. 그러나 개인 저장소에 이미 추가한 것이 있으면 그것이 삭제됩니다!
    :::
    충돌을 일으키지 않을 경우 오른쪽에 초록색 `Update Branch` 버튼을 사용하세요.

    에서 설명한 이 모든 것을 모르거나 알 필요가 없다면 왼쪽의 버튼을 클릭하세요.

    ```component Image
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-14-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-14-dark.png"
        }
      ]
    }
    ```

    그러면 3-14 단계를 반복하여 수정하고 PR을 제출하면 됩니다~
