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

[简体中文](README.md) | [繁體中文](README_zh-TW.md) | [English](README_en-US.md) | [日本語](README_ja-JP.md) | [한국어](README_ko-KR.md)

MAA means MAA Assistant Arknights

An Arknights assistant

Based on image recognition, helps you to complete daily quests efficiently!

Development in progress  ✿✿ヽ(°▽°)ノ✿  

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

Talk is cheap. Show me the pictures!  

![image](https://user-images.githubusercontent.com/18511905/198692842-1ad89bd0-f943-4763-bdd1-c212e09dc832.png)
![image](https://user-images.githubusercontent.com/99072975/181417869-bb312d7e-f867-45c6-84b9-d18c583232d5.png)
![image](https://user-images.githubusercontent.com/18511905/198689839-612f3bfc-c308-4d62-969d-8544764e794d.png)

## Download

[Stable](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)  
[Development](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

## How to Use

### Basic Settings

1. Please refer to [Emulator Supports](docs/en-us/1.3-EMULATOR_SUPPORTS.md) to configure your emulator.
2. Change your emulator resolution to `1280 * 720` or higher with `16:9` aspect ratio.
3. Enjoy it!

See also: [User Manual](docs/en-us/1.1-USER_MANUAL.md)

## FAQ

- The program crashes immediately when I try to run it.
- Connection error/not knowing how to fill in ADB path.
- Connected successfully, then it got stuck.
- Custom connection settings.
- Download speed is too slow, or the mirror site is not accessible.
- Download halfway and prompt "login"/"authentication"

Please refer to: [FAQ](docs/en-us/1.2-FAQ.md)

## Supports for overseas clients

- Global(EN) Client  
  Supports basic features like Combat, Credit Shopping, Visiting, Collecting, Auto Roguelike(beta), Recruitment calculate, etc. See also [README](resource/global/YoStarEN/readme.md)
- JP Client  
  Supports basic features like Combat, Auto Base shift, Credit Shopping, Auto Recruiting, Visiting, Collecting, Auto Roguelike(beta), Recruitment calculate, etc. See also [README](resource/global/YoStarJP/readme.md)
- KR Client  
  Supports basic features like Combat, Credit Shopping, Visiting, Collecting, Auto Roguelike(beta), Recruitment calculate, etc. See also [README](resource/global/YoStarKR/readme.md)
- ZH_CHT Client  
  Supports basic features like Combat, Auto Recruiting, Auto Roguelike(beta), Recruitment calculate, etc. See also [README](resource/global/txwy/readme.md)

Due to the small number of overseas clients users and the lack of project manpower, the overseas clients is currently only adapted to basic functions. If you have strong needs, welcome to ask the progress in the [discussions](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions), or join us to make MAA better! Please refer to [Overseas Clients Adaptation](#overseas-clients-adaptation)

## Associated Projects

- New GUI: [MaaAsstElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI) (Development in progress, welcome to join us!)
- Backend: [MaaBackendCenter](https://github.com/MaaAssistantArknights/MaaBackendCenter) (Development in progress, welcome to join us!)
- [Co-pilot (auto-battle) site](https://prts.plus)：[frontend](https://github.com/MaaAssistantArknights/maa-copilot-frontend)
- [Website](https://www.maa.plus): [frontend](https://github.com/MaaAssistantArknights/maa-website)

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
- Operators and infrastructure data: [PRTS Arknights Wiki (Chinese)](http://prts.wiki/)
- Stage data: [Penguin Stats](https://penguin-stats.io/)
- Gamedata and resources: [Arknights Bot Resource](https://github.com/yuanyan3060/Arknights-Bot-Resource)
- Gamedata: [Arknights Game Data](https://github.com/Kengxxiao/ArknightsGameData)

### Contributors

Thanks to all friends who contribute to development/testing for making MAA better! (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## Development Related Information

### Windows

- Open `MAA.sln` with Visual Studio 2022. All settings have been configured properly.
- Switch on the feature of clang-format. Please refer to [Using clang-format in Visual Studio](docs/en-us/2.2-DEVELOPMENT.md#using-clang-format-in-visual-studio).

### Linux/MacOS

Please refer to [Linux Tutorial](docs/en-us/2.1-LINUX_TUTORIAL.md)

### API

- [C interface](include/AsstCaller.h): [Integration Example](src/CppSample/main.cpp)
- [Python interface](src/Python/asst.py): [Integration Example](src/Python/sample.py)
- [Golang interface](src/Golang/): [Integration Example](src/Golang/maa/maa.go)
- [Dart interface](src/dart/)
- [Java interface](src/Java): [Integration Example](src/Java/src/main/java/com/iguigui/maaj/MaaJavaSample.java)
- [Rust interface](src/Rust/src/maa_sys/): [HTTP API](src/Rust)
- [HTTP interface](src/Java/Readme.md)
- [TypeScript interface](https://github.com/MaaAssistantArknights/MaaAsstElectronUI/tree/main/packages/main/coreLoader)
- [Integration Documentation](docs/en-us/3.1-INTEGRATION.md)
- [Callback Schema](docs/en-us/3.2-CALLBACK_SCHEMA.md)
- [Task Schema](docs/en-us/3.4-TASK_SCHEMA.md)
- [Copilot Schema](docs/en-us/3.3-COPILOT_SCHEMA.md)

### Overseas Clients Adaptation

Please refer to [Overseas clients Adaptation](docs/en-us/2.5-Overseas%20Clients%20Adaptation.md). For the functions already supported by the CN client, most of the overseas clients adaptation tasks only need screenshots and simple JSON modification.

### For Novice Users of GitHub

[Development](docs/en-us/2.2-DEVELOPMENT.md#introduction-to-github-pull-request-flow)

### Issue Bot

- Add labels with `Add {LABEL_NAME}`, and remove with `Remove {LABEL_NAME}`.
- Add `fixed` tag to the issue with `close #{ISSUE_NUMBER}` or `fix #{ISSUE_NUMBER}` in the comments.

Please refer to [Issue Bot Usage](docs/en-us/2.3-ISSUE_BOT_USAGE.md) for more details.

## Disclaimer

- The logo of this software is NOT granted rights under AGPL 3.0 License. [耗毛](https://weibo.com/u/3251357314) and Vie, the two artists and all developers of the software reserves all rights. The logo of the software shall not be used without authorization even if the project has an AGPL 3.0 License. Nor shall the logo be used for commercial purposes without authorization.
- The software is an open-source, free of charge software only for studying and communication purposes. There is no agreement or understanding between the developers of the software and the 3rd party person who uses this software as an assistant and charges you. In that case, the developers of the software is not responsible for the problems and consequences caused.

## Advertisement

[User Group (Telegram)](https://t.me/+Mgc2Zngr-hs3ZjU1), [QQ Group 1(full)](https://jq.qq.com/?_wv=1027&k=1giyMpPb), [QQ Group 2(full)](https://jq.qq.com/?_wv=1027&k=R3oleoKc), [QQ Group 3(full)](https://jq.qq.com/?_wv=1027&k=mKdOnhWV), [QQ Group 4(full)](https://jq.qq.com/?_wv=1027&k=ABkU8mCR), [QQ Group 5(full)](https://jq.qq.com/?_wv=1027&k=To6b6H6m), [QQ Group 6(full)](https://jq.qq.com/?_wv=1027&k=PYoCP2lS), [QQ Group 7(full)](https://jq.qq.com/?_wv=1027&k=xDT9vHvB), [QQ Group 8](https://jq.qq.com/?_wv=1027&k=PzvqFhOr)  
[Copilot JSON Sharing](https://prts.plus)  
[Bilibili Live](https://live.bilibili.com/2808861): live coding on this program  
[Technical Discussion & Talk(QQ Group)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)  
[Dev Group(QQ Group)](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)

Please click "Star" if you consider it helpful! Thank you for your support!
