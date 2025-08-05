---
order: 3
icon: teenyicons:linux-alt-solid
---

# Linux 지원

## 준비 작업

다음중 하나의 설치 방법을 선택하세요.

### maa-cli 사용

[maa-cli](https://github.com/MaaAssistantArknights/maa-cli)는 Rust로 작성된 간단한 MAA 커맨드 라인 도구입니다. 관련 설치 및 사용 방법은 [CLI 가이드](../cli/)를 참조하세요.

### Wine 사용

MAA WPF GUI는 현재 Wine을 통해 실행할 수 있습니다.

#### 설치 단계

1. [.NET 릴리스 페이지](https://dotnet.microsoft.com/en-us/download/dotnet/8.0)에서 Windows용 .NET **데스크톱** 런타임을 다운로드하고 설치합니다.

2. Windows용 MAA를 다운로드하고 압축을 푼 후 `wine MAA.exe`를 실행합니다.

::: info 주의
연결 설정에서 ADB 경로를 [Windows용 `adb.exe`](https://dl.google.com/android/repository/platform-tools-latest-windows.zip)로 설정해야 합니다.

ADB를 통해 USB 장치에 연결해야 하는 경우, 먼저 Wine 외부에서 `adb start-server`를 실행하여 네이티브 ADB 서버에 Wine을 통해 연결하세요.
:::

#### Linux 네이티브 MaaCore 사용 (실험적 기능)

[MAA Wine Bridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/MaaWineBridge) 소스 코드를 다운로드하고 빌드하여, 생성된 `MaaCore.dll`(ELF 파일)로 Windows 버전을 교체하고, Linux 네이티브 동적 라이브러리(`libMaaCore.so` 및 종속성)를 같은 디렉토리에 넣습니다.

이때 Wine을 통해 `MAA.exe`를 실행하면 Linux 네이티브 동적 라이브러리가 로드됩니다.

::: info 주의
Linux 네이티브 MaaCore를 사용할 때는 연결 설정에서 ADB 경로를 Linux 네이티브 ADB로 설정해야 합니다.
:::

#### Linux 데스크톱 통합 (실험적 기능)

데스크톱 통합은 네이티브 데스크톱 알림 지원과 fontconfig 글꼴 구성을 WPF에 매핑하는 기능을 제공합니다.

MAA Wine Bridge에서 생성된 `MaaDesktopIntegration.so`를 `MAA.exe`와 같은 디렉토리에 넣으면 활성화됩니다.

#### 알려진 문제

- Wine DirectWrite는 강제로 힌팅을 활성화하고 DPI를 FreeType에 전달하지 않아 글꼴 표시 효과가 좋지 않습니다.
- 네이티브 데스크톱 알림을 사용하지 않을 때, 팝업 알림이 전체 시스템의 마우스 포커스를 빼앗아 다른 창을 조작할 수 없게 됩니다. `winecfg`를 통해 가상 데스크톱 모드를 활성화하거나 데스크톱 알림을 비활성화하여 완화할 수 있습니다.
- Wine-staging 사용자는 MAA가 Wine 환경을 올바르게 감지할 수 있도록 `winecfg`의 `Wine 버전 숨기기` 옵션을 비활성화해야 합니다.
- Wine의 Light 테마는 WPF에서 일부 텍스트 색상 이상을 일으키므로, `winecfg`에서 테마 없음(Windows 클래식 테마)으로 전환하는 것을 권장합니다.
- Wine은 구식 XEmbed 트레이 아이콘을 사용하므로 GNOME에서 정상적으로 작동하지 않을 수 있습니다.
- Linux 네이티브 MaaCore를 사용할 때는 자동 업데이트가 지원되지 않습니다 (~~업데이트 프로그램: 내가 Windows 버전을 다운로드해야 한다고?~~)

### Python 사용

#### 1. MAA 동적 라이브러리 설치

1. [MAA 공식 웹사이트](https://maa.plus/)에서 리눅스 라이브러리를 다운로드하고 압축을 풉니다. 혹은 소프트웨어 저장소에서 설치합니다:

   - AUR：[maa-assistant-arknights](https://aur.archlinux.org/packages/maa-assistant-arknights)을 설치한 후에 설치 지침에 따라 파일을 편집합니다.
   - Nixpkgs: [maa-assistant-arknights](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)

2. `./MAA-v{버전}-linux-{아키텍처}/Python/` 폴더로 이동하여 `sample.py` 파일을 엽니다.

::: tip
사전 컴파일된 버전은 상대적으로 최신 버전의 리눅스 배포판 (Ubuntu 22.04)에서 컴파일된 동적 라이브러리를 포함하고 있습니다. 시스템의 libstdc++ 버전이 오래되어 ABI 호환성 문제가 발생할 수 있습니다. [Linux 컴파일 가이드](../../develop/linux-tutorial.md)를 참조하여 다시 컴파일하거나 컨테이너를 실행할 수 있습니다.
:::

#### 2. `adb` 구성

1. [`if asst.connect('adb.exe', '127.0.0.1:5554'):`](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L48) 줄을 찾습니다.

2. `adb` 도구 호출

   - 에뮬레이터가 `Android Studio`의 `avd`를 사용하는 경우 `adb`가 내장되어 있습니다. `adb.exe` 필드에 `adb` 경로를 입력합니다. 일반적으로 `$HOME/Android/Sdk/platform-tools/` 폴더에 있습니다. 예시:

   ```python
   if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "모바일 에뮬레이터의 adb 주소"):
   ```

   - 다른 에뮬레이터를 사용하는 경우 `adb`를 먼저 다운로드해야 합니다: `$ sudo apt install adb`를 실행한 후 경로를 입력하거나 `PATH` 환경 변수를 사용하여 `adb`를 입력합니다.

3. 에뮬레이터의 `adb` 경로 확인:

   - 직접 `adb` 도구를 사용할 수 있습니다: `$ adb경로 devices`를 실행하면 됩니다. 예시:

   ```shell
   $ /home/foo/Android/Sdk/platform-tools/adb devices
   List of devices attached
   emulator-5554 device
   ```

   - 반환된 `emulator-5554`가 에뮬레이터의 `adb` 주소이므로 `127.0.0.1:5555`를 덮어씁니다. 예를 들어:

   ```python
   if asst.connect("/home/foo/Android/Sdk/platform-tools/adb", "emulator-5554"):
   ```

4. 이제 테스트를 진행해볼 수 있습니다: `$ python3 sample.py`를 실행한 후 연결 성공이 표시되면 대부분의 작업이 완료된 것입니다.

#### 3. 작업 설정

사용자 정의 작업: [통합 문서](../../protocol/integration.md)를 참조하고, `sample.py`의 # 작업 및 매개 변수는 [예제](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/722f0ddd4765715199a5dc90ea1bec2940322344/src/Python/sample.py#L54)를 참조해 줄을 수정하세요.

## 모바일 시뮬레이터 지원

### ✅ [AVD](https://developer.android.com/studio/run/managing-avds)

필수 구성: 16:9 비율의 화면 해상도이며 해상도는 720p보다 커야 합니다.

권장 구성: MAA의 리눅스 x64 동적 라이브러리와 함께 x86_64 프레임워크 (R - 30 - x86_64 - Android 11.0) 사용

참고: Android 10 이상에서는 SELinux가 `Enforcing` 모드인 경우 `Minitouch`를 사용할 수 없습니다. 다른 터치 모드로 전환하거나 SELinux를 **임시**로 `Permissive` 모드로 전환하세요.

### ⚠️ [Genymotion](https://www.genymotion.com/)

상위 안드로이드 버전에는 x86_64 프레임워크가 기본으로 포함되어 있습니다. 가볍지만 Arknights를 실행하는 동안 종종 비정상 종료됩니다.

엄격한 테스트가 아직 진행되지 않았으며 adb 기능 및 경로 검색에 문제가 없습니다.

## 안드로이드 컨테이너 지원

::: tip
다음 솔루션은 일반적으로 커널 모듈을 필요로 합니다. 해당 솔루션 및 배포판에 맞는 내부 모듈을 설치하세요.
:::

### ✅ [Waydroid](https://waydro.id/)

설치 후 해상도를 다시 설정해야 합니다. (또는 720P 이상이고 16:9 비율의 해상도를 사용하고 다시 시작해야 합니다):

```shell
waydroid prop set persist.waydroid.width 1280
waydroid prop set persist.waydroid.height 720
```

adb의 IP 주소 설정: `설정` - `정보` - `IP 주소`를 열고 첫 번째 `IP`를 기록하여 `${기록된IP}:5555`를 `sample.py`의 adb IP에 입력하세요.

amdgpu를 사용하는 경우 `screencap` 명령이 stderr에 정보를 출력하여 이미지 디코딩이 실패할 수 있습니다. `adb exec-out screencap | xxd | head`를 실행하여 출력에 `/vendor/etc/hwdata/amdgpu.ids: No such file...`와 같은 텍스트가 있는지 확인하세요. `resource/config.json` 파일에서 스크린샷 명령을 `adb exec-out 'screencap 2>/dev/null'`로 변경해보세요.

### ✅ [redroid](https://github.com/remote-android/redroid-doc)

안드로이드 11 버전 이미지는 게임을 정상적으로 실행합니다. 5555 adb 포트를 공개해야 합니다.
