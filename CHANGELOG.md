## v5.13.0

### 相见欢 | Highlight

久违的正式版更新来了，在这个版本我们引入了多项新功能，还有一些优化：

#### 新的 MirrorChyan

这个版本开始，MAA 接入了第三方服务 [Mirror酱](https://mirrorchyan.com/)（一个给开源社区做有偿内容分发的平台）。它为我们提供了**免费的检查更新接口**，但它的下载是有偿的，需要用户付费使用。

不过，即使不购买 Mirror 酱的下载服务，你的 MAA 照样可以**白嫖他们的服务自动检查到新的更新**，然后自己再去设置里选择从海外源（GitHub）下载~  

如果你购买并填写了 CDK，则资源更新将会像之前那样完全自动化，不用每次点点点，也不用和自己的网络环境斗智斗勇咯。同时 MAA 本体更新也会优先走 Mirror 酱，下载更快！

同时我们也与 Mirror 酱达成了合作，其在 MAA 上的收益会共享一部分用于后续 MAA 官网、作业站及其他服务器建设。双赢（确信

#### 新的公告窗口

我们对公告窗口进行了一次大改，现在公告窗口左侧将有一个目录，方便跳转，但依然需要阅读完**所有**公告内容后才可以点击确定按钮关闭窗口。

#### 新的分辨率推荐

MAA 在处理图像识别时，会将图片压缩至 1280 × 720，以减轻计算负担、加快处理速度。

我们在处理用户反馈时发现，很多用户设置的分辨率不为 **1280 × 720** 或 **1920 × 1080**，导致 MAA 出现无法正确识别的情况。

我们**只推荐**用户使用这两种分辨率，以获得最佳的体验。你只需要确保模拟器设置里的分辨率设置是上述推荐的即可，窗口本身可以随意缩放。

如果你使用的是美服游戏客户端，则只建议使用 1920 × 1080。

#### 旧的配置文件保护

MAA 使用配置文件保存用户的设置，这里面**可能**会包含一些敏感信息。

例如，如果你在【外部通知】中设置了使用 ServerChan、SMTP（即邮件）、Discord 等通知方式，那么这些通知方式所需要的密码、密钥将会保存在你的配置文件。这个版本开始接入的 Mirror 酱的 CDK 也是如此。

但大部分用户其实并不知道，所以有可能在分享配置文件时无意中泄露了这些信息。

从 v5.10.0 版本开始，我们使用了 Windows DPAPI 技术来加密这些敏感信息，这样即使你分享了配置文件，也不会泄露你的密码。

这项技术使用你的 Windows 账户的数据来加密敏感信息，所以这些信息只能在你的当前 Windows 账户下被解密，其他 Windows 账户或是其他电脑是无法解密的。至于你的 Windows 账户是本地账户还是 Microsoft 账户，并不影响这项技术的使用。

这也意味着从一台电脑迁移到另一台电脑时，在复制配置文件后，仍然需要重新设置【外部通知】【资源更新】里的各项密码、密钥、CDK 等。

~~至于为什么 v5.10.0 版本引入的机制要在这个版本重新提及，牛牛是不会承认当时忘记写在更新日志里的~~

#### 其他方面

* 现在内置的资源更新功能将不再需要重启 MAA，而且更新后会自动重载战斗数据；
* 本次版本更新随附资源为 2 月 12 日版本；
* 【基建换班】适配了基建新的未进驻选项，现在“不将已进驻的干员放入宿舍”功能可以正常使用了；
* 【自动战斗】遇到不支持的关卡时，会尝试检测资源版本更新，如果有更新则会提示用户先更新资源，如果用户购买了 Mirror 酱的下载服务，则会自动下载；
* 【自动肉鸽】在商店存钱达到上限后不再尝试存钱，**修复大量错误**；
* 修正了诸多问题，例如日志文件不能正确轮替、自动编队干员选择错误等；

#### macOS 方面

* macOS 版本现在也支持账号切换功能了；
* macOS 版本修正了【自动肉鸽】参数问题；

----

以下是详细内容：

### 新增 | New

* 繁中服「巴別塔」活動導航 (#11863) @momomochi987
* 添加资源更新提示 @ABA2396
* CDK 改为密码框 @ABA2396
* 适配新 ui 未进驻选项 @ABA2396
* en announcement wpf logic @Constrat
* Paradox Simulation update UI for YoStar (#11793) @Constrat
* Wpf代理设置格式提示 | add format hint for proxy setting (#11781) @Rbqwow
* 添加 MirrorChyan 资源更新方式 (#11669) @ABA2396 @MistEO
* update maa self by mirrorc (#11812) @MistEO @ABA2396
* Mac支持Mirror酱资源更新 (#11768) @hguandl
* Mac开始唤醒支持账号切换 (#11803) @hguandl
* 添加mirror酱备用线路 (#11777) @MistEO
* 自动战斗遇到不支持的关卡时尝试检测资源版本更新 @ABA2396
* 自动战斗不再允许使用带TR的导航关卡名禁用自动编队，改为自动检测 (#11868) @status102
* Mac支持肉鸽参数配置新参数 (#11866) @hguandl
* YostarJP Sarkaz roguelike preload (#11850) @Manicsteiner

### 改进 | Improved

* 优化肉鸽难度显示 @ABA2396
* 添加资源更新提示翻译 @ABA2396
* 将Sarkaz开局添加负荷干员的进入任务改为范围点击 (#11100) @Daydreamer114
* 萨卡兹肉鸽不期而遇统一使用默认策略 (#11512) @Daydreamer114
* 小工具-仓库识别 隐藏黑边 @ABA2396
* 更新日志支持惯性运动 @ABA2396
* 优化存亡之战部署策略 (#11706) @Black1312
* 重构公告 (#11734) @ABA2396 @Constrat
* 肉鸽满级移除前置任务检查 @status102
* 添加 MirrorChyan 检查更新日志 @ABA2396
* 资源更新和检查更新分开 @ABA2396
* 开始任务自动关闭雷电模拟器 Google 套件窗口 (#11748) @THSLP13
* 取消更新源选择框 @ABA2396
* 资源更新后重载 BattleData (#11874) @ABA2396
* 资源更新后无需重启，优化手动更新逻辑 (#11857) @ABA2396
* MaaCore PackageTask中未进入的subtask将不再输出log (#11783) @status102
* 自动战斗BattleStartPre任务合并理智药检测 @status102
* 肉鸽投资在同一局内投资系统错误后不再进入投资插件 (#11826) @status102
* 自动战斗自动编队在选择职业前编入非干员组干员 (#11830) @status102
* 肉鸽局内参数重构遗留 (#11829) @status102
* 优化界面布局与翻译 @ABA2396

### 修复 | Fix

* JP 艾雅法拉 ocrReplace (#11685) @Saratoga-Official @status102 @Daydreamer114
* 重复添加干员信息 @ABA2396
* YostarJP 干员名 OCR (#11884) @Manicsteiner
* mirror-chyan notify error @MistEO
* missing `user_agent` param for mirrorchyan query @MistEO
* Wpf肉鸽烧水时使用分队UI为空 @status102
* 肉鸽临时招募预备干员时, 不额外提升权重 (#11442) @Daydreamer114
* 公告窗口触控板滚动异常 (#11684) @Rbqwow
* 添加不期而遇新事件空无前兆 (#11573) @DavidWang19
* Attempt retry once screencap for MumuExtras (#11550) @teldd1
* 肉鸽作战编队截图过快导致截图与实际不符 (#11527) @Daydreamer114
* 肉鸽烧水未获得目标奖励逻辑补漏 @status102
* 幸运墙领取奖励界面识别过早 @status102
* 临时处理肉鸽烧水flag异常 @status102
* 肉鸽开局干员使用助战失效 @status102
* fix return value of RecruitImageAnalyzer @horror-proton
* 肉鸽烧水使用分队失效 @status102
* 自动战斗开始战斗时使用理智药检测失效 @status102
* 自动战斗勾选使用理智药时自动编队卡住 @status102
* 招募测试函数修复 (#11723) @Roland125
* border not displaying for http proxy in versionupdatesettings @Constrat
* KR 黑角 OCR (#11794) @Daydreamer114
* JP 塑心 OCR (#11792) @Daydreamer114
* 肉鸽非投资模式禁用种子刷钱 @status102
* 肉鸽烧水分队兼容 @status102
* CheckLevelMax OCR (#11764) @BxFS
* mirrorc package name @MistEO
* 肉鸽开始探索反复重试后结束 @status102
* 修复国际服无法通过文字OCR识别关闭雷电模拟器弹窗的问题 (#11756) @THSLP13
* 狭路相逢事件识别失败 (#11752) @Daydreamer114
* 修复资源检查提示信息错误 @MistEO
* 繁中服「黍」辨識問題 (#11738) @momomochi987
* 繁中服無法進入薩米肉鴿 (#11733) @momomochi987
* 肉鸽开局烧水奖励领取失败 @status102
* YostarJP OCR mismatched parenthesis (#11877) @Alan-Charred
* 月度小队模式不再试图提前离开肉鸽 (#11872) @BxFS
* 肉鸽开局无法选择指挥分队时放弃探索 (#11847) @status102 @Constrat
* 自动编队干员选择错误 @status102
* Wpf自动战斗无法连接到模拟器后不能自动停止 @status102
* Wpf公告内容显示错误显示为上次内容 (#11824) @status102

### 文档 | Docs

* discord link for website docs (#11687) @Constrat @momomochi987 @HX3N @Rbqwow
* Discord link in About Us @Constrat
* 推荐分辨率720P或1080P (#11651) @Rbqwow
* 替换.NET8 桌面运行时下载链接为直链 (#11693) @wryx166
* 肉鸽推荐开局策略 (#11570) @Rbqwow @Constrat
* 更新 CHANGELOG.md @ABA2396
* 补充自动战斗可能遇到的问题 (#11749) @nmsl678 @Daydreamer114 @Rbqwow
* change gamedata repo @Constrat
* 肉鸽参数注释 @status102

### 其他 | Other

* remove wrong commit @status102
* rotate_check 位置错误 @ABA2396
* Resource Check ret 判断错误 @ABA2396
* Roguelike InitialDrop: SquadDefault -> Squad-EnterPoint (#11870) @BxFS
* mirrorchyan toast @MistEO
* 简化肉鸽任务使用助战参数内部存储流程 @status102
* styling @Constrat
* 禁止RoguelikeStatus拷贝 @status102
* 移除账号切换中不必要的任务 (#11820) @status102
* script to update version.json (#11875) @Constrat
* bypass update resources in formatting cases (#11867) @Constrat
* remove mirrorchyan line2 @MistEO
* Mirror酱说明调整 @status102
* NoSkland 放到 wpf 内部 @ABA2396
* 密钥改成 PasswordBox @ABA2396
* remove chinese punctuation from en @Constrat
* 调整肉鸽选择烧水奖励任务链，重新将Roguelike@LastRewardConfirm并入主任务链 (#11689) @status102
* add discord link to main readme @Constrat
* 修改划火柴设置界面布局 (#11682) @Rbqwow
* WpfGui划火柴相关说明调整 @status102
* Revert "perf: 肉鸽优先拿美愿 (#11558)" (#11565) @Daydreamer114
* 繁中服「懷黍離」導航入口更動 (#11662) @momomochi987
* 调整基建办公室阈值 @ABA2396
* 调整 InfrastBottomLeftTab 的 specificRect @ABA2396
* include @status102
* GetAsync catch 未处理 logUri @ABA2396
* 加个 json 解析 catch @ABA2396
* 移除RoguelikeLastRewardSelectTaskPlugin，合并烧水奖励选择 @status102
* 调整界面布局 @ABA2396
* 减少肉鸽插件不必要函数 @status102
* issue_template 的公告链接会导致 mention (#11762) @Daydreamer114
* 简化肉鸽局中数据存储 (#11581) @status102
* 减少肉鸽插件load_params log输出 @status102
* 在 issue_template 中添加链接 (#11751) @Daydreamer114
* 远程控制也添加存储加密 @ABA2396
* 修改 Copilot 界面提示 @ABA2396
* trim mirrorchyan cdk @MistEO
* HttpService调整部分默认值 @status102
* 繁中服「懷黍離」導航入口再更動 (#11732) @momomochi987
* 添加翻译 @ABA2396
* Wpf肉鸽任务界面主题参数调整 @status102
* issue 模板 将阅读提醒提至 label (#11804) @Daydreamer114 @Saratoga-Official
* 启动弹出公告前检查内容是否为空 @ABA2396
* 单独为肉鸽添加日志检查 @MistEO
* 移除过于消耗性能的检查 @MistEO
* remove duplicates in tasks for global @Constrat
* switch gamedata repos for workflow (#11799) @Constrat
* add missing text for YostarKR (#11798) @HX3N
* add missing text for YostarEN @Constrat
* 调整界面 @ABA2396
* 简化干员名正则 (#11876) @Saratoga-Official
* 修改mirrorc线路 @MistEO
* missing strings from zh-cn @Constrat
* 简化干员名正则 @ABA2396
* WpfGui引入AsstTaskType代替硬编码 (#11856) @status102
* MirrorChyan域名 @hguandl
* 修改 MirrorChyan 官网链接 @ABA2396
* issue_template bug-report Version 处添加提示 (#11848) @Daydreamer114
* Wpf肉鸽任务RoguelikeMode参数类型改为int (#11821) @status102
* update mirrorc tips (#11832) @MistEO @ABA2396
* 上调MaaCore Log Rotate阈值为64MB (#11834) @status102
* Wpf公告存储拆分 (#11825) @status102
* 添加翻译 @ABA2396
* 等待延迟前打个日志 @ABA2396
