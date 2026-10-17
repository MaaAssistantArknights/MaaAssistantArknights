---
order: 9
icon: material-symbols:route
---

# 活动导航

`ActivityNavigation` 是只进入活动分区的独立任务，不选择具体关卡、不编队、不开始战斗。参数为必填 `stage_id`（Tile-Pos 内部关卡 ID）、可选 `client_type`（未提供时使用当前连接的客户端）与 `is_raid`（默认 false，仅校验难度）。调用方仍需先连接游戏并完成登录。

`Copilot` 的 `copilot_list` 模式和 `Fight` 可通过 `navigate_to_activity: true` 启用同一入口，默认 false。Copilot 启用时必须显式传入 `client_type`；单作业也应使用一个元素的 `copilot_list`，与 `filename` 同时启用会拒绝。列表在接受任务时预检查已登记关卡，执行每项时再次检查；先进入正确分区，再沿用原生选关、编号复核、突袭切换、编队和战斗。WPF 的作业列表默认启用，单文件模式保持原行为。

映射位于 `resource/activity_navigation.json`：`schema_version` 为 1，`activities` 按活动保存 `id`、`enabled`、`clients`、`routes`、`stages`，`name` 与 `sources` 记录名称及来源。每个分区含标准任务名 `task`、带时区且精确到秒的 ISO 8601 `opens_at`／`closes_at`；每个内部关卡 ID 对应 `code`、`route` 和 `modes`（normal／raid）。开放区间为左闭右开。ID、显示编号、任务与 Tile-Pos 必须一致；已登记但关闭、禁用、客户端不符或难度不支持时失败，不回退。未知活动保持原有选关行为。有效完整映射替换上一份；空 `activities` 可清空映射，错误配置不替换。

兼容核心的活动任务放入 `resource/tasks/hotfix/activity_navigation/`。MaaResource 需同时放行映射文件和 `template/StageNavigation/`，主仓库同步工作流为这些改动更新资源版本。新增任务字段或核心能力仍需升级核心；图片和标准任务更新不能绕过核心兼容要求。`AsstGetActivityNavigationVersion()` 返回支持的协议版本（本实现为 1）；调用方应检查符号存在再调用，旧核心继续使用外部 Custom 导航。导航回调为 `ActivityNavigationStarted`、`ActivityNavigationCompleted`、`ActivityNavigationFailed`，含关卡、分区或失败原因。
