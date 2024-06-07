---
order: 2
icon: teenyicons:linux-alt-solid
---

# Linux 컴파일 가이드

**이 튜토리얼은 독자가 일정한 Linux 환경 설정 능력과 프로그래밍 기초를 가지고 있다고 가정합니다!**, 만약 여러분이 MAA를 직접 컴파일하는 대신에 단순히 설치하길 원한다면 [Linux 설명서](../manual/device/linux.md)를 참고하세요.

::: info 정보
MAA의 빌드 방법은 여전히 논의 중입니다. 이 튜토리얼의 내용은 오래되었을 수 있으므로 [GitHub workflow file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/workflows/ci.yml#L134)의 스크립트를 참고하세요. 또한 [AUR PKGBUILD](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=maa-assistant-arknights) 및 [nixpkgs](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/by-name/ma/maa-assistant-arknights/package.nix)을 참고할 수 있습니다.
:::

## 컴파일 과정

1. 컴파일에 필요한 종속성 다운로드

   - Ubuntu/Debian

   ```bash
   sudo apt install gcc-12 g++-12 cmake zlib1g-dev
   ```

2. 서드파티 라이브러리 빌드

   사전 빌드된 종속성 라이브러리를 다운로드하거나 직접 빌드할 수 있습니다.

   - 사전 빌드된 서드파티 라이브러리 다운로드 (권장됨)

     > **Note**
     > 상대적으로 최신의 Linux 배포판 (Ubuntu 22.04)에서 컴파일된 동적 라이브러리가 포함되어 있습니다. 시스템의 libstdc++ 버전이 오래되었을 경우 ABI 호환성 문제가 발생할 수 있습니다.

     ```bash
     python maadeps-download.py
     ```

   위의 방법으로 다운로드한 라이브러리가 시스템에서 실행되지 않거나 컨테이너와 같은 대안을 사용하지 않고 싶은 경우 직접 빌드해볼 수도 있습니다.

   - 서드파티 라이브러리 직접 빌드 (시간이 오래 걸릴 수 있음)

     ```bash
     git submodule update --init --recursive
     cd MaaDeps
     python build.py
     ```

3. MAA 컴파일

   ```bash
   CC=gcc-12 CXX=g++-12 cmake -B build \
       -DINSTALL_THIRD_LIBS=ON \
       -DINSTALL_RESOURCE=ON \
       -DINSTALL_PYTHON=ON
   cmake --build build
   ```

   경로에 MAA를 설치하기 위해 위의 명령을 사용합니다. MAA는 `LD_LIBRARY_PATH`를 지정하여 실행하는 것이 좋으며, MAA를 `/usr`에 설치하지 마세요.

   ```bash
   cmake --install build --prefix <target_directory>
   ```

## 통합 문서

[~~문서가 아닐수도 있습니다~~](../protocol/integration.md)

### Python

[Python demo](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py)에서 `__main__`의 구현을 참조하세요.

### C

[CppSample](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/CppSample/main.cpp)의 구현을 참조하세요.

### C\#

[MaaWpfGui](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/MaaWpfGui/Helper/AsstProxy.cs)의 구현을 참조하세요.
