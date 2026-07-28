## v6.15.0-beta.3

### Highlights

#### 作业站新格式适配

上游作业站 PRTS.plus 已于 2026 年 7 月 20 日更新作业代码格式。本版本全面适配新格式（`prts://` 前缀，自动区分单个作业与作业集），并移除已过期的 `maa://` 旧格式兼容入口，请及时更新以保证作业正常导入。

#### 库存保持任务

新增「库存保持」任务，支持在单个任务中添加多组保持计划，按从上到下的优先级将指定材料刷取至目标数量；可在任务开始前更新库存数据，并支持在 SideStory 活动与资源关卡限时全开放期间自动跳过，方便保持芯片、红票、龙门币等各类资源数量。本版本起亦支持 AUTO 代理倍率。

#### MuMu 模拟器触控增强

新增 MuMu 触控增强模式（实验性）：在启用截图增强的基础上可在触控模式中直接选择「MuMu 触控增强」，通过 MuMu IPC 直连输入并支持后台保活稳定运行；连接后自动检测触控是否实际生效，不支持时停止任务并提示用户。需 MuMu 模拟器 6.3.2 及以上。

<details>
<summary><b>English</b></summary>

#### Copilot Site New Format

The upstream copilot site PRTS.plus updated its copilot code format on July 20, 2026. This version fully adopts the new format (`prts://` prefix, automatically distinguishing single copilots from copilot sets) and removes the deprecated `maa://` legacy entry. Please update to this version to keep copilot imports working.

#### Depot Maintain Task

Added a Depot Maintain task that supports multiple plans within a single task, farming specified materials up to target quantities in top-to-bottom priority order. It can refresh depot data before the task starts and automatically skips during SideStory events and limited full-day resource stages, making it easy to maintain stocks of chips, vouchers, LMD, and other resources. This version also adds AUTO series multiplier support.

#### MuMu Emulator Touch Enhancement

Added experimental MuMu touch enhancement mode: with screenshot extras enabled, you can directly select "MuMu Touch" in the touch mode dropdown for stable background keep-alive operation via MuMu IPC, with automatic detection of touch availability that stops the task and notifies the user if unsupported. Requires MuMu emulator 6.3.2 or later.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.15.0-beta.3 (2026-07-28)</b></summary>

### 新增 | New

* 新增 MuMu 触控增强模式（实验性）：在截图增强启用时可在触控模式下直接选择「MuMu 触控增强」，通过 MuMu IPC 直连输入并支持后台保活稳定运行 ([#17425](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17425)) @MistEO @ABA2396
* 库存保持任务支持使用 AUTO 代理倍率，并在游戏更新后倍战锁定期间同步禁用该选项 @ABA2396
* 任务队列支持按住 Ctrl 点击清空以移除全部任务 @status102

### 改进 | Improved

* 增强跨平台 `call_command` 实现（工作目录、超时与管道/进程清理），避免子进程输出读取卡死 @MistEO
* 完善 MuMu Extras 包名与 display id 解析，缺失客户端类型时回退至有效默认值 @MistEO
* 优化连接设置页面布局，缩减各控件间距并调整对齐方式 @ABA2396
* 当 MuMu 后台保活开启但触控增强未实际生效时，直接停止任务并提示用户，避免进入无法操作的后台状态 @ABA2396

### 修复 | Fix

* 修复外服游戏客户端类型从旧配置迁移失败的问题 @status102
* 修复删除多余连接地址时可能抛出 Index was out of range 异常的问题 ([#17441](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17441)) @ABA2396

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
