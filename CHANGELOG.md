## v6.19.0-beta.1

### Highlights

#### 新增 ｢干员培养｣ 任务（测试功能）

为干员设定精英化、技能等级与专精目标后，任务将自动逐步完成培养流程，材料即缺即合自动合成。该功能为测试功能，可能存在较多未发现的 bug，遇到问题欢迎通过 ｢设置 - 问题反馈｣ 反馈。

#### 从指定任务处继续运行

任务列表新增 ｢从此处运行｣ 右键菜单项，可从指定任务处开始执行任务队列，跳过其之前的任务，任务失败后无需从头重跑；指定任务未启用时从其后首个已启用的任务开始。

#### 任务栏进度按 Core 任务链计数

任务栏进度条改为按 Core 任务链计数，执行一轮任务时按本轮产生的任务链逐条推进，更准确反映整体执行进度。

#### 界面显示干员头像

干员识别、自动战斗作业预览与肉鸽开局设置等界面新增干员头像与职业图标显示，干员识别卡新增 ｢头像展示｣ 样式选项。

<details>
<summary><b>English</b></summary>

#### New Task: Operator Progression (Testing)

Set elite promotion, skill level, and mastery targets for an operator, and the task completes the training flow step by step with on-demand material synthesis. This feature is still in testing and may contain undiscovered bugs; feedback is welcome via Settings - Issue Report.

#### Resume the Queue from a Selected Task

A "Run from Here" option is added to the task list context menu, letting you start the queue from any task and skip the ones before it, so a failed run no longer has to restart from scratch. If the selected task is disabled, the queue starts from the first enabled task after it.

#### Taskbar Progress by Core Task Chains

The taskbar progress bar now counts Core task chains: within a run, progress advances as each chain completes, giving a more accurate view of overall execution.

#### Operator Avatars in the UI

Operator avatars and class icons are now shown in the operator recognition view, copilot formation preview, and roguelike start-up settings; the recognition card gains an "Avatar View" style option.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.19.0-beta.1 (2026-09-28)</b></summary>

### 新增 | New

* 新增 ｢干员培养｣ 任务：为干员设定精英化、技能等级与专精目标后自动逐步完成培养流程，材料即缺即合自动合成，训练室专精自动选择协助者 ([#18137](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18137)) @Lancarus @youzibigg @HX3N @Constrat @status102 @Manicsteiner @momomochi987
* 任务列表支持右键 ｢从此处运行｣ ，从指定任务处开始执行任务队列，跳过其之前的任务；指定任务未启用时从其后首个已启用的任务开始 ([#18317](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18317)) @H2O-MERO
* 干员识别、自动战斗作业预览与肉鸽开局设置等界面新增干员头像与职业图标显示，干员识别卡新增 ｢头像展示｣ 样式选项 ([#18352](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18352)) @Lancarus @youzibigg @ABA2396
* 干员名、关卡名等 OCR 纠错规则拆分至独立热修复资源，此类修复随资源更新即时分发，无需等待完整版本更新 ([#18406](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18406)) @ABA2396
* 完成后动作新增 ｢锁屏｣（Windows），与休眠、关机、睡眠选项互斥 ([#18412](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18412)) @H2O-MERO
* 牛杂 ｢活动商店｣ 新增商品黑名单，可按商品名称或物品 ID 排除自动购买，选项列表显示材料图标 ([#18270](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18270)) @H2O-MERO @ABA2396
* PC 端连接设置新增 ｢最小化游戏窗口｣ 选项（连接设置 - Win32 附加选项），修复任务运行过程中无法最小化游戏窗口的问题 ([#18419](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18419)) @H2O-MERO

### 改进 | Improved

* 优化黑流树海肉鸽事件选择，按事件策略规则选择选项，界面日志显示实际选择的选项与策略理由 ([#18339](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18339)) @ZiyinLin
* 黑流树海肉鸽新增地图拓扑模板识别，节点预览揭示节点实际身份后立即重新规划路线 ([#18368](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18368)) @ZiyinLin
* 任务栏进度改为按 Core 任务链计数 ([#18338](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18338)) @ABA2396
* 库存保持列表启用虚拟化、像素滚动与惯性滚动，操作按钮区固定显示 @ABA2396
* 理智作战与库存保持的指定材料下拉、肉鸽开局干员下拉在收起状态显示已选物品图标或干员头像 @ABA2396
* PC 端跳过掉落上报的提示改为指明具体三方平台，避免重复输出相同内容 ([#18311](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18311)) @H2O-MERO
* PC 端截图测试结束后恢复游戏窗口位置 ([#18427](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18427)) @H2O-MERO
* Rename the roguelike start-up item option from "Thought" to "Plan" in the English UI @Constrat

### 修复 | Fix

* 修复黑流树海肉鸽 ｢失与得｣ ｢先行一步｣ 等事件卡住或报任务出错的问题 ([#18339](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18339)) @ZiyinLin
* 修复黑流树海肉鸽首次进层时楼层名识别失败导致地图重建缺少楼层记录的问题，现在优先在缩放后的画面上识别，持续失败时报错停止 ([#18390](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18390)) @ZiyinLin
* 修复基建常规换班将本轮已安排的干员重复分配到其他设施的问题 ([#18346](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18346)) @youzibigg
* 修复任务出错跳过完成后动作时日志重复输出出错任务名的问题 @status102
* 修复基建换班评分误将 ｢泰拉的方舟｣ （人力办公室技能）按加工站技能评分的问题 @Lancarus
* 修正基建训练室 ｢专精完成｣ 的识别模板与识别区域 @Lancarus
* 修正黑流树海肉鸽 ｢秘境行商｣ 事件名称的错别字（必境→秘境） ([#18420](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18420)) @youzibigg
* 修复界园肉鸽铜币交换完成后偶发识别失败导致任务异常的问题 ([#18423](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18423)) @Manicsteiner
* 补齐六个肉鸽主题中升变阿米娅术师与近卫形态的职业配置，修复使用对应形态时的配置缺失异常 ([#18414](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18414)) @ZiyinLin
* 修复生息演算 RA4 完成识别在部分分辨率下失败的问题 @Saratoga-Official
* 我是小猪（修正识别替换规则中半角括号的正则转义） @Saratoga-Official
* 修复 PC 端任务结束后游戏静音未正常恢复的问题 ([#18411](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18411)) @H2O-MERO
* 修复主窗口标题栏文本滚动启用后高度未恢复为单行的问题 ([#18422](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18422)) @H2O-MERO
* YostarJP add the StageEncounterOptionUnknown template for the Sami roguelike ([#18277](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18277)) @LinYanxi0
* YostarKR adjust the notification dot roi of AutoRaisePotential @HX3N

### 文档 | Docs

* 集成协议文档补充自动提升潜能任务（AutoRaisePotential）的参数说明 @ABA2396

### 其他 | Other

* MaaWpfGui 开启 TreatWarningsAsErrors 并清理 StyleCop 抑制 ([#18340](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18340)) @ABA2396
* CI 升级 pnpm/setup 至 v3 ([#18358](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18358)) @dependabot[bot]
* 资源同步脚本支持 hotfix 变更自动 bump version.json 与单跳同步 @ABA2396
* 统一 XAML 格式化并约定 XAML Styler 工具链 @ABA2396
* 更新开发规范：WPF 配置存储不保留原始 Json、MaaCore 不手动拼接业务任务流 @status102
* 统一计划列表注释与绑定属性格式 @ABA2396
* 新增 ItemImageConverter 物品图标转换器 @ABA2396
* 补充与调整界面层代码注释 @ABA2396
* 版权年份更新至 2026 @ABA2396

</details>
