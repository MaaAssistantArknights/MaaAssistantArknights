## v6.11.0

### Highlights

#### 任务队列交互全面升级

一键长草列表新增悬停操作按钮（复制、重命名、删除），支持一键复制任务配置；游戏内新增任务间快速导航，部分跨页面任务可通过小房子导航栏直接跳转，减少页面切换耗时。

#### 公招与理智作战功能增强

公招新增"保留指定词条"功能，识别到指定 Tag 时自动跳过当前槽位；理智作战新增目标材料最大库存模式，可参考仓库数据自动计算需刷取数量。

#### 设置页面搜索支持

设置页面新增搜索功能，支持关键词高亮与自动筛选，快速定位目标配置项。

<details>
<summary><b>English</b></summary>

#### Task Queue Interaction Overhaul

The farming list now shows hover action buttons (copy, rename, delete) for quick task management. In-game QuickSwitch navigation lets certain cross-screen tasks jump directly via the dock bar, reducing page-switching overhead.

#### Recruitment & Sanity Combat Enhancements

Recruitment now supports "preserve tags" — automatically skipping a slot when specified tags are detected. Sanity combat adds a target material inventory mode that calculates required runs based on depot data.

#### Settings Page Search

The settings page now features a search bar with keyword highlighting and section filtering for quick configuration lookup.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.11.0 (2026-06-02)</b></summary>

### 新增 | New

* 界面主题「出猎」 @SherkeyXD
* 增加怪猎联动二期 TD-6/7/8 (#16962) @ZiyinLin @ABA2396
* 支持 ImageCropper 截取 PC 窗口 (#16969) @DavidWang19
* 繁中服客户端完整支持账号切换流程，包括 GUI 启动设定显示帐号名栏位、资源补上帐号切换流程与模板、AccountSwitchTask 适配繁中服客户端 @momomochi987
* CustomWebhook 支持自定义 Headers @status102
* 新增 MarkdownDataHelper 统一公告数据读写 @ABA2396
* 一键长草列表新增任务复制、悬停操作按钮，替代原有右键菜单 (#16733) @H2O-MERO @status102 @ABA2396
* 公招支持保留指定词条，识别到指定 Tag 时自动跳过 (#16586) @ABA2396 @status102
* 理智作战支持设定目标材料最大库存，参考仓库数据自动计算刷取数量 (#16487) @ABA2396 @status102
* 游戏内新增任务间快速导航，部分跨页面任务可通过小房子导航栏直接跳转以减少切换耗时 (#16869) @ABA2396
* 触控模式下拉框新增 ToolTip 视频演示 (#16812) @ABA2396

### 改进 | Improved

* 优化信用商店复核逻辑 (#16932) @ABA2396
* 水月萨卡兹肉鸽关闭 Next 弹窗 (#16918) @status102
* 临期药下拉框显示 24h x n @status102
* 修改描述文本「过期关卡重置为」->「过期活动关卡重置为」以更加符合当前重置范围 @status102
* 悖论模拟作业列表变量名与实际不符 @status102
* 调整吐司通知禁用说明 @ABA2396
* 设置页面新增搜索支持，支持关键词高亮与自动筛选 (#16833) @H2O-MERO @ABA2396
* 优化配置转换器嵌套逻辑 @ABA2396
* 启动时检测 WinRT Toast 通知可用性，不可用时在日志中显示具体原因 (#16877) @status102 @momomochi987
* 统一使用强类型 GetValue 重载替换 Convert.To 调用 (#16866) @ABA2396
* 优化任务队列选中状态的视觉表现，突出当前选中项 @status102
* 任务队列 CheckBox 与添加任务按钮对齐 @status102

### 修复 | Fix

* 御龙装置识别 @Saratoga-Official
* 优化追加自定干员弹窗删除时的布局 (#16921) @ZiyinLin
* 修复宿舍自定义干员 + 信赖补位的 bug 及外服同步修复 (#16659) @ZiyinLin @Constrat @Roland125
* 回退 EN 服 OCR 变更以恢复识别正确性 @Constrat
* 修复 adb-lite 在连接阶段未正确绑定当前设备，减少切换设备或并发执行命令时的连接异常 (#16853) @wangl-cc
* 修复快捷切换在开启基建退出提醒时无法退出基建的问题 @ABA2396
* 修复快捷切换在遇到 LoadingText 时无法跳出循环的问题 @ABA2396
* 修复通过小房子导航后无法切换基建设施的问题 @ABA2396
* 修复繁中服界园肉鸽无法放弃探索的问题 (#16887) @momomochi987
* 修复界园肉鸽"见钱问柳"事件选择逻辑 @Saratoga-Official
* 修复界园司岁台分队招募券连续 NEXT 弹窗无法关闭的问题 (#16806) @ZiyinLin @status102
* 修复 ROI 高度越界及多次坐标校正后返回全图区域的问题 @status102
* 修复 Release 构建下日志宏输出多余作用域信息的问题 @status102
* 修复调试图清理时异常导致 Core SIGABRT 的问题 (#16233) @FireflySentinel
* 修复 POSIX 平台 fork 后子进程 exec 失败时缺少 _exit 的问题 @FireflySentinel
* 更新检查失败时优先尝试备用 API 而非直接使用过期缓存 (#16873) @glimmertouch
* 降低 macOS 平台 RA4、RA15 二倍速识别阈值 (#16860) @ColdSpellhere

### 文档 | Docs

* 帐号切换手册与整合协议补上繁中服说明 @momomochi987
* 更新帐号切换小提示，加入繁中服 Email 帐号说明 @momomochi987
* 补全理智药过期天数可配置参数的相关文档 @ABA2396
* 公招保留词条参数术语从 skip_tags 更新为 preserve_tags @ABA2396

### 其他 | Other

* update MaaDeps version to v2.11.0 (#16961) @Aliothmoon
* 繁中服「天想」主题 UI 模板 (#16893) @momomochi987
* CI GitHub Action 固定到 commit SHA 以加固安全性 (#16502) @FireflySentinel

</details>
