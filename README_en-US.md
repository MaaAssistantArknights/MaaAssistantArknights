<div align="center">

<img alt="LOGO" src="https://user-images.githubusercontent.com/18511905/148931479-23aef436-2fc1-4c1e-84c9-bae17be710a5.png" width=360 height=270/>

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

[中文（简体）](README.md) | [中文（繁體）](README_zh-TW.md) | [English](README_en-US.md) | [日本語](README_ja-JP.md)

MAA means MAA Assistant Arknights

An Arknights assistant

Based on image recognition, helps you to complete daily quests efficiently!

Development in progress  ✿✿ヽ(°▽°)ノ✿  

</div>

## Features

- Auto-battle, drop items recognition, auto-uploading to [Penguin Stats](https://penguin-stats.cn/)
- Auto-shifting in the infrastructure, auto efficiency calculation of operators, optimal solution in single facility
- Auto-recruitment, support for expedited plan
- Visiting friends, collecting credits and purchasing items, collecting daily rewards, completing daily quests in one click!
- Auto-battle for Integrated Strategy (I.S.) for collecting originium ingots and candles.
- **New feature! Importing JSON task file for auto-battle!** [Video](https://www.bilibili.com/video/BV14u411673q/)

Talk is cheap. Show me the pictures!  

![image](https://user-images.githubusercontent.com/99072975/181417856-0fb1ac8b-c0f6-4c67-904c-1c3ea2c309a8.png)
![image](https://user-images.githubusercontent.com/99072975/181417869-bb312d7e-f867-45c6-84b9-d18c583232d5.png)
![image](https://user-images.githubusercontent.com/99072975/181417872-563afc09-e610-45f5-8762-dbf69018a329.png)


## Download

***First time use? Please download `MaaBundle-vX.Y.Z.zip`***

[Stable](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)  
[Development](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

## How to Use

### Basic Settings

1. Follow the [Emulator Support](docs/en/EMULATOR_SUPPORTS.md) to configure your emulator.
2. Change your emulator resolution to `1280 * 720` or higher with `16:9` aspect ratio.
3. Enjoy it!

See also: [User Manual](docs/en/USER_MANUAL.md)

## FAQ

- The program crashes immediately when I try to run it.
- Connection error/not knowing how to fill in ADB path.
- Wrong recognition/program freezes after starting
- Custom connection settings

Please refer to: [FAQ](docs/en/FAQ.md)

## Supports for overseas clients

- Global(EN) Client  
  Supports basic features like Combat, Visiting, Collocting, Auto Roguelike(beta), Recruitment calculate, etc. See also [README](resource/global/YoStarEN/readme.md)
- JP Client  
  Supports basic features like Combat, Auto Recruiting, Visiting, Collocting, Auto Roguelike(beta), Recruitment calculate, etc. See also [README](resource/global/YoStarJP/readme.md)
- KR Client  
  Supports basic features like Combat, etc.  See also [README](resource/global/YoStarKR/readme.md)
- ZH_CHT Client  
  Supports basic features like Combat, etc.  See also [README](resource/global/txwy/readme.md)

Due to the small number of overseas clients users and the lack of project manpower, the overseas clients is currently only adapted to basic functions. If you have strong needs, welcome to ask the progress in the [discussions](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions), or join us to make MAA better!

## Associated Projects

- New GUI: [MaaAsstElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI) (Development in progress, welcome to join us!)
- Update server: [MaaDownloadServer](https://github.com/MaaAssistantArknights/MaaDownloadServer)
- [Website](https://www.maa.plus): [maa-website](https://github.com/MaaAssistantArknights/maa-website)
- [Co-pilot (auto-battle) site](https://www.prts.plus): [frontend](https://github.com/MaaAssistantArknights/maa-copilot-frontend), [backend](https://github.com/MaaAssistantArknights/MaaCopilotServer)
 
## Acknowledgements

### Open-source Libraries

- Image recognition: [opencv](https://github.com/opencv/opencv.git)
- ~~OCR: [chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- OCR: [PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- ~~Item drop recognition: [Penguin Stats recognizer](https://github.com/penguin-statistics/recognizer)~~
- Map tile recognition: [Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON library: [meojson](https://github.com/MistEO/meojson.git)
- C++ operator parser: [calculator](https://github.com/kimwalisch/calculator)
- ~~C++ Base64 encoding/decoding[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ ZIP library: [zlib](https://github.com/madler/zlib)
- C++ Gzip library: [gzip-hpp](https://github.com/mapbox/gzip-hpp)
- WPF MVVW framework: [Stylet](https://github.com/canton7/Stylet)
- WPF control library: [HandyControl](https://github.com/HandyOrg/HandyControl)
- C# JSON library: [Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json)
- Downloader: [aria2](https://github.com/aria2/aria2)

### Data Source

- ~~Recruitment data：[ArkTools](https://www.bigfun.cn/tools/aktools/hr)~~
- Operators and infrastructure data：[PRTS Arknights Wiki (Chinese)](http://prts.wiki/)
- Stage data: [Penguin Stats](https://penguin-stats.cn/)
- Gamedata and resources: [Arknights Bot Resource](https://github.com/yuanyan3060/Arknights-Bot-Resource)

### Contributors

Thanks to all friends who contribute to development/testing for making MAA better! (*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## Development Related Information

### Windows

Open `MeoAssistantArknights.sln` with Visual Studio 2019 ~~or higher version~~. All settings have been configured properly.

### Linux/MacOS

Please refer to [Linux Tutorial](docs/en/LINUX_TUTORIAL.md)

### API

- [C interface](include/AsstCaller.h): [Integration Example](tools/TestCaller/main.cpp)
- [Python interface](src/Python/asst.py): [Integration Example](src/Python/sample.py)
- [Golang interface](src/Golang/maa/): [Integration Example](src/Golang/cli.go) (deprecated)
- [Dart interface](src/dart/)
- [Java interface](src/Java/Maaj)：[Integration Example](src/Java/Maaj/src/main/java/com/iguigui/maaj/MaaJavaSample.java)
- [Rust interface](src/Rust/src/maa_sys/)：[HTTP api](src/Rust)
- [Integration Documentation](docs/en/INTEGRATION.md)
- [Callback Schema](docs/en/CALLBACK_SCHEMA.md)
- [Task Schema](docs/en/TASK_SCHEMA.md)
- [Copilot Schema](docs/en/COPILOT_SCHEMA.md)

### For Novice Users of GitHub

[Contributing](docs/en/CONTRIBUTING.md)

## Disclaimer

- The logo of this software is NOT granted rights under AGPL 3.0 License. The artist [耗毛](https://weibo.com/u/3251357314) and all developers of the software reserves all rights. The logo of the software shall not be used without authorization even if the project has an AGPL 3.0 License. Nor shall the logo be used for commercial purposes without authorization.
- The software is an open-source, free of charge software only for studying and communication purposes. There is no agreement or understanding between the developers of the software and the 3rd party person who uses this software as an assistant and charges you. In that case, the developers of the software is not responsible for the problems and consequences caused.

## Advertisement

[User Group (Telegram)](https://t.me/+Mgc2Zngr-hs3ZjU1)  
Copilot JSON Sharing: <https://www.prts.plus/>, [QQ Group(full)](https://jq.qq.com/?_wv=1027&k=1giyMpPb), [QQ Group 2(full)](https://jq.qq.com/?_wv=1027&k=R3oleoKc), [QQ Group 3](https://jq.qq.com/?_wv=1027&k=mKdOnhWV)  
[Bilibili Live](https://live.bilibili.com/2808861): live coding on this program  
[Technical Discussion & Talk(QQ Group)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)  
[Dev Group(QQ Group)](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)

Please click "Star" if you consider it helpful! Thank you for your support!
