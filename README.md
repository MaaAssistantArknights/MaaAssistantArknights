# MeoAssistance

<div>
    <img alt="C++" src="https://img.shields.io/badge/c++-17-%2300599C?logo=cplusplus">
    <img alt="VS" src="https://img.shields.io/badge/VisualStudio-19-%235C2D91?logo=visualstudio">
</div>
<div>
    <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2064bit-%230078D6?logo=windows">
</div>
<div>
    <img alt="license" src="https://img.shields.io/github/license/MistEO/MeoAssistance-Arknights">
    <img alt="commit" src="https://img.shields.io/github/commit-activity/m/MistEO/MeoAssistance-Arknights?color=%23ff69b4">
    <img alt="stars" src="https://img.shields.io/github/stars/MistEO/MeoAssistance-Arknights?style=social">
</div>
<br>

A game assistance for Arknights

一款明日方舟的游戏助手，自动刷理智、智能基建换班、公招识别等，一键完成所有日常，全自动长草！！！

纯图像识别，非内存挂，全图形化界面，兼容模拟器和安卓设备，开罐即食，绝赞开发中！✿✿ヽ(°▽°)ノ✿  
<br>

![界面截图1](images/gui.png)
![界面截图2](images/settings1.png)
![界面截图3](images/settings2.png)

## 下载地址

