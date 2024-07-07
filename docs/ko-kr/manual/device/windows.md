---
order: 1
icon: ri:windows-fill
---

# Windows 지원

아래의 목록은 무작위순이며, 성능순이 아닙니다.

<script setup>
import MarkdownIt from 'markdown-it'
import MarkdownItAnchor from 'markdown-it-anchor'

const shuffleArray = (array) => {
    for (let i = array.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [array[i], array[j]] = [array[j], array[i]];
    }
    return array;
}

const fullySupport = shuffleArray([
`
### ✅ [Bluestacks 5](https://www.bluestacks.com/ko/) (권장 👍)

완벽히 지원됩니다. \`설정\` - \`고급 설정\`에서 \`ADB 기능\`을 켜야 합니다.

- 번들 및 느린 설치를 피하기 위해 [오프라인 설치 파일](https://support.bluestacks.com/hc/ko/articles/4402611273485-%EB%B8%94%EB%A3%A8%EC%8A%A4%ED%83%9D-5-%EC%98%A4%ED%94%84%EB%9D%BC%EC%9D%B8-%EC%84%A4%EC%B9%98-%ED%94%84%EB%A1%9C%EA%B7%B8%EB%9E%A8)을 다운로드하는 것이 좋습니다. [Android 11](https://support.bluestacks.com/hc/ko/articles/4402611273485-%EB%B8%94%EB%A3%A8%EC%8A%A4%ED%83%9D-5-%EC%98%A4%ED%94%84%EB%9D%BC%EC%9D%B8-%EC%84%A4%EC%B9%98-%ED%94%84%EB%A1%9C%EA%B7%B8%EB%9E%A8) 버전을 권장합니다. 제거하려면, 남은 파일들을 제거하려면 [제거 도구](https://support.bluestacks.com/hc/ko/articles/360057724751-PC%EC%97%90%EC%84%9C-%EB%B8%94%EB%A3%A8%EC%8A%A4%ED%83%9D-5-%EB%B8%94%EB%A3%A8%EC%8A%A4%ED%83%9D-X-%EB%B0%8F-%EB%B8%94%EB%A3%A8%EC%8A%A4%ED%83%9D-%EC%84%9C%EB%B9%84%EC%8A%A4%EB%A5%BC-%EC%99%84%EC%A0%84%ED%9E%88-%EC%A0%9C%EA%B1%B0%ED%95%98%EB%8A%94-%EB%B0%A9%EB%B2%95)를 사용하세요.
- adb 포트 번호가 불규칙적으로 변경되고 시작할 때마다 다르면 컴퓨터에 [Hyper-V](https://support.bluestacks.com/hc/ko/articles/4415238471053-Hyper-V-%EC%A7%80%EC%9B%90-%EC%9C%88%EB%8F%84%EC%9A%B0-10-%EB%B0%8F-11%EC%9D%98-%EB%B8%94%EB%A3%A8%EC%8A%A4%ED%83%9D-5%EC%97%90-%EB%8C%80%ED%95%9C-%EC%8B%9C%EC%8A%A4%ED%85%9C-%EC%9A%94%EA%B5%AC-%EC%82%AC%ED%95%AD)가 활성화되어 있을 수 있습니다. MAA는 이제 블루스택 에뮬레이터 구성 파일 내에서 포트 번호를 자동으로 읽도록 시도할 것입니다. 작동하지 않거나 다중 오픈/다중 에뮬레이터 커널이 필요한 경우 [자주 묻는 질문](../faq.md#블루스택-에뮬레이터가-매번-시작될-때마다-포트-번호가-다릅니다-hyper-v)을 참조하여 변경하세요. Hyper-V가 관리자로 실행되므로 에뮬레이터 자동 종료, 자동 연결 감지 등 adb를 사용하지 않는 작업도 관리자로 실행해야 합니다.
`,
`
### ✅ [MuMu Emulator 12](https://mumu.163.com/)

완벽하게 호환되며, 독점적인 익스트림 컨트롤 모드를 추가로 지원합니다.

- \`완료 후 에뮬레이터 종료\` 기능은 때때로 비정상적일 수 있으며, 그런 경우에는 MuMu 공식으로 문의하세요.
- MuMu 12 버전 3.5.4 ~ 3.5.7을 사용 중인 경우, MuMu 12 설정 - 기타에서 \`백그라운드에서 활성 상태 유지\` 기능을 비활성화해야 합니다. \(자세한 내용은 [공식 공지사항](https://mumu.163.com/help/20230802/35047_1102450.html) 참고\)
- 여러 인스턴스를 열 때는 MuMu 12 멀티 인스턴스의 ADB 버튼을 통해 해당 인스턴스의 포트 정보를 확인하고, MAA \`설정\` → \`연결 설정\`에서 연결 주소의 포트 번호를 해당 포트로 변경해야 합니다.

#### MuMu 스크린샷 강화 모드

공식 MuMu 12 3.8.13 이후 버전을 사용해야 하며, 백그라운드 실행을 닫아야 합니다. 아크 에디션 및 국제 에디션은 현재 지원되지 않습니다.

##### 연결 설정

1. \`설정\` → \`연결 설정\`, \`MuMu 스크린샷 강화 모드 사용\` 확인란을 선택합니다.

2. \`MuMu 에뮬레이터 경로\`에는 \`MuMuPlayer-12.0\` 폴더의 경로를 입력합니다. 예: \`C:\\Program Files\\Netease\\MuMuPlayer-12.0\`.

3. \`인스턴스 번호\`에는 MuMu Multiplayer의 해당 에뮬레이터의 일련 번호를 입력합니다. 예: 주요 멀티플레이어의 경우 \`0\`.

4. \`인스턴스 스크린\`에는 \`0\`을 입력합니다.

##### 백그라운드 실행 상태에 대해

꺼놓는 것이 좋습니다. 이때 인스턴스 화면은 항상 \`0\`입니다.

켜져 있으면 MuMu 에뮬레이터 탭의 순서가 인스턴스 화면의 일련 번호여야 합니다. 예: 에뮬레이터 데스크톱은 \`0\`, 명일방주 클라이언트는 \`1\`입니다.

백그라운드 실행에 대한 적용은 매우 불완전하며, 원인 미상의 문제가 항상 발생하므로 권장하지 않습니다.
`,
`
### ✅ [Nox](https://kr.bignox.com/)

완벽히 지원됩니다.
`,
`
### ✅ [Nox Android 9](https://kr.bignox.com/)

완벽히 지원됩니다.
`,
`
### ✅ [Memu](https://www.memuplay.com/ko/)

지원되지만, 테스트 수가 적으며 알려지지 않은 오류가 있을 수 있습니다.
`,
`
### ✅ [Android Virtual Device](https://developer.android.com/studio/run/managing-avds)

지원됩니다.
`,
])

