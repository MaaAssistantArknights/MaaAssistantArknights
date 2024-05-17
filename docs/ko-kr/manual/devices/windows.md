---
order: 1
icon: ri:windows-fill
---

# Windows 에뮬레이터 지원

::: tip
문제가 발생하면 [FAQ](../faq.md)를 먼저 참조하세요.
:::

아래의 목록은 무작위순이며, 성능순이 아닙니다.

## ✅ 완벽한 지원

### ✅ [Bluestacks-CN 5](https://www.bluestacks.cn/)

완벽히 지원됩니다. `Settings` - `Engine Settings`에서 `Allow ADB connection`을 켜야 합니다.

### ✅ [Bluestacks 5](https://www.bluestacks.com/ko/) (권장 👍)

완벽히 지원됩니다. `Settings` - `Advanced`에서 `Android Debug Bridge`를 켜야 합니다.

### ✅ [Bluestacks 5 Hyper-V 버전](https://support.bluestacks.com/hc/ko-kr/articles/4415238471053-System-requirements-for-BlueStacks-5-on-Hyper-V-enabled-Windows-10-and-11-)

지원됩니다.

- `Settings`-`Advanced`에서 `Android Debug Bridge`를 켜 주세요.
- Bluestack Hyper-V 포트는 수시로 변경됩니다. 간단한 해법을 알려드립니다.

    1. 에뮬레이터의 데이터 경로에서 `bluestacks.conf` 파일을 찾으세요. (기본값: `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`)
    2. MAA를 최초로 사용하신다면, 실행하실 때 `gui.json` 파일이 생성됩니다.
    3. MAA를 **종료하고 나서**, `gui.json` 파일을 열고, `Bluestacks.Config.Path` 필드를 추가하세요. 값은 `bluestacks.conf`의 절대 경로로 설정합니다. (백슬래시는 `\\`와 같이 이스케이프되어야 합니다).
    예시: (파일이 `C:\ProgramData\BlueStacks_nxt\bluestacks.conf`에 위치해 있음을 가정함)

        ```json
        {
            "Configurations": {
                "Default": {
                    "Bluestacks.Config.Path":"C:\\ProgramData\\BlueStacks_nxt\\bluestacks.conf",
                    // ...
                }
            }
        }
        ```

    4. LinkStart!

- 여러 에뮬레이터를 구동해야 하는 경우 (해당되지 않으면 이 단계는 넘어가주세요), MAA의 키워드를 변경해서 구성 파일을 감지할 수 있습니다.
    `Bluestacks.Config.Keyword` 필드를 추가로 입력해주세요
    예시:

    ```json
    "Bluestacks.Config.Keyword":"bst.instance.Nougat64.status.adb_port",
    ```

### ✅ [Nox](https://kr.bignox.com/)

완벽히 지원됩니다.

### ✅ [Nox Android 9](https://kr.bignox.com/)

완벽히 지원됩니다.

### ✅ [Memu](https://www.memuplay.com/ko/)

지원되지만, 테스트 수가 적으며 알려지지 않은 오류가 있을 수 있습니다.

### ✅ [Android Virtual Device](https://developer.android.com/studio/run/managing-avds)

지원됩니다.

## ⚠️ 부분 지원

### ⚠️ [MuMu Player](https://www.mumuglobal.com/kr/)

지원됩니다. 단,

- minitouch 또는 maatouch의 효율적인 터치 방식을 사용하려면 adb를 강제 교체해야 합니다.
- MAA가 adb 경로와 주소를 얻기 위해 관리자 권한으로 실행되어야 합니다. (MuMu가 관리자 권한으로 실행되기 때문입니다)
- 관리자 권한으로 실행하고 싶지 않다면 adb 경로와 주소를 직접 입력할 수도 있습니다.
- MAA의 기본 해상도 사용은 권장되지 않습니다. `1280×720`, `1920×1080`, `2560×1440` 등의 일반적인 해상도를 사용해 주세요.
- MuMu는 여러 개를 열어도 하나의 adb 포트를 사용하므로, 여러 개의 클라이언트를 지원하지는 못합니다.

### ⚠️ [MuMu Player X](https://www.mumuglobal.com/kr/faq/system-requirement-mumu-player-x.html) (가장 부드러움 👍)

지원됩니다. 단,

- minitouch 또는 maatouch의 효율적인 터치 방식을 사용하려면 adb를 강제 교체해야 합니다.
- 에뮬레이터를 종료하는 기능을 사용하려면 버전 3.3.7 이상을 사용해야 합니다.
- 에뮬레이터를 종료하는 기능을 사용하려면 MuMu 12 멀티 오프너의 ADB 버튼으로 해당하는 인스턴스의 포트 정보를 확인하여 MAA 설정의 연결 설정에서 포트를 해당하는 포트로 변경해야 합니다.

### ⚠️ [LDPlayer](https://kr.ldplayer.net/)

지원됩니다.

- LDPlayer 9는 9.0.37 버전 이상을, LDPlayer 5는 5.0.44 버전 이상을 권장드립니다.
- minitouch 또는 maatouch의 효율적인 터치 방식을 사용하려면 adb를 강제 교체해야 합니다.

### ⚠️ [Windows Subsystem for Android](https://learn.microsoft.com/ko-kr/windows/android/wsa/)

부분적으로 지원됩니다.

- 수동 연결을 사용해야 합니다.
- WSA 2204 이상 버전의 경우 '일반 모드'를 시도해 보세요.
- WSA 2203 이하 버전의 경우 'WSA 레거시 버전'을 시도해 보세요.
- WSA는 해상도 변경을 지원하지 않으므로, 수동으로 720p 이상의 `16:9` 비율의 해상도가 되도록 창의 크기를 조절해주세요. (16:9 모니터를 사용하고 있다면 `F11`을 눌러 전체화면으로 전환하는 것으로도 가능합니다)
- 에뮬레이터의 창이 활성화된 상태(가장 위에 있는 상태)를 유지하고 다른 안드로이드 앱이 실행되고 있지 않게 해 주세요. 그렇지 않을 경우 게임이 일시정지되거나 인식에 실패할 수 있습니다.
- 가끔은 WSA의 스크린샷이 비어 있어 인식 실패를 유발합니다. WSA 사용은 권장되지 않습니다.





## 🚫 미지원

### 🚫 [Google Play Games Beta](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)

호환되지 않습니다. [소비자 클라이언트](https://developer.android.com/games/playgames/pg-emulator?hl=zh-cn#installing-game-consumer)의 ADB 포트를 사용할 수 없습니다.

단, KR의 경우 [PlayBridge](https://github.com/ACK72/PlayBridge)를 이용해 사용이 가능하지만, `MAA 5.2.3 버전` 이후는 사용이 불가능합니다.

### 🚫 Tencent Mobile Game Assistant

지원되지 않습니다. ADB 포트가 닫혀 있습니다.

### 🚫 MuMu Mobile Game Assistant

지원되지 않습니다. ADB 포트가 닫혀 있습니다.