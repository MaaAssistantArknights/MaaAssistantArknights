## v5.27.0

### 新增 | New

* 记录crash log时同时记录stacktrace, 并允许 ASST_DEBUG 在 debug 目录下生成 crash.log (#14526) @status102
* LogInfo 等宏支持is_enabled (#14551) @status102
* 界面主题「齐聚」 @SherkeyXD
* 资源更新完成后等待空闲时再加载 @ABA2396
* 界园DLC新关卡战斗逻辑 @Saratoga-Official
* 不是我认识的主题，直接切换 @ABA2396
* 界园 dlc2 通宝数据更新 @SherkeyXD
* 支持剿灭关卡名识别 @ABA2396
* 跳过使用未准备好的技能 && 全局计时器 (#13913) @Alan-Charred @pre-commit-ci[bot]
* 本次更新的公告右上角添加红点 @ABA2396
* 添加刷常乐节点策略 (#13868) @travellerse
* 界园肉鸽不期而遇实现选项识别 (#14540) @Alan-Charred @pre-commit-ci[bot]
* 增加纯数色匹配 (#14536) @ABA2396 @pre-commit-ci[bot]
* 添加主线 16 章导航 @ABA2396
* 支持生息演算与肉鸽遇到不认识的主题时自动切换 @ABA2396
* 添加 生活队凹开局密文板 与 坍缩范式列表 TooltipBlock @ABA2396
* core 异常退出 ui 添加日志记录 @ABA2396
* Update Submodules MaaMacGui, maa-cli @github-actions[bot]

### 改进 | Improved

* GetImageFromRoi 工具优化 @SherkeyXD
* 进入新任务后重置任务超时计时器, 以避免非单任务卡阻的误报 @status102
* ProcSubTask static @status102
* 繁中服主線、肉鴿導航 UI 更新 (#14433) @momomochi987 @pre-commit-ci[bot]
* 主线导航目标关卡默认在屏幕内时不再划到最右边后往前寻找 @ABA2396
* 更新 Windows 模拟器文档，调整支持列表并添加详细说明 (#14534) @AnnAngela

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
* 无法加载 cache 资源 @ABA2396
* 改用 CustomeSkip 来跳过新时装动画 (#14566) @Alan-Charred @pre-commit-ci[bot]
* 调换使用错误的排序函数 (#14555) @Alan-Charred
* 给生息演算点击 "开始行动" 的任务加点延迟，防止错过确认弹窗 @Alan-Charred
* 猪玛写 switch 不写 break @ABA2396
* 齐聚主题无法导航肉鸽和生息演算 @Saratoga-Official
* ocr for Executor the Ex Foedere for EN @Constrat
* ja-jp 泰拉大陆调查团 识别错误 @ABA2396

### 文档 | Docs

* 修正插件相关描述 @SherkeyXD
* Sync documents in all languages, Powered by AI (#14572) @MistEO @Manicsteiner
* 优化文档代码的高亮主题 @SherkeyXD
* Auto Update Changelogs of v5.27.0-beta.1 (#14523) @github-actions[bot] @ABA2396
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
* Auto Update Changelogs of v5.27.0-beta.2 (#14567) @github-actions[bot] @ABA2396
* 更正文档编写指南的一处typo @lucienshawls
* 对codespace相关链接进行小修小补 (#14564) @lucienshawls
* 接入DeepWiki (#14563) @lucienshawls
* 全语言开发文档章节整理 (#14562) @MistEO @pre-commit-ci[bot]
* 把codespace移到犄角旮旯 (#14560) @MistEO @pre-commit-ci[bot]

### 其他 | Other

* 集成 MaaUtils (#14578) @MistEO @pre-commit-ci[bot]
* Release v5.27.0-beta.2 (#14556) @ABA2396
* Release v5.27.0-beta.1 (#14502) @ABA2396
* 同心暂时去掉5选项 @Saratoga-Official
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
* Revert "chore: 同心暂时去掉5选项" @Saratoga-Official
* 调整雷电截图增强提示，强制要求模拟器分辨率为横屏模式 @ABA2396
* 调整连接失败尝试启动模拟器的描述 @ABA2396
* 移除devcontainer轻量环境的部分非必要依赖 (#14499) @lucienshawls
* 重复访问好友添加Ocr兜底 @Saratoga-Official
* 添加 png 后缀 @SherkeyXD
* WpfGui 增加对界园肉鸽 DLC 1 三个新分队的支持 @Alan-Charred
* polish PixelAnalyzer a bit (#14538) @Alan-Charred
* 增加wpf项目cmake build脚本 @status102
* ui 日志增加 adb devices 输出 @ABA2396
* 基建降低会清空其他干员效率的技能优先级 @ABA2396
