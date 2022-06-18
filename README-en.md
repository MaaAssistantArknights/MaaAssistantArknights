<div align="center">

<img alt="LOGO" src="https://user-images.githubusercontent.com/18511905/148931479-23aef436-2fc1-4c1e-84c9-bae17be710a5.png" width=360 height=270/>

# MaaAssistantArknights

<br>
<div>
    <img alt="C++" src="https://img.shields.io/badge/c++-17-%2300599C?logo=cplusplus">
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

[中文版本README](README.md)

MAA stands for MAA Assistant Arknights

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

![image](https://user-images.githubusercontent.com/99072975/172044929-3aae01a1-6c6d-4a9a-a7a8-ae753679b9ce.png)
![image](https://user-images.githubusercontent.com/99072975/172045492-698ab7fd-0413-4b08-aad7-0176f9480c05.png)
![image](https://user-images.githubusercontent.com/99072975/172045163-e9ead337-eb62-4f9f-a354-9e302f767a52.png)

## Download

[Stable](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)  
[Development](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

## How to Use

### Basic Settings

1. Follow the [Simulator Support](docs/en/SIMULATOR_SUPPORT.md) to configure your simulator.
2. Unzip the archive file to an **ASCII-only** directory
3. All settings are not changeable after running except `Auto shutdown`.

The program supports `16:9` resolution the best, with resolution larger than or equal to `1280 * 720`. Non-`16:9` resolution may cause some unknown issues and will be supported in future.

See also: [User Manual](docs/en/USER_MANUAL.md)

## FAQ

- The program crashes immediately when I try to run it.
- Connection error/not knowing how to fill in ADB path.
- Wrong recognition/program freezes after starting
- Custom connection settings

Please refer to: [FAQ](docs/en/FAQ.md)

## Supports for overseas servers

- YoStarEN Server  
  Supports basic features like Combat, Recruiting, etc. See also [README](resource/global/YoStarEN/readme.md)
- Other servers  
  TODO

## Associated Projects

- New GUI: [MaaAsstElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI) (Development in progress, welcome to join us!)
- Update server: [MaaDownloadServer](https://github.com/MaaAssistantArknights/MaaDownloadServer) (Development in progress, welcome to join us!)
- Co-pilot (auto-battle) server: [MaaCopilotServer](https://github.com/MaaAssistantArknights/MaaCopilotServer) (Development in progress, welcome to join us!)
- [Website](https://www.maa.plus): [maa-website](https://github.com/MaaAssistantArknights/maa-website) (Development in progress, welcome to join us!)

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
- [Integration Documentation](docs/en/INTEGRATION.md)
- [Callback Schema](docs/en/CALLBACK_SCHEMA.md)
- [Task Schema](docs/en/TASK_SCHEMA.md)
- [Copilot Schema](docs/en/COPILOT_SCHEMA.md)

### For Novice Users of Git

[Contributing](docs/en/CONTRIBUTING.md)

## Disclaimer

- The logo of this software is NOT granted rights under AGPL 3.0 License. The artist [耗毛](https://weibo.com/u/3251357314) and all developers of the software reserves all rights. The logo of the software shall not be used without authorization even if the project has an AGPL 3.0 License. Nor shall the logo be used for commercial purposes without authorization.
- The software is an open-source, free of charge software only for studying and communication purposes. There is no agreement or understanding between the developers of the software and the 3rd party person who uses this software as an assistant and charges you. In that case, the developers of the software is not responsible for the problems and consequences caused.

## Advertisement

[Bilibili Live](https://live.bilibili.com/2808861): live coding on this program  
[Technical Discussion & Talk](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)：QQ Group  
[Copilot JSON Sharing](https://jq.qq.com/?_wv=1027&k=1giyMpPb)  
[Dev Group](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)

Please click "Star" if you consider it helpful! Thank you for your support!
