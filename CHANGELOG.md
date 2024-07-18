## v5.5.0-beta.1

### 新增 | New

* 适配萨卡兹肉鸽 (#9677) @DavidWang19 @Daydreamer114 @Alan-Charred @SherkeyXD @Lemon-miaow
* Mac GUI 添加萨米肉鸽策略 @hguandl
* WPF Gui support for roguelike collapsal paradigm task plugin (#9648) @DavidWang19
* 由ci生成的非正式/公测/内测版判断为调试版本并且不再检查更新 (#9632) @SherkeyXD @zzyyyl
* 隐式空 text 检测仅对无基任务生效 @zzyyyl
* change glossary from markdown to JSON @AlisaAkiron
* update turbo to v2 @AlisaAkiron
* 添加萨米肉鸽天途半道关卡战斗策略 (#9337) @Daydreamer114
* 增加识别坍缩范式的插件，增加萨米肉鸽刷隐藏坍缩范式模式 (#9172) @Alan-Charred

### 改进 | Improved

* 增加 BattlePauseCancel 重试上限 @zzyyyl
* cache 统一默认为 false (#9642) @zzyyyl
* 重新加入 tasks.json 的默认值检查 (#9583) @zzyyyl
* Caching for ResourceUpdater @Constrat
* 优化夺路而跳部署策略 (#9726) @Daydreamer114
* update Sarkaz shopping.json (#9721) @CASUUU @SherkeyXD
* 自动战斗-战斗列表 批量导入关卡名正则优化 (#9723) @status102
* 优化第一层作战策略黑名单 (#9710) @Daydreamer114
* 适配新干员用法&调整萨卡兹肉鸽4星优先度 (#9713) @Lancarus
* 优化所有肉鸽文件的json排版 @SherkeyXD
* 优化战斗策略应对天灾年代之刺 (#9702) @Daydreamer114
* 优化格式 @ABA2396
* 复用RoguelikeCollapsalParadigmTaskPlugin插件 (#9682) @status102
* 优化更新策略 @ABA2396
* 更新部分配置修改为全局配置 @ABA2396
* 优化版本更新 @ABA2396
* 更新checkout至v4 这怎么还有个漏网之鱼） @SherkeyXD
* 删除 tasks.json 冗余行 (#9645) @zzyyyl
* add log output (#9636) @status102
* 肉鸽资源加载逻辑及目录结构重构 (#9555) @Alan-Charred @SherkeyXD
* 修改win7相关问题描述&更新运行库 @Rbqwow

### 修复 | Fix

* 尝试修复肉鸽第四层开头卡死的问题 @DavidWang19
* 修复 萨卡兹肉鸽 LastReward5 少前缀的问题 @zzyyyl
* 萨卡兹肉鸽战斗结束领奖励时因思绪异常而卡死 (#9731) @Daydreamer114 @DavidWang19
* 增加猩红甬道的OcrReplace @DavidWang19
* 修复肉鸽每层识别 @DavidWang19
* 调整失与得阈值 @DavidWang19
* 修复萨卡兹肉鸽卡死在结算界面的问题 @DavidWang19
* 修复萨卡兹肉鸽商店不能刷新的问题 @DavidWang19
* 修复肉鸽关卡通用策略无法识别费用的问题 @DavidWang19
* 删除萨卡兹肉鸽开局负荷干员与舍弃思绪功能模板匹配文件名重复描述 (#9722) @Daydreamer114
* 调整树洞阈值 @DavidWang19
* 萨卡兹肉鸽舍弃思绪识别位置修改 (#9717) @Daydreamer114
* 调整节点阈值 @DavidWang19
* 修复卡死在紧急安全检查的问题 @DavidWang19
* 修复萨卡兹肉鸽卡死在战斗失败的问题 @DavidWang19
* 修改材料名称注释 @ABA2396
* 修复被随机排序破坏的超链接 @Rbqwow
* tweak Reed Alter, Noir Corne alter regex @Constrat
* 自动战斗开始时移除召唤物类头像缓存，以避免跨局错误识别相似技能的召唤物 (#9649) @status102
* 修复连接前同步参数时日志报错的问题 (#9644) @zzyyyl
* roi out of bounds @zzyyyl
* 修复萨米肉鸽因插件注册顺序导致的无法运行问题 (#9633) @Alan-Charred
* 修复坍缩范式插件导致的Gui和傀影/水月肉鸽初始化Bug (#9573) @Alan-Charred
* 为InfrastEnteredFlag添加延迟以避免信用通知对右上角基建提醒的遮挡 (#9597) @Viel0320
* allow CMake build MaaCore with ASST_DEBUG under macOS @Alan-Charred
* leak fastdeploy objects to avoid crash @dantmnf
* Roguelike Invest System offset (#9590) @Alan-Charred
* add zh-tw glossary for weblate @AlisaAkiron
* merge glossary json into one for better webalte compatibility @AlisaAkiron
* fix typings in Recruit task @horror-proton
* 修复字体和评论区分类 @Rbqwow
* wrong proxy detection @Rbqwow
* switch account failed when MuMuEmulator12Extras is enabled @EvATive7

### 文档 | Docs

* update glossary @HX3N
* 修改Task协议文档，以符合cache默认值 @status102
* CHANGELOG中文档修改独立分类 @SherkeyXD
* fix Weblate status preview in README (#9616) @Lemon-miaow
* fix a bad link @Rbqwow
* make i18n warnings show only once @Rbqwow
* 增加关于肉鸽任务刷探索范式功能的说明 (#9552) @Alan-Charred
* website docs **translation needed** (#9287) @ABA2396 @HX3N @Constrat @wallsman @Rbqwow @wangl-cc @SherkeyXD

### 其他 | Other

* OD navigation (#9729) @Manicsteiner
* 添加弹窗提示 @ABA2396
* ignore more site (#9698) @wangl-cc
* use lychee to check dead links (#9675) @wangl-cc
* bump `maa-cli` to 0.4.8 and update documents (#9683) @wangl-cc
* update ja-jp.xaml @Manicsteiner
* 萨米肉鸽配置文件/插件单独文件夹 (#9651) @Alan-Charred
* make AskRestartToApplySettingsYoStarEN static @ABA2396
* 统一肉鸽模式介绍文字 (#9669) @Alan-Charred
* link for yostaren resolution [skip changelo] @Constrat
* Translations update from MAA Weblate (#9640) @AlisaAkiron @Rbqwow
* Style/gui consistency (#9567) @Constrat
* 删除 Qodana 静态检查 (#9553) @SherkeyXD
* 增加 ResourceUpdater 日志 @ABA2396
* pref: 将Head和ETag请求设置为`Connection: close` @ChingCdesu
* Translations update from MAA Weblate (#9598) @AlisaAkiron
* update python API (#9538) @EvATive7

### For Overseas

* YoStar -> Yostar only for interfaces (#9548) @Constrat
* SSS#4 Automaton Arena @Constrat
* Global SSS#4 copilot (Translation work in progress) @Constrat

#### txwy

* 繁中服部分角色無法正確辨識 (#9647) @momomochi987
* 補充繁中服保全派駐#3 相關內容 (#9701) @momomochi987
* 修正繁中服薩米肉鴿獎勵無法多選一的問題 (#9584) @momomochi987

#### YostarEN

* Global SSS#4 copilot adaptation @Constrat
* YoStarEN resolution warning on client switch (#9539) @Constrat @ABA2396
* YostarEN SSS#4 buffs and branches @Constrat

#### YostarJP

* YostarJP WB navigation (#9715) @Manicsteiner
* YostarJP add SSSBuffChoose (#9629) @Manicsteiner
* Update ja-jp.json @wallsman
* YostarJP cor fix and roguelike shop text (#9591) @Manicsteiner

#### YostarKR

* YostarKR add SSSBuffChoose @HX3N
* YostarKR WB navigation @HX3N
* YostarKR ocr fix and remove SkipThePreBattlePlot @HX3N
