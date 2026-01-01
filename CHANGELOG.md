## v6.1.1

### 新增 | New

* 隐秘战线支持系列任务 (#15249) @ABA2396 @HX3N @Constrat @Manicsteiner @momomochi987
* 新增模板路径同步工具 (#15254) @ABA2396 @Constrat @HX3N @momomochi987
* YostarEN base templates overhaul @Constrat
* 添加孤星主题配置 @SherkeyXD
* 添加孤星主题模板图 @SherkeyXD
* 优化 ImageCropper 输出 @SherkeyXD

### 改进 | Improved

* optimize LoneTrail templates @Constrat
* 减少string_view滥用 @MistEO
* 统一 `DEBUG_VERSION` @MistEO

### 修复 | Fix

* 自动战斗击杀数识别错误 (#15266) @status102
* EN Sami floor detection regex @Constrat
* bad reference @MistEO
* 盲点撤退兜底 @status102
* 自动战斗在未存在 debug/map 路径时无法生成地图截图 @status102
* 任务名字符串池的线程安全问题 @ABA2396
* 大地图技能, 撤离按钮及相机偏移修复, 连带引航者S6地图TN-2 ~ TN-4 view[1] (#15252) @status102 @status102
* Copper regex for Sui Garden EN @Constrat
* 满线索再放入 @ABA2396

### 其他 | Other

* YostarJP copper regex (#15257) @eyjafjallascomb
* KR tweak NightlyWarning and CheckBeforeReportingIssue @HX3N
* 更新截图 @ABA2396
* Revert "fix: 任务名字符串池的线程安全问题" @MistEO
* YostarJP copper regex @Manicsteiner
* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* 繁中服「紅絲絨」活動導航 (#15234) @momomochi987
* update template tool @SherkeyXD
