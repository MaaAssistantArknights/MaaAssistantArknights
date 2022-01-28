<div align="center">

<img alt="LOGO" src="https://user-images.githubusercontent.com/18511905/148931479-23aef436-2fc1-4c1e-84c9-bae17be710a5.png" width=360 height=270/>

# MeoAssistantArknights

<br>
<div>
    <img alt="C++" src="https://img.shields.io/badge/c++-17-%2300599C?logo=cplusplus">
</div>
<div>
    <img alt="platform" src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blueviolet">
</div>
<div>
    <img alt="license" src="https://img.shields.io/github/license/MistEO/MeoAssistantArknights">
    <img alt="commit" src="https://img.shields.io/github/commit-activity/m/MistEO/MeoAssistantArknights?color=%23ff69b4">
    <img alt="stars" src="https://img.shields.io/github/stars/MistEO/MeoAssistantArknights?style=social">
</div>
<br>

A Game Assistant for Arknights

一款明日方舟游戏小助手，简称 MAA

基于图像识别技术，一键完成全部日常任务！

绝赞更新中  ✿✿ヽ(°▽°)ノ✿  

</div>

## 亮点功能

- 刷理智，掉落识别及上传 [企鹅物流](https://penguin-stats.cn/)
- 智能基建换班，自动计算干员效率，单设施内最优解
- 自动公招，可选使用加急许可，一次全部刷完
- 访问好友、收取信用及购物、领取日常奖励等。一键全日常自动长草！
- 新功能！全自动肉鸽刷源石锭及收藏品

话不多说，看图！  

![image](https://user-images.githubusercontent.com/18511905/148376809-a80537b7-5e97-4978-959e-afada28c03c3.png)  
![image](https://user-images.githubusercontent.com/18511905/148376301-c3296ff4-8df9-4573-858c-5dd2d03569b2.png)

## 下载地址

[稳定版](https://github.com/MistEO/MeoAssistantArknights/releases/latest)  
[测试版](https://github.com/MistEO/MeoAssistantArknights/releases)

## 使用说明

### 基本说明

1. 请根据 [模拟器支持情况](docs/模拟器支持.md)，进行对应的操作。
2. 解压压缩包，到 **没有中文或特殊符号** 的文件夹路径。
3. 第一次运行软件，**请使用管理员权限** 打开 `MeoAsstGui.exe`。运行过一次后，后续不再需要管理员权限。
4. 开始运行后，除 `自动关机` 外，所有设置均不可再修改。

目前仅对 `16:9` 分辨率支持较好，其他分辨率勉强可用，但可能会有奇奇怪怪的问题，正在进一步适配中_(:з」∠)_

更多使用说明请参考 [详细介绍](docs/详细介绍.md)

## 常见问题

### 软件一打开就闪退

- 可能性 1: 运行库问题。  
  请尝试安装 [Visual C++ Redistributable](https://docs.microsoft.com/zh-CN/cpp/windows/latest-supported-vc-redist?view=msvc-160#visual-studio-2015-2017-2019-and-2022)、[.NET Framework 4.8](https://dotnet.microsoft.com/download/dotnet-framework/net48) 后重新运行本软件。
- 可能性 2: CPU 指令集不支持。  
  项目使用 `PaddleOCR` 对游戏部分界面进行识别。`PaddleOCR` 用到了较新发布的 CPU 才支持的 `AVX` 指令集，而一些较老的 CPU 可能并不支持该指令集。  
  您可以尝试下载 [NoAVX](../3rdparty/ppocr_noavx.zip) 版本的 `PaddleOCR`, 解压后替换本软件中同名的文件。这是对于使用不支持 `AVX` 指令集的 CPU 的用户的性能降低的替代方案，如非必要，请不要使用。  
  （具体可以下载 [CPU-Z](https://www.cpuid.com/softwares/cpu-z.html)，查看“指令集”中有无 `AVX` 这一项来判断）  
- 若上述均没有效果，请提 issue。

### 连接错误/捕获模拟器窗口错误

提示 : 请根据 [使用说明](#使用说明) 确定您的模拟器及打开方式正确

- 方法 1: 使用 [自定义连接](#自定义连接) 的方式连接模拟器
- 方法 2: 换模拟器，推荐 [蓝叠国际版](https://www.bluestacks.com/download.html) Nougat 64 bit
- 方法 3: _根本解决方法_ 编辑 `resource/config.json`, 修改（最好是新增）模拟器窗口句柄名，并修改对应的 adb 设置。若您修改后可以提 PR 给我，我会感激不尽的_(:з」∠)_

### 识别错误/任务开始运行后一直不动、没有反应

提示 : 依次进行，若问题解决，则无需往下

1. 请根据 [模拟器支持情况](docs/模拟器支持.md) 确定您的模拟器是受支持的
2. 修改模拟器分辨率设置，`320 dpi`
3. 修改模拟器分辨率设置，横屏 `1280 * 720` 分辨率
4. 换模拟器，推荐 [蓝叠国际版](https://www.bluestacks.com/download.html) Nougat 64 bit
5. 若仍有问题，请提 issue

### 自定义连接

- 下载 [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) ，将 `platform-tools` 文件夹解压到 `MeoAsstGui.exe` 的同级目录
- 进入软件 `设置` - `连接设置`，填写自定义 adb 地址（需要填写 IP + 端口，例如 `127.0.0.1:5555` ）

## 关联项目

- 全新 GUI: [MeoAsstElectronUI](https://github.com/MaaAssistantArknights/MeoAsstElectronUI) （正在开发中）
- 自动更新服务器: [MaaDownloadServer](https://github.com/MaaAssistantArknights/MaaDownloadServer)（正在开发中）

## 致谢

### 开源库

- 图像识别库：[opencv](https://github.com/opencv/opencv.git)
- ~~文字识别库：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- 文字识别库：[PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- 关卡掉落识别：[企鹅物流识别](https://github.com/KumoSiunaus/penguin-stats-recognize-v3)
- 地图格子识别：[Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON库：[meojson](https://github.com/MistEO/meojson.git)
- C++ 运算符解析器：[calculator](https://github.com/kimwalisch/calculator)
- C++ base64编解码：[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)
- C++ 解压压缩库：[zlib](https://github.com/madler/zlib)
- C++ Gzip封装：[gzip-hpp](https://github.com/mapbox/gzip-hpp)
- WPF MVVW框架：[Stylet](https://github.com/canton7/Stylet)
- WPF控件库：[HandyControl](https://github.com/HandyOrg/HandyControl)
- C# JSON库: [Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json)
- 下载器：[aria2](https://github.com/aria2/aria2)

### 数据源

- 公开招募数据：[明日方舟工具箱](https://www.bigfun.cn/tools/aktools/hr)
- 干员及基建数据：[PRTS明日方舟中文WIKI](http://prts.wiki/)
- 关卡数据：[企鹅物流数据统计](https://penguin-stats.cn/)

### 贡献/参与者

感谢所有参与到开发/测试中的朋友们，是大家的帮助让 MAA 越来越好！  
这里仅列出对 MAA 提供了巨大帮助的大佬们 (*´▽｀)ノノ

- 感谢 [tcyh035](https://github.com/tcyh035) 帮忙设计重构 WPF 图形界面
- 感谢 [LoveLoliii](https://github.com/LoveLoliii) 提供公开招募算法及数据
- 感谢 [dantmnf](https://github.com/dantmnf) 提供各种 adb 及其他逻辑处理思路
- 感谢 [泰迪](https://github.com/yuanyan3060/Arknights-Tile-Pos) 提供的地图格子识别算法及开源库

## 开发相关

### Windows

直接使用 Visual 2019 或更高版本打开 `MeoAssistantArknights.sln` 即可，所有环境都是配置好的

### Linux | macOS

请参考 [Linux 编译教程](docs/Linux编译教程.md)

### API

- [C 接口](https://github.com/MistEO/MeoAssistantArknights/blob/dev/include/AsstCaller.h)
- [Python 接口](https://github.com/MistEO/MeoAssistantArknights/wiki/Python-%E6%8E%A5%E5%8F%A3)
- [Golang 接口](https://github.com/MistEO/MeoAssistantArknights/wiki/Golang-%E6%8E%A5%E5%8F%A3)
- [回调消息协议](docs/回调消息协议.md)

## 声明

本项目 logo 并非使用 AGPL 3.0 协议开源，画师 [耗毛](https://weibo.com/u/3251357314) 及项目全体开发者保留所有权利。不得以 AGPL 3.0 协议已授权为由在未经授权的情况下使用本项目 logo, 不得在未经授权的情况下将本项目 logo 用于任何商业用途。

## 广告

[B站直播间](https://live.bilibili.com/2808861)：每晚直播敲代码，近期很长一段时间应该都是在写本助手软件  
[QQ群：内卷地狱](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)：欢迎加入~  

如果觉得软件对你有帮助，帮忙点个 Star 吧！~（网页最上方右上角的小星星），这就是对我最大的支持了！
