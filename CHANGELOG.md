## v6.1.0

### 新增 | New

* 极寒保全派驻作业 @Saratoga-Official
* Wpf日志文件输出分级 @status102
* YostarJP Roguelike JieGarden (#15116) @Manicsteiner
* YostarKR Roguelike JieGarden (#15113) @HX3N
* 满线索再一键置入 @ABA2396
* 支持点击标题栏左上角的检测到新版本提示触发更新 @ABA2396
* 右键图标菜单栏中增加日志悬浮窗切换 @ABA2396
* 悬浮窗支持自动战斗日志源 @ABA2396
* YostarEN SecretFront (Hidden Front) @Constrat
* 日志悬浮窗 (#15185) @ABA2396
* 在任务开始后的禁用组件内也能查看tooltip (#15186) @yali-hzy
* ProcessTask任务命中缓存结果 (#12651) @status102
* add ability to remove unused templates (#15207) @Constrat
* 更新期间退出应用添加二次确认 (#14964) @Hakuin123 @Hakuin123 @ABA2396
* YostarJP SecretFront edit (#15191) @Manicsteiner

### 改进 | Improved

* optimize templates size @Constrat
* use boost::regex instead of std::regex (#15126) @MistEO
* TemplResource 查找图片过程预构建索引 (#15092) @status102
* 让截图工具支持从src/获取的截图也缩放到目标分辨率 @DavidWang19
* 优化 StableHash @ABA2396
* RefreshCustomInfrastPlanIndexByPeriod 支持传入当前时间 @ABA2396
* optimize templates for secrentfront @Constrat
* reoptimize ALL templates from scratch @Constrat
* 优化悬浮窗布局 @ABA2396

### 修复 | Fix

* regex off all double letters for EN coppers @Constrat
* EN requires GetDropSwitch2 @Constrat
* remove case sensitivity from CoppersNameOcrReplace @Constrat
* Ruwu Gate ROI is too big and detects floor number (IV) @Constrat
* use template instead of reduced score @Constrat
* FastPass for EN  ref be08678 @Constrat
* 资源更新不应该复制 cache 文件夹 @ABA2396
* adb addressRegex (#15142) @HX3N
* 资源版本显示标题 @hguandl
* YostarKR StrategyChange_mode-FastPass @HX3N
* change tongbao name OCR to fit oversea (#15129) @travellerse @Constrat
* 更新后连带清除过往未下载成功的OTA包 @status102
* 修复 MacOS asst.log 自动清空的问题 (#15122) @Alan-Charred
* PlaatformWin32 for ResourceUpdater on windows @Constrat
* 去除路径左右的空格与控制字符 (#15082) @mayuri0v0
* regex for Leizi Alter for EN @Constrat
* 赠送线索后的弹窗会挡住自己新线索的图标 @ABA2396
* 繁中服 自動編隊不會使用職業欄 (#15090) @vonnoq
* YostarKR update Roguelike@TraderRandomShoppingConfirm @HX3N
* prettier @Constrat
* YostarKR remove problematic equivalence_classes @HX3N
* 增加巫恋的绣云鹤皮肤头像 @DavidWang19
* 关卡名中的 `-` 被识别成 `—` @ABA2396
* 基建使用无人机加速贸易站点击 `最多` 按钮边缘时可能点到 `+` @ABA2396
* 移除过时的基建Mode转换 @status102
* 自动战斗借非好友助战在关卡结束后卡在加好友界面 @ABA2396
* 逃离哥伦比亚 @ABA2396
* more copper regex for EN @Constrat
* update GetDropSwitch for EN to make it same size as CN for ROI Pickup reasons @Constrat
* 完善通宝识别失败时的分支处理 (#15180) @travellerse
* more tongbao EN regex @Constrat
* SendClues for txwy (#15178) @momomochi987
* 通宝识别失败时放弃通宝 (#15167) @travellerse @HX3N
* more Coppers EN regexes @Constrat
* missing Special Squad for EN @Constrat
* more regex for CoppersNameOcrReplace EN @Constrat
* SendClues for Yostar server (#15168) @HX3N
* Missing GetDropSelectRecruit for EN @Constrat
* missing CoppersAbandonExchange for EN @Constrat
* tonbgbao regex for EN @Constrat
* 避免肉鸽快速编队点太快可能点不上 @Saratoga-Official
* 肉鸽因模拟器卡顿可能点进招募界面 @Saratoga-Official
* allow usage of CLI build instead of only VS (#15190) @Constrat
* 萨米积冰岩骸识别 @Saratoga-Official
* 保全关卡名识别 @ABA2396
* 两个检测更新的按钮在更新时禁用 @ABA2396
* 未勾选自动下载更新包且无可用的 OTA 增量更新包时仍然提示“将下载完整包xxx” @ABA2396
* more tongbaso regex en @Constrat
* 单切换账号时，任务运行计时错误 @status102
* 招募助战后补充低信赖干员数量不足 (#15184) @yali-hzy
* BattleStageName requiring ^$  for EN @Constrat
* wrong template for EN @Constrat
* 部分情况下无法进入借助战界面 @ABA2396
* LoadingText 结束后 UI 延迟变化导致误匹配 (#15198) @status102
* roi out of range (#15204) @178619
* sendclue reception standardize templates (#15205) @Constrat
* 前往上一次作战增加重试 @ABA2396
* 修复通宝拾取时卡死并删除m_retry_times=1 (#15197) @travellerse
* 基建自定义配置迁移加个try @status102
* 保全自动战斗日志悬浮窗 @ABA2396
* 保全派驻因网络波动可能无法点击阵容确认 @ABA2396
* 未设置自定义基建排班路径时第一次启动会报错 @ABA2396
* 自定义基建计划指定某个计划后会在1分钟后被重置为定时计划 (#14649) @status102 @HX3N @Constrat

### 文档 | Docs

* Update CHANGELOG for version 6.0.1 @ABA2396
* Auto Update Changelogs of v6.1.0-beta.2 (#15135) @github-actions[bot] @github-actions[bot] @Constrat
* Auto Update Changelogs of v6.1.0-beta.3 (#15195) @github-actions[bot] @github-actions[bot] @Constrat
* Auto Update Changelogs of v6.1.0-beta.4 (#15209) @github-actions[bot] @github-actions[bot] @ABA2396

### 其他 | Other

* Release v6.1.0-beta.4 (#15208) @ABA2396
* Release v6.1.0-beta.3 (#15188) @Constrat
* Release v6.1.0-beta.2 (#15172) @Constrat
* Release v6.1.0-beta.1 (#15134) @Constrat
* YostarJP roguelike JieGarden ocr edits (#15162) @Manicsteiner
* YostarKR JieGarden ocr edit @HX3N
* KR tweak translation for IS nodes, achievements and general UI @HX3N
* EN tweak @Constrat
* various tweaks to IS nodes and stages @Constrat
* YostarJP roguelike JieGarden edits (#15153) @Manicsteiner
* 調整繁中服薩米與薩卡茲肉鴿的 OCR 辨識對應文字 (#15145) @momomochi987
* YostarJP JieGarden ocr edit @Manicsteiner
* KR tweak EncounterOptions translation @HX3N
* YostarKR wrap short OcrReplaces with anchors to prevent misrecognition @HX3N
* enable Roguelike JieGarden for Yostar servers @HX3N
* 调整正则 @ABA2396
* KR 同心 ocr @HX3N
* 繁中服保全派駐 8 清音安保派駐 (#15110) @momomochi987
* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* YostarKR JieGarden Encounter ocr @HX3N
* 更新 macos.cmake (#15173) @Alan-Charred
* KR tweak InvestmentReach @HX3N
* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* EN typo @Constrat
* remove codeql workflow @SherkeyXD
* 资源更新忽略 .gitignore 文件 @ABA2396
* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* Update C# EditorConfig for c# 13 & 14 (#15146) @status102
* KR improve client display names @HX3N
* 更新 pre-commit 配置 @SherkeyXD
* 移除 isort/black 配置 @SherkeyXD
* update pre-commit toolchain (#15179) @SherkeyXD
* devcontainer 迁移至 ruff @SherkeyXD
* 修复 roi updater 工具的问题 @SherkeyXD
* 悬浮窗移动时不隐藏，避免某些窗口点击就会触发闪烁（说的就是 QQ） @ABA2396
