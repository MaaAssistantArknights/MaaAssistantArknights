---
icon: ic:round-home
index: true
dir:
  order: 0
---

::: center

![MAA Logo](https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_512x512.png =256x256)

# MaaAssistantArknights

![C++](https://img.shields.io/badge/C++-20-%2300599C?logo=cplusplus)
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blueviolet)
![license](https://img.shields.io/github/license/MaaAssistantArknights/MaaAssistantArknights) ![commit](https://img.shields.io/github/commit-activity/m/MaaAssistantArknights/MaaAssistantArknights?color=%23ff69b4)
![stars](https://img.shields.io/github/stars/MaaAssistantArknights/MaaAssistantArknights?style=social) ![GitHub all releases](https://img.shields.io/github/downloads/MaaAssistantArknights/MaaAssistantArknights/total?style=social)

[简体中文](../zh-cn/readme.md) | 繁體中文 | [English](../en-us/readme.md) | [日本語](../ja-jp/readme.md) | [한국어](../ko-kr/readme.md)

MAA 的意思是 MAA Assistant Arknights

一款明日方舟遊戲小助手

基於圖像辨識技術，一鍵完成全部日常任務！

絕讚更新中  ✿✿ヽ(°▽°)ノ✿

:::

## 亮點功能

- 刷理智，掉落辨識及上傳 [企鵝物流數據統計](https://penguin-stats.io/)；
- 智能基建換班，自動計算幹員效率，單設施內最優解；同時也支援 [自定義排班](./3.6-基建排班協議.md)；
- 自動公招，可選使用加急許可，一次全部刷完！公招數據上傳 [企鵝物流數據統計](https://penguin-stats.io/result/stage/recruit/recruit) ， [一圖流](https://ark.yituliu.cn/survey/maarecruitdata) ；
- 支援手動辨識公招介面，方便對高星公招做出選擇 ~~（你的這個高資回費出的是推王呢還是推王呢）~~
- 支援辨識幹員列表，統計已有和未有幹員（還能為手動辨識公招介面提供已有潛能數據提示）；
- 支援辨識養成材料，並可匯出至 [企鵝物流刷圖規劃器](https://penguin-stats.io/planner) 和 [明日方舟工具箱](https://arkntools.app/#/material) 計算缺少的養成材料；
- 訪問好友、收取信用及購物、領取日常獎勵等，一鍵全日常自動長草；
- 肉鴿全自動刷源石錠和蠟燭，自動辨識幹員及練度；
- 選擇作業 JSON 文件，自動抄作業， [影片範例](https://www.bilibili.com/video/BV1H841177Fk/) ；
- 倉庫辨識並支援匯出至 [企鵝物流刷圖規劃器](https://penguin-stats.io/planner) ， [明日方舟工具箱](https://arkntools.app/#/material) ， [ARK-NIGHTS 幹員培養表](https://ark-nights.com/settings) ；
- 支援 C, Python, Java, Rust, Golang, Java HTTP, Rust HTTP 等多種接口，方便集成調用，自定義你的 MAA！

話不多說，看圖！

```component ImageGrid
{
  "imageList": [
    {
      "light": "image/zh-cn/readme/1-light.png",
      "dark": "image/zh-cn/readme/1-dark.png"
    },
    {
      "light": "image/zh-cn/readme/2-light.png",
      "dark": "image/zh-cn/readme/2-dark.png"
    },
    {
      "light": "image/zh-cn/readme/3-light.png",
      "dark": "image/zh-cn/readme/3-dark.png"
    },
    {
      "light": "image/zh-cn/readme/4-light.png",
      "dark": "image/zh-cn/readme/4-dark.png"
    }
  ]
}
```

## 下載地址

- [穩定版 / 公測版](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases)
- [內測版](https://github.com/MaaAssistantArknights/MaaRelease/releases)

## 使用說明

### 基本說明

1. 請根據 [模擬器支援情況](./1.3-模擬器支援.md)，進行對應的操作。
2. 修改模擬器解析度為 `16:9` 比例，最低 `1280 * 720`，最高 `2K`。  
   對於國際服（美服）玩家，由於介面佈局問題，我們則建議您將解析度修改為 `1920 * 1080`。
3. 開始使用吧！

更多使用說明請參考 [詳細介紹](./1.1-詳細介紹.md)

## 常見問題

- 軟體一打開就閃退
- 連接錯誤、不知道 adb 路徑怎麽填寫
- 連接成功了，但沒反應
- 如何連接自定義端口
- 下載速度慢，且鏡像站無法打開網頁
- 下載到一半提示 “登錄” / “鑒權”
- 連接正常，任務開始了，但是沒反應

請參考 [常見問題](./1.2-常見問題.md)

## 外服支援

目前國際服（美服）、日服、韓服、繁中服的絕大部分功能均已支援。但由於外服用戶較少及項目人手不足，很多功能並沒有進行全面的測試，所以請自行體驗。  
若您遇到了 Bug，或對某個功能有強需求，歡迎在 [Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) 和 [討論區](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions) 催更；或加入我們一起建設 MAA！請參考 [外服適配教學](#外服適配)

## 主要關聯項目

**目前項目組非常缺前端大佬，若您有相關經驗，歡迎加入我們！**

- 全新框架：[MaaFramework](https://github.com/MaaAssistantArknights/MaaFramework)
- 全新 GUI：[MaaAsstElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI)
- [作業站](https://prts.plus) 前端：[maa-copilot-frontend](https://github.com/MaaAssistantArknights/maa-copilot-frontend)
- [作業站](https://prts.plus) 後端：[MaaBackendCenter](https://github.com/MaaAssistantArknights/MaaBackendCenter)
- [官網](https://maa.plus) ： [前端](https://github.com/MaaAssistantArknights/maa-website)
- 深度學習：[MaaAI](https://github.com/MaaAssistantArknights/MaaAI)

## 致謝

### 開源庫

- 圖像辨識庫：[opencv](https://github.com/opencv/opencv.git)
- ~~文字辨識庫：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- 文字辨識庫：[PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- 深度學習部署庫：[FastDeploy](https://github.com/PaddlePaddle/FastDeploy)
- 機器學習加速器：[onnxruntime](https://github.com/microsoft/onnxruntime)
- ~~關卡掉落辨識：[企鵝物流辨識](https://github.com/penguin-statistics/recognizer)~~
- 地圖格子辨識：[Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON 庫：[meojson](https://github.com/MistEO/meojson.git)
- C++ 運算符解析器：[calculator](https://github.com/kimwalisch/calculator)
- ~~C++ base64 編解碼：[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ 解壓壓縮庫：[zlib](https://github.com/madler/zlib)
- C++ Gzip 封裝：[gzip-hpp](https://github.com/mapbox/gzip-hpp)
- 安卓觸控事件器：[Minitouch](https://github.com/DeviceFarmer/minitouch)
- 安卓觸控事件器：[MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)
- WPF MVVM 框架：[Stylet](https://github.com/canton7/Stylet)
- WPF 控件庫：[HandyControl](https://github.com/HandyOrg/HandyControl)
- C# JSON 庫：[Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json)
- 下載器：[aria2](https://github.com/aria2/aria2)

### 數據源

- ~~公開招募數據：[明日方舟工具箱](https://www.bigfun.cn/tools/aktools/hr)~~
- ~~幹員及基建數據：[PRTS明日方舟中文WIKI](http://prts.wiki/)~~
- 關卡數據：[企鵝物流數據統計](https://penguin-stats.io/)
- 遊戲數據及資源：[明日方舟用戶端素材](https://github.com/yuanyan3060/ArknightsGameResource)
- ~~遊戲數據：[《明日方舟》遊戲數據](https://github.com/Kengxxiao/ArknightsGameData)~~

### 貢獻 / 參與者

感謝所有參與到開發 / 測試中的朋友們，是大家的幫助讓 MAA 越來越好！ (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=114514&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## 開發相關

### Windows

1. 下載預構建的第三方庫

    ```cmd
    python maadeps-download.py
    ```

2. 使用 Visual Studio 2022 打開 `MAA.sln`，右鍵 `MaaWpfGui`，設為啟動項目
3. VS 上方配置選擇 `RelWithDebInfo`, `x64` （如果編譯 Release 包 或 ARM 平台，請忽略這步）
4. 右鍵 `MaaWpfGui` - 屬性 - 偵錯 - 啟用本地偵錯（這樣就能把斷點掛到 C++ Core 那邊了）
5. （可選）若準備提交 PR，建議啟用 [clang-format 支援](./2.2-開發相關.md#在-visual-studio-中啟用-clang-format)

### Linux | macOS

請參考 [Linux 編譯教學](./2.1-Linux編譯教學.md)

### API

- [C 接口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/include/AsstCaller.h) ： [集成範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Cpp/main.cpp)
- [Python 接口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/asst/asst.py) ： [集成範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Python/sample.py)
- [Golang 接口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Golang/) ： [集成範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Golang/maa/maa.go)
- [Dart 接口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Dart)
- [Java 接口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaCore.java) ： [集成範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaJavaSample.java)
- [Java HTTP 接口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/src/Java/Readme.md)
- [Rust 接口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Rust/src/maa_sys) ： [HTTP 接口](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/master/src/Rust)
- [TypeScript 接口](https://github.com/MaaAssistantArknights/MaaAsstElectronUI/tree/main/packages/main/coreLoader)
- [集成文件](./3.1-集成文件.md)
- [回呼訊息協議](./3.2-回呼訊息協議.md)
- [任務流程協議](./3.4-任務流程協議.md)
- [自動抄作業協議](./3.3-戰鬥流程協議.md)

### 外服適配

請參考 [外服適配教學](./2.5-外服適配教學.md)，對於陸服已支援的功能，絕大部分的外服適配工作僅需要截圖 + 簡單的 JSON 修改即可。

### 想參與開發，但不太會用 Github?

[Github Pull Request 流程簡述](./2.2-開發相關.md#github-pull-request-流程簡述)

### Issue bot

請參考 [Issue bot 使用方法](./2.3-IssueBot使用方法.md)

## 聲明

- 本軟體 logo 並非使用 AGPL 3.0 協議開源，[耗毛](https://weibo.com/u/3251357314)、vie 兩位畫師及軟體全體開發者保留所有權利。不得以 AGPL 3.0 協議已授權為由在未經授權的情況下使用本軟體 logo，不得在未經授權的情況下將本軟體 logo 用於任何商業用途。
- 本軟體開源、免費，僅供學習交流使用。若您遇到商家使用本軟體進行代練並收費，可能是設備與時間等費用，產生的問題及後果與本軟體無關。

## 廣告

用戶交流 QQ 群：[MAA 使用 & 粥遊交流 QQ 群](https://ota.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)  
用戶交流 TG 群：[Telegram 群](https://t.me/+Mgc2Zngr-hs3ZjU1)  
自動戰鬥 JSON 作業分享：[prts.plus](https://prts.plus) 或 [抄作業.com](http://抄作業.com)  
Bilibili 直播間：[直播間](https://live.bilibili.com/2808861) 每晚直播敲代碼，近期很長一段時間應該都是在寫本軟體 ~  

技術群（舟無關、禁水）：[內卷地獄！(QQ 群)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)  
開發者群：[QQ 群](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)  

如果覺得軟體對你有幫助，幫忙點個 Star 吧！~（網頁最上方右上角的小星星），這就是對我們最大的支持了！

<!-- markdownlint-disable-file MD034 MD041 -->
