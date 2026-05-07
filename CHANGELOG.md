## v6.9.3

### 许愿墙前挂满愿签，牛牛顺手把更新器、作战流程和海外服体验都打磨了一遍 | Highlights

这一版的重点仍然是把日常使用体验打磨得更顺手、更稳定。我们一边补齐周年活动与海外服适配，一边继续清理那些会打断长时间挂机流程的细碎问题。

#### 启动流程与更新可视化

MAA 现在会在重复启动时直接唤起已经打开的主窗口，不再额外弹出警告打断操作。与此同时，更新器补上了暗色模式、进度条与控制台输出，避免后台更新时因看不到进度而被重复打开，进一步降低更新流程出问题的概率。

#### 自动作战更稳，也更懂得何时停下

这一版为关卡未解锁、剿灭未启用全权代理等场景补上了提前停止检查，也支持保存代理指挥记录，并在合成玉 0 掉落时自动结束任务。针对 5.1 七周年版本更新后理智上限调整带来的影响，本次还修复了理智汇报与代理倍率选择异常；此前生息演算容易卡住的问题，同样也是由 5.1 更新后的变更引起，现在也已一并处理。

技能识别准确率也在这一版中大幅提升。默认情况下，MAA 会在 debug/skill_ready 下仅保留最近 50 次技能识别结果；如果你在自动战斗中遇到技能识别错误，欢迎及时携带日志和截图反馈。若根目录下存在 DEBUG_skill_ready.txt，则会无限保存相关截图；如果你愿意帮助我们训练技能识别模型，可以创建该文件，将错误分类的文件放入 对应标签_err 文件夹后发送至 uye[at]maa-org.net。

#### 七周年与海外服内容继续补齐

本次更新适配了七周年许愿墙、“重构”界面主题与争锋频道「绿藤城」，也补齐了 Yostar JP、EN、KR 小游戏相关的 SPA、资源与模板优化，让活动期间和海外服环境下的使用体验更完整。

----

### Highlights

This release continues to focus on making everyday use smoother and more reliable. Alongside support for anniversary content and overseas servers, we kept cleaning up the small but disruptive issues that can break long unattended runs.

#### Better Launch Flow and Update Visibility

MAA now re-activates the existing main window on repeated launches instead of interrupting users with an extra warning dialog. The updater also gains dark mode, a progress bar, and console output, helping users avoid reopening it while an update is still running in the background without visible feedback.

#### More Reliable Automation, with Smarter Stop Conditions

This version adds early-stop checks for locked stages and annihilation runs without proxy enabled, and it can now save auto-command records while ending runs automatically when 0 Orundum is detected. It also fixes incorrect sanity reporting and proxy multiplier selection caused by the sanity cap changes introduced in the 5.1 anniversary update. The Reclamation Algorithm freeze issue was caused by the same 5.1 update and has been fixed as well.

Skill recognition accuracy has also been significantly improved. By default, MAA keeps only the most recent 50 skill-recognition results under debug/skill_ready. If you encounter incorrect skill recognition during auto-battle, please report it together with the relevant logs and screenshots. If DEBUG_skill_ready.txt exists in the root directory, related screenshots will be saved without limit. If you would like to help train the skill-recognition model, you can create that file, move misclassified files into the corresponding label_err folder, and send them to uye[at]maa-org.net.

#### Continued Anniversary and Overseas Server Coverage

This update adapts to the 7th anniversary wishing wall, the “Reclamation” interface theme, and the Green Vine channel in SSS. It also rounds out Yostar JP, EN, and KR minigame SPA support together with related resource and template optimizations, making event-period and overseas-server usage more complete.

----

以下是详细内容：

## v6.9.3

### 新增 | New

* 优先使用更新包中的 updater @ABA2396
* 添加因为缺少 MAA.Updater.exe 导致更新失败的弹窗提示 @ABA2396
* 更新 153-4 基建作业 @ABA2396

### 改进 | Improved

* 自动战斗结束增加LoadingIcon等待项 @status102

### 修复 | Fix

* Analyzer执行前未检查 m_roi 是否未空 @status102
* 修复小游戏界面的开始按钮在连接模拟器失败时仍然发送开始信号 @ABA2396
* 自动战斗进入等待过长 @status102
* OF-1 战斗后等待过长导致部分后续流程失败 @status102
* 修复部分成就判断条件错误 @ABA2396

### 文档 | Docs

