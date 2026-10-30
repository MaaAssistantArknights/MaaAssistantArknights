## v6.21.0-beta.3

### Highlights

本版本按功能域全量重写了界面语言资源，个别界面可能出现文案显示异常；如遇到问题，请通过 ｢设置 - 问题反馈｣ 提交反馈。

#### 生息演算迁移到 ｢小工具-牛杂｣，外服支持 ｢重启锚点｣

生息演算从任务队列迁移到 ｢小工具-牛杂｣ 独立运行，任务队列中已有的条目与设置升级后自动迁入。YostarEN/JP/KR 新增支持 ｢重启锚点｣ 主题。

#### 黑流树海刷开局支持凹第一层实托邦

刷开局新增 ｢凹第一层实托邦｣ 选项，需难度 6 及以上。

#### 编队与选人提速

自动编队优先在当前页匹配待选干员，减少翻页；自定义基建与宿舍选人按职业分桶定位剩余目标干员。

<details>
<summary><b>English</b></summary>

The UI localization resources have been extensively rewritten by feature domain in this version; a few texts may display incorrectly. If you run into any, please report via Settings - Issue Report.

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
<summary><b>v6.21.0-beta.3 (2026-10-30)</b></summary>

### 新增 | New

* 基建收获时识别回收站产出的物品与数量并显示在日志中，鼠标悬停日志可查看物品图标 ([#18924](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18924)) @ABA2396
* 黑流树海支持 ｢在第五层 BOSS 前暂停｣ ([#18913](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18913)) @ZiyinLin
* Android 组件附带 NCNN 格式的 OCR 模型，下游可直接使用 ncnn 后端识别 ([#18738](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18738)) @wangl-cc
* YostarEN/JP/KR support Reclamation Algorithm 3 ([#18929](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18929), [#18910](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18910), [#18911](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18911)) @dragonheart107 @Manicsteiner @HX3N

### 改进 | Improved

* 肉鸽与生息演算的主题下拉框中，当前客户端未适配的主题显示 ｢未适配｣ 后缀 ([#18928](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18928)) @ABA2396 @Constrat
* gui.log 记录 MessageBox 与 Growl 通知的弹出内容及用户选择，便于排查问题 ([#18915](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18915)) @ABA2396
* 收到新公告时 gui.log 只记录带 (NEW!!!) 标记的区块，不再写入整篇公告 ([#18916](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18916)) @ABA2396
* 基建进入设施时可预期的失败（设施不在当前画面、序号超出实际建造数量）不再保存截图，避免挤掉有排查价值的截图 ([#18923](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18923)) @ABA2396

### 修复 | Fix

* 修复水月肉鸽进入商店时投资先于刷新命中，导致刷新商店（指路鳞）不生效的问题 ([#18883](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18883)) @ZiyinLin
* 修复 ｢领取限时签到活动奖励｣ 通过图标进入时打开专享活动弹窗后未关闭的问题 @HX3N
* 修复今日关卡小提示刷新后内容未更新的问题 @ABA2396
* 修复 PC 端黑流树海商店投资入口识别分数不足导致无法投资的问题 ([#18710](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18710)) @ylv01 @ZiyinLin
* YostarEN fix various infrastructure OCR at 720p and Ch'en OCR @Constrat
* YostarJP fix ocrReplace for Kal'tsit (ケルシー・エスペランタ) @Saratoga-Official

### 其他 | Other

* 将信用商店购买成功图标提取为通用物资获取图标模板 ([#18900](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18900)) @youzibigg
* 静态类日志器统一使用 `Log.ForContext(typeof(T))` ([#18917](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18917)) @ABA2396
* 本地化下拉列表条目改为链式配置 ([#18928](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18928)) @ABA2396
* 库存数据的同步时间统一以 UTC 保存 @ABA2396
* 修订仓库开发约定，补充本地化 key 前缀命名规范、排序脚本用法与本地化下拉列表约定 @ABA2396

</details>

<details>
<summary><b>v6.21.0-beta.2 (2026-10-27)</b></summary>

### 修复 | Fix

* 缩小萨米肉鸽战斗节点的点击区域，修复与敌人情报区域重合导致的误触 ([#18877](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18877)) @ZiyinLin
* 修复自定义基建与宿舍选人按职业分桶时，装置与多职业干员被错误分桶导致卡住或漏选的问题 @youzibigg
* 修复部分确认弹窗的确认与取消按钮语义反转的问题，危险操作弹窗默认聚焦取消按钮 @ABA2396
* PC 端连接失败时仅在未找到游戏窗口的情况下尝试启动客户端，修复窗口存在但连接失败（如分辨率不符）时误启动客户端的问题 ([#18890](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18890)) @moranfanhua

### 文档 | Docs

* 同步近期行为变更到五语言文档，协议文档补 GpuDeviceRemoved 与 UnknownTask 回调及未定义任务名停止任务链的语义，用户手册补库存保持与黑流树海刷开局等小节 ([#18881](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18881)) @ABA2396

### 其他 | Other

* 本地化 key 按功能域全量重命名，配套排序与引用校验工具 ([#18879](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18879)) @ABA2396
* 调整 NetBeauty 依赖收纳布局，放行运行时核心组件 @ABA2396
* 统一源码文件无 BOM，并以 pre-commit 钩子强制 @ABA2396
* 修订仓库开发约定 @ABA2396

</details>

<details>
<summary><b>v6.21.0-beta.1 (2026-10-26)</b></summary>

### 新增 | New

* 生息演算迁移到 ｢小工具-牛杂｣ 独立运行，任务队列中已有的条目与设置自动迁入 ([#18809](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18809)) @ABA2396
* 黑流树海刷开局新增 ｢凹第一层实托邦｣ 选项，需难度 6 及以上 ([#18840](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18840)) @ZiyinLin @ABA2396
* 黑流树海开局支援页按所选策略的优先级顺序选取奖励 ([#18837](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18837)) @ZiyinLin
* 菲亚梅塔心情恢复的恢复目标新增琴柳、承曦格雷伊、温蒂、槐琥、至简、旅骨 ([#18688](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18688)) @Reverse0xCC @ABA2396
* 库存保持新增 ｢芯片组计入双芯片折算｣ 选项，统计芯片组时计入双芯片的折算数量 @ABA2396

### 改进 | Improved

* 今日关卡小提示新增 PR 芯片本的双芯片库存显示，掉落名称与库存数按掉落着色 ([#16193](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16193)) @soundofautumn @ABA2396
* 自动编队优先在当前页匹配待选干员，减少翻页，编队更快 @ABA2396
* 自定义基建与宿舍选人按职业分桶定位剩余目标干员，选人更快 @ABA2396
* 库存刷取与库存保持在无库存数据时跳过刷取并提示更新库存，识别结果为空时保留已有库存数据 ([#18850](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18850)) @ABA2396
* 基建训练室日志附带当时的界面截图 @ABA2396
* 优化小工具干员识别结果卡片布局 @ABA2396
* 基建房间列表一键全选/清空改为一次写入 @ABA2396
* 黑流树海界面提示统一用词为 ｢层｣ @ABA2396
* 更新黑流树海说明文案，与当前行为一致 ([#18852](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18852)) @ZiyinLin
* Unify the activity rewards claiming tasks and reuse the CreditShop-Bought template @HX3N
* YostarEN/YostarKR update the Annihilation navigation templates @ABA2396

### 修复 | Fix

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
* 黑流树海新增 ｢三重身｣ ｢溯源｣ 节点的识别分派，修复相关节点卡住的问题 ([#18808](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18808)) @ZiyinLin
* 黑流树海招募确认兼容带藏品增益的按钮外观，修复拿到相关藏品后卡住的问题 ([#18826](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18826)) @ZiyinLin
* 黑流树海开局分队确认点击未生效时自动重试，修复卡在分队页的问题 ([#18856](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18856)) @ZiyinLin
* 扩大黑流树海开局支援确认框的识别区域，兼容带襁褓动物时的布局 ([#18804](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18804)) @chanshengbinying
* 修复黑流树海拾取自然物后弹窗未关闭的问题 ([#18822](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18822)) @youzibigg
* 更正黑流树海结局通关后的跳过按钮模板 @ZiyinLin
* 修正部分保全与肉鸽作业中部署方向的拼写错误，修复朝向不生效的问题 @Lancarus
* 定时执行的自定义配置提示由 ｢重启游戏｣ 更正为 ｢重启 MAA｣ ([#18851](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18851)) @youzibigg

### 其他 | Other

* 移除未接入 CI 的旧测试脚手架 @ABA2396
* 清理无引用的本地化键与无用代码 @ABA2396
* 统一界面文案的角引号与单位空格排版 @ABA2396
* CoreML 检测限定 Apple 平台，修复非 Apple 平台的编译失败 ([#18833](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18833)) @wangl-cc
* RoguelikeMapConfig 补缺失的头文件包含 ([#18750](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18750)) @wangl-cc

</details>
