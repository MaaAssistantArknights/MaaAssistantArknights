## v5.4.1

### 新增 | New

* WPF Gui support for roguelike collapsal paradigm task plugin (#9648) @DavidWang19
* 增加 BattlePauseCancel 重试上限 @zzyyyl
* cache 统一默认为 false (#9642) @zzyyyl
* 重新加入 tasks.json 的默认值检查 (#9583) @zzyyyl
* 由ci生成的非正式/公测/内测版判断为调试版本并且不再检查更新 (#9632) @SherkeyXD @zzyyyl
* 隐式空 text 检测仅对无基任务生效 @zzyyyl
* change glossary from markdown to JSON @AlisaAkiron
* update turbo to v2 @AlisaAkiron
* 添加萨米肉鸽天途半道关卡战斗策略 (#9337) @Daydreamer114
* 增加识别坍缩范式的插件，增加萨米肉鸽刷隐藏坍缩范式模式 (#9172) @Alan-Charred
* YoStarEN resolution warning on client switch (#9539) @Constrat @ABA2396

### 改进 | Improved

* 删除 tasks.json 冗余行 (#9645) @zzyyyl
* add log output (#9636) @status102
* 肉鸽资源加载逻辑及目录结构重构 (#9555) @Alan-Charred @SherkeyXD
* 修改win7相关问题描述&更新运行库 @Rbqwow

### 修复 | Fix

* 自动战斗开始时移除召唤物类头像缓存，以避免跨局错误识别相似技能的召唤物 (#9649) @status102
* 繁中服部分角色無法正確辨識 (#9647) @momomochi987
* 修复连接前同步参数时日志报错的问题 (#9644) @zzyyyl
* roi out of bounds @zzyyyl
* 修复萨米肉鸽因插件注册顺序导致的无法运行问题 (#9633) @Alan-Charred
* 修复坍缩范式插件导致的Gui和傀影/水月肉鸽初始化Bug (#9573) @Alan-Charred
* 为InfrastEnteredFlag添加延迟以避免信用通知对右上角基建提醒的遮挡 (#9597) @Viel0320
* allow CMake build MaaCore with ASST_DEBUG under macOS @Alan-Charred
* leak fastdeploy objects to avoid crash @dantmnf
* Roguelike Invest System offset (#9590) @Alan-Charred
* 修正繁中服薩米肉鴿獎勵無法多選一的問題 (#9584) @momomochi987
* add zh-tw glossary for weblate @AlisaAkiron
* merge glossary json into one for better webalte compatibility @AlisaAkiron
* fix typings in Recruit task @horror-proton
* 修复字体和评论区分类 @Rbqwow
* wrong proxy detection @Rbqwow
* switch account failed when MuMuEmulator12Extras is enabled @EvATive7

### 其他 | Other

* link for yostaren resolution [skip changelo] @Constrat
* Translations update from MAA Weblate (#9640) @AlisaAkiron @Rbqwow
* Style/gui consistency (#9567) @Constrat
* YostarJP add SSSBuffChoose (#9629) @Manicsteiner
* fix Weblate status preview in README (#9616) @Lemon-miaow
* Update ja-jp.json @wallsman
* 删除 Qodana 静态检查 (#9553) @SherkeyXD
* 增加 ResourceUpdater 日志 @ABA2396
* pref: 将Head和ETag请求设置为`Connection: close` @ChingCdesu
* Translations update from MAA Weblate (#9598) @AlisaAkiron
* YostarKR ocr fix and remove SkipThePreBattlePlot @HX3N
* YostarJP cor fix and roguelike shop text (#9591) @Manicsteiner
* update python API (#9538) @EvATive7
* fix a bad link @Rbqwow
* make i18n warnings show only once @Rbqwow
* YoStar -> Yostar only for interfaces (#9548) @Constrat
* 增加关于肉鸽任务刷探索范式功能的说明 (#9552) @Alan-Charred
* website docs **translation needed** (#9287) @ABA2396 @HX3N @wallsman @Constrat @SherkeyXD @Rbqwow
