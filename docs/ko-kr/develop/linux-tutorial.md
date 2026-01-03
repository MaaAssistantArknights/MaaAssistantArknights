---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux 컴파일 가이드

**이 가이드는 독자가 일정한 Linux 환경 설정 능력과 프로그래밍 기초를 가지고 있다고 가정합니다!** MAA를 직접 컴파일하는 대신 단순히 설치하길 원한다면 [사용자 매뉴얼 - Linux 에뮬레이터 및 컨테이너](../manual/device/linux.md)를 참고하세요.

::: info 주의
MAA의 빌드 방법은 여전히 논의 중입니다. 이 가이드의 내용은 최신 내용이 아닐 수 있으므로 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134)의 스크립트를 참고하는 것이 좋습니다.  
또한 [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) 또는 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)를 참고할 수도 있습니다.
:::

::: info
Mac은 `tools/build_macos_universal.zsh` 스크립트를 사용하여 컴파일할 수 있습니다.  
MaaAssistantArknights/MaaMacGui 프로젝트의 [README.md](https://github.com/MaaAssistantArknights/MaaMacGui/blob/master/README.md)를 참고하는 것을 권장합니다.
:::

## 컴파일 과정

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

   사전 빌드된 라이브러리를 다운로드하거나 처음부터 컴파일할 수 있습니다.
   - 사전 빌드된 서드파티 라이브러리 다운로드 (권장)

     > [!Note]
     > ~~상대적으로 최신의 Linux 배포판 (Ubuntu 22.04)에서 컴파일된 동적 라이브러리가 포함되어 있습니다. 시스템의 libstdc++ 버전이 오래되었을 경우 ABI 호환성 문제가 발생할 수 있습니다.~~  
     > 현재 크로스 컴파일을 기반으로 실행 환경 요구 사항을 낮추었으며, glibc 2.31 (ubuntu 20.04)만 있으면 됩니다.

     ```bash
     python tools/maadeps-download.py
     ```

   만약 위 방법으로 다운로드한 라이브러리가 ABI 버전 등의 이유로 시스템에서 실행되지 않고, 컨테이너 등을 사용하고 싶지 않다면 처음부터 컴파일을 시도해 볼 수 있습니다.
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
   cmake -B build \
       -DINSTALL_RESOURCE=ON \
       -DINSTALL_PYTHON=ON \
       -DCMAKE_TOOLCHAIN_FILE=src/MaaUtils/MaaDeps/cmake/maa-x64-linux-toolchain.cmake
   cmake --build build
   ```

   MAA를 대상 위치에 설치합니다. MAA는 `LD_LIBRARY_PATH`를 지정하여 실행하는 것을 권장하며, 관리자 권한을 사용하여 MAA를 `/usr`에 설치하지 마세요.

   > 지금은 실행 시 `LD_LIBRARY_PATH`를 지정할 필요가 없을 수도 있습니다.

   ```bash
   cmake --install build --prefix <target_directory>
   ```

4. 완료, 디렉터리에서 빌드 파일을 확인할 수 있을 것입니다.

::::

## 통합 문서

[~~문서라고 부르기도 애매한~~](../protocol/integration.md)

### Python

[Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py)에서 `__main__`의 구현을 참고하세요.

### C++

[CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp)의 구현을 참고하세요.

### C Sharp

<!-- Do not use C#, MD003/heading-style: Heading style [Expected: atx; Actual: atx_closed] -->

[MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Main/AsstProxy.cs)의 구현을 참고하세요.
