---
order: 2
icon: basil:apple-solid
---

# Mac 지원

## Apple Silicon 칩셋

### ✅ [PlayCover](https://playcover.io)（제일 부드럽습니다! 🚀）

실험적으로 지원되며 문제가 발생하면 Issue를 바로 보고하고, 제목에 iOS를 명시하세요.

참고: macOS 자체적인 문제로 인해 게임 창을 최소화하거나 배경으로 이동한 후 다른 창으로 전환하거나 창을 다른 데스크톱/화면으로 이동하는 경우 스크린샷이 문제가 발생하여 제대로 작동하지 않을 수 있습니다. 관련된 이슈 [#4371](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/4371#issuecomment-1527977512)

0. 요구 사항: MAA 버전 v4.13.0-rc.1 이상

1. [fork버전의 PlayCover](https://github.com/hguandl/PlayCover/releases)를 다운로드하고 설치하세요.

2. [복호화된 명일방주 설치 파일](https://decrypt.day/app/id1454663939)을 다운로드하고 PlayCover에 설치하세요.

3. PlayCover에서 명일방주를 우클릭하여 `설정` - `우회`를 선택하고 `PlayChain 사용`, `탈옥 검사 우회 사용`, `내장 라이브러리 삽입`, `MaaTools`를 체크한 다음 확인을 클릭하세요.

4. 이제 명일방주를 다시 시작하면 됩니다. 제목 표시줄 끝에 `[localhost:포트 번호]`가 있으면 성공적으로 활성화된 것입니다.

5. MAA에서 `설정` - `연결 설정`을 클릭하고 터치 모드를 `MacPlayTools`로 설정하세요. `연결 주소란`에 제목 표시줄 `[]` 안에 있는 내용을 입력하세요.

6. 설정이 완료되면 MAA에 연결할 수 있습니다. 이미지 인식 오류가 발생하는 경우 PlayCover 내에서 해상도를 1080P로 설정해 보세요.

7. 3-5 단계는 한 번만 수행하면 됩니다. 이후 명일방주를 다시 업데이트한 후에는 2단계를 다시 수행해야 합니다.

### ✅ [MuMu 에뮬레이터 Pro](https://mumu.163.com/mac/)

지원됩니다. 그러나 테스트는 적고 `MacPlayTools` 이외의 터치 모드를 사용해야 합니다. 관련된 이슈 [#8098](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/8098)

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

지원됩니다. 그러나 Android 10부터는 SELinux가 `Enforcing` 모드일 때 `Minitouch`를 사용할 수 없습니다. 다른 터치 모드로 전환하거나 SELinux를 **임시**로 `Permissive` 모드로 변경하세요.

### ✅ [블루스택 에어](https://www.bluestacks.com/mac) (무료, Apple M 시리즈 칩에 최적화된 버전)

지원되며, 테스트 결과 maatouch를 통해 `127.0.0.1:5555`로 연결할 수 있습니다.

에뮬레이터의 **`설정`** - **`고급`**에서 **`Android 디버깅(ADB)`**을 활성화해야 합니다。

## Intel 칩셋

::: tip
Mac 버전 개발에 인력이 부족하여 업데이트 속도가 비교적 느립니다. Mac에 내장된 멀티 시스템 기능을 사용해 Windows를 설치하고 Windows 버전 MAA를 사용하는 것을 추천합니다.
:::

### ✅ [블루스택 에뮬레이터 CN](https://www.bluestacks.cn/)

완벽하게 지원됩니다. 에뮬레이터에서 `설정` - `엔진 설정`에서 `ADB 연결 허용`을 활성화해야 합니다.

### ✅ [블루스택 에뮬레이터 글로벌](https://www.bluestacks.com/tw/index.html)

완벽하게 지원됩니다. 에뮬레이터에서 `설정` - `고급`에서 `Android 디버깅 브릿지`를 활성화해야 합니다.

### ✅ [녹스 에뮬레이터](https://www.yeshen.com/)

완벽하게 지원됩니다.

추가로, macOS에서 녹스 에뮬레이터의 adb 이진 파일의 위치는 `/Applications/NoxAppPlayer.app/Contents/macOS/adb`이며 부모 디렉토리 `macOS`에서 `adb devices` 명령을 사용하여 adb 포트를 확인할 수 있습니다.

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

지원됩니다. 그러나 Android 10부터는 SELinux가 `Enforcing` 모드일 때 `Minitouch`를 사용할 수 없습니다. 다른 터치 모드로 전환하거나 SELinux를 **임시**로 `Permissive` 모드로 변경하세요.
