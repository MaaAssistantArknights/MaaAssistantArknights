## v6.6.0

### 新增 | New

* 保全增加快速切换列表，悖论模拟验证增加输出 i18n，ss 正则增加骑兵与火蓝之心 @ABA2396
* 优化页签自动切换逻辑 @ABA2396
* 增加更新数据任务 (#16026) @ABA2396 @status102
* 同时启动多个模拟器时使用自动检测连接时提供弹窗选择 (#16020) @ABA2396
* 运行结束后保留上次运行结果 @ABA2396
* 支持雷电14注册表查询 @ABA2396
* AVD截图增强 (#15608) @satgo1546

### 改进 | Improved

* 刷理智任务仅在 stage设置为空 时检查是否为无掉落关卡 @status102
* 避免下载作业时等待全部作业下载结束, 并减少重复赋值 @status102
* 作业集解析时不再输出作业详细信息, 以避免刷屏 @status102
* 自动切换作业类型 @status102
* Revert "feat: 优化页签自动切换逻辑" @status102
* TaskQueueList 自适应高度 @ABA2396
* 更新后第一次重启只进行文件更新不加载多余数据 @ABA2396
* wpf下载框样式遵循是否使用卡片设置 (#16029) @status102
* 空配置默认任务拆分 @status102
* 添加任务/修改任务设置时返回taskId @status102
* TaskQueueList Height @status102
* 移除手动触发切换账号时的启动流程逻辑, 统一使用LinkStart @status102
* 移除自动战斗的页签检查, 改为检查作业关卡 (#16025) @status102
* 作业列表解析按钮icon @status102
* hoist image-side cvtColor out of template loop in Matcher (#16018) @Aliothmoon
* 刷理智任务使用理智药前进行药品数量检查 @status102

### 修复 | Fix

* remove link from title @Constrat
* Eyjafjalla Alter regex EN @Constrat
* 解决了访问好友任务VisitNextBlack任务难以触发导致任务循环的问题 (#15767) @sylw114
* 移除忘记移除的return @status102
* 主线H关和S关未能识别为主线关卡 @status102
* 9e846609aa30b5667c53315f5f23e735c55a870c 中的错误 @status102
* 自定干员技能范围检查 @status102
* 在使用空图片进行匹配时输出错误Log @status102
* 勾选手动输入关卡名时无法拖动候选关卡 @ABA2396
* 在使用空图片进行匹配时输出错误Log @status102
* 地图名查找 @status102
* 处理通宝置换/投钱后可能出现的藏品/通宝获得弹窗 (#15993) @travellerse
* LevelKey 中的空属性导致误匹配 @status102
* 当LevelKey中部分属性为空时, 会忽略该属性 @status102
* 刷理智任务不再返回未开放关卡 @status102
* 恢复StagePlan后, 修复AsstFightTask的Stage @status102

### 文档 | Docs

* Auto Update Changelogs of v6.6.0-beta.1 (#16045) @github-actions[bot] @github-actions[bot] @ABA2396
* Modify git clone command to use --single-branch (#16000) @AnnAngela @AnnAngela

### 其他 | Other

* 更新繁中服 "聘用候選人" 截圖 (#16056) @momomochi987
* Release v6.6.0-beta.1 (#16044) @ABA2396
* bot输出小问题修正 @MistEO
* JP JieGarden ocr edits @Manicsteiner
* JP Roguelike JieGarden DLC1 (#16050) @Manicsteiner
* EN Roguelike JieGarden DLC1 @Constrat
* 调整自适应布局阈值 @ABA2396
* KR JieGarden DLC1 Squad and Encounter @HX3N
* KR JieGarden DLC1 CoppersNameOcrReplace @HX3N
*  chore: KR EP16 ocr updates @HX3N
* KR tweak @HX3N
* Optimize English Output @MistEO
* Add English translation section to SKILL.md @MistEO
* 调整输出格式 @MistEO
* 优化bot提示词，输出行号和中文任务名 @MistEO
* 使用本地缓存数据时不显示 Growl @ABA2396
* JP ocr fix (#16027) @Manicsteiner
* 帕拉斯固定称呼“博士” @MistEO
* 牛牛！ @MistEO
* 赠送线索后多等待一段时间，避免赠送线索后的弹窗挡住自己新线索的图标 @ABA2396
* add issue analysis @MistEO
* 添加log分析skill @MistEO
