---
icon: ic:round-home
index: true
dir:
  order: 0
---

::: center

![MAA Logo](https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png =256x256)

# MaaAssistantArknights

![C++](https://img.shields.io/badge/C++-20-%2300599C?logo=cplusplus)
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blueviolet)
![license](https://img.shields.io/github/license/MaaAssistantArknights/MaaAssistantArknights) ![commit](https://img.shields.io/github/commit-activity/m/MaaAssistantArknights/MaaAssistantArknights?color=%23ff69b4)
![stars](https://img.shields.io/github/stars/MaaAssistantArknights/MaaAssistantArknights?style=social) ![GitHub all releases](https://img.shields.io/github/downloads/MaaAssistantArknights/MaaAssistantArknights/total?style=social)

[简体中文](../zh-cn/readme.md) | [繁體中文](../zh-tw/readme.md) | [English](../en-us/readme.md) | [日本語](../ja-jp/readme.md) | 한국어

MAA는 MAA Assistant Arknights의 약자입니다

명일방주 어시스턴트

이미지 인식을 기반으로, 한 번의 클릭만으로 그날의 모든 작업을 끝내드립니다!

개발 진행 중입니다  ✿✿ヽ(°▽°)ノ✿

:::

## 개요

- 이성을 사용하고, 아이템 드랍을 인식해 자동으로 통계 사이트에 업로드 합니다. [펭귄 물류](https://penguin-stats.cn/), [Yituliu](https://ark.yituliu.cn/)
- 자동으로 오퍼레이터 효율을 고려해 기반시설 교대를 하며, 동시에 사용자 커스텀 교대 기능도 지원합니다. [기반시설 스키마](./protocol/base-scheduling-schema.md)
- 자동으로 공개모집 및 즉시 모집을 선택할 수 있으며, 자동으로 통계 사이트에 업로드 합니다. [펭귄 물류](https://penguin-stats.cn/result/stage/recruit/recruit), [Yituliu](https://ark.yituliu.cn/survey/maarecruitdata)
- 공개 모집 화면에서 태그 인식을 할 수 있습니다.
- 오퍼레이터 목록을 인식해, 보유중인 오퍼레이터들 및 잠재를 통계화하여 공개모집 태그 인식 시 표시합니다.
- 현재 보유중인 육성 재화를 인식해 다음 사이트로 데이터를 내보낼 수 있습니다. [펭귄 물류 플래너](https://penguin-stats.cn/planner), [Arkntools](https://arkntools.app/#/material), [ark-nights](https://ark-nights.com/settings)
- 공개 모집 인터페이스에 수동 식별을 지원하여 높은 등급의 오퍼레이터 공개 모집을 용이하게 합니다.
- 오퍼레이터 목록을 식별하고, 보유 및 미보유 오퍼레이터와 그들의 잠재를 계산하며, 이를 공개 모집 인터페이스에서 표시하는 것을 지원합니다.
- 원클릭으로 친구 방문 및 크레딧 획득/구매를 하며, 임무 보상 수령 등의 모든 일과를 수행합니다.
- 통합전략에서 자동으로 오퍼레이터 레벨을 인식하며, 오리지늄 각뿔 획득 및 노드 개방과 레벨을 올려줍니다.
- Copliot 파일을 통한 자동 지휘가 가능합니다. [영상 설명](https://www.bilibili.com/video/BV1H841177Fk/)
- C, Python, Java, Rust, Golang, Java HTTP, Rust HTTP 다음과 같은 다양한 환경에서 MAA를 사용할 수 있습니다!！

말로만 설명하기보다는 사진으로 보여드리겠습니다!

<!-- markdownlint-disable -->

<div class="image-ko-kr">
  <img src="/image/ko-kr/readme/1-light.png" />
  <img src="/image/ko-kr/readme/2-light.png" />
</div>

<style>
  .image-ko-kr {
    display: flex;
    justify-content: space-evenly;
    align-items: center;
    flex-wrap: wrap;
  }

  .image-ko-kr > img {
     box-sizing: border-box;
     width: 50% !important;
     padding: 9px;
     border-radius: 16px;
  }

  @media (max-width: 419px){
    .image-ko-kr > img {
      width: 100% !important;
    }
  }
</style>

<!-- markdownlint-restore -->

## 다운로드

[안정 버전](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)  
[베타 버전](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

## 사용 방법

### 기본 설정

1. [Windows 지원 목록](./manual/device/windows.md) 문서를 참고하여 에뮬레이터/기기를 설정해 주세요.
2. 에뮬레이터 해상도를 `1280x720` 이상으로 변경하세요. 최대값은 `2K`이고 가로세로 비율은 `16:9`입니다.

::: tip
 YostarKR의 경우 게임 인터페이스 문제로 인해 `1280x720` 또는 `1920x1080` 해상도를 권장합니다.
:::

1. 어시스턴트 시작!

자세한 내용은 [사용자 설명서](./manual/introduction/)를 참조해 주세요.

### FAQ

- 프로그램이 실행하자마자 곧바로 튕겨요
- 연결 오류가 발생해요/ADB 경로를 어떻게 작성해야 하는지 모르겠어요
- 연결은 성공했는데, 그대로 멈춰 버렸어요
- 연결 설정(포트 등)을 바꾸고 싶어요
- 다운로드 속도가 너무 느려요/미러 사이트가 접속이 안 돼요
- 다운로드 도중에 “로그인”이나 “인증”이라는 메시지가 떠요

[FAQ](./manual/faq.md) 문서를 참조해 주세요.

### 해외 서버 지원

현재 글로버 서버 EN/KR/JP/txwy 서버의 대부분의 기능이 지원됩니다.
그러나, 해외 서버 사용자가 적고 프로젝트 인원이 부족하기 때문에 대부분의 기능이 자세하게 테스트를 거치지 않았습니다.
그러니 직접 사용해서 어떠한지 확인하시기 바랍니다.
만약 버그가 있거나, 특정 기능에 대한 강한 요구가 있다면 [Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) 및 [토론](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions) 에서 요청하거나 참여해주세요!
현지화는 다음을 참조해주세요. [해외 클라이언트 현지화](./develop/overseas-client-adaptation.md)

### CLI지원

MAA는 명령줄 인터페이스(CLI)를 지원하며, Linux,macOS 및 Window에서 사용할 수 있으며, 자동화 스크립트 작성 및 그래픽 인터페이스가 없는 서버에서 사용할 수 있습니다. [CLI 가이드](./manual/cli/)

## 관련 프로젝트

### 주요 관련 프로젝트

**현재 프로젝트 팀에 프론트엔드 전문가가 매우 부족합니다! 관련 경험이 있으시다면 우리와 함께 해주세요!**

- New Framework: [MaaFramework](https://github.com/MaaAssistantArknights/MaaFramework)
- New GUI: [MaaAsstElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI)
- [전략 파일 저장소](https://prts.plus): Frontend [maa-copilot-frontend](https://github.com/MaaAssistantArknights/maa-copilot-frontend)
- [전략 파일 저장소](https://prts.plus): Backend [MaaBackendCenter](https://github.com/MaaAssistantArknights/MaaBackendCenter)
- [공식 웹사이트](https://maa.plus): [frontend](https://github.com/MaaAssistantArknights/maa-website)
- Deep Learning: [MaaAI](https://github.com/MaaAssistantArknights/MaaAI)

### 다국어 지원 (i18n)

MAA는 다국어를 지원하며, Weblate를 사용하여 로컬라이징을 관리합니다. 여러 언어를 알고 계시다면 [MAA Weblate](https://weblate.maa-org.net)로 이동하여 번역에 도음을 주세요.

MAA는 중국어(간체)를 기본 언어로 하며, 번역 단어는 중국어(간체)를 기준으로 합니다.

[![Weblate](https://weblate.maa-org.net/widgets/maa-assistant-arknights/zh_Hans/maa-wpf-gui/multi-auto.svg)](https://weblate.maa-org.net/engage/maa-assistant-arknights/zh_Hans/)

### Windows

1. 미리 빌드된 서드파티 라이브러리 다운로드

    ```cmd
    python maadeps-download.py
    ```

2. Visual Studio 2022 로 `MAA.sln`파일을 열고, `MaaWpfGui`를 마우스 우클릭을 해 시작 프로젝트로 설정합니다.
3. VS 상단에서 `RelWithDebInfo` `x64`을 선택합니다. (릴리즈 패키지 또는 ARM 플랫폼을 컴파일하는 경우 이 단계를 무시하세요)
4. `MaaWpfGui` - 속성 - 디버깅 - 로컬 디버깅 활성화（이렇게 하면 C++ Core에 중단점을 설정할 수 있습니다）
5. （선택사항）PR을 제출하기전에, [clang-format 지원](./develop/development.md#visual-studio에서-clang-format-사용-설정)을 활성화하는 것이 좋습니다.

### Linux | macOS

[Linux 가이드](./develop/linux-tutorial.md)을 참조하세요

### API

- [C 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/include/AsstCaller.h)：[예제](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Cpp/main.cpp)
- [Python 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Python/asst/asst.py)：[예제](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Python/sample.py)
- [Golang 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Golang)：[예제](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Golang/maa/maa.go)
- [Dart 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Dart)
- [Java 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaCore.java)：[예제](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaJavaSample.java)
- [Java HTTP 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/Readme.md)
- [Rust 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Rust/src/maa_sys)：[HTTP 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Rust)
- [TypeScript 인터페이스](https://github.com/MaaAssistantArknights/MaaX/tree/main/packages/main/coreLoader)
- [Woolang 인터페이스](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Woolang/maa.wo)：[예제](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Woolang/demo.wo)
- [통합문서](./protocol/integration.md)
- [콜백 스키마](./protocol/callback-schema.md)
- [전투 스키마](./protocol/copilot-schema.md)
- [작업 스키마](./protocol/task-schema.md)

### 해외 서버 현지화

[해외 클라이언트 현지화](./develop/overseas-client-adaptation.md)를 참조하세요. 대부분은 스크린샷 + 간단한 json 수정만 필요합니다.

### 개발에 기여를 원하지만, GitHub 사용이 어려운 경우?

[GitHub Pull Request 가이드](./develop/pr-tutorial.md)

### Issue bot

[Issue bot 사용법](./develop/issue-bot-usage.md)

## 감사의 말

### 오픈소스 라이브러리

- 이미지 인식: [opencv](https://github.com/opencv/opencv.git)
- ~~글자 인식: [chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- 글자 인식: [PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- ML Deployment: [FastDeploy](https://github.com/PaddlePaddle/FastDeploy)
- ML accelerator: [onnxruntime](https://github.com/microsoft/onnxruntime)
- ~~아이템 드랍 인식: [Penguin Stats recognizer](https://github.com/penguin-statistics/recognizer)~~
- 맵 타일 인식: [Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON 라이브러리: [meojson](https://github.com/MistEO/meojson.git)
- C++ 오퍼레이터 파서: [calculator](https://github.com/kimwalisch/calculator)
- ~~C++ Base64 인코딩/디코딩: [cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ ZIP 라이브러리: [zlib](https://github.com/madler/zlib)
- C++ Gzip 라이브러리: [gzip-hpp](https://github.com/mapbox/gzip-hpp)
- 안드로이드 터치 이벤트 구현: [Minitouch](https://github.com/DeviceFarmer/minitouch)
- 안드로이드 터치 이벤트 구현: [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)
- WPF MVVM 프레임워크: [Stylet](https://github.com/canton7/Stylet)
- WPF 조작 라이브러리: [HandyControl](https://github.com/HandyOrg/HandyControl)
- C# JSON 라이브러리: [Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json)
- 다운로더: [aria2](https://github.com/aria2/aria2)

### 데이터 소스

- ~~공개모집 데이터: [ArkTools](https://www.bigfun.cn/tools/aktools/hr)~~
- ~~오퍼레이터/기반시설 데이터: [PRTS Arknights Wiki (Chinese)](http://prts.wiki/)~~
- 스테이지 데이터: [Penguin Stats](https://penguin-stats.io/)
- 게임 데이터/리소스: [Arknights Game Resource](https://github.com/yuanyan3060/ArknightsGameResource)
- ~~게임 데이터: [Arknights Game Data](https://github.com/Kengxxiao/ArknightsGameData)~~

### 기여자

MAA의 개선을 위한 개발/테스트에 기여해준 모든 친구들에게 감사드립니다! (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=114514&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## 주의사항

- 이 프로그램의 로고는 AGPL 3.0 라이센스의 적용 대상이 아닙니다. [耗毛](https://weibo.com/u/3251357314)와 Vie 두 명의 아티스트와 프로그램의 개발자들이 모든 권리를 가집니다. 프로젝트가 AGPL 3.0 라이센스 하에 있다고 하더라도 프로그램의 로고는 동의 없이 사용되어서는 안 되며, 동의 없는 상업적 이용 또한 금지됩니다.
- 이 프로그램은 오픈소스이자 무료이며 학습 및 커뮤니케이션 전용입니다. 이 프로그램을 이용해 장비값이나 시간당 수수료 등을 대가로 서비스를 제공하는 판매자로 인해 발생하는 문제와 결과는 프로그램의 개발자들에게는 책임이 없습니다.

## 광고

사용자 그룹: [Telegram](https://t.me/+Mgc2Zngr-hs3ZjU1), [QQ 그룹](https://ota.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)  
[전략 JSON 공유](https://prts.plus)  
[빌리빌리 라이브](https://live.bilibili.com/2808861): 코딩 과정을 밤마다 방송하고 있습니다. 이 프로그램에 대부분의 시간을 할애하고 있습니다.  
[명일방주 무관 기술 공유 & 만담 (QQ 그룹)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2): 지옥 같아요!  
[개발자 그룹 (QQ 그룹)](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)

프로그램이 도움이 된다고 생각하시면 Star를 눌러주세요! (페이지 우측 상단의 작은 별) 저희에게 가장 큰 도움이 됩니다!

<!-- markdownlint-disable-file MD034 MD041 -->
