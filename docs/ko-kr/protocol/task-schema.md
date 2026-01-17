---
order: 4
icon: material-symbols:task
---

# 작업 프로토콜

`resource/tasks`의 사용법 및 각 필드 설명

::: tip
[Visual Studio Code](https://code.visualstudio.com/)를 사용하고 [Maa Pipeline Support](https://marketplace.visualstudio.com/items?itemName=nekosu.maa-support) 확장을 설치하여 효율적으로 편집하는 것을 권장합니다. 자세한 내용은 확장 홈페이지와 [문서](../develop/vsc-ext-tutorial.md)를 참조하세요.
:::

::: warning
JSON 파일은 주석을 지원하지 않으므로, 텍스트 내의 주석은 예시용입니다. 직접 복사하여 사용하지 마세요.
:::

## 전체 필드 목록

```json
{
    "TaskName": {                           // 작업 이름, @가 포함된 경우 특수 작업일 수 있으며 기본값이 다를 수 있음. 아래 특수 작업 유형 참조

        "baseTask": "xxx",                  // xxx 작업을 템플릿(기반)으로 하여 작업 생성. 아래 특수 작업 유형의 '파생 작업' 참조

        "algorithm": "MatchTemplate",       // 선택 사항, 인식 알고리즘 유형
                                            // 비워둘 시 기본값: MatchTemplate
                                            //      - JustReturn:       인식 없이 바로 action 실행
                                            //      - MatchTemplate:    이미지 매칭
                                            //      - OcrDetect:        텍스트 인식
                                            //      - FeatureMatch:     특징 매칭

        "action": "ClickSelf",              // 선택 사항, 인식 성공 후 동작
                                            // 비워둘 시 기본값: DoNothing
                                            //      - ClickSelf:        인식된 위치 클릭 (인식된 목표 범위 내 무작위 지점)
                                            //      - ClickRect:        지정된 영역 클릭, specificRect 필드에 대응 (비권장)
                                            //      - DoNothing:        아무것도 하지 않음
                                            //      - Stop:             현재 작업 정지
                                            //      - Swipe:            스와이프, specificRect 및 rectMove 필드에 대응
                                            //      - Input:            텍스트 입력, algorithm이 JustReturn이어야 함, inputText 필드에 대응

        "sub": ["SubTaskName1", "SubTaskName2"],
                                            // 선택 사항, 서브 작업 (비권장). 현재 작업 실행 후 서브 작업들을 순차적으로 실행
                                            // 중첩 가능(서브 작업의 서브 작업). 단, 무한 루프에 주의

        "subErrorIgnored": true,            // 선택 사항, 서브 작업 오류 무시 여부
                                            // 비워둘 시 기본값: false
                                            // false: 서브 작업 중 하나라도 오류 발생 시 후속 작업 실행 안 함 (본 작업 오류로 간주)
                                            // true: 서브 작업 오류 여부가 영향을 주지 않음

        "next": ["OtherTaskName1", "OtherTaskName2"],
                                            // 선택 사항, 현재 작업 및 sub 작업 완료 후 실행할 다음 작업
                                            // 앞에서부터 순차적으로 인식하여 첫 번째로 매칭된 작업 실행
                                            // 비워둘 시 현재 작업 완료 후 바로 정지
                                            // 동일 작업에 대해 첫 번째 인식 후 두 번째는 인식하지 않음:
                                            //     "next": [ "A", "B", "A", "A" ] -> "next": [ "A", "B" ]
                                            // JustReturn 유형 작업은 마지막 항목이 아닌 곳에 위치할 수 없음

        "maxTimes": 10,                     // 선택 사항, 최대 실행 횟수
                                            // 비워둘 시 기본값: 무한
                                            // 최대 횟수 도달 후, exceededNext 필드가 있으면 실행, 없으면 작업 정지

        "exceededNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // 선택 사항, 최대 실행 횟수 도달 시 실행할 작업
                                            // 비워둘 시 정지; 작성 시 next 대신 이곳의 작업 실행
        "onErrorNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // 선택 사항, 실행 오류 발생 시 후속 실행할 작업

        "preDelay": 1000,                   // 선택 사항, 인식 성공 후 action 실행 전 지연 시간 (ms); 기본값 0
        "postDelay": 1000,                  // 선택 사항, action 실행 후 다음 next 인식 전 지연 시간 (ms); 기본값 0

        "roi": [0, 0, 1280, 720],           // 선택 사항, 인식 범위 [x, y, width, height]
                                            // 1280 * 720 기준 자동 스케일링; 기본값 [0, 0, 1280, 720]
                                            // 범위를 줄이면 성능 소모를 줄이고 인식 속도를 높일 수 있음

        "cache": true,                      // 선택 사항, 캐시 사용 여부, 기본값 true;
                                            // 첫 인식 성공 시 이후에는 해당 위치에서만 인식. 성능 대폭 절약 가능
                                            // 단, 인식 대상 위치가 절대 변하지 않는 경우에만 사용. 변할 수 있다면 false로 설정

        "rectMove": [0, 0, 0, 0],           // 선택 사항, 인식 후 목표 이동 (비권장). 1280 * 720 기준 자동 스케일링
                                            // 예: A를 인식했지만 실제 클릭은 A 아래 10픽셀 5*2 영역일 때
                                            // [0, 10, 5, 2]로 설정. 가능하면 클릭 위치를 직접 인식하는 것을 권장
                                            // 추가로, action이 Swipe일 때 유효하며 필수 (스와이프 종점)

        "reduceOtherTimes": ["OtherTaskName1", "OtherTaskName2"],
                                            // 선택 사항, 실행 후 다른 작업의 실행 카운트 감소
                                            // 예: 이성 회복제 사용 시, 이전에 누른 파란색 작전 시작 버튼이 무효화되었으므로 해당 작업 카운트 -1

        "specificRect": [100, 100, 50, 50],
                                            // action이 ClickRect일 때 유효하며 필수, 지정 클릭 위치 (범위 내 무작위 점)
                                            // action이 Swipe일 때 유효하며 필수, 스와이프 시점
                                            // 1280 * 720 기준 자동 스케일링
                                            // algorithm이 "OcrDetect"일 때, specificRect[0]과 [1]은 그레이스케일 상/하한 임계값

        "specialParams": [int, ...],        // 특정 인식기에 필요한 추가 파라미터
                                            // action이 Swipe일 때 선택 사항: [0] duration, [1] 추가 슬라이드 활성화 여부

        "highResolutionSwipeFix": false,    // 선택 사항, 고해상도 스와이프 보정 활성화 여부
                                            // 현재 스테이지 내비게이션만 Unity 스와이프를 사용하지 않으므로 켜야 함
                                            // 기본값 false

        /* 이하 필드는 algorithm이 MatchTemplate일 때만 유효 */

        "template": "xxx.png",              // 선택 사항, 매칭할 이미지 파일명 (문자열 또는 리스트)
                                            // 기본값 "TaskName.png"
                                            // 템플릿 파일은 template 폴더 및 하위 폴더에서 재귀적으로 검색됨

        "templThreshold": 0.8,              // 선택 사항, 템플릿 매칭 점수 임계값. 이 값 이상이어야 인식 성공 (숫자 또는 리스트)
                                            // 기본값 0.8, 실제 점수는 로그 확인

        "maskRange": [1, 255],              // 선택 사항, 매칭 시 그레이스케일 마스크 범위. array<int, 2>
                                            // 예: 인식 불필요 부분을 검은색(0)으로 칠하고
                                            // [1, 255]로 설정하면 검은색 부분 무시

        "colorScales": [                    // method가 HSVCount 또는 RGBCount일 때 유효하며 필수, 색상 카운트 마스크 범위
            [                               // list<array<array<int, 3>, 2>> / list<array<int, 2>>
                [23, 150, 40],              // 구조: [[lower1, upper1], [lower2, upper2], ...]
                [25, 230, 150]              //     내부가 int면 그레이스케일,
            ],                              //     array<int, 3>이면 3채널 색상 (method에 따라 RGB/HSV)
            ...                             //     중간층 array<*, 2>는 색상(또는 그레이스케일) 하한/상한
        ],                                  //     최외각은 서로 다른 색상 범위를 나타내며, 인식 대상 영역은 이들의 마스크 합집합임

        "colorWithClose": true,             // 선택 사항, method가 HSVCount/RGBCount일 때 유효, 기본값 true
                                            // 색상 카운트 시 마스크 범위에 대해 닫힘 연산(Closing) 수행 여부
                                            // 작은 구멍을 메워 매칭 효과를 높이나, 글자가 포함된 경우 false 권장

        "pureColor": false,                 // 선택 사항, method가 HSVCount/RGBCount일 때 유효, 기본값 false
                                            // true일 경우 템플릿 매칭 점수 무시, 색상 매칭 결과만 신뢰
                                            // 색상 특징은 뚜렷하나 템플릿 매칭이 잘 안 될 때 사용
                                            // 사용 시 templThreshold를 높이는 것을 권장

        "method": "Ccoeff",                 // 선택 사항, 템플릿 매칭 알고리즘 (리스트 가능)
                                            // 비워둘 시 기본값 Ccoeff
                                            //      - Ccoeff:       색상에 둔감한 템플릿 매칭 (cv::TM_CCOEFF_NORMED)
                                            //      - RGBCount:     색상에 민감. colorScales로 이진화 후
                                            //                      RGB 공간 내 유사도(F1-score) 계산,
                                            //                      그 결과를 Ccoeff 결과와 곱함
                                            //      - HSVCount:     RGBCount와 유사하나 HSV 공간 사용

        /* 이하 필드는 algorithm이 OcrDetect일 때만 유효 */

        "text": [ "작전 수행", "대리 지휘" ], // 필수, 인식할 텍스트 내용. 하나라도 매칭되면 성공

        "ocrReplace": [                     // 선택 사항, 자주 오인식되는 텍스트 교정 (정규식 지원)
            [ "머틀", "머틀" ],
            [ ".+격대원", "저격대원" ]
        ],

        "fullMatch": false,                 // 선택 사항, 전체 일치 여부, 기본값 false
                                            // false: 부분 문자열 매칭 허용 (예: text ["시작"] -> "작전 시작" 인식 성공)
                                            // true: 정확히 일치해야 함 ("시작" -> "작전 시작" 실패)

        "isAscii": false,                   // 선택 사항, 인식 대상이 ASCII 문자인지 여부
                                            // 기본값 false

        "withoutDet": false,                // 선택 사항, 디텍션 모델 미사용 여부
                                            // 기본값 false

        /* 이하 필드는 algorithm이 OcrDetect이고 withoutDet가 true일 때만 유효 */

        "useRaw": true,                     // 선택 사항, 원본 이미지 사용 여부
                                            // 기본값 true, false면 그레이스케일 매칭

        "binThreshold": [140, 255],         // 선택 사항, 이진화 임계값 (기본값 [140, 255])
                                            // 범위 밖 픽셀은 배경으로 간주되어 제외됨
                                            // [lower, upper] 구간 픽셀만 전경(글자)으로 남음

        /* 이하 필드는 algorithm이 JustReturn, action이 Input일 때만 유효 */

        "inputText": "A string text.",      // 필수, 입력할 문자열

        /* 이하 필드는 algorithm이 FeatureMatch일 때만 유효 */

        "template": "xxx.png",              // 선택 사항, 매칭할 이미지 파일명 (문자열/리스트)
                                            // 기본값 "TaskName.png"

        "count": 4,                         // 매칭 특징점 수 (임계값), 기본값 4

        "ratio": 0.6,                       // KNN 매칭 거리 비율 [0 - 1.0]. 클수록 느슨하여 매칭 쉬움. 기본값 0.6

        "detector": "SIFT",                 // 특징점 검출기. SIFT, ORB, BRISK, KAZE, AKAZE, SURF. 기본값 SIFT
                                            // SIFT: 느리지만 스케일/회전 불변성 우수. 성능 최고.
                                            // ORB: 매우 빠름, 회전 불변. 스케일 불변성 없음.
                                            // BRISK: 매우 빠름, 스케일/회전 불변.
                                            // KAZE: 2D/3D 적용 가능, 스케일/회전 불변.
                                            // AKAZE: 빠름, 스케일/회전 불변.
    }
}
```

## 표현식 계산

작업 목록 유형 필드 (`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`)는 표현식 계산을 지원합니다.

|    기호    |                           의미                            |                  예시                   |
| :--------: | :-------------------------------------------------------: | :-------------------------------------: |
|    `@`     |                        `@` 형 작업                        |            `Fight@ReturnTo`             |
| `#` (단항) |                         가상 작업                         |                 `#self`                 |
| `#` (이항) |                         가상 작업                         |          `StartUpThemes#next`           |
|    `*`     |                      다중 작업 반복                       | `(ClickCornerAfterPRTS+ClickCorner)*10` |
|    `+`     | 작업 목록 병합 (next 계열 필드에서 동명 작업은 앞쪽 우선) |                  `A+B`                  |
|    `^`     |        작업 목록 차집합 (전자에만 존재, 순서 유지)        |    `(A+A+B+C)^(A+B+D)` (결과는 `C`)     |

연산자 `@`, `#`, `*`, `+`, `^`의 우선순위: `#`(단항) > `@` = `#`(이항) > `*` > `+` = `^`.

## 특수 작업 유형

### 템플릿 작업

**템플릿 작업**은 파생 작업과 `@` 형 작업을 포함합니다. 템플릿 작업의 핵심은 기반 작업을 바탕으로 **필드의 기본값을 수정**하는 것입니다.

#### 파생 작업 (Derived Task)

`baseTask` 필드가 존재하는 작업이 파생 작업이며, `baseTask`에 지정된 작업을 해당 파생 작업의 **기반 작업(Base Task)**이라고 합니다. 파생 작업에서:

1. 템플릿 매칭 작업이라도 **`template` 필드**의 기본값은 여전히 `"TaskName.png"`입니다.
2. `algorithm` 필드가 기반 작업과 다르면, 파생 클래스 파라미터는 상속되지 않습니다 (TaskInfo 정의 파라미터만 상속).
3. 나머지 필드의 기본값은 모두 **기반 작업의 해당 필드**를 따릅니다.

#### 암시적 `@` 형 작업

작업 `"A"`가 존재할 때, 작업 파일에 직접 정의되지 않았지만 `"B@A"` 형태의 작업이 사용되면 이를 암시적 `@` 형 작업이라 합니다. 작업 `"A"`는 작업 `"B@A"`의 **기반 작업**이 됩니다. 암시적 `@` 형 작업에서:

1. 작업 목록 유형 필드(`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`)의 기본값은 기반 작업의 해당 필드에 **직접** `B@` 접두사를 붙인 값이 됩니다 (작업명이 `#`로 시작하면 `B` 접두사).
2. 나머지 필드의 기본값은 모두 **기반 작업의 해당 필드**를 따릅니다 (필드 `template` 포함).

#### 명시적 `@` 형 작업

작업 `"A"`가 존재하고, 작업 파일에 `"B@A"` 형태의 작업이 직접 정의되어 있으면 이를 명시적 `@` 형 작업이라 합니다. 작업 `"A"`는 작업 `"B@A"`의 **기반 작업**이 됩니다. 명시적 `@` 형 작업에서:

1. 작업 목록 유형 필드(`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`)의 기본값은 기반 작업의 해당 필드에 **직접** `B@` 접두사를 붙인 값이 됩니다 (작업명이 `#`로 시작하면 `B` 접두사).
2. 템플릿 매칭 작업이라도 **`template` 필드**의 기본값은 여전히 `"TaskName.png"`입니다.
3. `algorithm` 필드가 기반 작업과 다르면, 파생 클래스 파라미터는 상속되지 않습니다 (TaskInfo 정의 파라미터만 상속).
4. 나머지 필드의 기본값은 모두 **기반 작업의 해당 필드**를 따릅니다.

### 가상 작업 (`#` 형 작업)

가상 작업은 `"#{sharp_type}"` 또는 `"B#{sharp_type}"` 형태의 작업입니다. 여기서 `{sharp_type}`은 `none`, `self`, `back`, `next`, `sub`, `on_error_next`, `exceeded_next`, `reduce_other_times` 중 하나입니다.

가상 작업은 **명령 가상 작업**(`none` / `self` / `back`)과 **필드 가상 작업**(`next` 등)으로 나뉩니다.

| 가상 작업 유형 |         의미          |                                                        간단 예시                                                         |
| :------------: | :-------------------: | :----------------------------------------------------------------------------------------------------------------------: |
|      none      |        빈 작업        |   직접 건너뜀<sup>1</sup><br>`"A": {"next": ["#none", "T1"]}` -> `"A": {"next": ["T1"]}`<br>`"A#none + T1"` -> `"T1"`    |
|      self      |      현재 작업명      | `"A": {"next": ["#self"]}`의 `"#self"` -> `"A"`<br>`"B": {"next": ["A@B@C#self"]}`의 `"A@B@C#self"` -> `"B"`<sup>2</sup> |
|      back      |     # 앞의 작업명     |                         `"A@B#back"` -> `"A@B"`<br>`"#back"`이 직접 등장하면 건너뜀<sup>3</sup>                          |
|  next, sub 등  | # 앞 작업의 해당 필드 |                 `next` 예시:<br>`"A#next"` -> `Task.get("A")->next`<br>`"#next"`가 직접 등장하면 건너뜀                  |

_Note<sup>1</sup>: `"#none"`은 주로 템플릿 작업의 접두사 추가 특성과 함께 사용하거나, `baseTask` 사용 시 불필요한 필드 상속을 피하기 위해 사용합니다._

_Note<sup>2</sup>: `"XXX#self"`와 `"#self"`는 동일한 의미입니다._

_Note<sup>3</sup>: 여러 작업이 `"next": [ "#back" ]`을 가질 때, `"T1@T2@T3"`는 `T3`, `T2`, `T1` 순으로 실행됨을 의미합니다._

### 다중 파일 작업

나중에 로드된 작업 파일(예: 외섭 `tasks.json`, 이하 파일2)에 정의된 작업이 먼저 로드된 작업 파일(예: 중섭 `tasks.json`, 이하 파일1)에도 정의되어 있다면:

- 파일2 작업에 `baseTask` 필드가 **없으면**, 파일1의 동명 작업 필드를 직접 상속합니다.
- 파일2 작업에 `baseTask` 필드가 **있으면**, 파일1의 동명 작업 필드를 상속하지 않고 **덮어씁니다**. 특히, 템플릿이 없을 때 `"baseTask": "#none"`을 사용하여 불필요한 필드 상속을 막을 수 있습니다.

### 사용 예시

- 파생 작업 예시 (`baseTask` 필드)

  다음 두 작업이 정의되어 있다고 가정합니다:

  ```json
  "Return": {
      "action": "ClickSelf",
      "next": [ "Stop" ]
  },
  "Return2": {
      "baseTask": "Return"
  },
  ```

  `"Return2"` 작업은 `"Return"` 작업을 상속받아 실제로 다음 파라미터를 가집니다:

  ```json
  "Return2": {
      "algorithm": "MatchTemplate", // 직접 상속
      "template": "Return2.png",    // "TaskName.png" 규칙 (템플릿 매칭이므로)
      "action": "ClickSelf",        // 직접 상속
      "next": [ "Stop" ]            // 직접 상속 (Template Task와 달리 접두사 없음)
  }
  ```

- `@` 형 작업 예시

  다음 파라미터를 가진 작업 `"A"`가 있다고 가정합니다:

  ```json
  "A": {
      "template": "A.png",
      ...,
      "next": [ "N1", "#back" ]
  },
  ```

  작업 `"B@A"`가 직접 정의되지 않았다면(암시적), `"B@A"`는 실제로 다음 파라미터를 가집니다:

  ```json
  "B@A": {
      "template": "A.png",
      ...,
      "next": [ "B@N1", "B#back" ]
  }
  ```

  작업 `"B@A"`가 `"B@A": {}`로 정의되어 있다면(명시적), `"B@A"`는 실제로 다음 파라미터를 가집니다:

  ```json
  "B@A": {
      "template": "B@A.png",
      ...,
      "next": [ "B@N1", "B#back" ]
  }
  ```

- 가상 작업 예시

  ```json
  {
      "A": { "next": ["N1", "N2"] },
      "C": { "next": ["B@A#next"] },

      "Loading": {
          "next": ["#self", "#next", "#back"]
      },
      "B": {
          "next": ["Other", "B@Loading"]
      }
  }
  ```

  결과는 다음과 같습니다:

  ```cpp
  Task.get("C")->next = { "B@N1", "B@N2" };

  Task.get("B@Loading")->next = { "B@Loading", "Other", "B" };
  Task.get("Loading")->next = { "Loading" };
  Task.get_raw("B@Loading")->next = { "B#self", "B#next", "B#back" };
  ```

### 주의 사항

작업 목록 유형 필드(`next` 등)에 정의된 작업이 낮은 우선순위 연산을 포함할 경우, 실제 결과가 예상과 다를 수 있습니다.

1. `@`와 이항 `#` 연산 순서에 따른 특례

   ```json
   {
       "A": { "next": ["N0"] },
       "B": { "next": ["A#next"] },
       "C@A": { "next": ["N1"] }
   }
   ```

   이 경우, `"C@B" -> next` (즉 `C@A#next`)는 `[ "N1" ]`이 되며, `[ "C@N0" ]`가 아닙니다.

2. `@`와 `+` 연산 순서에 따른 특례

   ```json
   {
       "A": { "next": ["#back + N0"] },
       "B@A": {}
   }
   ```

   이 경우,

   ```cpp
   Task.get("A")->next = { "N0" };

   Task.get_raw("B@A")->next = { "B#back + N0" };
   Task.get("B@A")->next = { "B", "N0" }; // [ "B", "B@N0" ]가 아님에 주의
   ```

   사실 이 특성을 이용해 불필요한 접두사 추가를 피할 수 있습니다:

   ```json
   {
       "A": { "next": ["#none + N0"] }
   }
   ```

## 런타임 작업 수정

- `Task.lazy_parse()`는 런타임에 JSON 작업 설정을 로드할 수 있습니다. lazy_parse 규칙은 [다중 파일 작업](#다중-파일-작업)과 동일합니다.
- `Task.set_task_base()`는 작업의 `baseTask` 필드를 수정할 수 있습니다.

### 사용 예시

다음과 같은 작업 설정 파일이 있다고 가정합니다:

```json
{
    "A": {
        "baseTask": "A_default"
    },
    "A_default": {
        "next": ["xxx"]
    },
    "A_mode1": {
        "next": ["yyy"]
    },
    "A_mode2": {
        "next": ["zzz"]
    }
}
```

다음 코드는 `mode` 값에 따라 작업 "A"를 변경하며, 동시에 "B@A"와 같이 "A"에 의존하는 다른 작업들도 변경합니다:

```cpp
switch (mode) {
case 1:
    Task.set_task_base("A", "A_mode1");  // 기본적으로 A를 A_mode1 내용으로 덮어쓰는 것과 같음. 아래도 동일
    break;
case 2:
    Task.set_task_base("A", "A_mode2");
    break;
default:
    Task.set_task_base("A", "A_default");
    break;
}
```

## Schema 검증

본 프로젝트는 `tasks.json`에 대한 JSON schema 검증을 구성했습니다. 프로토콜 파일은 `docs/maa_tasks_schema.json`입니다.

### Visual Studio

`MaaCore.vcxproj`에 구성되어 있어 바로 사용 가능합니다. 힌트 효과는 다소 불명확하며 일부 정보가 누락될 수 있습니다.

### Visual Studio Code

`.vscode/settings.json`에 구성되어 있으며, VS Code로 해당 **프로젝트 폴더**를 열면 사용할 수 있습니다. 힌트 효과가 좋습니다.
