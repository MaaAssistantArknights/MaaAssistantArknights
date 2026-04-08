---
order: 2
icon: material-symbols:download-2-rounded
---

<!-- markdownlint-disable MD024 -->

# 다운로드 및 설치

::: tip
MAA GUI의 다운로드 및 설치 안내 문서를 보고 있습니다. maa-cli의 다운로드 및 설치 안내가 필요하다면 maa-cli의 [설치 및 빌드](./cli/install.md) 문서를 참조하세요. 현재 MAA Android 버전이 테스트 공개 중입니다. 자세한 내용은 [MAA-Meow](https://github.com/Aliothmoon/MAA-Meow)를 참조하세요.
:::

## MAA 다운로드

MAA는 공식 홈페이지 다운로드, 패키지 관리자 설치, QQ 그룹 파일 다운로드 등 다양한 방법을 제공합니다. 원하는 방법을 선택하여 다운로드하세요.

### [공식 홈페이지](https://maa.plus)에서 최신 MAA 패키지 다운로드

공식 홈페이지는 일반적으로 올바른 버전 아키텍처를 자동으로 선택합니다. 이 문서를 읽는 대부분의 사용자에게는 Windows x64가 해당됩니다. macOS 사용자는 macOS 유니버설 버전을 다운로드하세요.

### [Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install)에서 최신 MAA 패키지 다운로드

시스템 아키텍처를 확인하고 해당하는 패키지를 다운로드하세요. 이 문서를 읽는 대부분의 Windows 사용자에게는 Windows x64가 해당됩니다. Mac 사용자의 경우, Mirror酱은 유니버설 패키지를 제공하지 않으므로 칩 아키텍처(arm/x86)를 확인한 후 해당 패키지를 다운로드하세요.

::: tip
[Mirror酱](https://mirrorchyan.com/zh/projects?rid=MAA&source=maadocs-install)은 독립적인 서드파티 다운로드 가속 서비스로, MAA가 아닌 Mirror酱에서 유료로 운영됩니다. 운영 비용은 구독 수익으로 충당되며, 수익의 일부는 프로젝트 개발자에게 환원됩니다. CDK를 구독하여 고속 다운로드를 즐기면서 프로젝트의 지속적인 개발을 지원해 주세요.
:::

### Windows 패키지 관리자(Winget)로 설치

::: tip
이 방법은 Windows 사용자에게만 적용됩니다.
:::

터미널에서 다음 명령을 실행하세요:

```bash
winget install maa
```

이 방법으로 설치하는 경우 기본 설치 경로는 `C:\Users\사용자이름\AppData\Local\Microsoft\WinGet\Packages`입니다.

### QQ 그룹 파일에서 최신 MAA 패키지 다운로드

1. [MAA 공식 QQ 그룹](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)에 참가하세요.
2. 그룹 파일에서 최신 MAA 패키지를 찾아 다운로드하세요.

### [GitHub Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)에서 최신 MAA 패키지 다운로드

시스템 아키텍처를 확인하고 해당하는 패키지를 다운로드하세요. 이 문서를 읽는 대부분의 Windows 사용자에게는 `MAA-<버전>-win-x64.zip`이 해당됩니다. macOS 사용자는 `MAA-<버전>-macos-universal.dmg`를 선택하세요.

## Linux 및 기타 운영 체제

MAA GUI는 Linux 및 기타 운영 체제를 **현재 지원하지 않습니다**. 이러한 시스템에서 MAA 기능을 사용하려면 **maa-cli**를 사용하세요. 자세한 내용은 maa-cli의 [설치 및 빌드](./cli/install.md) 문서를 참조하세요.

## MAA 설치

### Windows

다운로드가 완료되면 `.zip` 파일을 얻게 됩니다. 압축 해제 프로그램으로 완전히 압축을 해제하면 MAA의 모든 파일이 포함된 폴더를 얻을 수 있습니다.

::: warning

1. `C:\`, `C:\Program Files\` 등 UAC 권한이 필요한 경로에 MAA를 압축 해제하지 마세요.
2. MAA는 .NET 런타임이 내장되어 있습니다(자체 포함 배포). 하지만 Visual C++ Redistributable x64(VCRedist x64)가 여전히 필요합니다. 압축 해제된 MAA 디렉토리에서 관리자 권한으로 `DependencySetup_依赖库安装.bat`를 실행하여 해당 종속성을 설치하세요. 설치 완료 후 `MAA.exe`를 실행하세요.

자세한 내용은 [자주 묻는 질문](./faq.md) 공지를 참조하세요.
:::

`MAA.exe`를 더블 클릭하면 MAA가 실행됩니다.

::: tip
Windows 패키지 관리자(Winget)로 설치한 사용자는 압축 해제, 런타임 설치 등의 추가 작업 없이 명령줄에서 `maa`를 입력하여 MAA를 바로 실행할 수 있습니다. PATH에 `maa-cli`가 있는 경우 둘을 구분하기 위한 추가 단계가 필요할 수 있습니다.
:::

### macOS

다운로드가 완료되면 `.dmg` 파일을 얻게 됩니다. `.dmg`를 더블 클릭하여 열고, `MAA.app`을 `/Applications`로 드래그하여 설치를 완료하세요.

## 다음 단계

설치가 완료되었으면 [초보자 가이드](./newbie.md)로 돌아가 설정을 계속하거나, [기능 소개](./introduction/)에서 MAA가 지원하는 다양한 기능을 확인해 보세요! 설치 중 문제가 발생한 경우 [자주 묻는 질문](./faq.md)을 참조하여 해결해 보세요.
