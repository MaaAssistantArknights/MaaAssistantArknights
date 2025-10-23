## v5.27.0-beta.1

### 新增 | New

* 界面主题「齐聚」 @SherkeyXD
* 资源更新完成后等待空闲时再加载 @ABA2396
* 界园DLC新关卡战斗逻辑 @Saratoga-Official
* 不是我认识的主题，直接切换 @ABA2396
* 界园 dlc2 通宝数据更新 @SherkeyXD
* 支持剿灭关卡名识别 @ABA2396
* 跳过使用未准备好的技能 && 全局计时器 (#13913) @Alan-Charred @pre-commit-ci[bot]
* 本次更新的公告右上角添加红点 @ABA2396
* 添加刷常乐节点策略 (#13868) @travellerse

### 改进 | Improved

* GetImageFromRoi 工具优化 @SherkeyXD
* 进入新任务后重置任务超时计时器, 以避免非单任务卡阻的误报 @status102
* ProcSubTask static @status102
* 繁中服主線、肉鴿導航 UI 更新 (#14433) @momomochi987 @pre-commit-ci[bot]

### 修复 | Fix

* 降低萨米肉鸽进入 “诡秘的预感” 所需模版匹配分数阈值 (#14521) @Alan-Charred
* 公招按钮roi错误 @Saratoga-Official
* JP Task.png (#14516) @Daydreamer114
* 掉落识别修复 (#14508) @Alan-Charred
* 变更弃用的参数 @ABA2396
* 开始唤醒会回到主界面 @ABA2396
* 尝试为活动商店小游戏适配新的时装购买动画 (#14490) @Alan-Charred @pre-commit-ci[bot]
* precommit maacore @Constrat
* change default issue + remove always as not necessary @Constrat
* add always() @Constrat
* 无人机协助点击无效时增加自动重试机制 @ABA2396
* 每日任务领取延迟载荷移位 @SherkeyXD
* 每日任务领取因为延迟卡住 @SherkeyXD
* 繁中服無法進入資源收藏關卡 (#14469) @momomochi987
* 为界园放弃探索任务添加备用模版 (#14466) @Alan-Charred @pre-commit-ci[bot]
* 肉鸽跳一战打一战刷钱 (#14450) @Alan-Charred @pre-commit-ci[bot]
* 傀影不期而遇选项 @Saratoga-Official
* 界园 dlc 背景无法放弃探索 (#14425) @Alan-Charred @pre-commit-ci[bot]
* 界园狭路相逢选项 @Saratoga-Official
* YostarKR RoguelikeCustom-HijackSquad squad recognition @HX3N
* Award模板阈值还是小了 @Saratoga-Official
* 掉落识别增加越界检查 (#14312) @MistEO @Alan-Charred
* 界园杂疑选项 @Saratoga-Official
* 修复 "设置-外部通知" 存在多个通知渠道时只会发送到第一个成功的渠道问题 (#14380) @Jim-Happy
* 界园肉鸽选项 @Saratoga-Official
* 给ClickToStartPoint加点延迟 @Saratoga-Official
* 萨卡兹去伪存真弹窗有延迟无法退出 @Saratoga-Official
* cleanup ifdef + fix AsstDestroy missed calls + lldb @Constrat

### 文档 | Docs

* 补充模拟器相关文档和协议文档 (#14478) @Alan-Charred
* 添加关于实体设备上账号切换问题的说明 (#14468) @Initial-heart-1 @lucienshawls @HX3N @Constrat @Manicsteiner @momomochi987 @pre-commit-ci[bot]
* 在不支持列表中添加腾讯应用宝 (#14477) @JasonHuang79 @pre-commit-ci[bot]
* 增加换行以提升文档在GitHub的阅读体验 (#14338) @status102
* vscode插件文档视觉优化 @SherkeyXD
* 利用tabs优化视觉表现-part2 @SherkeyXD
* 利用tabs优化视觉表现 @SherkeyXD
* 利用步骤容器优化视觉效果-part2 @SherkeyXD
* 利用步骤容器优化视觉效果 @SherkeyXD
* 更新algolia配置 @SherkeyXD

### 其他 | Other

* remove global templates for 3294b29f54dadc4198a40538e85b71902c79c875 @Constrat
* txwy add SwitchTheme@ConfirmThemeChange.png (#14511) @momomochi987 @pre-commit-ci[bot]
* 将 Copilot ActionType::ResetTimer 更改为更适合的 ResetStopwatch (#14507) @Alan-Charred
* 调整截图 @ABA2396
* 文件路径错误 @ABA2396
* YostarJP SwitchTheme@ConfirmThemeChang @Manicsteiner
* YostarKR add SwitchTheme@ConfirmThemeChang @HX3N
* add SwitchTheme@ConfirmThemeChange.png @Constrat
* remove cancelled() and add mentions @Constrat
* 我是铸币 @SherkeyXD
* ALL~ the Announcements 中新公告标题中加 * @ABA2396
* translate ElapsedTime ref: 1edd00698200b7c2a14406d7a29f51689f6871d1 @Constrat
* 拆出界园岁兽残识地图导航 (#14432) @Alan-Charred
* 繁中服「輓歌燃燒殆盡」活動導航 (#14434) @momomochi987 @pre-commit-ci[bot]
* YostarKR SSS#8 SSSBuffChoose @HX3N
* update SSS#8 buff choose ocr for EN @Constrat
* SSS#8 global changes (add acahuallan, remove dossoles and barrenbeasts) @Constrat
