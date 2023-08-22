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

MAA 的意思是 MAA Assistant Arknights

一款明日方舟遊戲小助手

基於圖像辨識技術，一鍵完成全部日常任務！

絕讚更新中  ✿✿ヽ(°▽°)ノ✿<br>

</div>

## 亮點功能

- 刷理智，掉落辨識及上傳 [企鵝物流](https://penguin-stats.cn/)
- 智能基建換班，自動計算幹員效率，單設施內最優解
- 自動公招，可選使用加急許可，一次全部刷完！公招数据上傳 [企鵝物流](https://penguin-stats.cn/result/stage/recruit/recruit)，[一圖流](https://yituliu.site/maarecruitdata)
- 訪問好友、收取信用及購物、領取日常獎勵等。一鍵全日常自動長草！
- 肉鴿全自動刷源石錠和蠟燭，自動辨識幹員及練度
- 導入作業 JSON 檔案，自動抄作業！ [影片演示](https://www.bilibili.com/video/BV1H841177Fk/)
- 新功能：倉庫辨識！支援導出至 [企鵝物流刷圖規劃器](https://penguin-stats.cn/planner), [明日方舟工具箱](https://arkn.lolicon.app/#/material), [ARK-NIGHTS 幹員培養表](https://ark-nights.com/settings)

話不多說，看圖！<br>

![tw1](https://user-images.githubusercontent.com/99072975/232383228-5bc20497-fbaf-44d5-a808-957c0a2fdccb.png)
![wt2](https://user-images.githubusercontent.com/99072975/232383235-7b2f66d4-518c-4cc9-a3e1-d850a107699d.png)

## 下載連結

[穩定版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases/latest)<br>
[開發版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)

## 使用說明

### 基本說明

1. 請根据 [模擬器支援情況](./1.3-模擬器支援.md)，進行對應的操作。
2. 修改模擬器解析度為 `16:9` 比例，最低 `1280 * 720`, 更高不限。
3. 開始使用吧！

更多使用說明請參考 [詳細介紹](./1.1-詳細介紹.md)

## 常見問題

- 軟體一打開就閃退
- 連接錯誤、不知道 adb 路徑怎麼填寫
- 辨識錯誤/任務開始後一直不動、沒有反應
- 如何連接自定義通訊埠
- 下載速度慢，且鏡像站無法打開網頁
- 下載到一半提示“登陸”/“鑑權”

請參考 [常見問題](./1.2-常見問題.md)

## 外服支援

目前已支持國際客戶端（英文客戶端）、日語客戶端、韓語客戶端、繁體中文客戶端的大部分功能。 但由於海外用戶少，項目人員緊缺，很多功能還沒有完全測試，還請大家自行體驗。<br>
如果遇到BUG，或者對某個功能有強烈需求，歡迎到 [Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) 和 [討論](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions) 催更；或加入我們建設 MAA！ 請參考[外服适配教程](#外服適配)

## 主要關聯項目

- 全新框架: [MaaFramework](https://github.com/MaaAssistantArknights/MaaFramework)
- 全新 GUI：[MaaAsstElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI)
- [作業站](https://prts.plus)：[前端](https://github.com/MaaAssistantArknights/maa-copilot-frontend)
- 後端：[MaaBackendCenter](https://github.com/MaaAssistantArknights/MaaBackendCenter)
- [官網](https://maa.plus)：[前端](https://github.com/MaaAssistantArknights/maa-website)
- Deep Learning: [MaaAI](https://github.com/MaaAssistantArknights/MaaAI)

## 致謝

### 開源庫

- 圖像辨識庫：[opencv](https://github.com/opencv/opencv.git)
- ~~字元辨識庫：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- 字元辨識庫：[PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- 深度學習部署庫：[FastDeploy](https://github.com/PaddlePaddle/FastDeploy)
- 機器學習加速器：[onnxruntime](https://github.com/microsoft/onnxruntime)
- ~~關卡掉落辨識：[企鵝物流辨識](https://github.com/penguin-statistics/recognizer)~~
- 地圖格子辨識：[Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON庫：[meojson](https://github.com/MistEO/meojson.git)
- C++ 算子分析器：[calculator](https://github.com/kimwalisch/calculator)
- ~~C++ base64編解碼：[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ 解壓壓縮庫：[zlib](https://github.com/madler/zlib)
- C++ Gzip封裝：[gzip-hpp](https://github.com/mapbox/gzip-hpp)
- 安卓觸控事件器：[minitouch](https://github.com/openstf/minitouch)
- 安卓觸控事件器：[MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)
- WPF MVVM框架：[Stylet](https://github.com/canton7/Stylet)
- WPF部件庫：[HandyControl](https://github.com/HandyOrg/HandyControl)
- C# JSON庫：[Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json)
- 下載器：[aria2](https://github.com/aria2/aria2)

### 資料源

- ~~公開招募資料：[明日方舟工具箱](https://www.bigfun.cn/tools/aktools/hr)~~
- ~~幹員及基建資料：[PRTS明日方舟中文WIKI](http://prts.wiki/)~~
- 關卡資料：[企鵝物流資料統計](https://penguin-stats.cn/)
- 遊戲數據及資源：[明日方舟客户端素材](https://github.com/yuanyan3060/ArknightsGameResource)
- ~~遊戲數據：[《明日方舟》游戏数据](https://github.com/Kengxxiao/ArknightsGameData)~~

### 貢獻/參與者

感謝所有參與到開發/測試中的朋友們，是大家的幫助讓 MAA 越來越好！ (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## 開發相關

### Windows

1. 下載預構建的第三方庫

    ```cmd
    python maadeps-download.py
    ```

2. 使用 Visual Studio 2022 打開 `MAA.sln`，右鍵 `MaaWpfGui`，設為啟動項目
3. 右鍵 `MaaWpfGui` - 屬性 - 調試 - 啟用本地調試（這樣就能把斷點掛到 C++ Core 那邊了）
4. （可選）若準備提交 PR，建議啟用 [clang-format 支持](./2.2-開發相關.md#在-visual-studio-中启用-clang-format)

### Linux | macOS

請參考 [Linux 編譯教程](./2.1-Linux編譯教程.md)

### API

- [C 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/include/AsstCaller.h)：[整合示例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp)
- [Python 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/asst/asst.py)：[整合示例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py)
- [Golang 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Golang/)：[整合示例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Golang/maa/maa.go)
- [Dart 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Dart)
- [Java 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaCore.java)：[整合示例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaJavaSample.java)
- [Java HTTP 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/Readme.md)
- [Rust 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Rust/src/maa_sys)：[HTTP 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Rust)
- [TypeScript 介面](https://github.com/MaaAssistantArknights/MaaAsstElectronUI/tree/main/packages/main/coreLoader)
- [整合文件](./3.1-集成文件.md)
- [回呼訊息協定](./3.2-回呼訊息協定.md)
- [任務流程協定](./3.4-任務流程協定.md)
- [自動抄作業協定](./3.3-戰鬥流程協定.md)

### 外服適配

請參考 [外服適配教程](./2.5-外服適配教程.md)，對於國服已支持的功能，絕大部分的外服適配工作僅需要截圖 + 簡單的 JSON 修改即可。

### 想參與開發，但不太會用 Github?

[Github Pull Request 流程簡述](./2.2-開發相關.md)

### Issue Bot

請參考 [Issue Bot 使用方法](./2.3-IssueBot使用方法.md)。

## 聲明

- 本軟體 logo 並非使用 AGPL 3.0 條款開源，[耗毛](https://weibo.com/u/3251357314)、Vie 兩位畫師以及軟體全體開發者保留所有權利。不得以 AGPL 3.0 條款已授權為由在未經授權的情況下使用本軟體 logo, 不得在未經授權的情況下將本軟體 logo 用於任何商業用途。
- 本軟體開源、免費，僅供學習交流使用。若您遇到商家使用本軟體進行代練並收費，可能是設備與時間等費用，產生的問題及後果與本軟件無關。

## 廣告

用戶交流QQ群：[MAA使用 & 舟遊交流群](https://ota.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)<br>
用戶交流TG群：[Telegram 群](https://t.me/+Mgc2Zngr-hs3ZjU1)<br>
自動戰鬥 JSON 作業分享：[prts.plus](https://prts.plus)<br>
Bilibili 直播間：[直播間](https://live.bilibili.com/2808861) 每晚直播敲代碼，近期很長一段時間應該都是在寫本軟體~<br>

技術交流群(舟無關)：[內捲地獄！(QQ 群)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)<br>
開發者群：[QQ 群](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)<br>

如果覺得軟體對你有幫助，幫忙點個 Star 吧！~（網頁最上方右上角的小星星），這就是對我們最大的支持了！
