## v6.21.2

### Highlights

已适配明日方舟 26.11.1 版本的游戏新界面，修复新版主界面下肉鸽与生息演算入口的识别问题。后续不做日间主题以外的主题适配；如需使用其他主题，可在任务列表末尾自行添加 ｢更换主题｣ 任务。

本版本按功能域全量重写了 WPF 语言资源，个别界面可能出现文案显示异常；如遇到问题，请通过 ｢设置 - 问题反馈｣ 提交反馈。

#### 设置指引改为任务设置演示教学，新增用户协议确认

设置指引 ｢任务设置｣ 步骤改为分步演示教学，按引导顺序完成任务配置；新增用户协议确认步骤，已完成旧版指引的用户升级后重新进入指引。

#### 生息演算迁移到 ｢小工具-牛杂｣，外服支持 ｢重启锚点｣

生息演算从任务队列迁移到 ｢小工具-牛杂｣ 独立运行，任务队列中已有的条目与设置升级后自动迁入。YostarEN/JP/KR 新增支持 ｢重启锚点｣ 主题。

#### 黑流树海刷开局支持凹第一层实托邦

刷开局新增 ｢凹第一层实托邦｣ 选项，需难度 6 及以上。

#### 编队与选人提速

自动编队优先在当前页匹配待选干员，减少翻页；自定义基建与宿舍选人按职业分桶定位剩余目标干员。

<details>
<summary><b>English</b></summary>

Adapted to the new UI in the Arknights 26.11.1 update, and fixed Roguelike and Reclamation Algorithm entry recognition on the new main screen. No themes other than Day will be adapted going forward; to use a different theme, add a ｢Switch Theme｣ task at the end of the task list.

The WPF localization resources have been extensively rewritten by feature domain in this version; a few texts may display incorrectly. If you run into any, please report via Settings - Issue Report.

#### First-Run Guide Revamped as Task Settings Demo Tutorial with User Agreement Confirmation

The ｢Task Settings｣ step of the first-run guide is now a step-by-step demo tutorial that walks through task configuration in a guided order; a user agreement confirmation step has been added, and users who completed the old guide will go through the new one again after upgrading.

#### Reclamation Algorithm Moved to Toolbox "Useful Tasks", "RELAUNCH ANCHOR" on Overseas Servers

Reclamation Algorithm now runs standalone in the Toolbox "Useful Tasks" section; existing entries in the task queue are migrated automatically with their settings preserved. YostarEN/JP/KR now support the "RELAUNCH ANCHOR" theme.

#### BlackFlow Start Run: Reroll Until Floor 1 is 实托邦

New option for start-run farming to reroll until Floor 1 is 实托邦 (a Floor 1 variant); requires difficulty 6 or above.

#### Faster Formation and Operator Selection

Auto formation now matches operators on the current page first, reducing paging; custom infrast and dormitory selection locate remaining targets by class.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.21.2 (2026-11-03)</b></summary>

### 改进 | Improved

