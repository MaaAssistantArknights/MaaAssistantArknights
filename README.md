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

## 功能介绍

- 刷理智
- 基建换班，智能计算效率
- 自动公招
- 访问好友
- 收取信用及购物
- 收取日常奖励
- 全自动长草！

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

### 刷理智

- 若选项中没有你需要的关卡，请手动进入游戏 **蓝色开始按钮** 界面，并选择 `当前关卡`。
- 主界面上的 `吃理智`、`吃石头`、`指定次数` 三个选项为短路开关，即三个选项中的任一条件达到，均会视为任务完成，停止刷理智。
    - 举例1：设置 `吃理智药` : `999`、`吃石头` : `10`、`指定次数` : `1`。则在刷完 **一次** 后，由于满足了 `指定次数`:`1` 的条件，视为任务完成，停止刷理智。
    - 举例2：不勾选 `吃理智药`、不勾选 `吃石头`，设置 `指定次数` : `100`。则在当前可用理智全部刷完后（可能只刷了几次），由于满足了 `不吃理智药`、`不吃石头` 的条件，视为任务完成，停止刷理智。
- 其他优势
    - 刷完自动上传 [企鹅物流数据统计](https://penguin-stats.cn/)
    - 可自定义企鹅物流 ID
    - 识别并显示材料掉落
    - 掉线后会重连，继续刷上次的图
    - 凌晨 4 点刷新后也会重连，继续刷上次的图
    - 支持剿灭模式
    - 支持打完升级了的情况
    - 支持代理失败的情况，会自动放弃本次行动

### 基建换班

#### 换班策略

自动计算并选择 **单设施内的最优解**，支持所有通用类技能和特殊技能组合；支持识别经验书、赤金、原石碎片、芯片，分别使用不同的干员组合！

#### 宿舍入驻心情阈值

识别心情进度条的百分比；心情小于该阈值的干员，不会再去上班，直接进驻宿舍。

#### 特殊说明

- 基建换班目前均为单设施最优解，但非跨设施的全局最优解。例如：`巫恋+龙舌兰`、`红云+稀音` 等这类单设施内的组合，都是可以正常识别并使用的；`迷迭香`、`红松骑士团` 这类多个设施间联动的体系，暂不支持使用。
- 会客室仅缺一个线索时，会选择对应流派的干员；否则会选择通用干员。
- 会客室仅当自有线索满时，才会送出线索，并且只送三个。有需要的同学可自行修改 `resource/tasks.json` 中 `SelectClue` - `maxTimes` 字段，自定义送出个数。
- 控制中枢策略太过复杂，目前只考虑 `阿米娅`、`诗怀雅`、`凯尔希`、`彩虹小队` 及其他心情 +0.05 的干员，后续逐步优化。

### 信用商店随缘买

从左到右依次买，但不会买 `碳` 和 `家具零件` 。有需要的同学可自行修改 `resource/tasks.json` 中 `CreditShop-NotToBuy` - `text` 字段，自定义不买的物品。后续版本会尝试开放界面选项

### 公开招募识别

- 自动公招和公招识别是两个独立的功能！
- 自动公招支持使用 `加急许可`，全自动连续公招！请进入设置中选择~
- 出 5、6 星都会有弹窗提示

### 其他乱七八糟的说明

- 主界面上要执行的任务，是可以拖动改变顺序的。同样设置中基建换班的顺序，也是可以拖动改变的。
- 新的活动关卡刚上线的时候可能无法正常识别，一般一两天内软件会自动 OTA 更新资源，更新后即可正常识别。
- 活动关卡掉落识别到的 `未知材料`，一般就是活动商店的票据。
- `resource/config.json` 中有一些自定义选项，可以尝试根据自己的需要进行修改。
- 所有点击操作，都是点击按钮内随机位置，并模拟泊松分布（按钮偏中间位置点的概率大，越往旁边点到的概率越小）。
- 底层算法纯 C++ 开发，并设计了多重的缓存技术，最大限度降低 CPU 和内存占用。
- 软件支持自动更新✿✿ヽ(°▽°)ノ✿ 推荐非杠精的同学使用测试版，一般来说更新快且 bug 少（什么 MIUI (╯‵□′)╯︵┻━┻
- 如果新版本自动下载失败，可手动下载后，直接把压缩包放到同目录下，会自动更新的。

## 常见问题

### 软件一打开就闪退

- 可能性 1: 运行库问题。  
  请尝试安装 [Visual C++ Redistributable](https://docs.microsoft.com/zh-CN/cpp/windows/latest-supported-vc-redist?view=msvc-160#visual-studio-2015-2017-2019-and-2022)、[.NET Framework 4.8](https://dotnet.microsoft.com/download/dotnet-framework/net48) 后重新运行本软件。
- 可能性 2: CPU 指令集不支持。  
  项目使用 `PaddleOCR` 对游戏部分界面进行识别。`PaddleOCR` 用到了较新发布的 CPU 才支持的 `AVX` 指令集，而一些较老的 CPU 可能并不支持该指令集。  
  您可以尝试下载 [NoAVX](3rdparty/ppocr_noavx.zip) 版本的 `PaddleOCR`, 解压后替换本软件中同名的文件。这是对于使用不支持 `AVX` 指令集的 CPU 的用户的性能降低的替代方案，如非必要，请不要使用。  
  （具体可以下载 [CPU-Z](https://www.cpuid.com/softwares/cpu-z.html)，查看“指令集”中有无 `AVX` 这一项来判断）  
- 若上述均没有效果，请提 issue。

### 连接错误/捕获模拟器窗口错误

提示 : 请根据 [基本使用说明](#基本说明) 确定您的模拟器及打开方式正确

- 方法 1: 使用 [自定义连接](#自定义连接) 的方式连接模拟器
- 方法 2: 换模拟器，推荐 [蓝叠国际版](https://www.bluestacks.com/download.html)
- 方法 3: _根本解决方法_ 编辑 `resource/config.json`, 修改（最好是新增）模拟器窗口句柄名，并修改对应的 adb 设置。若您修改后可以提 PR 给我，我会感激不尽的_(:з」∠)_

### 自定义连接

- 下载 [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) ，将 `platform-tools` 文件夹解压到 `MeoAsstGui.exe` 的同级目录
- 进入软件 `设置` - `连接设置`，填写自定义 adb 地址（需要填写 IP + 端口，例如 `127.0.0.1:5555` ）

### 我是国际服玩家，可以使用么？ | I'm an international server player, could I use it?

可以！但是目前只支持少部分功能，请参考 [这个链接](resource/international/)

Yes, but there are few supported features, please refer to [this link](resource/international/)

## 致谢

### 开源库

- 图像识别库：[opencv](https://github.com/opencv/opencv.git)
- ~~文字识别库：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- 文字识别库：[PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- 关卡掉落识别：[企鹅物流识别](https://github.com/KumoSiunaus/penguin-stats-recognize-v3)
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

- 感谢 [tcyh035](https://github.com/tcyh035) 帮忙设计重构图形界面
- 感谢 [GengGode](https://github.com/GengGode) 和 [DbgDebug](https://github.com/DbgDebug) 提供图像算法思路并协助验证
- 感谢 [LoveLoliii](https://github.com/LoveLoliii) 提供公开招募算法及数据、部分功能逻辑思路
- 感谢 [dantmnf](https://github.com/dantmnf) 大佬提供各种 adb 及其他逻辑处理思路
- 感谢 [LmeSzinc](https://github.com/LmeSzinc) 提供的界面样式参考，~~虽然我抄了个四不像orz~~
- 感谢 [内卷地狱](https://jq.qq.com/?_wv=1027&k=ypbzXcA2) 的大佬们提供的各种帮助！
- 感谢参与软件测试、提 bug 的小伙伴们~
- ~~感谢 [B站直播间](https://live.bilibili.com/2808861) 的小伙伴们陪我弹幕吹水~~

## 开发相关

### Windows

直接使用 Visual 2019 或以上版本打开 `MeoAssistantArknights.sln` 即可，所有环境都是配置好的

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