* update JP preview image (#16485) @Manicsteiner
* 更新 README 预览图片 @ABA2396

### 其他 | Other

* local-install 使用 ci 同款处理方法 @ABA2396

## v6.9.2

### 改进 | Improved

* 剿灭入口检测支持代理卡耗尽情况 @status102

### 修复 | Fix

* 开始唤醒流程 @status102

### 文档 | Docs

* 添加 DLL 注入问题解决方案并优化 FAQ 描述 (#16404) @ocsin1

## v6.9.1

### 新增 | New

* 新增完整包更新时的强制提示，安装在根目录或部分特殊目录根目录时禁止启动与更新 (#16435) @ABA2396

### 改进 | Improved

* 支持沙盘战斗结束识别，重构战斗失败识别逻辑 (#16449) @status102
* 修正应用宝连接时额外的 ClientType 设置 @status102 @ABA2396
* 优化自动编队在 Elite 图标匹配失败或无精英化干员时的判定 @status102 @ABA2396
* 临期理智药到期时间 OCR 与处理逻辑进一步优化，遇到时效识别失败时会取消本次吃药 @status102

### 修复 | Fix

* 修复部分场景下无法自动启动游戏，以及开始唤醒后过早切换账号的问题 (#16422) @1b2c @status102
* 修复临期理智药库存位数、日期前缀与剩余时间输出等识别问题，并移除不再生效的日服 ROI 覆盖 @status102
* 修复绿票商店稳定性问题，并调整信用商店识别区域以支持 4 位数信用识别 (#16369) @Roland125 @ABA2396 @ZiyinLin
* 修复剿灭入口图片资源与结算合成玉基线识别问题 (#16458) (#16460) @Roland125
* 修复凯尔希与 GALLUS² 识别问题 @Saratoga-Official
* 修复设置指引中连接设置“每次重新检测”提示块隐藏错误 @ABA2396
* 修复自动战斗多作业模式导航 retry_time 异常 @status102
* 修复公招计数返回值，适配更精细的招募计数 (#16355) (#16371) @Roland125
* 修复若干正确性问题（含肉鸽投资存款校验、pixel analyzer 灰度阈值、rect 越界裁剪、肉鸽招募时间解析、密文板识别等） @status102 @FireflySentinel

### 文档 | Docs

* 补全连接阶段的 ClientType 参数说明 @ABA2396

### 其他 | Other

* PC 端连接跳过数据上报 @ABA2396

## v6.9.0

### 新增 | New

* 重复启动时通过跨进程事件激活主窗口，替代弹窗警告提示 (#16363) @ABA2396
* 新增关卡未解锁代理或剿灭未启用全权代理时的错误停止检查 (#16357) @ABA2396
* 新增保存代理指挥记录功能，并支持合成玉掉落检查，0 掉落时自动结束任务 (#16356) @Roland125
* 掉落物识别额外输出剿灭进度信息 @status102
* 适配“重构”界面主题 (#16349) @SherkeyXD
* 支持腾讯应用宝 5.10.56.xx (#16292) @srdr0p
* 新增 updater 暗色模式支持 @ABA2396
* 新增 updater 进度条与控制台输出支持 @ABA2396
* 支持 PC 端 `完成后退出明日方舟` (#16351) @glimmertouch
* 新增争锋频道「绿藤城」支持 (#16345) @Daydreamer114
* 适配七周年许愿墙 @Copilot
* YostarJP/EN/KR MiniGame SPA  @Manicsteiner @Constrat @HX3N

### 改进 | Improved

* 统一 CheckComboBox 样式 @ABA2396
* 大幅提升技能就绪识别准确率，优化技能截图保存策略 (#16393) @ABA2396
* 合并并简化任务状态逻辑 @status102
* 统一 ProcessTask 匹配命中状态更新逻辑 @status102
* 优化 1 星词条选项操作描述 @status102
* 理智上限提升至 210 @status102
* 调整完整后 `无其他 MAA` 选项绑定逻辑，不再强制勾选退出模拟器 @ABA2396
* 调整选项 `IsEnabled` 逻辑 @ABA2396
* 使用 PC 端连接方式时，自动禁用不支持的完成后操作 @ABA2396
* optimize templates yostarkr SPA @Constrat

### 修复 | Fix

* 修复 3 星 tag 标题显示异常 @ABA2396
* 修复 CheckComboBox 不显示标题的问题 @ABA2396
* 修复生息演算 5.1 更新后无法使用无存档刷分的问题 (#16402) @ABA2396
* 修复剿灭后出现升级界面导致任务无法继续的问题 (#16255) (#16370) @Roland125
* 修复干员仓储识别中升变阿米娅的识别问题 @status102
* 修复若干正确性问题（含 null check / race / clamp / retry 等 9 处） (#16332) @FireflySentinel
* 修复干员仓储识别未跳过数据查找失败干员的问题 @status102
* 修复 EN IS6 trail 正则匹配问题 @Constrat
* 修复月度小队隐藏分队选择的问题 @SherkeyXD
* 修复小游戏列表中下拉框偶现无法展开的问题 @ABA2396
* 修复蓝叠模拟器关闭失败的问题 (#16388) @lengyanyu258
* 升级结算时闪退 @ABA2396
* lower YoStarJP office mini threshold (#16390) @Rememorio

### 文档 | Docs

* 更新部分代码注释 (#16215) @JasonHuang79

### 其他 | Other

* 任务排序支持 JSONC @ABA2396
* 调整资源更新环境 @ABA2396
* 修改 issue template @ABA2396
* 更新周年月卡相关文本 @SherkeyXD
