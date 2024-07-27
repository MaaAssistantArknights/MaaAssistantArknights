## v5.5.0-beta.2

### 新增 | New

* first implementation (#9927) @Constrat
* 印象加深后卡住时截图 @zzyyyl
* optimize templates in resource updater @Constrat
* 肉鸽五层 BOSS 前暂停功能 (#9801) @SherkeyXD @Lancarus @DavidWang19
* 禁止插件修改自身 enable @zzyyyl
* MultiTemplMatcher (#9850) @Alan-Charred
* 使用 set_params + set_enable 管理插件而不是延迟注册插件 @zzyyyl
* 隐藏托盘图标选项 (#9819) @Icexbb
* feat!: 允许插件Verify期间自变更enable，以禁用非预期的启用 (#9876) @status102
* close inactive issues (#9866) @Constrat @zzyyyl @pre-commit-ci[bot]
* 添加并修改三层boss肉鸽作业 (#9868) @Saratoga-Official
* 肉鸽插件添加设置参数功能，设置失败的插件会被自动disable (#9862) @Alan-Charred
* 插件 verify 前先判断 enable @zzyyyl
* feat!: 增加图像匹配算法 RGBCount, HSVCount (#9795) @zzyyyl
* 增加特定情况下检查到 roi 为全屏的警告 (#9674) @pre-commit-ci[bot] @zzyyyl
* 添加几个肉鸽作业 (#9854) @Saratoga-Official
* AbstractTask新增附加已有插件、插件查找 (#9816) @status102
* feat!: mask_range 支持彩色掩码 (#9818) @zzyyyl
* 删除 HashTaskInfo 相关 (#9724) @zzyyyl
* 肉鸽插件增加每局重置，以免全部堆积到Config (#9828) @status102
* Mac GUI 关闭游戏任务 @hguandl
* Mac GUI 肉鸽凹精二和生息演算设置 @hguandl
* numbers only in TimerSettings @Constrat
* 投资模式增加非开局干员直接招募第一位 (#9803) @Lancarus
* 萨卡兹肉鸽自动印象重建 (#9789) @Lancarus
* 支持自定义 MatchTemplate 匹配方法 (#9785) @zzyyyl
* 添加思绪数据 @SherkeyXD
* Mac GUI 支持萨卡兹肉鸽、开始/停止任务快捷键 @hguandl
* allow cache overwriting @Constrat

### 改进 | Improved

* prvent double consecutive runs @Constrat
* 更新 DebugTask 方便模板匹配测试 @zzyyyl
* oxipng @zzyyyl
* 简单处理 DEBUG 的时候 OCR 很卡的问题 @zzyyyl
* 使用 oxipng 优化模板图片 @zzyyyl
* 更新 mask_range 生成工具 @zzyyyl
* 简化RoguelikeTask set_params @status102
* 修改 set_params 以避免语义不一致 @status102
* 迁移肉鸽投资参数设置 @status102
* 非萨米肉鸽时禁用仅萨米用插件 (#9888) @status102
* 复用RoguelikeControl停止流程 @status102
* find_plugin 追加 const 修饰 @status102
* WpfGui添加对坍缩范式插件callback的nullable check (#9848) @Alan-Charred
* 非萨米肉鸽时禁用仅萨米用插件 (#9880) @status102
* 让我也康康今天又更新了啥 (#9879) @AnnAngela
* 存款满后禁用肉鸽投资插件 @status102
* 优化萨卡兹肉鸽部分策略 (#9871) @Lancarus
* 萨卡兹肉鸽藏品排序.json (#9761) @CASUUU
* 优化资源更新流程 (#9635) @zzyyyl
* 拆分肉鸽数据 (#9841) @status102
* 修改开始战斗时的理智药输出分隔符 @status102
* 迁移部分肉鸽数据 @status102
* 减少肉鸽Config拷贝 @status102
* 简化肉鸽Config和部分设置 (#9822) @status102
* macOS编译脚本支持ccache @hguandl
* 优化macOS编译脚本 @hguandl
* 优化模板图片 @zzyyyl
* 避免空 method 警告 @zzyyyl
* 蓝图队速刷肉鸽 (#9773) @DavidWang19 @Lancarus @status102 @pre-commit-ci[bot]
* 肉鸽Config部分返回改为const ref，移除部分变量 (#9741) @status102
* cache and targeting for prettier res update game (#9777) @Constrat
* 更新 OptimizeTemplates 使得图像优化的结果必定通过 pre-commit-ci 的检查 @zzyyyl
* 优化不期而遇策略 @SherkeyXD
* 优化弹窗判断逻辑 @ABA2396
* 优化萨卡兹肉鸽公害部署策略 (#9766) @Daydreamer114 @pre-commit-ci[bot]
* 优化萨卡兹肉鸽安全检查部署策略 (#9759) @Daydreamer114
* 使用 oxipng 优化模板图片 @zzyyyl
* 优化萨卡兹肉鸽拆东补西部署策略 (#9744) @HYY1116
* 优化萨卡兹肉鸽安全检查部署策略 (#9739) @Daydreamer114
* parallelism for resource updater action (#9736) @Constrat
* do the name and cropping direction feat with the CropRoi tool (#9679) @zayn7lie

### 修复 | Fix

* 修复萨卡兹肉鸽去伪存真印象重建后卡死 @zzyyyl
* macOS CI Xcode版本 @hguandl
* top operator unrecognized recruitment @Constrat
* wrong variable format i'm dumb @Constrat
* 修复 StrategyChange 时 next 丢失的问题 @zzyyyl
* fix zip filename encoding issue @horror-proton
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
* 肉鸽插件set_params补充 (#9893) @status102
* set_params语义一致化 @status102
* compile error @zzyyyl
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

### 文档 | Docs

* Auto Update Changelogs of v5.5.0-beta.2 (#9926) @github-actions[bot] @ABA2396 @status102
* 添加 adb-lite 说明 @Rbqwow
* KR 添加 MAA 格式化要求说明 @HX3N
* 添加 MAA 格式化要求说明 @SherkeyXD

### 其他 | Other

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
* Release v5.5.0-beta.2 (#9922) @ABA2396
* rollback @vuepress/plugin-docsearch version close #9835 @SherkeyXD
* cleaned up comments + alias @Constrat
* git diff output formatting @Constrat
* use github-actions[bot] credentials @Constrat
* GenerateMaskRange 可以直接展示 mask_range 掩码后的图片 @zzyyyl
* 繁中服「綠野幻夢」復刻活動導航 (#9913) @momomochi987
* do not close enhancement issue @Rbqwow
* dont close stale issue too late (#9899) @AnnAngela
* 肉鸽坍缩范式插件代码整理 (#9833) @Alan-Charred
* revert: 移除find_plugin @status102
* Revert "feat!: 允许插件Verify期间自变更enable，以禁用非预期的启用 (#9876)" @zzyyyl
* dont close issue too early (#9887) @AnnAngela
* Manual Resource Update @Constrat
* dependabot back to default [skip changelo] @Constrat
* bump github actions versions (#9859) @dependabot[bot] @Constrat
* dependabot gh actions automation (#9836) @Constrat
* update-game-resources 时再输出 user 信息 @zzyyyl
* Translations update from MAA Weblate (#9845) @AlisaAkiron
* clang-format 格式修改 (#9586) @zzyyyl
* format @status102
* tweaks @Constrat
* 完善pr-checker的提示 @SherkeyXD
* markdown-checker exclude https://ark.yituliu.cn/ @zzyyyl
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