[稳定版](https://github.com/MistEO/MeoAssistance/releases/latest)  
[测试版](https://github.com/MistEO/MeoAssistance/releases)

## 功能介绍

- **全自动基建换班（新功能！）**
    - 自动识别干员技能，自动计算设施内最优解并换班！
    - 支持如`巫恋组`、`温蒂组`等等所有单设施内组合
    - 自动识别干员心情并入驻宿舍
    - 可设置自动使用无人机给制造站/贸易站加速
    - 可识别赤金、经验书、搓玉，分别使用不同的干员组合
    - `迷迭香`等跨设施体系正在开发中！
- 自动刷理智
    - 支持刷完自动上传[企鹅物流数据统计](https://penguin-stats.cn/)
    - 支持企鹅物流自定义ID
    - 界面支持统计掉落数量
    - 可设置是否吃完理智药及数量
    - 可设置是否吃石头及数量
    - 可设置刷的次数（用来刷剿灭啥的）
    - 可设置刷完自动关机
    - 支持剿灭模式
    - 支持打完升级了的情况
    - 支持代理失败的情况，会自动放弃本次行动
    - 支持刷完自动截图
    - 支持掉线后重连，继续刷上次的图
    - 支持凌晨4点更新后重连，继续刷上次的图
- 公开招募识别
    - 自动识别当前招募页所有Tags
    - 自动计算可能出的干员组合并显示
    - 自动帮你点击最优解Tags
    - 自动帮你点击时间9小时
    - 出5、6星干员弹窗提示
    - 最新版本已支持2.5新增的`阿`等干员
- 自动访问好友
    - 访问完了还会贴心的帮你点进信用商店~
    - 可设置访问完自动买信用商店的材料
- 其他优势
    - 所有点击操作，都是点击按钮内随机位置，并模拟泊松分布（按钮偏中间位置点的概率大，越往旁边点到的概率越小）
    - 刷理智及访问好友的点击操作，支持设置随机延时，没有封号风险~
    - 底层算法纯C++开发，并设计了多重的缓存技术，最大限度降低CPU和内存占用
    - 模拟器窗口可以被遮挡、可以最小化、甚至可以老板键隐藏！即使全屏看视频、玩游戏，也完全不影响软件运行
    - 软件支持自动更新✿✿ヽ(°▽°)ノ✿
- 支持多款主流模拟器
- 兼容安卓手机（USB调试、无线调试）
- 兼容非16:9分辨率：包括宽屏、方屏，也兼容在游戏设置中的异形屏适配（基建换班功能除外，正在适配中）。但仍推荐使用16:9分辨率
- 自适应分辨率及屏幕缩放
- 未来更多功能见[Todo](#Todo)

### 模拟器支持

#### 蓝叠模拟器

完美支持。需要在模拟器`设置`-`引擎设置`中打开`允许ADB连接`

#### 蓝叠模拟器国际版

完美支持。需要在模拟器`设定`-`进阶`中打开`Android调试桥`

#### 蓝叠模拟器Hyper-V版本

支持。

1. 需要在模拟器`设定`-`进阶`中打开`Android调试桥`
2. 需要在软件`设置`-`连接设置`中添加蓝叠安装目录下`bluestacks.conf`文件的完整路径

#### 夜神模拟器

完美支持

#### MuMu模拟器

完美支持

#### 雷电模拟器

勉强支持，雷电总有莫名其妙的问题，可以试试看，不保证能用（

#### 逍遥模拟器

支持

#### 腾讯手游助手

不支持，新版本的腾讯好像也是自研引擎了，没开放ADB端口。但是测试是能响应Win32 Api的，有需求再做

#### MuMu手游助手（星云引擎）  

不支持，星云引擎这个版本不支持adb控制，甚至不响应Win32 Api鼠标消息，无解_(:з」∠)_

#### 其他模拟器

若有其他需要，欢迎给我提[ISSUE](https://github.com/MistEO/MeoAssistance/issues)，会根据情况尽量适配~

#### 安卓手机/平板

部分功能支持，正在开发中……  
需要下载[谷歌官方ADB](https://dl.google.com/android/repository/platform-tools-latest-windows.zip)，将`platform-tools`文件夹解压到`MeoAsstGui.exe`的同级目录

## 使用说明

### 基本说明

1. 根据上面模拟器支持情况，进行对应的`ADB`相关操作
2. 解压压缩包，到**没有中文或特殊符号**的文件夹路径
3. 第一次运行软件，**请使用管理员权限**打开`MeoAsstGui.exe`。运行过一次后，后续不再需要管理员权限（之后的版本会尝试完全去掉管理员权限）
4. 开始运行后，所有设置均不可再修改
5. 运行期间，模拟器窗口可以最小化，全屏玩游戏、看视频等，完全不影响

目前非16:9分辨率下，刷理智、访问好友、公招识别已初步可用，基建功能暂不可用，后期将逐步优化。但仍推荐使用16:9分辨率，经过的测试验证最多，也最稳定。

### Tips

若明日方舟处于**蓝色开始按钮**界面，则会刷当前关卡。否则会前往上一次作战的关卡！（自动选关功能正在开发中

### 基建换班

#### 换班策略

自动计算并选择**单设施内的最优解**，支持所有通用类技能和特殊技能组合；支持识别经验书、赤金、原石碎片、芯片，分别使用不同的干员组合！

#### 宿舍入驻心情阈值

计算心情进度条的百分比；心情小于该阈值的干员，不会再去上班，直接进驻宿舍

#### 特殊说明

- 基建换班目前均为单设施最优解，但非跨设施的全局最优解，例如`迷迭香`这类多个设施间联动的体系，都是不支持的
- 会客室仅缺一个线索时，会选择对应流派的干员；否则会选择通用干员
- 控制中枢策略太过复杂，目前只考虑`阿米娅`、`诗怀雅`、`凯尔希`、`彩虹小队`及其他心情+0.05的干员，后续逐步优化

### 公开招募识别

**请注意：公招识别和自动公招是两个独立的功能！**

1. 明日方舟打开公开招募，有Tag选择的界面
2. 软件勾选你需要的选项，点击"开始识别"
3. 请检查识别结果是否正确，自行判断是否确定开始招募

### 连接自定义模拟器端口，或安卓手机/平板

1. 下载[ADB程序](https://dl.google.com/android/repository/platform-tools-latest-windows.zip)，将`platform-tools`文件夹解压到`MeoAsstGui.exe`的同级目录
2. 使用USB有线连接安卓手机和电脑
3. 请在手机`设置`-`开发者选项`中打开`USB调试`、`USB调试（安全设置）`两个选项。具体操作方式不同品牌手机各不相同，请自行百度查询
4. 请手动修改`resource\config.json`文件中：
    1. `options`.`connectType`字段值为`1`
    2. `emulator`.`Custom`.`adb`.`addresses`字段填写为要连接的地址，请注意这是个数组，会以此尝试所有的（若不填写，或`addresses`中所有的都没连上，则会使用`adb devices`自动查找）

目前非16:9分辨率下，刷理智、访问好友、公招识别已初步可用，基建功能暂不可用，后期将逐步优化。但仍推荐使用16:9分辨率，经过的测试验证最多，也最稳定。

## Todo

- [x] 任务队列功能
- [ ] 常用关卡选关
- [x] 自动收任务功能
- [x] 基建智能换班功能
    - [x] 图形化界面
    - [x] 干员技能识别
    - [x] 干员识别准确率提高到100%
    - [x] 宿舍心情识别及入驻
    - [x] 制造站、贸易站智能换班
    - [x] 发电站、办公室换班
    - [x] 使用无人机
    - [x] 控制中枢智能换班
    - [x] 会客室智能换班
    - [x] 会客室智能线索交流
    - [ ] 支持`迷迭香`等复杂基建体系
    - [x] `激进换班模式`
    - [ ] 自定义换班（手动修改配置文件）
    - [ ] 宿舍换班支持加速心情的干员识别
    - [ ] 贸易站无人机支持设置给经验书还是赤金
- [ ] 使用GPU进行识别的版本
- [x] 企鹅物流汇报，自定义ID
- [ ] 指定刷某种材料xx个
- [ ] `config`中部分选项做成图形化界面
- [x] 界面拖动顺序保存
- [ ] 进一步的异形屏支持
- [x] 后台自动更新
- [x] 忽略当前版本更新
- [ ] 提供log接口，以及界面log
- [ ] 更换OCR库，提高公开招募识别率
- [x] 终极目标！全自动长草机！！！

## 致谢

### 开源库

- 图像识别库：[opencv](https://github.com/opencv/opencv.git)
- 文字识别库：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)
- 关卡掉落识别：[企鹅物流识别](https://github.com/KumoSiunaus/penguin-stats-recognize-v3)
- C++ JSON库：[meojson](https://github.com/MistEO/meojson.git)
- C++ 运算符解析器：[calculator](https://github.com/kimwalisch/calculator)
- WPF MVVW框架：[Stylet](https://github.com/canton7/Stylet)
- WPF控件库：[HandyControl](https://github.com/HandyOrg/HandyControl)
- C# JSON库: [Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json)

### 数据源

- 公开招募数据：[明日方舟工具箱](https://www.bigfun.cn/tools/aktools/hr)
- 干员及基建数据：[PRTS明日方舟中文WIKI](http://prts.wiki/)
- 关卡数据：[企鹅物流数据统计](https://penguin-stats.cn/)

### 贡献/参与者

- 非常感谢 [tcyh035](https://github.com/tcyh035) 帮忙设计重构图形界面
- 非常感谢 [GengGode](https://github.com/GengGode) 和 [DbgDebug](https://github.com/DbgDebug) 提供图像算法思路并协助验证
- 非常感谢 [LoveLoliii](https://github.com/LoveLoliii) 提供公开招募算法及数据、部分功能逻辑思路
- 感谢[AAH](https://github.com/ninthDevilHAUNSTER/ArknightsAutoHelper)的大佬们协助提供部分图像、操作思路
- 感谢参与软件测试、提bug的小伙伴们~
- ~~感谢[B站直播间](https://live.bilibili.com/2808861)的小伙伴们陪我弹幕吹水~~

## 广告

[B站直播间](https://live.bilibili.com/2808861)：每晚直播敲代码，近期很长一段时间应该都是在写本助手软件  
[QQ群](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)：欢迎加入~  

另：作者前端苦手，寻大佬帮忙一起写界面，继续现在的WPF也行，用electron或者别的重写也行，球球了QAQ
