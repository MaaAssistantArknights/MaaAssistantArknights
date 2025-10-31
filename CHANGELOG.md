## v5.27.0

### 难道说？ | Highlights

由于近期《明日方舟》的更新引入了部分不兼容的改动（比如公开招募界面的部分按钮改动，界园肉鸽的部分按钮改动），所以**旧版本可能无法使用自动公招、自动肉鸽等功能，建议更新到最新版本**。

当然，本次版本更新恰逢即将到来的半周年更新，所以如果半周年更新也引入了新的不兼容改动，MAA 可能无法第一时间适配，敬请关注后续更新。

~~号外：《明日方舟：终末地》开启三测啦！（内容全面，资格就不全面.jpg）~~

#### 自动肉鸽方面

本次更新，我们支持了界园肉鸽新 DLC 的新关卡、新通宝、新不期而遇等，添加了“刷常乐节点”策略。

我们也添加了萨米肉鸽的“刷稀有坍缩范式”策略，欢迎你尝试~

#### 界面主题方面

我们在这个版本正式适配了新的界面主题「齐聚」。并且，在部分自动任务中，如果当前的界面主题无法识别，牛牛会尝试自动切换到日间主题。~~这就是机械革命~~

#### 其他方面

我们继续修复了大量 bug，例如外部通知功能现在会正确地将消息发送到所有渠道了。如果还有漏网之鱼，欢迎你来反馈。

----

