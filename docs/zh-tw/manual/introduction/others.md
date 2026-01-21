---
order: 11
icon: icon-park-solid:other
---

# 其他

## GPU 加速推論

使用 DirectML 呼叫 GPU 進行辨識推論加速 <sup>[PR](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9236)</sup>，可透過少量的 GPU 佔用降低大量的 CPU 佔用，推薦啟用。

經測試，部分顯示卡因缺少功能或效能較低，在使用本功能時會出現辨識問題。MAA 已內建了部分 GPU 黑名單 <sup>[PR1](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9990)[PR2](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12134)</sup>，若列表外的顯示卡在啟用本功能後也出現辨識問題，請發 Issue。

## 僅一次

主介面和設定中的配置變更通常會自動儲存，但以下幾種會在 MAA 重啟後重置。

- 標有 `*` 號的選項
- 標有 `（僅一次）` 的選項
- 透過右鍵點選核取方塊得到的半選開關

## 啟動時自動切換配置

MAA 支援透過啟動參數自動切換配置，在 MAA 程序名稱後附加 `--config <配置名稱>` 即可。範例：`./MAA/MAA.exe --config 官服`。

部分符號需要轉義，參考 JSON。範例：在配置名稱為 `"官服"` 時，參數應為 `--config \"官服\"`。

## 開始前 / 結束後腳本

v4.13.0 後支援設定開始前 / 結束後腳本，可在任務前後自動執行批次檔。輸入框內需填寫批次檔即 `*.bat` 的絕對或相對路徑。

## 配置遷移

在 Windows 版本中，MAA 的所有配置都存放於 `config` 資料夾中的 `gui.json` 裡。遷移此資料夾即可遷移 MAA 的所有設定。

## 其他說明

- 首頁左側任務可以拖曳改變順序，基建換班設定中設施順序同理。
- 所有點選操作，皆為點選按鈕內隨機位置，並模擬帕松分佈（Poisson distribution，按鈕中心的點選機率最高，距離中心越遠，點選機率越低）。
- 底層演算法純 C++ 開發，並設計了多重快取技術，最大限度降低 CPU 和記憶體佔用。
- 軟體支援自動更新 ✿✿ ヽ(°▽°)ノ ✿ ，推薦非槓精的同學使用公測版，一般來說更新快且 Bug 少。（什麼 MIUI (╯‵□′)╯︵┻━┻
- 如果新版本自動下載失敗，可手動下載 OTA 壓縮檔後直接放到 MAA 目錄下，會自動更新。
