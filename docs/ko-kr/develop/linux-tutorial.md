---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux 컴파일 가이드

::: warning
**이 가이드는 독자가 일정한 Linux 환경 설정 능력과 프로그래밍 기초를 가지고 있다고 가정합니다!** MAA를 직접 컴파일하는 대신 단순히 실행하길 원한다면 [사용자 매뉴얼 - Linux 에뮬레이터 및 컨테이너](../manual/device/linux.md)를 참고하세요.
:::

::: info 주의
MAA의 빌드 방법은 여전히 논의 중입니다. 이 가이드의 내용은 최신 내용이 아닐 수 있으므로 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/.github/workflows/ci.yml#L264#:~:text=ubuntu%3A)의 스크립트를 참고하는 것이 좋습니다.  
또한 [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) 또는 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)를 참고할 수도 있습니다.
:::

::: info
Mac은 `tools/build_macos_universal.zsh` 스크립트를 사용하여 컴파일할 수 있습니다.  
MaaAssistantArknights/MaaMacGui 프로젝트의 [README.md](https://github.com/MaaAssistantArknights/MaaMacGui/blob/master/README.md)를 참고하는 것을 권장합니다.
:::

## MaaCore 컴파일

:::: steps

1. 컴파일에 필요한 종속성 다운로드
   ::: code-tabs
   @tab:active Ubuntu/Debian

   ```bash :no-line-numbers
   sudo apt install cmake
   ```

   @tab Arch

   ```bash :no-line-numbers
   sudo pacman -S --needed cmake
   ```

   :::

2. 서드파티 라이브러리 빌드

   다음 방법 중 하나를 선택하세요:

   - 사전 빌드된 서드파티 라이브러리 다운로드 (권장)

     ```bash
     python tools/maadeps-download.py
     ```

     ::: info
     사전 빌드된 서드파티 라이브러리는 [MaaLinuxToolchain](https://github.com/MaaXYZ/MaaLinuxToolchain) 툴체인을 기반으로 크로스 컴파일되었으며, glibc 2.31(Ubuntu 20.04)만 필요합니다. 여전히 ABI 호환성 문제가 발생하는 경우 컨테이너를 사용하거나 서드파티 라이브러리를 직접 빌드해 보세요.
     :::

   - 서드파티 라이브러리 직접 빌드 (오랜 시간이 소요됩니다)

     ```bash
     git clone https://github.com/MaaAssistantArknights/MaaDeps
     cd MaaDeps
     # 시스템 환경이 너무 낮아 사전 빌드된 llvm 20을 사용할 수 없는 경우, 크로스 컴파일을 사용하지 않고 로컬 컴파일 환경을 직접 사용해 보세요.
     # src/MaaUtils/MaaDeps/cmake의 toolchain 구성을 조정해야 합니다.
     python linux-toolchain-download.py
     python build.py
     ```

3. MAA 컴파일

   ```bash
   cmake --preset linux-x64 -DINSTALL_RESOURCE=ON -DINSTALL_PYTHON=ON
   cmake --build build
   cmake --install build --prefix <target_directory>
   ```

   처음 2개 명령어로 `build/bin/Debug/libMaaCore.so`(및 `libMaaUtils.so`)가 생성되고, 3번째 명령어로 컴파일 결과물이 대상 디렉토리에 설치(복사)됩니다.

   ::: info CMake 옵션 설명
   `-DINSTALL_RESOURCE=ON`은 `resource` 디렉토리를 설치 디렉토리로 복사합니다. MaaCore는 `resource` 디렉토리와 함께 실행되어야 합니다.

   `-DINSTALL_PYTHON=ON`은 Python 통합(`src/Python`)을 설치 디렉토리로 복사합니다. Python 연동을 사용하지 않거나 리포지토리 소스 코드에서 직접 사용할 계획이라면 이 옵션을 생략할 수 있습니다.
   :::

   ::: tip
   동적 라이브러리 파일 경로 또는 `LD_LIBRARY_PATH`를 지정하여 MAA를 실행하는 것을 권장하며, root 권한을 사용하여 MAA를 `/usr`에 설치하지 마세요.
   :::

4. MaaFramework 관련 컴포넌트 컴파일

   MaaFwAdbController(MaaFwAdb 터치 방식) 관련 기능을 디버깅하려면 [MaaFramework의 Debug 버전을 직접 컴파일](https://maafw.com/docs/4.1-BuildGuide)하여 `libMaaAdbControlUnit.so`를 설치 디렉토리에 넣어야 합니다.

5. 실행

   [다양한 언어의 API](../readme.md#api), [Python 사용](../manual/device/linux.md#python-사용) 등의 문서를 참고하여 MAA 동적 라이브러리를 호출하세요.

::::

## MaaWpfGui 컴파일

::: info 주의
MaaWpfGui는 Windows용 프로그램으로, 빌드 결과물을 Linux에서 실행하려면 Wine이 필요합니다. 자세한 내용은 [Wine 사용](../manual/device/linux.md#wine-사용)을 참고하세요.
:::

:::: steps
1. `MaaCore.dll` 준비

   MaaWpfGui는 `MaaCore.dll`(및 종속된 기타 DLL)에 의존합니다. 현재 Linux에서 Windows용 `MaaCore.dll`을 크로스 컴파일할 수 없습니다. MAA의 Windows 설치 패키지에서 복사해 올 수도 있지만, 버전 불일치 문제가 발생하기 쉽습니다.

   따라서 권장하는 방식은 이전 절의 지침에 따라 먼저 MaaCore를 컴파일한 후(설치 불필요), [MaaWineBridge](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev-v2/src/MaaWineBridge)를 컴파일하여 생성된 `MaaCore.dll`을 `build/bin/Debug` 디렉토리에 배치하는 것입니다.

2. .NET SDK 설치

   ::: code-tabs
   @tab:active Ubuntu/Debian

   ```bash
   sudo apt install dotnet-sdk-10.0
   ```

   @tab Arch

   ```bash
   sudo pacman -S --needed dotnet-sdk
   ```

   :::

   [Linux에 .NET 설치](https://learn.microsoft.com/en-us/dotnet/core/install/linux)도 참고하세요.

3. MaaWpfGui 컴파일

   리포지토리 루트 디렉토리에서 실행합니다:

   ```bash
   dotnet build src/MaaWpfGui/MaaWpfGui.csproj -p:Platform=x64
   ```

   첫 빌드 시 종속성이 자동으로 다운로드됩니다. 이 단계가 끝나면 `build/bin/Debug/MAA.exe`가 생성됩니다.

4. 실행

   ```bash
   wine build/bin/Debug/MAA.exe
   ```
::::
