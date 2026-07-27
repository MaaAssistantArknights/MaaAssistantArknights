## v6.15.0-beta.3

### Highlights

#### 作业站新格式适配

上游作业站 PRTS.plus 已于 2026 年 7 月 20 日更新作业代码格式。本版本全面适配新格式（`prts://` 前缀，自动区分单个作业与作业集），并移除已过期的 `maa://` 旧格式兼容入口，请及时更新以保证作业正常导入。

#### 库存保持任务

新增「库存保持」任务，支持在单个任务中添加多组保持计划，按从上到下的优先级将指定材料刷取至目标数量；可在任务开始前更新库存数据，并支持在 SideStory 活动与资源关卡限时全开放期间自动跳过，方便保持芯片、红票、龙门币等各类资源数量。本测试版起支持可选 AUTO 代理倍率。

#### MuMu 触控增强

支持通过 MuMu 外置渲染器进行触控输入（实验性，需 MuMu 6.3.2+）。在启用截图增强的基础上勾选触控增强后，可在模拟器后台保活场景下更稳定地完成截图与操作；触控失败时自动回退至现有触控通路。

#### 配置迁移至 gui.new.json

GUI 配置正式迁移至 `gui.new.json`，后续版本将不再使用 `gui.json`；配合 WPF 配置体系全量迁移至强类型 JSON，统一管理外部通知、性能、远程控制等设置，提升配置可靠性与可维护性。

<details>
<summary><b>English</b></summary>

#### Copilot Site New Format

The upstream copilot site PRTS.plus updated its copilot code format on July 20, 2026. This version fully adopts the new format (`prts://` prefix, automatically distinguishing single copilots from copilot sets) and removes the deprecated `maa://` legacy entry. Please update to this version to keep copilot imports working.

#### Depot Maintain Task

Added a Depot Maintain task that supports multiple plans within a single task, farming specified materials up to target quantities in top-to-bottom priority order. It can refresh depot data before the task starts and automatically skips during SideStory events and limited full-day resource stages, making it easy to maintain stocks of chips, vouchers, LMD, and other resources. This beta adds an optional AUTO series multiplier.

#### MuMu Touch Enhancement

Adds touch input via MuMu's external renderer (experimental, requires MuMu 6.3.2+). With both screenshot enhancement and touch enhancement enabled, screenshot and input remain more stable under MuMu background keep-alive; touch automatically falls back to existing input paths on failure.

#### Configuration Migration to gui.new.json

GUI configuration has been migrated to `gui.new.json`; future versions will no longer use `gui.json`. Combined with the full WPF configuration migration to strongly typed JSON (covering external notification, performance, remote control, and more), this improves reliability and maintainability.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.15.0-beta.3 (2026-07-27)</b></summary>

### 新增 | New

* 支持 MuMu 模拟器触控增强：通过外置渲染器 IPC 发送触控/按键/文本输入（需 MuMu 6.3.2+，失败时自动回退现有触控通路）；WPF 在截图增强下新增触控增强开关，同时开启截图与触控增强时可在后台保活下更稳定运行 ([#17425](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17425)) @MistEO
* 库存保持任务支持 AUTO 代理倍率（默认按 1 倍刷取；开启后按当前理智可用的最大倍率代理，单次可能超过目标库存上限） @ABA2396

### 改进 | Improved

* 任务队列支持 Ctrl+清空：按住 Ctrl 点击清空可一键移除全部任务（需确认） @status102
* 库存保持在 8/1 游戏更新后代理倍率暂未适配期间，与作战设置一致锁定倍率切换 @ABA2396
* 完善异格阿米娅（升变）干员数据生成逻辑，补充 sortIndex、子职业等字段 @status102

### 修复 | Fix

* 修复 Windows 下 `call_command` 因管道/重叠 IO 处理不当可能卡死的问题，并支持指定工作目录与超时 @MistEO
* 修复 MuMu 在 `client_type` 缺失时包名与 display id 获取失败的问题，改进查询失败时的降级逻辑 @MistEO

</details>

<details>
<summary><b>v6.15.0-beta.2 (2026-07-26)</b></summary>

### 修复 | Fix

* 修复 WPF 配置迁移引入的 `client_type` 参数序列化错误（枚举值被序列化为数字而非字符串），导致开始唤醒、战斗、关闭客户端等任务添加失败的问题 @status102
* 修复连接地址历史记录因在非 UI 线程更新集合而无法正确记录的问题 @status102

</details>

<details>
<summary><b>v6.15.0-beta.1 (2026-07-26)</b></summary>

### 新增 | New

* 新增「库存保持」任务 ([#17042](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17042)) @status102 @ABA2396
* 繁中服新增「雅賽努斯復仇記」活动关卡导航 ([#17398](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17398)) @momomochi987
* YostarJP add TA stage navigation ([#17389](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17389)) @Manicsteiner

### 改进 | Improved

* WPF 配置体系全量迁移至强类型 JSON 配置，统一管理外部通知、性能、远程控制等设置 ([#17392](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17392)) @status102 @ABA2396
* 优化 WPF 设置页设置项排序与折叠/展开状态管理，并支持从旧版本迁移设置项顺序 @status102 @ABA2396
* 自动战斗移除已过期的 `maa://` 旧格式兼容入口，全面采用作业站新格式（`prts://` 前缀，自动区分单个作业与作业集） @ABA2396
* 优化库存保持任务的显示效果 @ABA2396
* 游戏更新后临时禁用代理倍率切换，提示在游戏内手动设置 @ABA2396
* 支持 Array 移除未识别选项，避免任务队列等配置因未知类型加载失败 @ABA2396
* 繁中服更新界园主题模板，并修正进入主线章节方式 ([#17399](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17399)) @momomochi987
* YostarJP improve operator and JieGarden item OCR mappings ([#17389](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17389)) @Manicsteiner
* YostarJP fix SSS cold bomb device OCR recognition @Manicsteiner

### 修复 | Fix

* 修复设置页可搜索下拉框二次打开时导致崩溃的问题 @ABA2396
* 修复模拟器卡顿二次返回退出基建后状态丢失的问题 ([#17395](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17395)) @ABA2396
* 修复萨卡兹肉鸽进入第四层后弹出获得灵感窗口出错的问题 @Saratoga-Official
* 修复催影肉龟可能会误入二结局的问题 @Saratoga-Official
* 修复 Bark 通知图标未被输出到请求体的问题 ([#17305](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17305)) @FSPaul2498 @status102
* 升级 MaaDeps 至 v2.14.1，修复 Linux 下 MaaFwAdb 控制模块因 OpenCV 动态库版本不匹配无法加载的问题 ([#17391](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17391)) @MistEO @Aliothmoon

</details>
