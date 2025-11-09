## v5.27.3

### 新增 | New

* MaaCore 记录 Exception 时额外记录模块基址 @status102
* Wpf i18n字符串支持直接string.Frmat @status102
* 怎么还有 GTII @ABA2396
* 增加华硕 GTIII 显卡超频工具注入导致崩溃提示 @ABA2396
* 战斗列表启用时, 浏览单个作业时自动添加到战斗列表 (#14625) @status102

### 改进 | Improved

* asst Log增加crash时额外输出core版本, build时间, 路径 @status102
* 更好的随机点分布 (#14652) @MistEO
* 优化切换主题逻辑 @ABA2396
* 优化切换主题流程 @ABA2396
* 空stack trace输出 @status102

### 修复 | Fix

* 提高干员部署滑动的最小时长，减少因模拟器丢帧导致的部署失败 @ABA2396
* Exusiai Alter ocr regex for EN @Constrat
* 肉鸽刷开局期望奖励无法动态隐藏/显示 @ABA2396
* Dorothy S2 ocr regex @Constrat
* Wpf增加悖论模拟模式非悖论作业检测 @status102
* 界园入暂亭ocrfix @Saratoga-Official
* update to new data repo for sh script @Constrat
* update to new data repo for ps1 script @Constrat
* move to public arkntools repo for txwy @Constrat
* update Sami floor regex fix 14630 (hopeful) @Constrat
* 给 mujica 擦屁股 @ABA2396
* move from Template to OcrDetect for BattleQuickFormationClear @Constrat
* 部分情况下 钼铅 识别错误 @ABA2396
* JP 1ns ocr @Daydreamer114
* OS 小游戏回合结束改用 ocr @ABA2396
* 结束回合按钮怎么还会闪烁啊 @ABA2396
* 动画或卡顿导致概率无法使用无人机 @ABA2396
* 增加从主页进入终端的延迟 @Saratoga-Official

### 其他 | Other

* add logs @MistEO
* CropRoi -> ImageCropper (#14379) (#14647) @MistEO @lucienshawls
* remove expired token @Constrat
* wpf拆分添加作业和作业集按钮 (#14624) @status102 @ABA2396
* use MaaUtils instead of Utils (#14579) @MistEO
* Revert "fix: 增加从主页进入终端的延迟" @ABA2396