Due to recent updates to *Arknights* introducing some incompatible changes (such as changes to some buttons in the *Recruitment* interface and some in the *Sui's Garden Of Grotesqueries*), **older versions may not be able to use features like *Auto Recruit* and *Auto I.S.*. We recommend updating to the latest version.**

Of course, this update coincides with the upcoming thank-you celebration update, so if this update also introduces new incompatible changes, MAA may not be able to adapt immediately. Please stay tuned for future updates.

~~Breaking News: *Arknights: Endfield* has started its third closed beta test! (ALL content, but not for everyone)~~

#### *Auto I.S.*

[CN ONLY] In this update, we support new stages, new *Tongbaos*, and new *Encounters* in the *Sui's Garden Of Grotesqueries* Expansion I, and added the "Farm playtime(常乐) nodes" strategy.

We have also added the "Farm collapse paradigms" strategy for *Sarkaz's Furnaceside Fables*. Feel free to try it out!

#### Interface Theme

[CN ONLY] We have officially adapted the new interface theme "Gathering(齐聚)" in this version.

Furthermore, in some automated tasks, if the current interface theme cannot be recognized, MAA will attempt to automatically switch to the *Light* theme. ~~This is Mechanic Revolution~~

#### Others

We have continued to fix numerous bugs; for example, the external notification function now correctly sends messages to all channels. If there are any bugs that have been missed, please feel free to provide feedback.

----

以下是详细内容：

### 新增 | New

* 界面主题「齐聚」 @SherkeyXD
* 界园DLC新关卡战斗逻辑 @Saratoga-Official
* 界园 dlc2 通宝数据更新 @SherkeyXD
* 界园肉鸽不期而遇实现选项识别 (#14540) @Alan-Charred @pre-commit-ci[bot]
* 添加主线 16 章导航 @ABA2396
* 支持剿灭关卡名识别 @ABA2396
* 添加刷常乐节点策略 (#13868) @travellerse
* 添加 生活队凹开局密文板 与 坍缩范式列表 TooltipBlock @ABA2396
* 支持生息演算与肉鸽遇到不认识的主题时自动切换，不认识的主题直接切换 @ABA2396
* 本次更新的公告右上角添加红点 @ABA2396
* 跳过使用未准备好的技能 && 全局计时器 (#13913) @Alan-Charred @pre-commit-ci[bot]
* 增加纯数色匹配 (#14536) @ABA2396 @pre-commit-ci[bot]
* 资源更新完成后等待空闲时再加载 @ABA2396
* core 异常退出 ui 添加日志记录 @ABA2396
* 记录crash log时同时记录stacktrace, 并允许 ASST_DEBUG 在 debug 目录下生成 crash.log (#14526) @status102
* LogInfo 等宏支持is_enabled (#14551) @status102
* Update Submodules MaaMacGui, maa-cli @github-actions[bot]

### 改进 | Improved

* 繁中服主線、肉鴿導航 UI 更新 (#14433) @momomochi987 @pre-commit-ci[bot]
* 主线导航目标关卡默认在屏幕内时不再划到最右边后往前寻找 @ABA2396
* 更新 Windows 模拟器文档，调整支持列表并添加详细说明 (#14534) @AnnAngela
* 进入新任务后重置任务超时计时器, 以避免非单任务卡阻的误报 @status102
* GetImageFromRoi 工具优化 @SherkeyXD
* ProcSubTask static @status102

### 修复 | Fix

* 齐聚主题无法导航肉鸽和生息演算 @Saratoga-Official
* 繁中服無法進入資源收藏關卡 (#14469) @momomochi987
* 修复 "设置-外部通知" 存在多个通知渠道时只会发送到第一个成功的渠道问题 (#14380) @Jim-Happy
* 开始唤醒会回到主界面 @ABA2396
* 无法加载 cache 资源 @ABA2396
* 无人机协助点击无效时增加自动重试机制 @ABA2396
* 每日任务领取因为延迟卡住、延迟载荷移位 @SherkeyXD
* 公招按钮roi错误 @Saratoga-Official
* 掉落识别修复、增加越界检查 (#14508) (#14312) @Alan-Charred @MistEO
* 尝试为活动商店小游戏适配新的时装购买动画、改用 CustomeSkip 来跳过新时装动画 (#14490) (#14566) @Alan-Charred @pre-commit-ci[bot]
* 给生息演算点击 "开始行动" 的任务加点延迟，防止错过确认弹窗 @Alan-Charred
* 肉鸽跳一战打一战刷钱 (#14450) @Alan-Charred @pre-commit-ci[bot]
* 界园肉鸽选项识别：不期而遇、狭路相逢、杂疑 @Saratoga-Official
* 傀影不期而遇选项 @Saratoga-Official
* 为界园放弃探索任务添加备用模版、dlc 背景无法放弃探索 (#14466) (#14425) @Alan-Charred @pre-commit-ci[bot]
* 降低萨米肉鸽进入 "诡秘的预感" 所需模版匹配分数阈值 (#14521) @Alan-Charred
* 萨卡兹去伪存真弹窗有延迟无法退出 @Saratoga-Official
* 给ClickToStartPoint加点延迟 @Saratoga-Official
* 调换使用错误的排序函数 (#14555) @Alan-Charred
* 猪玛写 switch 不写 break @ABA2396
* 变更弃用的参数 @ABA2396
* Award模板阈值还是小了 @Saratoga-Official
* YostarKR RoguelikeCustom-HijackSquad squad recognition @HX3N
* ocr for Executor the Ex Foedere for EN @Constrat
* ja-jp 泰拉大陆调查团 识别错误 @ABA2396
* JP Task.png (#14516) @Daydreamer114
* cleanup ifdef + fix AsstDestroy missed calls + lldb @Constrat
* precommit maacore @Constrat
* change default issue + remove always as not necessary、add always() @Constrat

### 文档 | Docs

* 补充模拟器相关文档和协议文档 (#14478) @Alan-Charred
* 添加关于实体设备上账号切换问题的说明 (#14468) @Initial-heart-1 @lucienshawls @HX3N @Constrat @Manicsteiner @momomochi987 @pre-commit-ci[bot]
* 在不支持列表中添加腾讯应用宝 (#14477) @JasonHuang79 @pre-commit-ci[bot]
* 修正插件相关描述 @SherkeyXD
* Sync documents in all languages, Powered by AI (#14572) @MistEO @Manicsteiner
* 全语言开发文档章节整理 (#14562) @MistEO @pre-commit-ci[bot]
* 对codespace相关链接进行小修小补、把codespace移到犄角旮旯 (#14564) (#14560) @lucienshawls @MistEO @pre-commit-ci[bot]
* 接入DeepWiki (#14563) @lucienshawls
* vscode插件文档视觉优化 @SherkeyXD
* 优化文档代码的高亮主题 @SherkeyXD
* 利用tabs优化视觉表现 @SherkeyXD
* 利用步骤容器优化视觉效果 @SherkeyXD
* 增加换行以提升文档在GitHub的阅读体验 (#14338) @status102
* 更正文档编写指南的一处typo @lucienshawls
* 更新algolia配置 @SherkeyXD
* Auto Update Changelogs of v5.27.0-beta.2 (#14567) @github-actions[bot] @ABA2396
* Auto Update Changelogs of v5.27.0-beta.1 (#14523) @github-actions[bot] @ABA2396

### 其他 | Other

* 集成 MaaUtils (#14578) @MistEO @pre-commit-ci[bot]
* Release v5.27.0-beta.2 (#14556) @ABA2396
* Release v5.27.0-beta.1 (#14502) @ABA2396
* WpfGui 增加对界园肉鸽 DLC 1 三个新分队的支持 @Alan-Charred
* 拆出界园岁兽残识地图导航 (#14432) @Alan-Charred
* 基建降低会清空其他干员效率的技能优先级 @ABA2396
* 重复访问好友添加Ocr兜底 @Saratoga-Official
* ui 日志增加 adb devices 输出 @ABA2396
* ALL~ the Announcements 中新公告标题中加 * @ABA2396
* 调整雷电截图增强提示，强制要求模拟器分辨率为横屏模式 @ABA2396
* 调整连接失败尝试启动模拟器的描述 @ABA2396
* 调整截图 @ABA2396
* 繁中服「輓歌燃燒殆盡」活動導航 (#14434) @momomochi987 @pre-commit-ci[bot]
* SSS#8 global changes (add acahuallan, remove dossoles and barrenbeasts) @Constrat
* YostarKR SSS#8 SSSBuffChoose、update SSS#8 buff choose ocr for EN @HX3N @Constrat
* 同心暂时去掉5选项、Revert "chore: 同心暂时去掉5选项" @Saratoga-Official
* add SwitchTheme@ConfirmThemeChange.png @Constrat @Manicsteiner @HX3N @momomochi987 @pre-commit-ci[bot]
* 将 Copilot ActionType::ResetTimer 更改为更适合的 ResetStopwatch (#14507) @Alan-Charred
* translate ElapsedTime ref: 1edd00698200b7c2a14406d7a29f51689f6871d1 @Constrat
* 增加wpf项目cmake build脚本 @status102
* 移除devcontainer轻量环境的部分非必要依赖 (#14499) @lucienshawls
* polish PixelAnalyzer a bit (#14538) @Alan-Charred
* 添加 png 后缀 @SherkeyXD
* 文件路径错误 @ABA2396
* remove global templates for 3294b29f54dadc4198a40538e85b71902c79c875 @Constrat
* remove cancelled() and add mentions @Constrat
* 我是铸币 @SherkeyXD
