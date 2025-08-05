---
icon: ic:round-home
index: true
dir:
  order: 0
---

<!-- markdownlint-disable -->

::: center

![MAA Logo =256x256](/images/maa-logo_512x512.png)

<!-- markdownlint-restore -->
# MaaAssistantArknights

![C++](https://img.shields.io/badge/C++-20-%2300599C?logo=cplusplus)
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blueviolet)
![license](https://img.shields.io/github/license/MaaAssistantArknights/MaaAssistantArknights) ![commit](https://img.shields.io/github/commit-activity/m/MaaAssistantArknights/MaaAssistantArknights?color=%23ff69b4)
![stars](https://img.shields.io/github/stars/MaaAssistantArknights/MaaAssistantArknights?style=social) ![GitHub all releases](https://img.shields.io/github/downloads/MaaAssistantArknights/MaaAssistantArknights/total?style=social)

MAA stands for MAA Assistant Arknights

An Arknights assistant

Based on image recognition technology, complete all daily tasks with one click!

Development actively in progress  ✿✿ヽ(°▽°)ノ✿

:::

## Download and Installation

Please read the [documentation](./manual/newbie.md) and then visit the [official website](https://maa.plus) or [Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) to download. Refer to [Getting Started](./manual/newbie.md) for installation instructions.

## Key Features

- Auto-farming sanity, drop item recognition and auto-uploading to [Penguin Stats](https://penguin-stats.io/), [Yituliu](https://ark.yituliu.cn/).
- Intelligent base shift management, auto-calculating operator efficiency for optimal solutions within facilities; also supports [customized scheduling](./protocol/base-scheduling-schema.md).
- Auto-recruitment with option to use expedited plans, clearing all at once! Auto-uploading recruitment data to [Penguin Stats](https://penguin-stats.io/result/stage/recruit/recruit), [Yituliu](https://ark.yituliu.cn/survey/maarecruitdata).
- Manual recruitment tag identification for better high-star recruitment decisions ~~(Will that Senior Operator tag give you SilverAsh or SilverAsh?)~~
- Operator list recognition, counting owned and missing operators and their potentials, displayed during recruitment identification.
- Material recognition and export to [Penguin Stats Planner](https://penguin-stats.io/planner), [Arknights Toolbox](https://arkntools.app/#/material), [ARK-NIGHTS Operator Builds](https://ark-nights.com/settings).
- Visiting friends, collecting credits and shopping, collecting daily rewards, etc. - one-click daily automation.
- Fully automatic Integrated Strategy (IS) for farming Originium Ingots and levels, auto-farming and direct upgrades, smart operator recognition.
- Select JSON copilot files for automatic stage clearing, [Video demonstration](https://www.bilibili.com/video/BV1H841177Fk/).
- Support for C, Python, Java, Rust, Golang, Java HTTP, Rust HTTP and other interfaces, easy integration and customization for your MAA!

See it in action!

```component Image4
{
  "imageList": [
    {
      "light": "images/en-us/readme/1-light.png",
      "dark": "images/en-us/readme/1-dark.png"
    },
    {
      "light": "images/en-us/readme/2-light.png",
      "dark": "images/en-us/readme/2-dark.png"
    },
    {
      "light": "images/en-us/readme/3-light.png",
      "dark": "images/en-us/readme/3-dark.png"
    },
    {
      "light": "images/en-us/readme/4-light.png",
      "dark": "images/en-us/readme/4-dark.png"
    }
  ]
}
```

## Usage Instructions

### Feature Introduction

Please refer to the [User Manual](./manual/).

### Support for Overseas Clients

Most features now support Global (EN), Japanese, Korean, and Traditional Chinese servers. However, due to the smaller number of overseas users and limited project personnel, many features haven't been thoroughly tested, so please try them yourself.  
If you encounter bugs or have specific feature requests, please let us know in [Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) and [Discussions](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions), or join us to help improve MAA! Please refer to the [Overseas Clients Adaptation](#overseas-clients-adaptation) section.

::: tip
For Global (EN) server, due to interface layout issues, the only supported resolution is `1920x1080`
:::

### CLI Support

MAA supports command-line interface (CLI) operation on Linux, macOS, and Windows, useful for automation scripts or on servers without graphical interfaces. Please refer to the [CLI Usage Guide](./manual/cli/)

## Join Us

### Main Related Projects

**The project team is currently in great need of front-end developers. If you have relevant experience, we welcome you to join us!**

- New Framework: [MaaFramework](https://github.com/MaaXYZ/MaaFramework)
- [Copilot Site](https://prts.plus) Frontend: [maa-copilot-frontend](https://github.com/MaaAssistantArknights/maa-copilot-frontend)
- [Copilot Site](https://prts.plus) Backend: [MaaBackendCenter](https://github.com/MaaAssistantArknights/MaaBackendCenter)
- [Official Website](https://maa.plus): [Frontend](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/website)
- Deep Learning: [MaaAI](https://github.com/MaaAssistantArknights/MaaAI)

### Multilingual Support (i18n)

MAA supports multiple languages and uses Weblate for localization management. If you're fluent in multiple languages, please visit [MAA Weblate](https://weblate.maa-org.net) to help with translations.

MAA uses Chinese (Simplified) as its primary language, and all translation entries are based on Chinese (Simplified).

[![Weblate](https://weblate.maa-org.net/widget/maa/wpf-gui/multi-auto.svg)](https://weblate.maa-org.net/engage/maa/)

### Development Participation

#### Windows

Please refer to the [Development Prerequisites](./develop/development.md).

#### Linux | macOS

Please refer to the [Linux Build Tutorial](./develop/linux-tutorial.md).

#### APIs

- [C interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/include/AsstCaller.h): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Cpp/main.cpp)
- [Python interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Python/asst/asst.py): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Python/sample.py)
- [Golang interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Golang): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Golang/maa/maa.go)
- [Dart interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Dart)
- [Java interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaCore.java): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaJavaSample.java)
- [Java HTTP interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/Readme.md)
- [Rust interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Rust/src/maa_sys): [HTTP interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Rust)
- [TypeScript interface](https://github.com/MaaAssistantArknights/MaaX/tree/main/packages/main/coreLoader)
- [Woolang interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Woolang/maa.wo): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Woolang/demo.wo)
- [Integration Documentation](./protocol/integration.md)
- [Callback Message Protocol](./protocol/callback-schema.md)
- [Task Workflow Protocol](./protocol/task-schema.md)
- [Auto-Copilot Protocol](./protocol/copilot-schema.md)

#### Overseas Clients Adaptation

Please refer to the [Overseas Clients Adaptation Guide](./develop/overseas-client-adaptation.md). For features already supported in the CN server, most adaptation work for overseas clients only requires screenshots + simple JSON modifications.

#### Want to contribute but not familiar with GitHub?

[GitHub Pull Request Process Overview](./develop/development.md#introduction-to-github-pull-request-flow)

#### Issue Bot

Please refer to the [Issue Bot Usage Guide](./develop/issue-bot-usage.md)

## Acknowledgements

### Open-source Libraries

- Image recognition library: [opencv](https://github.com/opencv/opencv.git)
- ~~Text recognition library: [chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- Text recognition library: [PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- Deep learning deployment library: [FastDeploy](https://github.com/PaddlePaddle/FastDeploy)
- Machine learning accelerator: [onnxruntime](https://github.com/microsoft/onnxruntime)
- ~~Stage drop recognition: [Penguin Stats Recognizer](https://github.com/penguin-statistics/recognizer)~~
- Map tile recognition: [Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON library: [meojson](https://github.com/MistEO/meojson.git)
- C++ operator parser: [calculator](https://github.com/kimwalisch/calculator)
- ~~C++ base64 encoding/decoding: [cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ compression library: [zlib](https://github.com/madler/zlib)
- C++ Gzip wrapper: [gzip-hpp](https://github.com/mapbox/gzip-hpp)
- Android touch event generator: [Minitouch](https://github.com/DeviceFarmer/minitouch)
- Android touch event generator: [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)
- WPF MVVM framework: [Stylet](https://github.com/canton7/Stylet)
- WPF control library: [HandyControl](https://github.com/HandyOrg/HandyControl) -> [HandyControls](https://github.com/ghost1372/HandyControls)
- C# logging: [Serilog](https://github.com/serilog/serilog)
- C# JSON library: [Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json) & [System.Text.Json](https://github.com/dotnet/runtime)
- ~~Downloader: [aria2](https://github.com/aria2/aria2)~~

### Data Sources

- ~~Recruitment data: [Arknights Toolbox](https://www.bigfun.cn/tools/aktools/hr)~~
- ~~Operator and base data: [PRTS Wiki](http://prts.wiki/)~~
- Stage data: [Penguin Statistics](https://penguin-stats.io/)
- Game data and resources: [Arknights Client Resources](https://github.com/yuanyan3060/ArknightsGameResource)
- Game data: [Arknights Yostar Game Data](https://github.com/ArknightsAssets/ArknightsGamedata)

### Contributors

Thanks to all friends who have participated in development and testing - your help makes MAA better and better! (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=105&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## Disclaimer

- This software is open source under the [GNU Affero General Public License v3.0 only](https://spdx.org/licenses/AGPL-3.0-only.html) license, with additional [Terms of Service](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/terms-of-service.md).
- The software's logo is NOT licensed under AGPL 3.0. The artists [耗毛](https://weibo.com/u/3251357314), vie, and all the software developers reserve all rights. The logo must not be used without authorization, even with AGPL 3.0 license claims, and must not be used for any commercial purposes without authorization.
- This software is open source and free, intended only for learning and communication purposes. If you encounter merchants using this software for paid services, they may be charging for equipment and time costs; any problems and consequences arising from this are not related to this software.

### DirectML Support Information

This software supports GPU acceleration, which relies on Microsoft's standalone component [DirectML](https://learn.microsoft.com/en-us/windows/ai/directml/) on Windows. DirectML is not part of this project's open-source components and is not subject to AGPL 3.0. For user convenience, we include an unmodified DirectML.dll file with the installation package. If you don't need GPU acceleration, you can safely delete this DLL file, and the core functionality will still work normally.

## Community

User Exchange QQ Groups: [MAA Usage & Arknights Exchange QQ Group](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)  
Discord Server: [Invite Link](https://discord.gg/23DfZ9uA4V)  
User Exchange Telegram Group: [Telegram Group](https://t.me/+Mgc2Zngr-hs3ZjU1)  
Auto-battle JSON strategy sharing: [prts.plus](https://prts.plus)  
Bilibili Live: [MrEO Live](https://live.bilibili.com/2808861) coding streams & [MAA-Official Live](https://live.bilibili.com/27548877) gaming/chat  

Technical Group (non-Arknights, no casual chat): [Internal Competition Hell! (QQ Group)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)  
Developer Group: [QQ Group](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)  

If you find the software helpful, please give us a Star! ~ (the small star at the top right of the webpage), that's the biggest support
