## v6.6.0-beta.1

### 新增 | New

* 自动战斗增加页签自动切换逻辑、保全增加快速切换列表 @ABA2396 @status102
* 增加更新数据任务 (#16026) @ABA2396 @status102
* 同时启动多个模拟器时使用自动检测连接提供弹窗选择 (#16020) @ABA2396
* 运行结束后保留上次运行结果 @ABA2396
* 支持雷电 14 注册表查询 @ABA2396
* AVD 截图增强 (#15608) @satgo1546

### 改进 | Improved

* 刷理智任务仅在 stage 为空时检查无掉落关卡，并在使用理智药前检查药品数量 @status102
* 避免下载作业时等待全部完成并减少重复赋值 @status102
* 作业集解析不再输出详细信息以避免刷屏，并优化解析按钮 icon @status102
* 优化 TaskQueueList 列表高度 @status102 @ABA2396
* 更新后首次重启仅进行文件更新不加载多余数据 @ABA2396
* WPF 下载框样式遵循是否使用卡片设置 (#16029) @status102
* 空配置默认任务拆分，并在添加/修改任务设置时返回 taskId @status102
* 移除手动触发切换账号的启动流程逻辑，统一使用 LinkStart @status102
* hoist image-side cvtColor out of template loop in Matcher (#16018) @Aliothmoon

### 修复 | Fix

* 修复 VisitNextBlack 难以触发导致任务循环的问题 (#15767) @sylw114
* 修复自定干员技能范围检查问题 @status102
* 修复勾选手动输入关卡名时无法拖动候选关卡 @ABA2396
* 修复使用空图片匹配时错误 Log 输出 @status102
* 修复地图名查找问题 @status102
* 修复通宝置换/投钱后可能出现的藏品或通宝获得弹窗问题 (#15993) @travellerse
* 修复 LevelKey 空属性导致误匹配 @status102
* 修复刷理智任务返回未开放关卡问题 @status102
* 修复恢复 StagePlan 后 AsstFightTask 的 Stage 问题 @status102
* 修复 Eyjafjalla Alter 正则（EN）@Constrat

### 文档 | Docs

* 修改 git clone 命令使用 --single-branch (#16000) @AnnAngela

### 其他 | Other

* 使用本地缓存数据时不显示 Growl @ABA2396
* 赠送线索后增加等待以避免弹窗遮挡新线索图标 @ABA2396
* 添加 log 分析与 issue analysis @MistEO
* 多语言内容更新与优化（JP/EN/KR OCR、Roguelike JieGarden DLC1、EP16 等）@Manicsteiner @Constrat @HX3N
* 优化英文输出并为 SKILL.md 添加英文翻译部分 @MistEO
