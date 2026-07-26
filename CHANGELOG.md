## v6.15.0-beta.1

### Highlights

#### 作业站新格式适配

上游作业站 PRTS.plus 已于 2026 年 7 月 20 日更新作业代码格式。本版本全面适配新格式（`prts://` 前缀，自动区分单个作业与作业集），并移除已过期的 `maa://` 旧格式兼容入口，请及时更新以保证作业正常导入。

#### 库存保持任务

新增「库存保持」任务，支持在单个任务中添加多组保持计划，按从上到下的优先级将指定材料刷取至目标数量；可在任务开始前更新库存数据，并支持在 SideStory 活动与资源关卡限时全开放期间自动跳过，方便保持芯片、红票、龙门币等各类资源数量。

#### 配置迁移至 gui.new.json

GUI 配置正式迁移至 `gui.new.json`，后续版本将不再使用 `gui.json`；配合 WPF 配置体系全量迁移至强类型 JSON，统一管理外部通知、性能、远程控制等设置，提升配置可靠性与可维护性。

<details>
<summary><b>English</b></summary>

#### Copilot Site New Format

The upstream copilot site PRTS.plus updated its copilot code format on July 20, 2026. This version fully adopts the new format (`prts://` prefix, automatically distinguishing single copilots from copilot sets) and removes the deprecated `maa://` legacy entry. Please update to this version to keep copilot imports working.

#### Depot Maintain Task

Added a Depot Maintain task that supports multiple plans within a single task, farming specified materials up to target quantities in top-to-bottom priority order. It can refresh depot data before the task starts and automatically skips during SideStory events and limited full-day resource stages, making it easy to maintain stocks of chips, vouchers, LMD, and other resources.

#### Configuration Migration to gui.new.json

GUI configuration has been migrated to `gui.new.json`; future versions will no longer use `gui.json`. Combined with the full WPF configuration migration to strongly typed JSON (covering external notification, performance, remote control, and more), this improves reliability and maintainability.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.15.0-beta.1 (2026-07-26)</b></summary>

### 新增 | New

* 新增「库存保持」任务 ([#17042](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17042)) @status102 @ABA2396
* 繁中服新增「雅賽努斯復仇記」活动关卡导航 ([#17398](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17398)) @momomochi987
* YostarJP add TA stage navigation ([#17389](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17389)) @Manicsteiner

### 改进 | Improved

* WPF 配置体系全量迁移至强类型 JSON 配置，统一管理外部通知、性能、远程控制等设置 ([#17392](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17392)) @status102 @ABA2396
* 自动战斗移除已过期的 `maa://` 旧格式兼容入口，全面采用作业站新格式（`prts://` 前缀，自动区分单个作业与作业集） @ABA2396
* 优化库存保持任务的显示效果 @ABA2396
* 游戏更新后临时禁用代理倍率切换，提示在游戏内手动设置 @ABA2396
* 支持 Array 移除未识别选项，避免任务队列等配置因未知类型加载失败 @ABA2396
* 繁中服更新界园主题模板，并修正进入主线章节方式 ([#17399](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17399)) @momomochi987
* YostarJP improve operator and JieGarden item OCR mappings ([#17389](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17389)) @Manicsteiner

### 修复 | Fix

* 修复设置页可搜索下拉框二次打开时导致崩溃的问题 @ABA2396
* 修复模拟器卡顿二次返回退出基建后状态丢失的问题 ([#17395](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17395)) @ABA2396
* 修复催影肉龟可能会误入二结局的问题 @Saratoga-Official
* 修复 Bark 通知图标未被输出到请求体的问题 ([#17305](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17305)) @FSPaul2498 @status102
* 升级 MaaDeps 至 v2.14.1，修复 Linux 下 MaaFwAdb 控制模块因 OpenCV 动态库版本不匹配无法加载的问题 ([#17391](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17391)) @MistEO @Aliothmoon

</details>
