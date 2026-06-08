## v6.12.0-beta.1

### 新增 | New

* 切换主题时保存当前画面截图 (#16993) @ABA2396
* 统一 SearchBar 样式 @ABA2396
* 悖论模拟支持跳过战斗失败的作业, 自动战斗作业增加对应结构 (#16985) @status102
* 启动设置添加模拟器启动测试按钮，便于测试是否配置成功 @ABA2396

### 改进 | Improved

* 肉鸽弹窗类事件处理重构 CloseCollectionClose (#17005) @status102
* 基于灰度阈值预处理的自动战斗导航, 适配H关及怪猎2期 (#16990) @status102
* InvokeProcSubTaskMsg 重构 (#16979) @status102

### 修复 | Fix

* EN IS6 bosky updated template @Constrat
* EN IS6 bosky text size changed @Constrat
* 修正特克诺干员名 OCR 误识别 (#17030) @ZiyinLin
* 绿票商店状态回退错误 @status102
* 绿票商店2阶段check @status102 @ZiyinLin
* MaskedCcoeffMatcher 稀疏路径累加器改用 CV_64F 防止大数相减精度损失 (#16983) @Aliothmoon
* 修复 MaaMacGui changelog 贡献者 mention (#16978) @ColdSpellhere
* 降低 PlayCover 下肉鸽部分任务的模版匹配分数阈值 (#16968) @Alan-Charred

### 文档 | Docs

* README 自动抄作业 --> 自动战斗 @Rbqwow

### 其他 | Other

* revert all changes to remainingcandleflame IS6 EN @Constrat
* YostarKR winden colorScale for compatibility @HX3N
* 自动战斗-视频链接 始终显示 @ABA2396
* 补充可露希尔基建数值 @Saratoga-Official

### MaaMacGui

### 修复 | Fix

* 统一 gui.log 文件日志中的日期与时间格式 ([#93](https://github.com/MaaAssistantArknights/MaaMacGui/pull/93)) @Alan-Charred
