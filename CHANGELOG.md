## v5.5.0

### 新增 | New

* change penguin CN mirror domain to alvorna.com (#9959) @GalvinGao
* 适配萨卡兹肉鸽 (#9677) @DavidWang19 @SherkeyXD @Daydreamer114 @Lemon-miaow @Alan-Charred
* Mac GUI 添加萨米肉鸽策略 @hguandl
* WPF Gui support for roguelike collapsal paradigm task plugin (#9648) @DavidWang19
* 添加萨米肉鸽天途半道关卡战斗策略 (#9337) @Daydreamer114
* 增加识别坍缩范式的插件，增加萨米肉鸽刷隐藏坍缩范式模式 (#9172) @Alan-Charred
* 肉鸽五层 BOSS 前暂停功能 (#9801) @SherkeyXD @Lancarus @DavidWang19
* MultiTemplMatcher (#9850) @Alan-Charred
* 隐藏托盘图标选项 (#9819) @Icexbb
* 添加几个肉鸽作业 (#9854, #9868) @Saratoga-Official
* Mac GUI 关闭游戏任务 @hguandl
* Mac GUI 肉鸽凹精二和生息演算设置 @hguandl
* 投资模式增加非开局干员直接招募第一位 (#9803) @Lancarus
* 萨卡兹肉鸽自动印象重建 (#9789) @Lancarus
* 添加思绪数据 @SherkeyXD
* Mac GUI 支持萨卡兹肉鸽、开始/停止任务快捷键 @hguandl

### 改进 | Improved

* Wpf肉鸽非刷等级模式时，隐藏第五层BOSS前停下设置 @status102
* Mumu截图增强设置增加测试按钮 (#9881) @status102 @ABA2396
* 优化夺路而跳部署策略 (#9726) @Daydreamer114
* update Sarkaz shopping.json (#9721) @CASUUU @SherkeyXD
* 自动战斗-战斗列表 批量导入关卡名正则优化 (#9723) @status102
* 优化第一层作战策略黑名单 (#9710) @Daydreamer114
* 适配新干员用法&调整萨卡兹肉鸽4星优先度 (#9713) @Lancarus
* 优化战斗策略应对天灾年代之刺 (#9702) @Daydreamer114
* 非萨米肉鸽时禁用仅萨米用插件 (#9888) @status102
* 非萨米肉鸽时禁用仅萨米用插件 (#9880) @status102
* 让我也康康今天又更新了啥 (#9879) @AnnAngela
* 存款满后禁用肉鸽投资插件 @status102
* 优化萨卡兹肉鸽部分策略 (#9871) @Lancarus
* 萨卡兹肉鸽藏品排序.json (#9761) @CASUUU
* 修改开始战斗时的理智药输出分隔符 @status102
* 蓝图队速刷肉鸽 (#9773) @DavidWang19 @Lancarus @status102 @pre-commit-ci[bot]
* 优化不期而遇策略 @SherkeyXD
* 优化弹窗判断逻辑 @ABA2396
* 优化萨卡兹肉鸽公害部署策略 (#9766) @Daydreamer114 @pre-commit-ci[bot]
* 优化萨卡兹肉鸽安全检查部署策略 (#9759) @Daydreamer114
* 优化萨卡兹肉鸽拆东补西部署策略 (#9744) @HYY1116
* 优化萨卡兹肉鸽安全检查部署策略 (#9739) @Daydreamer114

### 修复 | Fix

* 印象加深后卡住时截图 @zzyyyl
* 肉鸽部分范围点击任务点进萨卡兹肉鸽思绪界面 (#9953) @Daydreamer114
* 萨卡兹肉鸽命运所指关卡识别使用 HSVCount 避免达不到阈值 @zzyyyl
* 更新 先行一步 & 失与得 的掩码范围以增加得分 @zzyyyl
* 萨卡兹肉鸽部分关卡识别使用 HSVCount 避免达不到阈值 (#9931) @zzyyyl
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
* 自动战斗开始时移除召唤物类头像缓存，以避免跨局错误识别相似技能的召唤物 (#9649) @status102
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
* 完成制造通知弹出时，无法正确识别到所有宿舍 @ABA2396
* 尝试修复萨卡兹肉鸽刷新节点次数耗尽卡死的问题 @zzyyyl
* 尝试修复萨卡兹肉鸽刷新节点次数耗尽卡死的问题 @zzyyyl
* prettified res-update workflow @Constrat
* res-updt bypasses branch protection @Constrat
* 修复印象加深后卡住的问题 @zzyyyl
* 修复萨卡兹肉鸽刷新节点次数耗尽卡死的问题 @zzyyyl
* 调整主界面多个掉落物识别阈值 @DavidWang19
* 修复萨卡兹肉鸽的自动战斗文件 拆东补西.json 的语法问题 (#9903) @SST-03
* 调整刷新按钮阈值 @DavidWang19
* 合成思绪出现藏品二选一时卡住 (#9883) @Lancarus
* 增加肉鸽分队存储及判断，避免蓝图队影响其他肉鸽存钱 @Lancarus
* 修复 docsearch 路径错误的问题 @SherkeyXD
* 肉鸽卡在干员技能面板 (#9843) @Lancarus
* 肉鸽高级设置-投资模式启用购物选项说明补充：进入2层 @status102
* 未开启自定义基建可能遇到的除数为0 @ABA2396
* 修复基建排版表数量为为0时引起的崩溃问题 @SherkeyXD
* 修正文档网页右上角search组件本地化显示问题 (#9804) @sevmeowple
* 修复更换不同班次数的基建排班表可能遇到的数组越界 @ABA2396
* 调整iOS萨卡兹肉鸽节点刷新模板识别阈值 @hguandl
* 错误获取到萨米塌缩插件 (#9827) @status102
* 修复肉鸽插件重复注册 (#9817) @status102
* 更改开局选上一局奖励的顺序 @DavidWang19
* 修复拿取掉落物时卡死的问题 @DavidWang19
* 修复卡死在先行一步/驮兽归队等问题 @DavidWang19
* 修复离开不期而遇后点进负荷界面的问题 @DavidWang19
* 修复当蓝图队为第一个分队时无法识别的问题 @DavidWang19
* 修复紧急作战刷新次数达到上限后卡死的问题 @DavidWang19
* switch downloading artifact order @Constrat
* 修复 methods 为空时直接崩溃的问题 @zzyyyl
* 扩大肉鸽投资检测阈值，减少识别错误 (#9787) @status102
* Revert "fix: 修复多次设置参数时肉鸽插件重复注册 (#9786)" @zzyyyl
* hsv 匹配时不要修改原图 @zzyyyl
* 修复多次设置参数时肉鸽插件重复注册 (#9786) @status102
* 修复萨卡兹肉鸽无法识别思绪阻滞 (#9767) @Daydreamer114
* 修复 pre-commit ci @zzyyyl
* 修复肉鸽主题选择的导航问题 (#9756) @Lemon-miaow
* 修复萨卡兹肉鸽卡死在思绪界面 (#9746) @Daydreamer114
* 萨卡兹肉鸽重复使用思绪导致卡死 (#9738) @Daydreamer114
* 修复萨卡兹肉鸽去伪存真印象重建后卡死 @zzyyyl
* macOS CI Xcode版本 @hguandl
* top operator unrecognized recruitment @Constrat
* wrong variable format i'm dumb @Constrat
* 修复 StrategyChange 时 next 丢失的问题 @zzyyyl
* fix zip filename encoding issue @horror-proton

### 文档 | Docs

* 本地加载所有图片 @Rbqwow
* update glossary @HX3N
* 修改Task协议文档，以符合cache默认值 @status102
* CHANGELOG中文档修改独立分类 @SherkeyXD
* fix Weblate status preview in README (#9616) @Lemon-miaow
* fix a bad link @Rbqwow
* make i18n warnings show only once @Rbqwow
* 增加关于肉鸽任务刷探索范式功能的说明 (#9552) @Alan-Charred
* 添加 adb-lite 说明 @Rbqwow
* KR 添加 MAA 格式化要求说明 @HX3N
* 添加 MAA 格式化要求说明 @SherkeyXD

### 其他 | Other

* 修复连接前同步参数时日志报错的问题 (#9644) @zzyyyl
* roi out of bounds @zzyyyl
* test data @ABA2396
* 肉鸽插件set_params补充 (#9893) @status102
* set_params语义一致化 @status102
* compile error @zzyyyl
* 使用 oxipng 优化模板图片 @zzyyyl
* parallelism for resource updater action (#9736) @Constrat
* do the name and cropping direction feat with the CropRoi tool (#9679) @zayn7lie
* prvent double consecutive runs @Constrat
* 更新 DebugTask 方便模板匹配测试 @zzyyyl
* 肉鸽Config部分返回改为const ref，移除部分变量 (#9741) @status102
* cache and targeting for prettier res update game (#9777) @Constrat
* 更新 OptimizeTemplates 使得图像优化的结果必定通过 pre-commit-ci 的检查 @zzyyyl
* 优化资源更新流程 (#9635) @zzyyyl
* 拆分肉鸽数据 (#9841) @status102
* 迁移部分肉鸽数据 @status102
* 减少肉鸽Config拷贝 @status102
* 简化肉鸽Config和部分设置 (#9822) @status102
* macOS编译脚本支持ccache @hguandl
* 优化macOS编译脚本 @hguandl
* 优化模板图片 @zzyyyl
* 避免空 method 警告 @zzyyyl
* 删除 tasks.json 冗余行 (#9645) @zzyyyl
* add log output (#9636) @status102
* 肉鸽资源加载逻辑及目录结构重构 (#9555) @Alan-Charred @SherkeyXD
* 修改win7相关问题描述&更新运行库 @Rbqwow
* oxipng @zzyyyl
* 简单处理 DEBUG 的时候 OCR 很卡的问题 @zzyyyl
* 使用 oxipng 优化模板图片 @zzyyyl
* 更新 mask_range 生成工具 @zzyyyl
* 简化RoguelikeTask set_params @status102
* 修改 set_params 以避免语义不一致 @status102
* 迁移肉鸽投资参数设置 @status102
* 复用RoguelikeControl停止流程 @status102
* find_plugin 追加 const 修饰 @status102
* WpfGui添加对坍缩范式插件callback的nullable check (#9848) @Alan-Charred
* 优化所有肉鸽文件的json排版 @SherkeyXD
* 更新 mask-range 工具 @zzyyyl
* Caching for ResourceUpdater @Constrat
* 优化格式 @ABA2396
* 复用RoguelikeCollapsalParadigmTaskPlugin插件 (#9682) @status102
* 优化更新策略 @ABA2396
* 更新部分配置修改为全局配置 @ABA2396
* 优化版本更新 @ABA2396
* 更新checkout至v4 这怎么还有个漏网之鱼） @SherkeyXD
* 简化RoguelikeTask set_param (#9905) @status102
* implement actionsx/prettier for resource updater (#9949) @Constrat
* 更新 DebugTask 方便模板匹配测试 @zzyyyl
* RGBCount 和 HSVCount 改为 数色 和 模板匹配 的几何平均；优化 count 的实现方法 @zzyyyl
* allow cache overwriting @Constrat
* feat!: mask_range 支持彩色掩码 (#9818) @zzyyyl
* 删除 HashTaskInfo 相关 (#9724) @zzyyyl
* 肉鸽插件增加每局重置，以免全部堆积到Config (#9828) @status102
* numbers only in TimerSettings @Constrat
* 支持自定义 MatchTemplate 匹配方法 (#9785) @zzyyyl
* 禁止插件修改自身 enable @zzyyyl
* 使用 set_params + set_enable 管理插件而不是延迟注册插件 @zzyyyl
* feat!: 允许插件Verify期间自变更enable，以禁用非预期的启用 (#9876) @status102
* close inactive issues (#9866) @Constrat @zzyyyl @pre-commit-ci[bot]
* 肉鸽插件添加设置参数功能，设置失败的插件会被自动disable (#9862) @Alan-Charred
* 插件 verify 前先判断 enable @zzyyyl
* feat!: 增加图像匹配算法 RGBCount, HSVCount (#9795) @zzyyyl
* 增加特定情况下检查到 roi 为全屏的警告 (#9674) @pre-commit-ci[bot] @zzyyyl
* first implementation (#9927) @Constrat
* optimize templates in resource updater @Constrat
* 增加 BattlePauseCancel 重试上限 @zzyyyl
* cache 统一默认为 false (#9642) @zzyyyl
* 重新加入 tasks.json 的默认值检查 (#9583) @zzyyyl
* 由ci生成的非正式/公测/内测版判断为调试版本并且不再检查更新 (#9632) @SherkeyXD @zzyyyl
* 隐式空 text 检测仅对无基任务生效 @zzyyyl
* change glossary from markdown to JSON @AlisaAkiron
* update turbo to v2 @AlisaAkiron
* 删去 CcoeffHSV, HSVCount 时的模板匹配采用 RGB 的 Ccoeff @zzyyyl
* RGBCount 和 HSVCount 改为 数色 和 模板匹配 的算术平均 @zzyyyl
* 数色算法现在改为原来的结果与 ccoeff 结果的点积 @zzyyyl
* 萨卡兹肉鸽混乱或阻滞时先使用一个再舍弃 (#9727) @Daydreamer114
* 萨卡兹肉鸽添加混乱与阻滞时舍弃思维功能 (#9711) @Daydreamer114
* 修改弹窗尺寸 @ABA2396
* 肉鸽继续尝试探索但并不直接放弃 @ABA2396
* add res-update-game-unix.yml @Constrat
* only check github and github raw links (#9855) @wangl-cc
* Add requirements.txt for MaskRangeTool @zzyyyl
* 整理 mask_range 工具 @zzyyyl
* remove stale cache for ci (#9935) @SherkeyXD
* remove testing image @zzyyyl
* 调整 DebugTask 测试模板匹配时的输出信息 @zzyyyl
* generate_mask_range 增加用于比较两图的函数 @zzyyyl
* OD navigation (#9729) @Manicsteiner
* 添加弹窗提示 @ABA2396
* ignore more site (#9698) @wangl-cc
* use lychee to check dead links (#9675) @wangl-cc
* bump `maa-cli` to 0.4.8 and update documents (#9683) @wangl-cc
* update ja-jp.xaml @Manicsteiner
* 萨米肉鸽配置文件/插件单独文件夹 (#9651) @Alan-Charred
* make AskRestartToApplySettingsYoStarEN static @ABA2396
* 统一肉鸽模式介绍文字 (#9669) @Alan-Charred
* Style/gui consistency (#9567) @Constrat
* Update ja-jp.json @wallsman
* 删除 Qodana 静态检查 (#9553) @SherkeyXD
* 增加 ResourceUpdater 日志 @ABA2396
* pref: 将Head和ETag请求设置为`Connection: close` @ChingCdesu
* update python API (#9538) @EvATive7
* website docs **translation needed** (#9287) @ABA2396 @wallsman @SherkeyXD @HX3N @Rbqwow @Constrat @wangl-cc
* rollback @vuepress/plugin-docsearch version @SherkeyXD
* cleaned up comments + alias @Constrat
* git diff output formatting @Constrat
* use github-actions[bot] credentials @Constrat
* GenerateMaskRange 可以直接展示 mask_range 掩码后的图片 @zzyyyl
* do not close enhancement issue @Rbqwow
* dont close stale issue too late (#9899) @AnnAngela
* 肉鸽坍缩范式插件代码整理 (#9833) @Alan-Charred
* revert: 移除find_plugin @status102
* Revert "feat!: 允许插件Verify期间自变更enable，以禁用非预期的启用 (#9876)" @zzyyyl
* dont close issue too early (#9887) @AnnAngela
* Manual Resource Update @Constrat
* bump github actions versions (#9859) @dependabot[bot] @Constrat
* dependabot gh actions automation (#9836) @Constrat
* update-game-resources 时再输出 user 信息 @zzyyyl
* Translations update from MAA Weblate (#9845) @AlisaAkiron
* clang-format 格式修改 (#9586) @zzyyyl
* format @status102
* tweaks @Constrat
* 完善pr-checker的提示 @SherkeyXD
* markdown-checker exclude <https://ark.yituliu.cn/> @zzyyyl
* 有 core 标签不打 ambiguous @zzyyyl
* issue-bot 增加关键词 (#9771) @Rbqwow
* avoid generate package.json & package-lock.json while res-update-game @zzyyyl
* Add prettier in res-update-game @zzyyyl
* Add prettier in res-update-game @zzyyyl
* Update tools/OptimizeTemplates/optimize_templates.json @zzyyyl
* 删去 pre-commit-ci 中 oxipng 的 --alpha 参数 @zzyyyl
* Update .prettierignore @zzyyyl
* Update .prettierignore @zzyyyl
* OptimizeTemplates 脚本改为使用 oxipng @zzyyyl
* bump maatouch to v1.1.0 (#9749) @Manicsteiner
* 使用 pre-commit-ci 来格式化与压缩文件 (#9732) @SherkeyXD
* Revert "feat(ci): allow cache overwriting" @Constrat
* remove unused include + style @Constrat
* add testing image @Constrat
* remove testing image @Constrat
* add testing image @Constrat
* change name action @Constrat
* install oxipng @Constrat
* remove testing png @Constrat
* fix docsearch base @SherkeyXD
* auto optimize-templates.yml @Constrat
* add requirements.txt for optimize-templates.py @Constrat
* optimize templates perf and fixes @Constrat
* Revert "ci: optimize templates perf and fixes" (#9929) @Constrat
* optimize templates perf and fixes (#9928) @Constrat

### For Overseas

* Global SSS#4 copilot adaptation @Constrat
* tweak Reed Alter, Noir Corne alter regex @Constrat
* SSS#4 Automaton Arena @Constrat
* Global SSS#4 copilot (Translation work in progress) @Constrat
* Translations update from MAA Weblate (#9640) @AlisaAkiron @Rbqwow
* YoStar -> Yostar only for interfaces (#9548) @Constrat

#### txwy

* 補充繁中服保全派駐#3 相關內容 (#9701) @momomochi987
* 繁中服「綠野幻夢」復刻活動導航 (#9913) @momomochi987
* 修正繁中服薩米肉鴿獎勵無法多選一的問題 (#9584) @momomochi987
* 繁中服部分角色無法正確辨識 (#9647) @momomochi987

#### YoStarEN

* YoStarEN resolution warning on client switch (#9539) @Constrat @ABA2396
* YostarEN SSS#4 buffs and branches @Constrat

#### YoStarJP

* YostarJP HS navigation (#9958) @Manicsteiner
* YostarJP WB navigation (#9715) @Manicsteiner
* YostarJP add SSSBuffChoose (#9629) @Manicsteiner
* YostarJP cor fix and roguelike shop text (#9591) @Manicsteiner

#### YoStarKR

* YostarKR WB navigation @HX3N
* YostarKR add SSSBuffChoose @HX3N
* YostarKR ocr fix and remove SkipThePreBattlePlot @HX3N
