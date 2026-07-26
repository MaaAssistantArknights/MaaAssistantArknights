## v6.15.0-beta.1

### Highlights

#### 库存保持任务

新增「库存保持」任务，可配置多组计划按目标库存自动补刷材料，支持任务开始前更新库存，以及在活动期间与资源收集限时全天开放期间跳过。

#### WPF 配置体系全量迁移

将性能、外部通知、远程控制等设置统一迁移至强类型 JSON 配置，提升配置可靠性与可维护性。

#### 基建宿舍与代理倍率适配

修复模拟器卡顿导致二次返回后宿舍筛选/排序状态丢失的问题；代理倍率选项扩展至 10 倍，国服/B 服在适配完成前临时锁定为「不切换」。

#### 多服活动与识别更新

繁中服新增「雅賽努斯復仇記」关卡导航并更新界园主题；YostarJP 新增 TA 关卡导航并改进 OCR 映射。

<details>
<summary><b>English</b></summary>

#### Depot Maintain Task

Added a Depot Maintain task that can plan fights against target inventory levels, with optional depot refresh before start and skip options during events or full-day resource collection periods.

#### Full WPF Configuration Migration

Migrated performance, external notification, remote control, and related settings to strongly typed JSON configuration for better reliability and maintainability.

#### Base Dorm and Series Multiplier Compatibility

Fixed dorm filter/sort state loss after emulator lag caused a double-back exit from the base. Series multiplier options now go up to 10×; Official/Bilibili clients temporarily lock series switching until full adaptation is ready.

#### Multi-server Event and Recognition Updates

Traditional Chinese server adds stage navigation for 「雅賽努斯復仇記」 and updates JieGarden themes. YostarJP adds TA stage navigation and improves OCR mappings.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.15.0-beta.1 (2026-07-26)</b></summary>

### 新增 | New

* 新增「库存保持」任务，可配置多组计划按目标库存自动补刷材料，支持任务开始前更新库存数据，以及活动期间与资源收集限时全天开放期间跳过 ([#17042](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17042)) @status102 @ABA2396
* 繁中服新增「雅賽努斯復仇記」活动关卡导航 ([#17398](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17398)) @momomochi987
* YostarJP add TA stage navigation ([#17389](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17389)) @Manicsteiner

### 改进 | Improved

* WPF 配置体系全量迁移至强类型 JSON 配置，统一管理外部通知、性能、远程控制等设置 ([#17392](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17392)) @status102
* 代理倍率选项扩展至 10 倍；国服/B 服在游戏更新适配完成前临时将代理倍率锁定为「不切换」，并提示在游戏内手动设置 @ABA2396
* 配置反序列化支持从数组中移除无法识别的选项，避免任务队列等配置因未知类型导致加载失败 @ABA2396
* 繁中服更新界园主题模板，并修正进入主线章节方式 ([#17399](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17399)) @momomochi987
* YostarJP improve operator and JieGarden item OCR mappings ([#17389](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17389)) @Manicsteiner

### 修复 | Fix

* 修复模拟器卡顿导致二次返回退出基建后宿舍筛选/排序状态丢失，进而把已进驻干员误选入宿舍的问题 ([#17395](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17395)) @ABA2396
* 修复傀影肉鸽「解脱」事件可能误入二结局的问题 @Saratoga-Official
* 修复 Bark 通知图标未正确写入请求体的问题 ([#17305](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17305)) @FSPaul2498 @status102
* 修复 Linux 下 MaaFwAdb 控制模块因 OpenCV 动态库版本不匹配无法加载的问题（升级 MaaDeps 至 v2.14.1）([#17391](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17391)) @MistEO

</details>
