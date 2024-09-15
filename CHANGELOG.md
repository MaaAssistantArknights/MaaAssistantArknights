## v5.6.0

### 新增 | New

* SideStory「泰拉饭」 @ABA2396
* Mac GUI 适配重构后的生息演算任务(#10425) @hguandl
* 外部通知支持多选 (#10395) @ABA2396
* 允许手动指定WPFGUI中干员名称显示语言 (#10310) @ABA2396 @Manicsteiner
* 生息演算添加沙中之火选择项 @ABA2396
* 适配「词祭」界面主题 (#10331) @Constrat @ManicSteiner @HX3N @SherkeyXD
* 允许选择 WPFGUI 中的干员名称显示语言 (#10079) @Manicsteiner
* winapi 关机/睡眠/休眠 @ABA2396
* 不義之財 導航更新 (#10249) @XuQingTW
* SideStory「太阳甩在身后」二三阶段导航 @SherkeyXD
* 开启mumu增强截图时尝试读取注册表地址并自动填充 (#9076) @octopusYan
* 基建列表增加 全/反选 按钮，列表区域可响应滚动 @ABA2396
* 远程控制支持生息演算2 @ABA2396
* 肉鸽干员用法添加莎草、娜仁图亚、衡沙 (#10069) @Lancarus
* Mac GUI自动编队添加信赖干员 @hguandl
* date format language based (#10424) @Constrat @ABA2396 @status102
* YostarKR siege theme (#10483) @HX3N
* YostarJP siege theme (#10485) @Manicsteiner
* YostarEN CR navigation @Constrat
* add Qmsg notification (#10358) @octopusYan
* GetLocalizedNames for Infrast and Copilot output (#10335) @Constrat
* Reclamation for YostarJP (#10414) @Manicsteiner
* visiting only once a day logic and gui (#10113) @Constrat
* YostarJP Reclamation2 (#10131) @Manicsteiner
* YostarEN Reclamation2 (#10143) @zayn7lie

### 改进 | Improved

* 重构CheckAfterCompleted，添加睡眠 (#10458) @status102 @ABA2396
* 优化资源版本号、UI版本号、Core版本号在外文及其他情况下的显示 @status102
* 更新 mask-range 工具 @zzyyyl
* 全肉鸽招募适配玛露西尔 (#10442) @Daydreamer114
* 优化自动编队日志输出 @ABA2396
* 优化生息演算 (#10411) @Alan-Charred @status102 @ABA2396
* 全肉鸽招募适配娜仁图亚、艾拉 (#10385) @Daydreamer114
* Mumu截图增强路径清空时不再检查路径是否存在 @status102
* 优化外部通知界面显示 (#10363) @ABA2396
* 更新 bug issue 模板 (#10357) @Rbqwow
* 重构 OperBox 输出与显示 (#10320) @ABA2396
* 重构定时器和重启询问 (#10078) @ABA2396
* Win10以上系统在退出时Wpf不再清除Toast (#10307) @status102
* 第一次启动时默认不勾选肉鸽和生息演算 @ABA2396
* 修改 版本更新 界面 @ABA2396
* 将生息演算：沙洲遗闻由任务树重构为任务环，以便使用出口、后续重构 (#10223) (#10229) (#10230) @Daydreamer114
* 完成后动作界面添加清空按钮 @ABA2396
* 优化完成后动作仅一次提示文本的显示和描述 (#10182) @status102
* 为完成后选项追加右键激活仅一次，且与当前选项激活状态同步 @status102
* Mumu截图增强启用但未生效时禁止运行，直到关闭增强或配置有效 @status102
* duplicates templates from I.S. (#10376) @Constrat

### 修复 | Fix

* CharsNameOcrReplace添加"宝箱的肉？" (#10459) @Daydreamer114
* 界面更新后悖论模拟无法开始战斗 (#10439) @ABA2396
* 生息演算无法循环 @ABA2396
* 允许吃源石保存状态在任务完成时仍会重置状态 @ABA2396
* 识别不到基建入口时会在其他界面滑动导致误触 @ABA2396
* sarkaz 仓库识别错误 @ABA2396
* 生息演算主题读取配置错误 @ABA2396
* 萨卡兹肉鸽多选招募券模板错误 @ABA2396
* 肉鸽编队检测在未触底时返回 true (#10396) @Alan-Charred
* DoDragDrop 拖动操作已在进行中 (#10368) @ABA2396
* 使用匹配后偏移代替每日任务 @status102
* 勾选启动MAA后直接最小化后点击隐藏托盘图标后无法显示MAA @ABA2396
* SL 导航错误 @ABA2396
* 修复调试版本判断条件 @SherkeyXD
* 多配置下公告和更新日志显示异常 @ABA2396
* 修复保全战斗在core干员重复时只会放1次bug (#10306) @status102
* ProxyType 重启不生效 @ABA2396
* 截图速度过快时可能卡在公招放弃界面 @status102 @ABA2396
* 获得导能元件时可能错误判断是否在战斗中 @ABA2396
* 生息演算可能无法退出组装台 @ABA2396
* 修复 "-MO-" 会被替换为 "-M0-" 导致的关卡识别错误 (#10263) @zzyyyl
* 国服干员名ocr替换:夕、华法琳、伊芙利特、提丰 (#10262、#10269) @Daydreamer114
* 修复采购中心字被挡住后出错 @zzyyyl
* 傀影肉鸽卡在商店界面 (#10236) @Daydreamer114
* 日服理智药正在使用数量识别错误 @status102
* 战斗结束掉落过程弹窗 (#10218) @Daydreamer114
* 修复部分设备萨卡兹肉鸽商店 StageTraderEnter 得分过低 @zzyyyl
* delay between Mall and CreditStoreOcr for refresh lag @Constrat
* 在舍弃思绪过程中识别可关闭弹窗 (#10184) @Daydreamer114
* 不必要的重置连接 @ABA2396
* 肉鸽探索失败后执行ClickToStartPointAfterFailed点进讲述者 (#10177) @Daydreamer114
* 识别思绪太快导致无法点击驮兽归队 (#10109) @Daydreamer114
* 修改肉鸽编队滑页触底判断 (#9243, #9328) (#10116) @Alan-Charred
* 萨米肉鸽无法正常识别Level name (#10115) @Daydreamer114
* 开启 MuMu 截图增强后重启模拟器未重启 MAA 后出现截图失败 @ABA2396
* 远程控制领取奖励事件报错 @ABA2396
* 修复幸运墙点击次数不定的问题 @SherkeyXD
* 台服目前坍縮值改为當前坍縮值 (#10186) @Saratoga-Official
* FC rerun navigation fix EN @Constrat
* insert delay after SquadConfirm @Constrat
* add ocrReplace for JP "Reclamation2CopiousCoppice" (#10362) @Daydreamer114
* add delay after selecting clue @Constrat
* EN needs templates for clue exchange the number font is different, score too low @Constrat
* invert bypass daily logic from #10113 @Constrat
* Stuck after recruit in Trader Stage @Daydreamer114
* YostarKR Reclamation2CopiousCoppice add ocrReplace @HX3N
* reduce Mizuki IS en select recruit threshold @Constrat
* modified ROI for Phantom EN fix #10244 @Constrat
* Phantom EN IS encounter fix #10243 @Constrat
* Encounter IS4 EN fix #10087 #10090 @Constrat
* YostarJP Reclamation2Ex text @Manicsteiner
* YostarKR CheckCollapsalParadigms @HX3N
* Reclamation2EX Yostar confirmation window @HX3N
* Reclamation2EX YostarKR specific notification @HX3N


### 文档 | Docs

* 添加缺少的 HSV 文档 (#10445) @ABA2396
* 添加缺失的生息演算参数 (#10446) @ABA2396
* 贡献者头像添加 105 上限 (#10351) @MistEO
* 生息演算2用户界面提醒 (#10281) @Daydreamer114
* 更新备选关卡文档 @Rbqwow
* 修改 README @ABA2396
* 修改 mumu 截图增强模式支持版本 @ABA2396
* 方舟专版现支持截图增强 @Rbqwow
* add minor tip in CN doc and sync EN doc. (#10155) @zkywalker230061 @cid02142311 @Constrat
* link to change for Global RA2 @Constrat
* typos, fluidity and ML fixes for en-us docs (#10231) @dragonheart107 @Constrat

### 其他 | Other

* 更新 bug issue 模板 (#10475) @Daydreamer114
* 更新 issue-checker (#10434) @AnnAngela
* 移除错误的基建作业 (#10457) @status102
* 查找肉鸽招募中未提及的干员 (#9865) @Manicsteiner
* 截图延迟过大时可能会关闭可关闭技能 @ABA2396
* 启动时删除多余文件 @ABA2396
* roi 错误 @ABA2396
* 生息演算2刷开局清空编队干员 (#10359) @Daydreamer114
* 重构 FightSettingsUserControl (#10407) @ABA2396
* 优化界面显示 @ABA2396
* smoking-test中肉鸽参数更新 @SherkeyXD
* 使用变换后的图像进行技能按钮识别 (#10293) @horror-proton
* OTA打包时对跳过的版本做删除处理 (#10020) @SherkeyXD
* 公招错误时保存截图 @zzyyyl
* 调用PowerManagement.Shutdown();后再次调用Bootstrapper.Shutdown(); @ABA2396
* 关机前尝试保存配置 @ABA2396
* 调整令牌关闭强度 @ABA2396
* 迁移公告相关配置 (#10399) @status102
* 调整 check link 提示样式 @ABA2396
* 对comment中的未知链接进行提醒 (#10379) @IzakyL @ABA2396
* 获取任务端口无效时不进行轮询 (#10321) @ABA2396
* 删除子模块 @ABA2396
* 公招识别拥有全干员时不显示未拥有干员数量 @ABA2396
* 修改过时的Binding方法 @SherkeyXD
* 整理 tasks.json 中记录的肉鸽插件参数 (#10290) @Alan-Charred
* MuMu12EmulatorPath Placeholder 添加示例提示 @ABA2396
* smoking-test添加领取奖励的测试 @SherkeyXD
* 移除tasks中的默认值 @SherkeyXD
* 去除干员 OCR 识别结果中的前导下划线 (#10280) @Constrat @wangl-cc
* 肉鸽招募设置 recruitment.json 文件维护工具 (#9700) @Alan-Charred
* 优化 mumu 注册表路径读取 @ABA2396
* 合并肉鸽插件中止控制 @status102
* 优化截图增强显示 @ABA2396
* 优化截图测试 @ABA2396
* 繁中 tasks 格式错误 @ABA2396
* gui_new.json 退出不保存，死锁问题 @ABA2396
* 使用 views::join @zzyyyl
* 不要在酒吧里点酒吧 @ABA2396
* 代理增加SOCKS5支持 (#10061) @Linisdjxm
* 拆分 maskRange，允许禁用数色时闭运算 (#10250) @zzyyyl
* 数色前先通过闭运算填充小空洞，避免数色的得分过低 @zzyyyl
* 减少保全截图间隔 @ABA2396
* 移动旧保全文件至old文件夹 @ABA2396
* 替換繁中服薩米肉鴿結算截圖 @nifaOwO
* 允许在生息演算内任意位置开始任务，进出关卡、跳过天数任务复用 (#10238) @Daydreamer114
* 允许用户在不同位置开始生息演算任务 (#10237) @Daydreamer114
* 减少debug下截图数量最大值 @ABA2396
* 生息演算停止任务返回主页面OCR @Daydreamer114
* YostarJP Reclamation2QuitToMainPage @Manicsteiner
* 生息演算停止任务 @ABA2396
* 扩大组装台寻找范围 @ABA2396
* 修改自动检测连接后重置连接状态 @ABA2396
* 修改生息演算不同模式的介绍 (#10169) @ABA2396 @HX3N @Constrat
* 发布 bug issue 时提供 gui.json @Rbqwow
* 为肉鸽编队滑页触底判定的代码添加注释 (#10156) @Alan-Charred
* 修改截图测试提示 @ABA2396
* 修改 mumu 截图增强 日志输出颜色 @ABA2396
* 简化肉鸽刷塌缩范式的日志输出 @status102
* UI 增加缺少的回调 @ABA2396
* 清理跨版本垃圾 @ABA2396
* Revert "chore: 肉鸽继续尝试探索但并不直接放弃" @Daydreamer114
* Revert "chore: Add OCR for EN Roguelike" (#10101) @Constrat
* Revert "ci: 允许Merge出现在pr中的commit name (#10098)" @status102
* Mac GUI StopGame (#10347) @hguandl
* update ignore list after RA refactor @Constrat
* YostarJP CR navigation @Manicsteiner
* YostarKR CR stage navigation @HX3N
* `std::ranges::views::join` with LLVM clang 16 on darwin  (#10309) @Cryolitia
* impossiblity of fetch-depth modification. reverting + generic perfs @Constrat
* rev-list instead of rev-parse @Constrat
* revert to simple if @Constrat
* fetching depth 0 @Constrat
* remove "" in nightly fix #10308 @Constrat
* CopilotViewModel (#10099) @Manicsteiner
* git blame ignore @Constrat
* bump maa-cli to 0.4.12 (#10390) @wangl-cc
* update ignore templates @Constrat
* use CsWin32 source generator instead of random pinvoke library (#10361) @dantmnf
* remove MaaDeps submodule (#10354) @dantmnf
* RoguelikeRoutingTaskPlugin.h missing VS22 filter @Constrat
* bump zzyyyl/issue-checker from 1.8 to 1.9 @zzyyyl
* YostarJP ocr fix @Manicsteiner
* JP ZH-TW GPU option & reclamation translation @Manicsteiner
* KR GpuDeprecated translation @HX3N
* fix WPF Warning @SherkeyXD
* YostarJP FC navigation (#10316) @Manicsteiner
* clearout git blame @Constrat
* remove last checked commit @Constrat
* auto blame ignore @github-actions[bot]
* git blame added styling commits (#10283) @Constrat
* remove unneccesary Grid.Column fields @Constrat
* english tweak @Constrat
* wrong key in translation en-us @Constrat
* validate filename before using it to avoid crash (#10138) @wangl-cc
* improve recruitment_tool (#10152) @Alan-Charred
* update maadeps to 2024-08-17 (#10268) @hguandl @dantmnf
* bump maa-cli to 0.4.11 (#10274) @wangl-cc
* refactor(config) : reorganize settings loading (#10133) @Acture
* add flag `-fexperimental-library` for clang < 16 (#10265) @wangl-cc
* YostarJP fix ocr 涙目のペッロー @Manicsteiner
* YostarKR tweak ocrReplace 雨！ @HX3N
* YostarJP reclamation2 continue @Manicsteiner
* subtask iteration output tweak @Constrat
* update TaskSorter (#10224) @Alan-Charred
* polish code for collapsal paradigm plugin (#10198) @Alan-Charred
* YostarKR Reclamation2ExClickProduct add ocrReplace @HX3N
* StopGame (#9658) @Rbqwow @ABA2396 @wangl-cc
* MAA webplate manual changes ref: #10178 @Constrat
* Replace Poptip with ToolTip and Add InitialShowDelay for PostActionSetting.Once @ABA2396
* bump maa-cli to 0.4.10 (#10179) @wangl-cc
* coding style upgrade (#10157) @zkywalker230061
* add int convertion to release-ota (#10174) @Constrat @pre-commit-ci[bot]
* remove unused templates from ResourceUpdater @Constrat
* adjust gpu information log level @dantmnf
* changed config name to make more logical sense @Constrat
* change HS navigation to template because of interface change @Constrat
* TimerProperties/MuMuEmulator12ConnectionExtras Property changed notify @ABA2396
* YostarJP Reclamation2 ocr replace (#10132) @Manicsteiner
* EN translate for reclamation @cid02142311
* YostarJP HS navigation update @Manicsteiner
* YostarKR tweak translation @HX3N
* combined AskRestartToApplySettings methods through parameter @Constrat
* reclamation algorithm #2 except txwy @Constrat
* bump maa-cli to 0.4.9 (#10081) @wangl-cc
* update ocr text for YostarJP @Manicsteiner
* 1ef2e664f1124ee8404220cafb5e42a6ee331078 for YostarKR @HX3N
