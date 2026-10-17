---
order: 9
icon: material-symbols:route
---

# 活動導覽

`ActivityNavigation` 僅進入活動分區，不選擇具體關卡、不編隊、不開始戰鬥。參數包含必填的 `stage_id`（Tile-Pos 內部關卡 ID）、可選的 `client_type`（省略時使用目前連線的客戶端）與 `is_raid`（預設 false，僅驗證難度）。呼叫前需連線遊戲並完成登入。

`Copilot` 的 `copilot_list` 模式和 `Fight` 可用 `navigate_to_activity: true` 啟用相同入口，預設 false。Copilot 必須明確傳入 `client_type`；單份作業也使用單元素列表，與 `filename` 同時啟用會拒絕。接受列表及執行每項時都會檢查已登錄關卡。進入分區後沿用既有選關、編號複核、突襲切換、編隊與戰鬥。WPF 作業列表預設啟用，單檔模式維持原行為。

`resource/activity_navigation.json` 的 `schema_version` 為 1，`activities` 保存各活動的 `id`、`enabled`、`clients`、`routes`、`stages`，另以 `name` 與 `sources` 記錄名稱和來源。分區含標準任務名 `task` 及帶時區、精確到秒的 ISO 8601 `opens_at`／`closes_at`。每個內部關卡 ID 對應 `code`、`route` 和 `modes`（normal／raid）。開放區間含開始、不含結束；任務、編號需與 Tile-Pos 一致。已登錄但關閉、停用、客戶端不符或難度不支援時失敗，不回退；未知活動維持既有行為。有效完整映射取代上一份；空 `activities` 清空映射，無效資料不取代。

相容核心的任務放在 `resource/tasks/hotfix/activity_navigation/`。MaaResource 需放行映射和 `template/StageNavigation/`；主倉庫同步流程為相關改動更新資源版本。新增任務欄位或核心能力仍需升級核心。`AsstGetActivityNavigationVersion()` 回傳支援的協定版本（此處為 1），呼叫前需檢查符號存在；舊核心繼續使用外部 Custom 導覽。回呼為 `ActivityNavigationStarted`、`ActivityNavigationCompleted`、`ActivityNavigationFailed`，包含關卡、分區或失敗原因。
