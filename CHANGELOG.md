## v5.17.0

### ⚠️ 重要提醒

本次更新调整了模板图的目录结构，若您正在从 v5.16.x 及以下版本**手动**更新至该版本，那么**请勿直接覆盖旧版本文件夹**，否则可能导致部分旧文件未被正确删除，从而引发识别问题。

推荐的更新方式有两种：
1. 使用自动更新功能（推荐）：自动更新将会正确清理旧文件。
2. 手动更新时：请将新版本解压到一个全新的文件夹，再将原有的 config 与 data 文件夹复制过去，以保留您的配置。

### 性能大优化～(∠・ω< )⌒★ | Highlight

牛牛这次火速适配了基建的修改呢～同时还给博士们准备了红丝绒活动的全新地图和导航资源哦！

另外呢，牛牛还偷偷优化了自动战斗的流程，给费用和杀敌数识别加了缓存机制，现在抄作业的时候应该不会那么卡顿啦～(´∀｀)♡

最重要的是，牛牛还学会了打扫卫生！会自动清理那些缓存的地图截图，让 MAA 变得更苗条，不会占用博士们太多硬盘空间啦～✨

不过呢，这次牛牛为了赶紧修复一些紧急的小bug，还没来得及适配傀影肉鸽新难度选择呢(┬┬﹏┬┬)但是博士们可以先手动选取难度后再交给牛牛来代理哦～

总之这次更新后，牛牛变得更聪明更节省资源了呢！快来试试看吧～ヾ(≧▽≦*)o

---

以下是详细内容：

### 新增 | New

* SideStory「红丝绒」 @SherkeyXD
* 清理 map 中的 png 类型截图 @ABA2396
* 不允许通过吐司通知启动应用 @ABA2396 @status102
* 截图测试可以在自动检测的情况下使用 @ABA2396
* 自动战斗击杀数缓存, 减少性能消耗 (#12879) @status102
* 自动战斗费用识别缓存, 减少性能消耗 (#12765) @status102
* RA store (#12833) @Daydreamer114
* 繁中服 薩卡茲肉鴿 (#12800) @momomochi987 @pre-commit-ci[bot]
* 繁中服「泰拉飯」活動導航 +「滋味」主題 (#12870) @momomochi987 @pre-commit-ci[bot]

### 改进 | Improved

* MirrorChyan 更新支持判断完整包 @ABA2396 @status102
* 自动战斗使用战斗列表执行单作业时, 增加一条警告; 追加部分提示的i18n (#12832) @status102
* 肉鸽投资达限时, 增加输出提示 (#12818) @status102

### 修复 | Fix

* 基建又双叒叕改了 @ABA2396
* 放弃与驮兽同行，等谁想加识别了再改回来） @ABA2396
* 绿票商店跳过赤金及家具灵茧 @status102
* 界面窗口标题滚动失效 @ABA2396
* 繁中服 薩卡茲肉鴿 月度小隊無法開始探索 (#12871) @momomochi987 @pre-commit-ci[bot]
* EN IS4 Squad Name @Daydreamer114
* WMI 无效类报错 @ABA2396
* Telegram外部通知请求日志uri调整 @status102
* 非部署动作时, 动作前等待结束后不必要的部署区更新可能触发暂停识别, 导致动作滞后 @status102
* 自动编队添加自定义干员时, 点击位置错误 @status102
* KR 驮兽旅行家 OCR @Daydreamer114
* BattleQuickFormationOCR in EN @status102
* global templates are not loaded (#12787) @HX3N
* RA2 Swipe Save @Daydreamer114
* 外服使用 AUTO模式 代理倍率等待战斗结束时间过长 @status102
* 繁中服薩卡茲肉鴿 ocr (#12829) @momomochi987

### 文档 | Docs

* Auto Update Changelogs of v5.17.0-beta.2 (#12886) @github-actions[bot] @status102 @ABA2396
* issue-checkbox-checker 文档 @Daydreamer114
* 移除因勾选"我未仔细阅读"的自动回复 增加文档描述 (#12875) @Lemon-miaow
* `连战次数` -> `代理倍率`; `指定次数` 描述优化 (#12835) @status102 @pre-commit-ci[bot]

### 其他 | Other

* 修复 feature match sence 内无特征点时的死循环问题 @status102
* OperBox Roi @status102
* 我面前站不下这许多人 @AnnAngela
* wpf任务状态并发修改 @status102
* 7e9806e697198e3e50e38a5dd790b294702e233f @status102
* findHomography should have at least 4 points, 优化性能 @MistEO @status102
* WebStages加载临时补救 @status102
* 自动更新无ota包时, 残留旧版本文件 @status102
* Toast死锁 @status102
* 使用系统回收站替代强制删除 @status102
* 无 OTA 更新包更新逻辑 @status102 @ABA2396
* 旧 config 移除简化 @status102
* 特征匹配 ASST_DEBUG 增加匹配点位绘制 @status102
* Log.debug 是否写入判断优化 @status102
* 刷理智任务每次开始行动时的文本优化 @status102
* 新增 JsonDataHelper，抽离 gui.json 中巨大的 data 类型数据到 data 文件夹中 (#12795) @ABA2396
* template 重复检查 @status102
* 更新识别工具逻辑与显示效果 (#12791) @ABA2396
* 模板图允许从子文件夹加载 (#12717) @SherkeyXD @pre-commit-ci[bot]
* FeatureMatcher muplti results @status102 @MistEO
* 日志压缩包打包 cache/resource @ABA2396
* 调整 tasks 移动逻辑 @ABA2396
* 红丝绒 地图 @status102
* blank @status102
* 干员识别存储 (#12881) @status102
* 提高可读性 @status102
* mumu内测路径注释 @status102
* MuMuExtras 5.0+ (#12876) @Daydreamer114
* 特征匹配调试绘制 @status102
* add mirrorchyan url source (#12869) @MistEO @ABA2396
* warning @status102
* 持有费用缓存判定拆分 @status102
* 增加“我未仔细阅读”陷阱选项 (#12864) @Lemon-miaow @Daydreamer114
* LogDebug, LogTrace, LogInfo, LogWarn, LogError补上scope输出 (#12855) @status102
* YostarEN/KR/JP EP navigation @Constrat @HX3N @Manicsteiner
* HttpService日志中uri统一使用UriPartial限制 @status102
* wpf CN 修改描述`连战次数` -> `代理倍率` (#12827) @status102
* 生成时清理template, tasks, global缓存 @status102
* 迁移肉鸽任务部分回调 @status102
* 还原 RA/Fire.json @ABA2396
* 重启函数添加 CallerMemberName，UntilIdleAsync 添加消抖 @ABA2396
* YostarJP roguelike ocr edit (#12789) @Manicsteiner
* bump maa-cli to 0.5.5 (#12664) @wangl-cc
* 防止缓存的任务文件被意外覆盖 @status102
* YostarJP FormationOCR params (#12783) @Manicsteiner
* 调整技能识别保存截图的阈值 @ABA2396
* 干员名识别封装 @status102
* EN tweak @Constrat
* TooltipBlock封装 (#12773) @status102 @ABA2396
