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

MAA 的意思是 MAA Assistant Arknights

一款明日方舟游戏小助手

基于图像识别技术，一键完成全部日常任务！

绝赞更新中  ✿✿ヽ(°▽°)ノ✿  

</div>

## 亮点功能

- 刷理智，掉落识别及上传 [企鹅物流](https://penguin-stats.cn/)
- 智能基建换班，自动计算干员效率，单设施内最优解；同时也支持 [自定义排班](docs/3.6-基建排班协议.md)
- 自动公招，可选使用加急许可，一次全部刷完！公招数据上传 [企鹅物流](https://penguin-stats.cn/result/stage/recruit/recruit)，[一图流](https://yituliu.site/maarecruitdata)
- 访问好友、收取信用及购物、领取日常奖励等。一键全日常自动长草！
- 肉鸽全自动刷源石锭和蜡烛，自动识别干员及练度
- 选择作业 JSON 文件，自动抄作业！ [视频演示](https://www.bilibili.com/video/BV1H841177Fk/)
- 仓库识别并支持导出至 [企鹅物流刷图规划器](https://penguin-stats.cn/planner), [明日方舟工具箱](https://arkn.lolicon.app/#/material), [ARK-NIGHTS 干员培养表](https://ark-nights.com/settings)
- 支持 C, Python, Java, Rust, Golang, Java HTTP, Rust HTTP 等多种接口，方便集成调用，自定义你的 MAA！

话不多说，看图！  

![image](https://user-images.githubusercontent.com/18511905/189659056-67587818-9844-4f5a-b2f5-838a8932b387.png)
![image](https://user-images.githubusercontent.com/99072975/181418131-f4eefa07-6623-470e-ba4a-0ad15cb8aa91.png)
![image](https://user-images.githubusercontent.com/99072975/172045163-e9ead337-eb62-4f9f-a354-9e302f767a52.png)

## 下载地址

[稳定版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)  
[开发版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

## 使用说明

### 基本说明

1. 请根据 [模拟器支持情况](docs/1.3-模拟器支持.md)，进行对应的操作。
2. 修改模拟器分辨率为 `16:9` 比例，最低 `1280 * 720`，更高不限。
3. 开始使用吧！

更多使用说明请参考 [详细介绍](docs/1.1-详细介绍.md)

## 常见问题

- 软件一打开就闪退
- 连接错误、不知道 adb 路径怎么填写
- 连接成功了，但没反应
- 如何连接自定义端口
- 下载速度慢，且镜像站无法打开网页
- 下载到一半提示“登陆”/“鉴权”
- 连接正常，任务开始了，但是没反应

请参考 [常见问题](docs/1.2-常见问题.md)

## 外服支持

- 国际服（美服）  
  支持基本的刷理智、信用购物、访问好友、领取奖励、自动肉鸽（测试版本）、公招识别，请参考 [说明](resource/global/YoStarEN/readme.md)
- 日服  
  支持基本的刷理智、自动基建、信用购物、自动公招、访问好友、领取奖励、自动肉鸽（测试版本）、公招识别，请参考 [说明](resource/global/YoStarJP/readme.md)
- 韩服  
  支持基本的刷理智、信用购物、访问好友、领取奖励、自动肉鸽（测试版本）、公招识别，请参考 [说明](resource/global/YoStarKR/readme.md)
- 繁中服  
  支持基本的刷理智、自动公招、自动肉鸽、领取日常、公招识别功能，请参考 [说明](resource/global/txwy/readme.md)

由于外服用户较少及项目人手不足的问题，外服目前都仅适配了基本的功能。若您有强需求，欢迎在 [讨论区](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions) 催更，或加入我们一起建设 MAA！

## 主要关联项目

- 全新 GUI：[MaaAsstElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI) （正在开发中，欢迎加入！）
- 数据服务器：[MaaGameDataServer](https://github.com/MaaAssistantArknights/MaaGameDataServer)
- 官网 [maa.plus](https://maa.plus)：[maa-website](https://github.com/MaaAssistantArknights/maa-website)
- 作业分享站 [prts.plus](https://prts.plus) 或 [抄作业.com](http://抄作业.com)：[前端](https://github.com/MaaAssistantArknights/maa-copilot-frontend)，[后端](https://github.com/MaaAssistantArknights/MaaCopilotServer)

## 致谢

### 开源库

- 图像识别库：[opencv](https://github.com/opencv/opencv.git)
- ~~文字识别库：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- 文字识别库：[PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- 深度学习部署库：[FastDeploy](https://github.com/PaddlePaddle/FastDeploy)
- 机器学习加速器：[onnxruntime](https://github.com/microsoft/onnxruntime)
- ~~关卡掉落识别：[企鹅物流识别](https://github.com/penguin-statistics/recognizer)~~
- 地图格子识别：[Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON库：[meojson](https://github.com/MistEO/meojson.git)
- C++ 运算符解析器：[calculator](https://github.com/kimwalisch/calculator)
- ~~C++ base64编解码：[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ 解压压缩库：[zlib](https://github.com/madler/zlib)
- C++ Gzip封装：[gzip-hpp](https://github.com/mapbox/gzip-hpp)
- 安卓触控事件器：[minitouch](https://github.com/openstf/minitouch)
- 安卓触控事件器：[MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)
- WPF MVVM框架：[Stylet](https://github.com/canton7/Stylet)
- WPF控件库：[HandyControl](https://github.com/HandyOrg/HandyControl)
- C# JSON库：[Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json)
- 下载器：[aria2](https://github.com/aria2/aria2)

### 数据源

- ~~公开招募数据：[明日方舟工具箱](https://www.bigfun.cn/tools/aktools/hr)~~
- 干员及基建数据：[PRTS明日方舟中文WIKI](http://prts.wiki/)
- 关卡数据：[企鹅物流数据统计](https://penguin-stats.cn/)
- 游戏数据及資源：[明日方舟bot常用素材](https://github.com/yuanyan3060/Arknights-Bot-Resource)
- 游戏数据：[《明日方舟》游戏数据](https://github.com/Kengxxiao/ArknightsGameData)

### 贡献/参与者

感谢所有参与到开发/测试中的朋友们，是大家的帮助让 MAA 越来越好！ (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## 开发相关

### Windows

- 直接使用 Visual Studio 2022 打开 `MAA.sln` 即可，所有环境都是配置好的
- 建议启用 clang-format 支持，详细内容可以参考 [在 Visual Studio 中启用 clang-format](docs/2.2-开发相关.md#在-visual-studio-中启用-clang-format)

### Linux | macOS

请参考 [Linux 编译教程](docs/2.1-Linux编译教程.md)

### API

- [C 接口](include/AsstCaller.h)：[集成示例](src/CppSample/main.cpp)
- [Python 接口](src/Python/asst.py)：[集成示例](src/Python/sample.py)
- [Golang 接口](src/Golang/)：[集成示例](src/Golang/maa/maa.go)
- [Dart 接口](src/dart/)
- [Java 接口](src/Java/src/main/java/com/iguigui/maaj/easySample/MaaCore.java)：[集成示例](src/Java/src/main/java/com/iguigui/maaj/easySample/MaaJavaSample.java)
- [Java HTTP 接口](src/Java/Readme.md)
- [Rust 接口](src/Rust/src/maa_sys/)：[HTTP 接口](src/Rust)
- [TypeScript 接口](https://github.com/MaaAssistantArknights/MaaAsstElectronUI/tree/main/packages/main/coreLoader)
- [集成文档](docs/3.1-集成文档.md)
- [回调消息协议](docs/3.2-回调消息协议.md)
- [任务流程协议](docs/3.4-任务流程协议.md)
- [自动抄作业协议](docs/3.3-战斗流程协议.md)

### Issue bot

- 使用 `Add {LABEL_NAME}` 增加标签，使用 `Remove {LABEL_NAME}` 删除标签。
- 在提交说明中用 `close #{ISSUE_NUMBER}` 或 `fix #{ISSUE_NUMBER}` 给对应议题增加 `fixed` 标签。

更多说明请参考 [Issue bot 使用方法](docs/2.3-IssueBot使用方法.md)。

### 想参与开发，但不太会用 Github?

[Github Pull Request 流程简述](docs/2.2-开发相关.md#github-pull-request-流程简述)

## 声明

- 本软件 logo 并非使用 AGPL 3.0 协议开源，[耗毛](https://weibo.com/u/3251357314)、vie 两位画师及软件全体开发者保留所有权利。不得以 AGPL 3.0 协议已授权为由在未经授权的情况下使用本软件 logo，不得在未经授权的情况下将本软件 logo 用于任何商业用途。
- 本软件开源、免费，仅供学习交流使用。若您遇到商家使用本软件进行代练并收费，可能是设备与时间等费用，产生的问题及后果与本软件无关。

## 广告

用户交流群：[Telegram 群](https://t.me/+Mgc2Zngr-hs3ZjU1)，[QQ 一群（已满）](https://jq.qq.com/?_wv=1027&k=ig786LJZ)，[QQ 二群（已满）](https://jq.qq.com/?_wv=1027&k=R3oleoKc)，[QQ 三群（已满）](https://jq.qq.com/?_wv=1027&k=mKdOnhWV)，[QQ 四群（已满）](https://jq.qq.com/?_wv=1027&k=ABkU8mCR)，[QQ 五群（已满）](https://jq.qq.com/?_wv=1027&k=To6b6H6m)，[QQ 六群（已满）](https://jq.qq.com/?_wv=1027&k=PYoCP2lS)，[QQ 七群（已满）](https://jq.qq.com/?_wv=1027&k=xDT9vHvB)，[QQ 八群](https://jq.qq.com/?_wv=1027&k=PzvqFhOr)  
自动战斗 JSON 作业分享：[prts.plus](https://prts.plus) 或 [抄作业.com](http://抄作业.com)  
[Bilibili 直播间](https://live.bilibili.com/2808861)：每晚直播敲代码，近期很长一段时间应该都是在写本软件~  
[舟无关技术交流 & 吹水群（QQ 群）](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)：内卷地狱！  
[开发者群（QQ 群）](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)

如果觉得软件对你有帮助，帮忙点个 Star 吧！~（网页最上方右上角的小星星），这就是对我们最大的支持了！