* 设置指引 ｢任务设置｣ 步骤改为分步演示教学，按引导顺序完成添加任务、选择关卡、重命名排序、周计划与更换主题配置；新增用户协议确认步骤，已完成旧版指引的用户升级后重新进入指引 ([#19050](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19050)) @ABA2396
* 补全 ｢战时新闻委员会｣ 流程，当日已完成时自动进入下一天，五天完成后自动结束 ([#19039](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19039)) @Daydreamer114
* 更换主题确认切换后延长等待时间，点击未生效时自动重试 @ABA2396 @XXLC

### 修复 | Fix

* 修复复用上次编队时干员与编队组映射错位、编队干员未被正确恢复的问题 ([#19014](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19014)) @status102
* 修复 26.11.1 新版主界面下肉鸽与生息演算入口识别失败的问题 ([#19010](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19010)) @moranfanhua @Saratoga-Official
* 修复黑流树海换层时地图缩放未生效、进层弹窗遮挡导致的层名识别失败问题；识别到误入奇境的标题时重开本局 ([#19052](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19052)) @ZiyinLin
* 修复萨卡兹肉鸽 ｢已深化｣ 与 ｢可重建｣ 状态节点不显示节点日志的问题 @Constrat
* YostarJP fix Relaunch Anchor RA-1 wood delivery OCR recognition ([#19028](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19028)) @Radekyspec
* YostarJP fix Reclamation Algorithm 3 deliver wood OCR regex @Daydreamer114
* YostarEN fix depot entry icon recognition ([#19005](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19005)) @Daydreamer114 @Constrat
* YostarEN fix SanityMatch misreading sanity as 7/1210 @Constrat

### 文档 | Docs

* 更换 README 的 Ask DeepWiki 徽章 @Constrat

### 其他 | Other

* CI 规范化各工作流的 job 名称 @Rbqwow
* CI 更新 release-ota 的 macOS 镜像版本 @Rbqwow
* CI 仅允许 base 为 dev-v2 的 PR 触发 CI 工作流 @Rbqwow

</details>

<details>
<summary><b>v6.21.1 (2026-11-02)</b></summary>

### 新增 | New

* 小工具-牛杂 新增 ｢战时新闻委员会｣ 小游戏 ([#18991](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18991)) @Daydreamer114 @ABA2396
* 新增 ｢时间啊，向前！｣ TL-4 搓玉关卡导航 ([#18994](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18994)) @jinghero

### 改进 | Improved

* 适配 ｢时间啊，向前！｣ 通关后切换形态的活动界面 @ABA2396

### 修复 | Fix

* 修复主题切换按钮模板误缩放导致外服主题切换识别失败的问题 @ABA2396
* YostarEN/JP/KR fix Reclamation Algorithm 3 Deliver Wood OCR regex ([#18996](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18996)) @Daydreamer114
* YostarEN fix dormitory icon recognition at 720p @Constrat

### 其他 | Other

* CI 升级 actions/setup-python 至 v7 ([#18947](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18947))

</details>

<details>
<summary><b>v6.21.0 (2026-11-01)</b></summary>

### 新增 | New

* 生息演算迁移到 ｢小工具-牛杂｣ 独立运行，任务队列中已有的条目与设置自动迁入 ([#18809](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18809)) @ABA2396
* 黑流树海刷开局新增 ｢凹第一层实托邦｣ 选项，需难度 6 及以上 ([#18840](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18840)) @ZiyinLin @ABA2396
* 黑流树海支持 ｢在第五层 BOSS 前暂停｣ ([#18913](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18913)) @ZiyinLin
* 黑流树海开局支援页按所选策略的优先级顺序选取奖励 ([#18837](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18837)) @ZiyinLin
* 基建收获时识别回收站产出的物品与数量并显示在日志中，鼠标悬停日志可查看物品图标 ([#18924](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18924)) @ABA2396
* 菲亚梅塔心情恢复的恢复目标新增琴柳、承曦格雷伊、温蒂、槐琥、至简、旅骨 ([#18688](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18688)) @Reverse0xCC @ABA2396
* 库存保持新增 ｢芯片组计入双芯片折算｣ 选项，统计芯片组时计入双芯片的折算数量 @ABA2396
* Android 组件附带 NCNN 格式的 OCR 模型，下游可直接使用 ncnn 后端识别 ([#18738](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18738)) @wangl-cc
* 新增 ｢时间啊，向前！｣ 活动的关卡导航 @ABA2396
* YostarEN/JP/KR support Reclamation Algorithm 3 ([#18929](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18929), [#18910](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18910), [#18911](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18911), [#18951](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18951), [#18936](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18936)) @dragonheart107 @Manicsteiner @HX3N

### 改进 | Improved

* 自动编队优先在当前页匹配待选干员，减少翻页，编队更快 @ABA2396
* 自定义基建与宿舍选人按职业分桶定位剩余目标干员，选人更快 @ABA2396 @youzibigg
* 今日关卡小提示新增 PR 芯片本的双芯片库存显示，掉落名称与库存数按掉落着色 ([#16193](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16193)) @soundofautumn @ABA2396
* gui.log 记录 MessageBox 与 Growl 通知的弹出内容及用户选择，新公告只记录带 (NEW!!!) 标记的区块 ([#18915](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18915), [#18916](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18916)) @ABA2396
* 肉鸽与生息演算的主题下拉框中，当前客户端未适配的主题显示 ｢未适配｣ 后缀 ([#18928](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18928)) @ABA2396 @Constrat
* 库存刷取与库存保持在无库存数据时跳过刷取并提示更新库存，识别结果为空时保留已有库存数据 ([#18850](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18850)) @ABA2396
* 基建训练室日志附带当时的界面截图 @ABA2396
* 基建进入设施时可预期的失败（设施不在当前画面、序号超出实际建造数量）不再保存截图，避免挤掉有排查价值的截图 ([#18923](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18923)) @ABA2396
* 危险操作确认弹窗默认聚焦取消按钮 @ABA2396
* 优化小工具干员识别结果卡片布局 @ABA2396
* 基建房间列表一键全选/清空改为一次写入 @ABA2396
* 统一黑流树海界面提示的用词并更新说明文案，与当前行为一致 ([#18852](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18852)) @ZiyinLin @ABA2396
* YostarEN/YostarKR update the Annihilation navigation templates @ABA2396

### 修复 | Fix

* 适配明日方舟 26.11.1 主页界面改版，功能入口识别统一使用日间主题模板（新旧两套并存），主题切换按钮模板同步更新 ([#18955](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18955)) @Daydreamer114 @ABA2396
* 任务链引用未定义任务时停止任务并提示资源可能损坏，附修复引导 @ABA2396
* 使用 GPU 推理时若推理设备失效（如运行中更新显卡驱动），停止任务并提示重启 MAA ([#18472](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18472)) @crazysmile-PhD
* 修复 ｢运行任务时阻止休眠｣ 在定时启动时概率性失效的问题 ([#18849](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18849)) @zytyan
* 任务完成后动作的确认倒计时期间保持系统唤醒，避免完成后动作不执行 ([#18789](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18789)) @crazysmile-PhD
* 修复再次启动 MAA 唤醒已有实例时，第二实例误入更新流程导致的更新异常 @ABA2396
* 修复自动编队复用上一轮编队时多余干员未取消选中与漏选干员的问题 @status102 @ABA2396
* 修复自定义基建选人完成后未回到干员列表首页，导致后续收尾排序出错的问题 ([#18823](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18823)) @youzibigg @ABA2396
* 修复基建房间列表出现重复条目的问题 @ABA2396
* 修复干员培养点击干员卡片顶部无法进入详情页的问题 @ABA2396
* 材料合成选择干员后增加等待，降低界面动画未刷新导致的误识别 @Lancarus
* 修复库存保持 ｢手动输入关卡｣ 悬停提示显示错误的问题 @ABA2396
* 修复水月肉鸽进入商店时投资先于刷新命中，导致刷新商店（指路鳞）不生效的问题 ([#18883](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18883)) @ZiyinLin
* 缩小萨米肉鸽战斗节点的点击区域，修复与敌人情报区域重合导致的误触 ([#18877](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18877)) @ZiyinLin
* 黑流树海新增 ｢三重身｣ ｢溯源｣ 节点的识别分派，修复相关节点卡住的问题 ([#18808](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18808)) @ZiyinLin
* 黑流树海招募确认兼容带藏品增益的按钮外观，修复拿到相关藏品后卡住的问题 ([#18826](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18826)) @ZiyinLin
* 修复黑流树海第三、五层险路恶敌节点被误标为险路尽头的问题 ([#18966](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18966)) @ZiyinLin
* 黑流树海第四层地图识别兼容偏上布局 ([#18940](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18940)) @ZiyinLin
* 黑流树海开局分队与开局奖励确认点击未生效时自动重试，修复卡在分队页或开局奖励页的问题 ([#18856](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18856), [#18965](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18965)) @ZiyinLin
* 扩大黑流树海开局支援确认框的识别区域，兼容带襁褓动物时的布局 ([#18804](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18804)) @chanshengbinying
* 修复黑流树海拾取自然物后弹窗未关闭的问题 ([#18822](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18822)) @youzibigg
* 更正黑流树海结局通关后的跳过按钮模板 @ZiyinLin
* 修正部分保全与肉鸽作业中部署方向的拼写错误，修复朝向不生效的问题 @Lancarus
* 修正新活动关卡 TL-10 的地图数据 @status102
* 修复 ｢领取限时签到活动奖励｣ 通过图标进入时打开专享活动弹窗后未关闭的问题 @HX3N
* 定时执行的自定义配置提示由 ｢重启游戏｣ 更正为 ｢重启 MAA｣ ([#18851](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18851)) @youzibigg
* 修复 PC 端连接失败时仅在未找到游戏窗口的情况下尝试启动客户端，修复窗口存在但连接失败（如分辨率不符）时误启动客户端的问题 ([#18890](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18890)) @moranfanhua
* 修复 PC 端黑流树海商店投资入口识别分数不足导致无法投资的问题 ([#18710](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18710)) @ylv01 @ZiyinLin
* YostarEN fix various OCR replacements: infrastructure at 720p, Ch'en, and operator name regexes @Constrat
* YostarJP improve Mizuki floor OCR and squad confirmation ([#18905](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18905)) @wallsman
* YostarJP fix ocrReplace for Kal'tsit (ケルシー・エスペランタ) @Saratoga-Official

### 文档 | Docs

* 同步近期行为变更到五语言文档，协议文档补 GpuDeviceRemoved 与 UnknownTask 回调及未定义任务名停止任务链的语义，用户手册补库存保持与黑流树海刷开局等小节 ([#18881](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18881)) @ABA2396
* 文档站 ::: 容器统一补行尾硬换行，便于不支持容器渲染的预览逐行显示 ([#18937](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18937)) @ABA2396

### 其他 | Other

* 本地化 key 按功能域全量重命名，配套排序与引用校验工具 ([#18879](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18879)) @ABA2396
* 本地化下拉列表条目改为链式配置 ([#18928](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18928)) @ABA2396
* 静态类日志器统一使用 `Log.ForContext(typeof(T))` ([#18917](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18917)) @ABA2396
* 将信用商店购买成功图标提取为通用物资获取图标模板 ([#18900](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18900)) @youzibigg
* Unify the activity rewards claiming tasks and reuse the CreditShop-Bought template @HX3N
* 调整 NetBeauty 依赖收纳布局，放行运行时核心组件 @ABA2396
* 统一源码文件无 BOM，并以 pre-commit 钩子强制 @ABA2396
* 统一界面文案的角引号与单位空格排版 @ABA2396
* 清理无引用的本地化键与无用代码 @ABA2396
* 移除未接入 CI 的旧测试脚手架 @ABA2396
* 库存数据的同步时间统一以 UTC 保存 @ABA2396
* CoreML 检测限定 Apple 平台，修复非 Apple 平台的编译失败 ([#18833](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18833)) @wangl-cc
* RoguelikeMapConfig 补缺失的头文件包含 ([#18750](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18750)) @wangl-cc
* 修订仓库开发约定，补充本地化 key 前缀命名规范、排序脚本用法、本地化下拉列表与文档容器硬换行约定 @ABA2396

</details>
