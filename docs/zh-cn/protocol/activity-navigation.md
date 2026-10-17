---
order: 9
icon: material-symbols:route
---

# 活动导航

默认导航直接复用客户端已经加载的任务资源：从 Tile-Pos 内部关卡 ID 取得显示编号，如 `YW-1` 对应 `YW-OpenOcr`，`YW-EX-1` 对应 `YW-EX-OpenOcr`。公共任务先从可返回的当前界面恢复到终端，再执行已有 OCR 入口及章节任务，看到准确关卡编号后交回原选关流程。EX 入口缺失时报告 `MissingTask`，不会退入普通区。

入口和章节任务沿用 MaaRelease 的 `resource/tasks.json` 热更新渠道；核心本身不下载 API。新增活动或分区应在同一 JSON 中提供完整 OCR 任务链，避免引入需要同时下载的新截图。公共终端恢复使用随核心已有的基础图片；本功能不扩充模板白名单，也不改变必需模板缺失时的检查。已存在的具体关卡模板选关保持原行为。

`ActivityNavigation` 是进入目标关卡所在地图的独立任务，不编队或开始战斗。必填 `stage_id`，可选 `client_type`、`is_raid`。调用方须先连接并登录。`Copilot` 的 `copilot_list` 和 `Fight` 通过 `navigate_to_activity: true` 启用，API 默认 false，WPF 作业列表默认开启。Copilot 启用时须显式提供 `client_type`；单作业使用单元素列表，与 `filename` 同时启用会拒绝。已有导航支持的主线及资源关也可自动恢复终端入口；没有可用导航的未知关卡保留原有当前地图选关行为。Fight 的 `is_raid` 默认 false，用于活动导航及选关后的难度切换。

`resource/activity_navigation.json` 改为可选的用户覆盖。`schema_version: 1`，`activities` 中保留 `id`、`name`、`enabled`、`clients`、`sources`、`routes`、`stages`；分区含 `task`、带时区且精确到秒的 `opens_at`／`closes_at`，关卡含 `code`、`route`、`modes`（normal／raid）。覆盖优先于自动发现，并按左闭右开区间校验开放时间、客户端和模式。默认 OCR 任务没有这些额外元数据，不会从任务名推断日期或客户端限制；调用方继续使用现有 StageActivity API 处理活动时间。有效整份覆盖替换上一份，空 `activities` 清除覆盖并恢复自动发现，错误覆盖不替换。

Copilot 保存接受时的路由值，缺失或变化时停止；相同内容重载允许继续。用户覆盖查询与替换由读写锁同步。回调为 `ActivityNavigationStarted`、`ActivityNavigationCompleted`、`ActivityNavigationFailed`。预检失败含 `phase: Preflight`，原因包括 `MissingTask`、`InvalidEntryTask` 及用户策略对应的 `Closed`、`Disabled`、`WrongClient`、`StageCodeMismatch`、`UnsupportedMode`；排队路由变化为 `RouteChanged`。`AsstGetActivityNavigationVersion()` 返回协议 2（含基于现有任务的自动发现），调用前须检查符号；旧核心可继续使用外部 Custom 导航。