const particallySupport = shuffleArray([
`
### ⚠️ [Google Play Games Beta](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)

호환되지 않습니다. [클라이언트](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)의 ADB 포트를 사용할 수 없습니다.

단, KR의 경우 [PlayBridge](https://github.com/ACK72/PlayBridge)를 이용해 사용이 가능하지만, 공식 지원이 아니므로 서비스가 불안정할 수 있습니다.
`,
`
### ⚠️ [MuMu Player](https://www.mumuglobal.com/kr/)

지원됩니다. 단,

- minitouch 또는 maatouch의 효율적인 터치 방식을 사용하려면 adb를 강제 교체해야 합니다.
- MAA가 adb 경로와 주소를 얻기 위해 관리자 권한으로 실행되어야 합니다. (MuMu가 관리자 권한으로 실행되기 때문입니다)
- 관리자 권한으로 실행하고 싶지 않다면 adb 경로와 주소를 직접 입력할 수도 있습니다.
- MAA의 기본 해상도 사용은 권장되지 않습니다. \`1280×720\`, \`1920×1080\`, \`2560×1440\` 등의 일반적인 해상도를 사용해 주세요.
- MuMu는 여러 개를 열어도 하나의 adb 포트를 사용하므로, 여러 개의 클라이언트를 지원하지는 못합니다.
`,
`
### ⚠️ [LDPlayer](https://kr.ldplayer.net/)

지원됩니다.

- LDPlayer 9는 9.0.37 버전 이상을, LDPlayer 5는 5.0.44 버전 이상을 권장드립니다.
- minitouch 또는 maatouch의 효율적인 터치 방식을 사용하려면 adb를 강제 교체해야 합니다.
`,
`
### ⚠️ [Windows Subsystem for Android](https://learn.microsoft.com/ko-kr/windows/android/wsa/)

MAA v5.2.0부터 지원이 중단되었으며, 마이크로소프트가 2025년 03월 05일에 서비스를 중단할 예정입니다.

- WSA 2204 이상 (버전 번호는 서브시스템 설정의 'About' 페이지에 있음)의 경우 연결 구성에서 \`일반 구성\`을 선택합니다.
- WSA 2203 또는 그 이전 버전 (버전 번호는 서브시스템 설정 페이지 상단에 있음)의 경우 연결 구성에서 \`이전 버전 WSA\`를 선택합니다.
- 이 소프트웨어는 720p 이상의 \`16:9\` 해상도를 지원합니다. 모니터가 16:9인 경우 가능한 한 창 크기를 수동으로 최대한 16:9 비율로 조정하세요. (모니터가 16:9인 경우 전체 화면으로 전환하려면 \`F11\`을 누르세요)
- 내일의 방주가 화면에 보이고 다른 Android 앱이 동시에 전경에서 실행되지 않도록 최대한 노력하세요. 그렇지 않으면 게임이 일시 중지되거나 작업 인식 오류가 발생할 수 있습니다.
- WSA의 스크린샷은 종종 백색 화면을 캡처하여 인식의 이상 또는 사용을 권장하지 않습니다.
`,
])

const notSupport = shuffleArray([
`
### 🚫 Tencent Mobile Game Assistant

지원되지 않습니다. ADB 포트가 닫혀 있습니다.
`,
`
### 🚫 MuMu Mobile Game Assistant

지원되지 않습니다. ADB 포트가 닫혀 있습니다.
`,
])

const md = (new MarkdownIt()).use(MarkdownItAnchor, { permalink: MarkdownItAnchor.permalink.linkInsideHeader()})

const fullySupportHtml = md.render(fullySupport.join(''))
const partiallySupportHtml = md.render(particallySupport.join(''))
const notSupportHtml = md.render(notSupport.join(''))

</script>

## ✅ 완벽한 지원

<ClientOnly><div v-html="fullySupportHtml"></div></ClientOnly>

## ⚠️ 부분 지원

<ClientOnly><div v-html="partiallySupportHtml"></div></ClientOnly>

## 🚫 미지원

<ClientOnly><div v-html="notSupportHtml"></div></ClientOnly>
