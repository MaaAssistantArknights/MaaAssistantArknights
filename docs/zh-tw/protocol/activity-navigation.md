---
order: 9
icon: material-symbols:route
---

# 活動導航

預設導航直接重用客戶端已載入的任務資源：從 Tile-Pos 內部關卡 ID 取得顯示編號，例如 `YW-1` 對應 `YW-OpenOcr`，`YW-EX-1` 對應 `YW-EX-OpenOcr`。公共任務先從可返回的目前畫面恢復到終端，再執行既有 OCR 入口及章節任務，辨識準確關卡編號後交回原選關流程。EX 入口缺失時回報 `MissingTask`，不退入普通區。

入口和章節任務沿用 MaaRelease 的 `resource/tasks.json` 熱更新管道，核心本身不下載 API。新增活動或分區應在同一 JSON 提供完整 OCR 任務鏈，避免依賴必須同時下載的新截圖。公共終端恢復使用隨核心既有的基礎圖片；此功能不擴充模板分發，也不變更必需模板缺失時的檢查。既有的個別關卡模板選關維持原行為。

`ActivityNavigation` 是進入目標關卡所在地圖的獨立任務，不編隊或開始戰鬥。必填 `stage_id`，可選 `client_type`、`is_raid`，呼叫前須先連線並登入。`Copilot` 的 `copilot_list` 和 `Fight` 透過 `navigate_to_activity: true` 啟用，API 預設 false，WPF 作業清單預設開啟。Copilot 須明確提供 `client_type`，單作業使用單元素清單，與 `filename` 同時啟用會拒絕。既有導航支援的主線與資源關也可自動恢復終端入口；未知關卡保留目前地圖選關行為。Fight 的 `is_raid` 預設 false，用於活動校驗及選關後的難度切換。

`resource/activity_navigation.json` 為可選的使用者覆寫。版本 1 保留活動的 `id`、`name`、`enabled`、`clients`、`sources`、`routes`、`stages`；分區含 `task` 與附時區、精確到秒的 `opens_at`／`closes_at`，關卡含 `code`、`route`、`modes`（normal／raid）。覆寫優先於自動探索，校驗左閉右開的開放時間、客戶端與模式。預設 OCR 任務沒有這些額外資料，不會由任務名稱推測日期或客戶端限制；呼叫端繼續透過 StageActivity API 處理活動時間。有效完整覆寫替換舊值，空 `activities` 清除覆寫並恢復自動探索，錯誤資料不替換。

Copilot 保存接受時的路由值，缺失或變更時停止，相同內容重載可繼續。使用者覆寫查詢和替換透過讀寫鎖同步。回呼為 `ActivityNavigationStarted`、`ActivityNavigationCompleted`、`ActivityNavigationFailed`。預檢失敗帶 `phase: Preflight`，原因包含 `MissingTask`、`InvalidEntryTask` 與使用者策略的 `Closed`、`Disabled`、`WrongClient`、`StageCodeMismatch`、`UnsupportedMode`；排隊路由變更為 `RouteChanged`。`AsstGetActivityNavigationVersion()` 回傳協議 2（含既有任務的自動探索），呼叫前須檢查符號；舊核心可繼續使用外部 Custom 導航。
