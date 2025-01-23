## v5.12.1

### 新增 | New

* 繁中服 生息演算【沙洲遺聞】 (#11620) @momomochi987

### 修复 | Fix

* 基建修复2 (#11618) @ABA2396
* “围攻”主题 基建无法打开 (#11615) @ClozyA

## v5.12.0

### 没有测试的稳定版真的能叫稳定版吗 | Highlight

* 为了修基建包的饺子

### 新增 | New

* 繁中服 抽幸運牆的功能 (#11602) @momomochi987
* Colored logs for I.S. (#11593) @Constrat
* SideStory「相见欢」导航 (#11597) @SherkeyXD
* 优化肉鸽刷开局模式的流程和逻辑 (#11559) @DavidWang19 @ABA2396 @status102 @pre-commit-ci[bot] @HX3N @momomochi987 @Constrat
* 肉鸽月度小队/深入调查基础策略+自动切换 (#11566) @BxFS @Constrat @pre-commit-ci[bot] @status102
* 萨卡兹肉鸽卫士不语功战斗策略调整 (#11582) @ClozyA @pre-commit-ci[bot]
* SimpleEncryptionHelper 新增 DataProtectionScope 参数 @ABA2396
* 萨卡兹肉鸽卫士不语功战斗策略调整 (#11577) @ClozyA
* 萨卡兹肉鸽添加丛林密布，卡兹瀑布，遮天蔽日，有序清场，血脉之辩 (#11564) @MYAknight @pre-commit-ci[bot] @Daydreamer114
* Bazaar Orundum activities YostarEN @Constrat
* InvertNullFunction method @Constrat
* 优化下载速度显示 (#11541) @502y @status102
* 萨卡兹肉鸽刷开局 2 构想 (#11277) @Alan-Charred
* preload YostarEN AS navitation @Constrat
* 点刺、后勤种子存钱 & ProcessTask 添加 Input 方法 (#11521) @Daydreamer114 @ABA2396
* 肉鸽满级自动停止选项 (#11466) @BxFS @Constrat @HX3N @momomochi987 @status102
* 为肉鸽开始探索添加 cd 识别 (#11443) @Daydreamer114
* 萨卡兹肉鸽冰川期作战策略 @Daydreamer114
* 萨卡兹内容拓展II点刺进入商店获得构想 (#11509) @Daydreamer114
* 不自动招募1/5/6星干员时，不计入最大确认招募次数 (#11380) @Roland125 @pre-commit-ci[bot] @horror-proton
* 干员识别排除当前客户端未出干员 @ABA2396
* 肉鸽开局干员列表排除当前客户端未出干员 @ABA2396

### 改进 | Improved

* give priority to higher value choices in I.S. encounter.json (#11580) @Constrat
* 肉鸽战斗完成判定MissionCompletedFlag移除重复模板 @status102
* WpfGui重构 一键长草任务callback拆分 (#11548) @status102
* 肉鸽优先拿美愿 (#11558) @WWPXX233
* 简化萨卡兹肉鸽种子入口 @status102
* 延后WpfGui肉鸽开始探索次数输出至SubTaskCompleted @status102
* 肉鸽投资移除手中没钱时的额外检测重试 @status102
* Wpf重构 拆分模拟器关闭相关 (#11547) @status102
* Wpf重构 拆分`开始唤醒`任务 (#11533) @status102
* 肉鸽难度设置移动当前选项至列表顶部 @status102
* 新增投掷手干员组并调整优先级 @Daydreamer114
* 优化傀影肉鸽雪山上的来客ew部署 (#11195) @Daydreamer114
* manual recursion + robocopy for smoke-testing (#11458) @Constrat
* implement cache for smoke-test (#11457) @Constrat

### 修复 | Fix

* 修复基建 (#11598) @SherkeyXD @pre-commit-ci[bot] @ABA2396
* 修复基建排班错误和加工站卡住 (#11590) @HauKuen
* 萨卡兹肉鸽种子存钱分队选择失败 @status102
* tw 黍 百炼嘉维尔 OcrReplace (#11591) @Saratoga-Official
* reduce ROI to detect special value in Sami I.S. for EN @Constrat
* EN Sami GamePassSkip2 @Constrat
* 萨卡兹肉鸽使用种子刷钱参数存储分类错误 @status102
* Phantom IS GamePass EN template @Constrat
* EN Mizuki stuck on end screen @Constrat
* incorrect ROI AS navigation for YostarEN & KR @Constrat
* Resource Updater for YostarEN (AS-Event) @Constrat
* add delay to Roguelike@QuickFormation @Constrat
* 弹出公告或窗口时开始任务，进度条会显示在新窗口 @ABA2396
* 修复肉鸽种子模式引起的异常卡死 (#11552) @weinibuliu @Daydreamer114
* 94a8e01543a420bf70b6eef2539fea46466f2699 (#11553) @status102
* Return-White.png 识别 (#11469) @BxFS
* 截图失败时清空image_payload @status102
* 修复肉鸽开始探索重试时额外占用开始次数 @status102
* empty string option @Constrat
* get display for General (#11516) @jbwfu
* 生息演算为点击右上角天数任务添加重试机制 (#11540) @Alan-Charred
* 肉鸽烧热水没烧出来会从预设难度开始，而不是返回n0 @ABA2396
* 删除傀影肉鸽远方来客意义不明的撤退 (#11194) @Daydreamer114
* delay and retry downloads on resource updater (#11504) @Constrat
* use read/write secret to delete cache on pr merge @Constrat
* 博朗台计算等待时间失败数据处理 @status102
* increase fetch depth for release nightly-ota to generate tags (might need successive increases) @Constrat
* 修正nothing to select情况下的判断逻辑 @Roland125
* update Collect Rewards template for EN fix #11485 @Constrat
* tw OcrReplace 肉鸽招募助战 (#11487) @Saratoga-Official
* 繁中服作戰失敗畫面卡住 (#11479) @momomochi987
* InitialDrop.png更新 @Constrat @BxFS
* txwy duplicates in tasks.json @Constrat
* checkout depth for nightly ota @Constrat
* 更新 “视相“ 主题后未关闭退出基建弹窗时无法回到主界面 @ABA2396
* `手动输入关卡名` 与  `使用剩余理智` 选项无法保存 @ABA2396

### 文档 | Docs

* 补充 Roguelike 任务 investment_with_more_score 与 start_with_two_ideas 参数的文档 (#11546) @Alan-Charred
* changelog to v5.12.0-b1 @status102
* changelog 难度描述 (#11532) @Daydreamer114
* changelog @status102
* Auto Update Changelogs of v5.11.2 (#11530) @github-actions[bot] @Daydreamer114
* 为肉鸽参数 start_with_seed 添加文档 (#11531) @Daydreamer114
* 肉鸽辅助协议文档翻译 (#11360) @Windsland52

### 其他 | Other

* 我知道稳定版不稳定，所以就叫正式版吧 @ABA2396
* 隐藏公告关闭窗口，底部添加关闭按钮 @ABA2396
* refactor：提取 HandlePreviewMouseWheel @ABA2396
* YostarKR Mizuki DataTraceBack (#11584) @HX3N @pre-commit-ci[bot]
* YostarJP Mizuki DataTraceBack (#11579) @Manicsteiner
* 目录下存在 DEBUG_skill_ready.txt 时自动收集技能准备状态截图 (#11571) @ABA2396
* 不将已进驻的干员放入宿舍能用了吗？修了不知道有没有修好 @ABA2396
* [ recruitment tool ] use StrEnum instead of Enum for Theme (#11421) @Alan-Charred
* 长草任务ViewModel调整 @status102
* YostarKR update OrudumActivities (#11563) @HX3N
* YostarJP ocr update (#11561) @Manicsteiner
* 剿灭任务失败不报错 @ABA2396
* tasks.json配置JsonSchema @status102
* reapply reclamation rft on taskqueueviewmodel @Constrat
* WpfGui长草任务迁移至TaskQueue @status102
* YostarEN Phantom IS update run end skip @Constrat
* missing templates yostaren phantom IS @Constrat
* remove -Werror from CMakeLists.txt @horror-proton
* PlayToolsController start_game及input方法增加log输出提示 @status102
* YostarKR AS navigation (#11537) @HX3N
* YostarJP AS navigation (#11535) @Manicsteiner
* 修改雷电截图增强描述 @ABA2396
* Release v5.12.0-beta.1 (#11529) @status102
* Release 模式下，如文件夹中包含 DEBUG.txt 也会输出 DBG 日志 (#11496) @ABA2396
* 移动企鹅物流及一图流上报设置 至 运行设置 @status102
* Translations update from MAA Weblate (#11524) @AlisaAkiron
* ignore blame for e3d63894b28b2ef5e2405e144a32a6981de5e1b2 oxipng optimization @Constrat
* disable link checker in issues and PRs (#11506) @Constrat
* use API for cache-deletion @Constrat
* 移除不再使用的代码 for 最小化启动模拟器 @status102
* move `push tag` later in the workflow in case or errors (#11480) @Constrat
* 上报添加 User-Agent @ABA2396
* 修改上报抬头 @ABA2396
* Use %B to consider header for skip changelog @Constrat
* try setup dotnet cache @Constrat
* EN duplicates in tasks.json + SSS Buffs @Constrat
* YostarJP phantom roguelike game pass, SSS#6 (#11473) @Manicsteiner
* 繁中服「源石塵行動」復刻活動導航 @momomochi987
* battle_data 未实装干员添加字段提示 @ABA2396
* 别用 1234567890ABCDEF 去连模拟器了 @ABA2396
* Revert "refactor: move resource copy to test script" @Constrat
* `启动 MAA 后直接运行` 和 `启动 MAA 后自动开启模拟器` 改为独立配置 @ABA2396
* 只有一个配置的时候不显示 `此选项页为全局配置` @ABA2396
* 当前配置不存在时尝试读取全局配置 @ABA2396
* Config序列化参数不转义中文 @status102
