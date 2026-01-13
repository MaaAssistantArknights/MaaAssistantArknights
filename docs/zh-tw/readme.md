---
icon: ic:round-home
index: true
dir:
  order: 0
---

::: center

![MAA Logo =256x256](/images/maa-logo_512x512.png)

# MAA

![C++](https://img.shields.io/badge/C++-20-%2300599C?logo=cplusplus)  
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blueviolet)  
![license](https://img.shields.io/github/license/MaaAssistantArknights/MaaAssistantArknights) ![commit](https://img.shields.io/github/commit-activity/m/MaaAssistantArknights/MaaAssistantArknights?color=%23ff69b4)  
![stars](https://img.shields.io/github/stars/MaaAssistantArknights/MaaAssistantArknights?style=social) ![GitHub all releases](https://img.shields.io/github/downloads/MaaAssistantArknights/MaaAssistantArknights/total?style=social)

MAA 的意思是 MAA Assistant Arknights

一款明日方舟遊戲小助手

基於影像辨識技術，一鍵完成全部日常任務！

絕讚更新中 ✿✿ヽ(°▽°)ノ✿

:::

## 下載與安裝

請閱讀 [文件](./manual/newbie.md) 後前往 [官網](https://maa.plus) 或 [Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 下載，並參考 [新手上路](./manual/newbie.md) 進行安裝。

## 亮點功能

- 理智作戰，掉落辨識及上傳 [企鵝物流](https://penguin-stats.io/)，[一圖流](https://ark.yituliu.cn/)
- 智慧基建換班，自動計算幹員效率，單設施內最佳解；同時也支援 [自定義排班](./protocol/base-scheduling-schema.md)
- 自動公招，可選使用加急許可，一次全部刷完！公招數據自動上傳 [企鵝物流](https://penguin-stats.io/result/stage/recruit/recruit)，[一圖流](https://ark.yituliu.cn/survey/maarecruitdata)
- 支援手動辨識公招介面，方便對高星公招做出選擇 ~~（你的這個高資回費出的是推進之王呢還是推進之王呢）~~
- 支援辨識幹員列表，統計已有和未有幹員及潛能，並在公招辨識顯示
- 支援辨識養成材料，並匯出至 [企鵝物流刷圖規劃](https://penguin-stats.cn/planner)、[明日方舟工具箱](https://arkntools.app/#/material)、[ARK-NIGHTS 幹員培養表](https://ark-nights.com/settings)
- 造訪好友、收取信用及購物、領取日常獎勵等，一鍵全日常自動長草
- 肉鴿全自動刷源石錠和等級，自動燒水和凹直升，智慧辨識幹員及練度
- 選擇作業 JSON 檔案，自動抄作業， [影片展示](https://www.bilibili.com/video/BV1H841177Fk/)
- 支援 C, Python, Java, Rust, Golang, Java HTTP, Rust HTTP 等多種介面，方便整合呼叫，自定義你的 MAA！

話不多說，看圖！

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  },
  {
    light: 'images/zh-cn/readme/3-light.png',
    dark: 'images/zh-cn/readme/3-dark.png'
  },
  {
    light: 'images/zh-cn/readme/4-light.png',
    dark: 'images/zh-cn/readme/4-dark.png'
  }
]" />

## 使用說明

### 功能介紹

請參閱 [用戶手冊](./manual/)。

### 外服支援

目前國際服（美服）、日服、韓服、繁中服的絕大部分功能均已支援。但由於外服用戶較少及項目人手不足，很多功能並沒有進行全面的測試，所以請自行體驗。  
若您遇到了 Bug，或對某個功能有強烈需求，歡迎在 [Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) 和 [討論區](https://github.com/MaaAssistantArknights/MaaAssistantArknights/discussions) 催更；或加入我們一起建設 MAA！請參閱 [外服適配教學](#外服適配)

### CLI 支援

MAA 支援命令列介面（CLI）操作，支援 Linux，macOS 和 Windows，可用於自動化指令碼或在無圖形介面的伺服器上使用。請參閱 [CLI 使用指南](./manual/cli/)

## 加入我們

### 主要關聯專案

- 全新框架：[MaaFramework](https://github.com/MaaXYZ/MaaFramework)
- [作業站](https://prts.plus) 前端：[zoot-plus-frontend](https://github.com/ZOOT-Plus/zoot-plus-frontend)
- [作業站](https://prts.plus) 後端：[ZootPlusBackend](https://github.com/ZOOT-Plus/ZootPlusBackend)
- [官網](https://maa.plus)：[前端](https://github.com/MaaAssistantArknights/maa-website)
- 深度學習：[MaaAI](https://github.com/MaaAssistantArknights/MaaAI)

### 多語言 (i18n)

MAA 以中文（簡體）為第一語言，翻譯詞條均以中文（簡體）為準。

### 參與開發

請參閱 [開發指南](./develop/development.md)。

### API

- [C 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/include/AsstCaller.h)：[整合範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Cpp/main.cpp)
- [Python 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Python/asst/asst.py)：[整合範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Python/sample.py)
- [Golang 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Golang)：[整合範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Golang/maa/maa.go)
- [Dart 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Dart)
- [Java 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaCore.java)：[整合範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/src/main/java/com/iguigui/maaj/easySample/MaaJavaSample.java)
- [Java HTTP 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Java/Readme.md)
- [Rust 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Rust/src/maa_sys)：[HTTP 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/tree/dev/src/Rust)
- [TypeScript 介面](https://github.com/MaaX/tree/main/packages/main/coreLoader)
- [Woolang 介面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Woolang/maa.wo)：[整合範例](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/src/Woolang/demo.wo)
- [整合文件](./protocol/integration.md)
- [回呼訊息協定](./protocol/callback-schema.md)
- [任務流程協定](./protocol/task-schema.md)
- [自動抄作業協定](./protocol/copilot-schema.md)

### 外服適配

請參閱 [外服適配教學](./develop/overseas-client-adaptation.md)，對於國服已支援的功能，絕大部分的外服適配工作僅需要截圖 + 簡單的 JSON 修改即可。

### Issue bot

請參閱 [Issue Bot 使用方法](./develop/issue-bot-usage.md)

## 致謝

### 開源庫

- 影像辨識庫：[opencv](https://github.com/opencv/opencv.git)
- ~~文字辨識庫：[chineseocr_lite](https://github.com/DayBreak-u/chineseocr_lite.git)~~
- 文字辨識庫：[PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR)
- 深度學習佈署庫：[FastDeploy](https://github.com/PaddlePaddle/FastDeploy)
- 機器學習加速器：[onnxruntime](https://github.com/microsoft/onnxruntime)
- ~~關卡掉落辨識：[企鵝物流辨識](https://github.com/penguin-statistics/recognizer)~~
- 地圖格子辨識：[Arknights-Tile-Pos](https://github.com/yuanyan3060/Arknights-Tile-Pos)
- C++ JSON 庫：[meojson](https://github.com/MistEO/meojson.git)
- C++ 運算子解析器：[calculator](https://github.com/kimwalisch/calculator)
- ~~C++ base64 編解碼：[cpp-base64](https://github.com/ReneNyffenegger/cpp-base64)~~
- C++ 解壓縮庫：[zlib](https://github.com/madler/zlib)
- C++ Gzip 封裝：[gzip-hpp](https://github.com/mapbox/gzip-hpp)
- Android 觸控事件器：[Minitouch](https://github.com/DeviceFarmer/minitouch)
- Android 觸控事件器：[MaaTouch](https://github.com/MaaAssistantArknights/MaaTouch)
- WPF MVVM 框架：[Stylet](https://github.com/canton7/Stylet)
- WPF 控制項庫：[HandyControl](https://github.com/HandyOrg/HandyControl) -> [HandyControls](https://github.com/ghost1372/HandyControls)
- C# 記錄檔：[Serilog](https://github.com/serilog/serilog)
- C# JSON 庫：[Newtonsoft.Json](https://github.com/JamesNK/Newtonsoft.Json) & [System.Text.Json](https://github.com/dotnet/runtime)
- ~~下載器：[aria2](https://github.com/aria2/aria2)~~

### 數據源

- ~~公開招募數據：[明日方舟工具箱](https://www.bigfun.cn/tools/aktools/hr)~~
- ~~幹員及基建數據：[PRTS Wiki](http://prts.wiki/)~~
- 關卡數據：[企鵝物流數據統計](https://penguin-stats.io/)
- 遊戲數據及資源：[明日方舟客戶端素材](https://github.com/yuanyan3060/ArknightsGameResource)
- 遊戲數據：[《明日方舟》Yostar 遊戲數據](https://github.com/ArknightsAssets/ArknightsGamedata)

### 貢獻/參與者

感謝所有參與到開發/測試中的朋友們，是大家的幫助讓 MAA 越來越好！ (\*´▽｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=105&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)

## 聲明

- 本軟體使用 [GNU Affero General Public License v3.0 only](https://spdx.org/licenses/AGPL-3.0-only.html) 開源，並附帶額外 [用戶協議](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev/terms-of-service.md)。
- 本軟體 logo 並非使用 AGPL 3.0 協議開源，[耗毛](https://weibo.com/u/3251357314)、vie 兩位畫師及軟體全體開發者保留所有權利。不得以 AGPL 3.0 協議已授權為由在未經授權的情況下使用本軟體 logo，不得在未經授權的情況下將本軟體 logo 用於任何商業用途。
- 本軟體開源、免費，僅供學習交流使用。若您遇到商家使用本軟體進行代練並收費，可能是設備與時間等費用，產生的問題及後果與本軟體無關。

### DirectML 支援說明

本軟體支援 GPU 加速功能，其在 Windows 平台上依賴於 Microsoft 提供的獨立元件 [DirectML](https://learn.microsoft.com/en-us/windows/ai/directml/)。DirectML 並非本專案的開源部分，也不受 AGPL 3.0 的約束。為方便用戶，我們隨安裝包附帶了一個未經修改的 DirectML.dll 檔案。如果您無需 GPU 加速功能，可安全刪除該 DLL 檔案，軟體的核心功能仍可正常執行。

## 廣告

用戶交流 QQ 群：[MAA 使用 & 粥遊交流 QQ 群](https://api.maa.plus/MaaAssistantArknights/api/qqgroup/index.html)  
Discord 伺服器：[Discord](https://discord.gg/23DfZ9uA4V)  
用戶交流 TG 群：[Telegram 群](https://t.me/+Mgc2Zngr-hs3ZjU1)  
自動戰鬥 JSON 作業分享：[prts.plus](https://prts.plus)  
Bilibili 直播間：[MrEO 直播間](https://live.bilibili.com/2808861) 直播敲代碼 & [MAA-Official 直播間](https://live.bilibili.com/27548877) 遊戲/雜談

技術群（舟無關、禁閒聊）：[內卷地獄！(QQ 群)](https://jq.qq.com/?_wv=1027&k=ypbzXcA2)  
開發者群：[QQ 群](https://jq.qq.com/?_wv=1027&k=JM9oCk3C)

如果覺得軟體對你有幫助，幫忙點個 Star 吧！~（網頁最上方右上角的小星星），這就是對我們最大的支持了！