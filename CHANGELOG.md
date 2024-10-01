## v5.7.0-beta.2

### 新增 | New

* ZT關卡導引更新 @XuQingTW
* 掉落统计显示当前关卡名称 @ABA2396

### 改进 | Improved

* 优化可清除内容的下拉框显示效果 @ABA2396 @Alan-Charred

### 修复 | Fix

* 肉鸽重新编队应在首页执行 @YueLengM
* 自动编队偶现无法匹配职业图标 @ABA2396
* 强制定时启动前显示窗口 导致程序崩溃 @ABA2396
* 强制定时启动报错 @ABA2396

### 文档 | Docs

* 更新 bug-issue 模板 (#10675) @Daydreamer114
* LD screenshot JP translate (#10659) @Manicsteiner @ABA2396

### 其他 | Other

* switch to StageSideStory.png for OD YostarKR @HX3N
* switch to StageSideStory.png for OD YostarEN @Constrat
* YostarKR switch OD navigation to template @HX3N
* switch OD navigation to template @Constrat
* txwy gamedata + various refactoring for resource updater @Constrat
* perf!: Full multithread implementation for ResourceUpdater.exe + method refactoring @Constrat
* InvokeAsync 前判断是否已经在 ui 线程 @ABA2396
* 执行完成后动作前输出当前选择的动作 @ABA2396
* uncommented update stages section. @Constrat
* 调整肉鸽 StageTraderRefreshWithDice 任务的 roi 参数 (#10686) @Alan-Charred
* renamed input directory @Constrat
* resupd clone data script wrong location @Constrat
* 降低 Tales@RA@LeaveCurrentZone 任务的 templThreshold (#10658) @Alan-Charred
* ResourceUpdater always returning 0 even on error @Constrat
* ResourceUpdater git diff not finding pngs @Constrat
* ResourceUpdater.exe implements multi-threading + fix for missing foldartals Lighter implementation compared to #10683 @Constrat
* 移除 ConnectSettingsUserControl 设计时高度限制 @ABA2396
* removed wrong tooltip from AllowUseStoneSave @Constrat
* EN tweak for Use Originium option @Constrat
* EN tweak @Constrat
* 调整 bug report @ABA2396
* Update ResourceUpdater script and clone_data_repo.ps1 @Constrat
* cd to starting dir for script tools @Constrat
* ResourceUpdater tweaks and code optimizations (#10689) @Constrat
* more threading, namespace refactoring for ResourceUpdater @Constrat
* TimerSettings 改用 NumericUpDown 验证输入 @ABA2396
* 定时启动添加日志 @ABA2396
* 主定时器改用 Timer @ABA2396
* 在 ui 线程清除日志 @ABA2396
