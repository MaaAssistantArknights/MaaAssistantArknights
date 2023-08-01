<div align="center">

<img alt="LOGO" src="https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png" width="256" height="256" />

# MaaAssistantArknights

<br>
<div>
    <img alt="C++" src="https://img.shields.io/badge/c++-20-%2300599C?logo=cplusplus">
</div>
<div>
    <img alt="platform" src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blueviolet">
</div>
<div>
    <img alt="license" src="https://img.shields.io/github/license/MaaAssistantArknights/MaaAssistantArknights">
    <img alt="commit" src="https://img.shields.io/github/commit-activity/m/MaaAssistantArknights/MaaAssistantArknights?color=%23ff69b4">
    <img alt="stars" src="https://img.shields.io/github/stars/MaaAssistantArknights/MaaAssistantArknights?style=social">
</div>
<br>

[简体中文](../readme.md) | [繁體中文](../zh-tw/readme.md) | [English](../en-us/readme.md) | [日本語](../ja-jp/readme.md) | [한국어](../ko-kr/readme.md)

MAA means MAA Assistant Arknights

An Arknights assistant

Based on image recognition, helps you to complete daily quests efficiently!

Development in progress  ✿✿ヽ(°▽°)ノ✿<br>

</div>

## Features

- Auto-battle, drop items recognition, auto-uploading to [Penguin Stats](https://penguin-stats.io/).
- Auto-shifting in the Base, auto efficiency calculation of operators, optimal solution in single room. (Only CN client is supported now.)
- Auto-recruitment, support for expedited plan. Auto-uploading recruitment data to [Penguin Stats](https://penguin-stats.io/result/stage/recruit/recruit), [Yituliu](https://yituliu.site/maarecruitdata).
- Visiting friends, collecting credits and purchasing items, collecting daily rewards, completing daily quests in one click!
- Auto-battle for Integrated Strategy (I.S.) for collecting originium ingots and candles.
- Importing JSON task file for auto-battle! [Video](https://www.bilibili.com/video/BV1H841177Fk/)
- Depot recognition and upports exporting to [Penguin Stats Planner](https://penguin-stats.cn/planner), [Arknight Tools](https://arkn.lolicon.app/#/material), and [ARK-NIGHTS Operator Builds](https://ark-nights.com/settings)
- Support C, Python, Java, Rust, Golang, Java HTTP, Rust HTTP and other interfaces, easy to integrate and call, customize your MAA!

Talk is cheap. Show me the pictures!<br>

![en1](https://user-images.githubusercontent.com/99072975/232383365-0b6f657a-c482-4a82-86eb-c129bf676f0c.png)
![en2](https://user-images.githubusercontent.com/99072975/232383370-250eec85-15a1-49cd-8d8d-77545658ae74.png)

## Download

[Stable](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)<br>
[Development](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

## How to Use

### Basic Settings

1. Please refer to [Emulator Supports](./1.3-EMULATOR_SUPPORTS.md) to configure your emulator.
2. Change your emulator resolution to `1280 * 720` or higher with `16:9` aspect ratio.
3. Enjoy it!

See also: [User Manual](./1.1-USER_MANUAL.md)

## FAQ

- The program crashes immediately when I try to run it.
- Connection error/not knowing how to fill in ADB path.
- Connected successfully, then it got stuck.
- Custom connection settings.
- Download speed is too slow, or the mirror site is not accessible.
- Download halfway and prompt "login"/"authentication"

Please refer to: [FAQ](./1.2-FAQ.md)

## Supports for overseas clients

At present, most of the functions of the International client (US client), Japanese client, Korean client, and traditional Chinese client have been supported. However, due to the small number of overseas users and the shortage of project personnel, many functions have not been fully tested, so please experience it yourself.<br>
If you encounter a bug, or have a strong demand for a certain function, welcome to [Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) and [Discussions](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions) to ask questions. And feel free to join us in building the MAA! Please refer to [Overseas Clients Adaptation](#overseas-clients-adaptation)

## Associated Projects

- New Framework: [MaaFramework](https://github.com/MaaAssistantArknights/MaaFramework)
- New GUI: [MaaAsstElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI)
- [Co-pilot (auto-battle) site](https://prts.plus): [frontend](https://github.com/MaaAssistantArknights/maa-copilot-frontend)
- Backend: [MaaBackendCenter](https://github.com/MaaAssistantArknights/MaaBackendCenter)
- [Website](https://www.maa.plus): [frontend](https://github.com/MaaAssistantArknights/maa-website)
- Deep Learning: [MaaAI](https://github.com/MaaAssistantArknights/MaaAI)

## Acknowledgements

### Open-source Libraries

- Image recognition: [opencv](https://github.com/opencv/opencv.git)
- ~~OCR: [chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- OCR: [PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- ML Deployment: [FastDeploy](https://github.com/PaddlePaddle/FastDeploy)
- ML accelerator: [onnxruntime](https://github.com/microsoft/onnxruntime)
- ~~Item drop recognition: [Penguin Stats recognizer](https://github.com/penguin-statistics/recognizer)~~
- Map tile recognition: [Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON library: [meojson](https://github.com/MistEO/meojson.git)
- C++ operator parser: [calculator](https://github.com/kimwalisch/calculator)
- ~~C++ Base64 encoding/decoding[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ ZIP library: [zlib](https://github.com/madler/zlib)
- C++ Gzip library: [gzip-hpp](https://github.com/mapbox/gzip-hpp)
- Touch event producer for Android: [minitouch](https://github.com/openstf/minitouch)
- Touch event producer for Android: [MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)
- WPF MVVM framework: [Stylet](https://github.com/canton7/Stylet)
- WPF control library: [HandyControl](https://github.com/HandyOrg/HandyControl)
- C# JSON library: [Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json)
- Downloader: [aria2](https://github.com/aria2/aria2)

### Data Source

- ~~Recruitment data: [ArkTools](https://www.bigfun.cn/tools/aktools/hr)~~
- ~~Operators and infrastructure data: [PRTS Arknights Wiki (Chinese)](http://prts.wiki/)~~
- Stage data: [Penguin Stats](https://penguin-stats.io/)
- Gamedata and resources: [Arknights Game Resource](https://github.com/yuanyan3060/ArknightsGameResource)
- ~~Gamedata: [Arknights Game Data](https://github.com/Kengxxiao/ArknightsGameData)~~

### Contributors

Thanks to all friends who contribute to development/testing for making MAA better! (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## Development Related Information

### Windows

1. Download pre-built third-party libraries.

     ```cmd
     python maadeps-download.py
     ```

2. Open `MAA.sln` with Visual Studio 2022, right-click `MaaWpfGui` and set it as the startup project.
3. Right-click `MaaWpfGui` - Properties - Debugging - Enable Local Debugging (so you can hang the breakpoint on the C++ Core).
4. (Optional) If you are going to submit a PR, I recommend you enable [clang-format support](./2.2-DEVELOPMENT.md#using-clang-format-in-visual-studio).

### Linux/MacOS

Please refer to [Linux Tutorial](./2.1-LINUX_TUTORIAL.md)

### API

- [C interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/include/AsstCaller.h): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp)
- [Python interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/asst/asst.py): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py)
- [Golang interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Golang/): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Golang/maa/maa.go)
- [Dart interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Dart)
- [Java interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaCore.java): [Integration Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaJavaSample.java)
- [Java HTTP interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/Readme.md)
- [Rust interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Rust/src/maa_sys): [HTTP interface](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Rust)
- [TypeScript interface](https://github.com/MaaAssistantArknights/MaaAsstElectronUI/tree/main/packages/main/coreLoader)
- [Integration Documentation](./3.1-INTEGRATION.md)
- [Callback Schema](./3.2-CALLBACK_SCHEMA.md)
- [Task Schema](./3.4-TASK_SCHEMA.md)
- [Copilot Schema](./3.3-COPILOT_SCHEMA.md)

### Overseas Clients Adaptation

Please refer to [Overseas clients Adaptation](./2.5-OVERSEAS_CLIENTS_ADAPTATION.md). For the functions already supported by the CN client, most of the overseas clients adaptation tasks only need screenshots and simple JSON modification.

### For Novice Users of GitHub

[Development](./2.2-DEVELOPMENT.md#introduction-to-github-pull-request-flow)

### Issue Bot

Please refer to [Issue Bot Usage](./2.3-ISSUE_BOT_USAGE.md)

## Disclaimer

- The logo of this software is NOT granted rights under AGPL 3.0 License. [耗毛](https://weibo.com/u/3251357314) and Vie, the two artists and all developers of the software reserves all rights. The logo of the software shall not be used without authorization even if the project has an AGPL 3.0 License. Nor shall the logo be used for commercial purposes without authorization.
- The software is an open-source, free of charge software only for studying and communication purposes. There is no agreement or understanding between the developers of the software and the 3rd party person who uses this software as an assistant and charges you. In that case, the developers of the software is not responsible for the problems and consequences caused.

## Advertisement

User Group: [Telegram](https://t.me/+Mgc2Zngr-hs3ZjU1), [QQ Group](https://ota.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)<br>
[Copilot JSON Sharing](https://prts.plus)<br>
[Bilibili Live](https://live.bilibili.com/2808861): live coding on this program<br>
[Technical Discussion & Talk(QQ Group)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)<br>
[Dev Group(QQ Group)](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)

Please click "Star" if you consider it helpful! Thank you for your support!
