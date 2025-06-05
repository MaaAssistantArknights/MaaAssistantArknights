## v5.17.0-beta.2

### 新增 | New

* 自动战斗费用击杀数缓存, 减少性能消耗 (#12879) @status102
* 繁中服「泰拉飯」活動導航 +「滋味」主題 (#12870) @momomochi987
* YostarEN/YostarKR/YostarJP EP navigation @Constrat @HX3N @Manicsteiner
* RA store (#12833) @Daydreamer114
* 清理 map 中的 png 类型截图 @ABA2396

### 改进 | Improved

* 刷理智任务每次开始行动时的文本优化 @status102
* 自动战斗使用战斗列表执行单作业时, 增加一条警告; 追加部分提示的i18n (#12832) @status102
* 肉鸽投资达限时, 增加输出提示 (#12818) @status102

### 修复 | Fix

* 放弃与驮兽同行，等谁想加识别了再改回来） @ABA2396
* 绿票商店跳过赤金及家具灵茧 @status102
* 界面窗口标题滚动失效 @ABA2396
* WMI 无效类报错 @ABA2396
* Telegram外部通知请求日志uri调整 @status102
* 非部署动作时, 动作前等待结束后不必要的部署区更新可能触发暂停识别, 导致动作滞后 @status102
* 繁中服薩卡茲肉鴿 ocr (#12829) @momomochi987
* 繁中服 薩卡茲肉鴿 月度小隊無法開始探索 (#12871) @momomochi987
* EN IS4 Squad Name @Daydreamer114
* RA2 Swipe Save @Daydreamer114
* 外服使用 AUTO模式 代理倍率等待战斗结束时间过长 @status102

### 文档 | Docs

* `连战次数` -> `代理倍率`; `指定次数` 描述优化 (#12835) @status102

### 其他 | Other

* 旧config移除简化 @status102
* 特征匹配ASST_DEBUG增加匹配点位绘制 @status102
* Log.debug是否写入判断优化 @status102
* 修复feature match sence内无特征点时的死循环问题 @status102
* wpf任务状态并发修改 @status102
* 7e9806e697198e3e50e38a5dd790b294702e233f @status102
* findHomography should have at least 4 points, 优化性能 @MistEO @status102
* template重复检查 @status102
* FeatureMatcher muplti results @status102 @MistEO
* 新增 JsonDataHelper，抽离 gui.json 中巨大的 data 类型数据到 data 文件夹中 (#12795) @ABA2396
* 干员识别存储 (#12881) @status102
* 提高可读性 @status102
* mumu内测路径注释 @status102
* MuMuExtras 5.0+ (#12876) @Daydreamer114
* 特征匹配调试绘制 @status102
* add mirrorchyan url source (#12869) @MistEO @ABA2396
* warning @status102
* 持有费用缓存判定拆分 @status102
* LogDebug, LogTrace, LogInfo, LogWarn, LogError补上scope输出 (#12855) @status102
* using ToastNotification @ABA2396
* HttpService日志中uri统一使用UriPartial限制 @status102
* wpf CN 修改描述`连战次数` -> `代理倍率` (#12827) @status102
* 生成时清理template, tasks, global缓存 @status102
* 迁移肉鸽任务部分回调 @status102
