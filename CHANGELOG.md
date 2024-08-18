## v5.6.0-beta.1

### 新增 | New

* 允许选择WPFGUI中的干员名称显示语言 (#10079) @Manicsteiner
* 增加SOCKS5支持 (#10061) @Linisdjxm
* Mac GUI自动编队添加信赖干员 @hguandl
* winapi 关机/睡眠/休眠 @ABA2396
* ci 在 macOS 上使用 clang 18.1.8 (#10254) @Alan-Charred
* 拆分 maskRange，允许禁用数色时闭运算 (#10250) @zzyyyl
* 不義之財 導航更新 (#10249) @XuQingTW
* 数色前先通过闭运算填充小空洞，避免数色的得分过低 @zzyyyl
* 完成后勾选界面添加清空按钮 @ABA2396
* improve recruitment_tool (#10152) @Alan-Charred
* SideStory「太阳甩在身后」二三阶段导航 @SherkeyXD
* YostarEN Reclamation2 (#10143) @zayn7lie
* 肉鸽招募设置 recruitment.json 文件维护工具 (#9700) @Alan-Charred
* 开启mumu增强截图时尝试读取注册表地址并自动填充 (#9076) @octopusYan
* YostarJP Reclamation2 (#10131) @Manicsteiner
* visiting only once a day logic and gui (#10113) @Constrat
* 基建列表增加 全/反选 按钮，列表区域可响应滚动 @ABA2396
* 远程控制支持生息演算2（？ @ABA2396
* 不要在酒吧里点酒吧 @ABA2396
* 肉鸽适配新干员用法 (#10069) @Lancarus

### 改进 | Improved

* 修改 版本更新 界面 @ABA2396
* 更新 README @ABA2396
* 使用 views::join @zzyyyl
* 将生息演算：沙洲遗闻由任务树重构为任务环，以便使用出口、后续重构 (#10223) (#10229) (#10230) @Daydreamer114
* 优化完成后动作仅一次提示文本的显示和描述 (#10182) @status102
* Mumu截图增强启用但未生效时禁止运行，直到关闭增强或配置有效 @status102
* 优化 mumu 注册表路径读取 @ABA2396
* 移除测试代码 @status102
* 为完成后选项追加右键激活仅一次，且与当前选项激活状态同步 @status102
* 合并肉鸽插件中止控制 @status102
* 优化截图增强显示 @ABA2396
* 优化截图测试 @ABA2396
* 更新备选关卡文档 @Rbqwow

### 修复 | Fix

* 截图速度过快时可能卡在公招放弃界面 @status102
* 获得导能元件时可能错误判断是否在战斗中 @ABA2396
* 截图速度过快时可能卡在公招放弃界面 @ABA2396
* 生息演算可能无法退出组装台 @ABA2396
* 国服CharsNameOcrReplace (#10269) @Daydreamer114
* 修复 "-MO-" 会被替换为 "-M0-" 导致的关卡识别错误 (#10263) @zzyyyl
* 国服干员名ocr替换 (#10262) @Daydreamer114
* modified ROI for Phantom EN fix #10244 @Constrat
* 繁中 tasks 格式错误 @ABA2396
* Phantom EN IS encounter fix #10243 @Constrat
* 修复采购中心字被挡住后出错 @zzyyyl
* 傀影肉鸽卡在商店界面 (#10236) @Daydreamer114
* reduce Mizuki IS en select recruit threshold @Constrat
* 日服理智药正在使用数量识别错误 @status102
* 战斗结束掉落过程弹窗 (#10218) @Daydreamer114
* 修复部分设备萨卡兹肉鸽商店 StageTraderEnter 得分过低 @zzyyyl
* delay between Mall and CreditStoreOcr for refresh lag @Constrat
* 在舍弃思绪过程中识别可关闭弹窗 (#10184) @Daydreamer114
* 错误的判断条件 @ABA2396
* YostarKR Reclamation2CopiousCoppice add ocrReplace @HX3N
* 台服目前坍縮值改为當前坍縮值 (#10186) @Saratoga-Official
* 不必要的重置连接 @ABA2396
* YostarJP Reclamation2Ex text @Manicsteiner
* 肉鸽探索失败后执行ClickToStartPointAfterFailed点进讲述者 (#10177) @Daydreamer114
* wrong key in translation en-us @Constrat
* validate filename before using it to avoid crash (#10138) @wangl-cc
* gui_new.json 退出不报错，死锁问题 @ABA2396
* Reclamation2EX Yostar confirmation window @HX3N
* Reclamation2EX YostarKR specific notification @HX3N
* 识别思绪太快导致无法点击驮兽归队 (#10109) @Daydreamer114
* invert bypass daily logic from #10113 @Constrat
* 修改肉鸽编队滑页触底判断 (#9243, #9328) (#10116) @Alan-Charred
* YostarKR CheckCollapsalParadigms @HX3N
* 萨米肉鸽无法正常识别Level name (#10115) @Daydreamer114
* 开启 MuMu 截图增强后重启模拟器未重启 MAA 后出现截图失败 @ABA2396
* Encounter IS4 EN fix #10087 #10090 @Constrat
* 远程控制领取奖励事件报错 @ABA2396
* Stuck after recruit in Trader Stage @Daydreamer114
* 修复幸运墙点击次数不定的问题 @SherkeyXD

### 文档 | Docs

* 修改 README @ABA2396
* 修改 mumu 截图增强模式支持版本 @ABA2396
* typos, fluidity and ML fixes for en-us docs (#10231) @dragonheart107 @Constrat
* 方舟专版现支持截图增强 @Rbqwow
* add minor tip in CN doc and sync EN doc. (#10155) @zkywalker230061 @cid02142311 @Constrat
* link to change for Global RA2 @Constrat

### 其他 | Other

* update maadeps to 2024-08-17 (#10268) @hguandl @dantmnf
* 减少保全截图间隔 @ABA2396
* Revert "fix: 截图速度过快时可能卡在公招放弃界面" @status102
* 移动旧保全文件至old文件夹 @ABA2396
* bump maa-cli to 0.4.11 (#10274) @wangl-cc
* refactor(config) : reorganize settings loading (#10133) @Acture
* add flag `-fexperimental-library` for clang < 16 (#10265) @wangl-cc
* ci "(]"\"" @ABA2396
* 移除不必要的引用 "()" @ABA2396
* Revert "test: ci(123)" @ABA2396
* ci @ABA2396
* Revert "test: test ci(123)" @ABA2396
* test ci @ABA2396
* Revert "test: test ci" @ABA2396
* test ci @ABA2396
* 替換繁中服薩米肉鴿結算截圖 @nifaOwO
* Revert "feat: ci 在 macOS 上使用 clang 18.1.8 (#10254)" @zzyyyl
* YostarJP fix ocr 涙目のペッロー @Manicsteiner
* YostarKR tweak ocrReplace 雨！ @HX3N
* YostarJP reclamation2 continue @Manicsteiner
* 允许在生息演算内任意位置开始任务，进出关卡、跳过天数任务复用 (#10238) @Daydreamer114
* 允许用户在不同位置开始生息演算任务 (#10237) @Daydreamer114
* subtask iteration output tweak @Constrat
* update TaskSorter (#10224) @Alan-Charred
* polish code for collapsal paradigm plugin (#10198) @Alan-Charred
* YostarKR Reclamation2ExClickProduct add ocrReplace @HX3N
* 减少debug下截图数量最大值 @ABA2396
* 生息演算停止任务返回主页面OCR @Daydreamer114
* YostarJP Reclamation2QuitToMainPage @Manicsteiner
* 生息演算停止任务 @ABA2396
* 扩大组装台寻找范围 @ABA2396
* StopGame (#9658) @Rbqwow @ABA2396 @wangl-cc
* MAA webplate manual changes ref: #10178 @Constrat
* Replace Poptip with ToolTip and Add InitialShowDelay for PostActionSetting.Once @ABA2396
* bump maa-cli to 0.4.10 (#10179) @wangl-cc
* 修改自动检测连接后重置连接状态 @ABA2396
* 修改生息演算不同模式的介绍 (#10169) @ABA2396 @HX3N @Constrat
* coding style upgrade (#10157) @zkywalker230061
* add int convertion to release-ota (#10174) @Constrat @pre-commit-ci[bot]
* remove unused templates from ResourceUpdater @Constrat
* 发布 bug issue 时提供 gui.json @Rbqwow
* adjust gpu information log level @dantmnf
* 为肉鸽编队滑页触底判定的代码添加注释 (#10156) @Alan-Charred
* 修改截图测试提示 @ABA2396
* changed config name to make more logical sense @Constrat
* change HS navigation to template because of interface change @Constrat
* TimerProperties/MuMuEmulator12ConnectionExtras Property changed notify @ABA2396
* 修改 mumu 截图增强 日志输出颜色 @ABA2396
* YostarJP Reclamation2 ocr replace (#10132) @Manicsteiner
* EN translate for reclamation @cid02142311
* YostarJP HS navigation update @Manicsteiner
* YostarKR tweak translation @HX3N
* combined AskRestartToApplySettings methods through parameter @Constrat
* reclamation algorithm #2 except txwy @Constrat
* Revert "chore: 肉鸽继续尝试探索但并不直接放弃" @Daydreamer114
* 简化肉鸽刷塌缩范式的日志输出 @status102
* UI 增加缺少的回调 @ABA2396
* Revert "chore: Add OCR for EN Roguelike" (#10101) @Constrat
* Revert "ci: 允许Merge出现在pr中的commit name (#10098)" @status102
* Revert "chore: 提醒用户如gui相关的bug可以跳过配置信息 (#10066)" @status102
* bump maa-cli to 0.4.9 (#10081) @wangl-cc
* 清理跨版本垃圾 @ABA2396
* update ocr text for YostarJP @Manicsteiner
* 1ef2e664f1124ee8404220cafb5e42a6ee331078 for YostarKR @HX3N
* 提醒用户如gui相关的bug可以跳过配置信息 (#10066) @Daydreamer114
