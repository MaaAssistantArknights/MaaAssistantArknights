## v5.7.0

### 新增 | New

* CV-4 navigation @SherkeyXD
* capture fresh image (#10460) @ABA2396
* LD 截图 (#10581) @ABA2396
* ResourceUpdater.exe implements multi-threading + fix for missing foldartals Lighter implementation compared to #10683 @Constrat
* multithreaded resource updater (#10683) @Constrat

### 改进 | Improved

* 优化编队模板图片，识别不到对应职业图标时在 `全部` 中寻找 @ABA2396
* 优化 RecognizerViewModel (#10641) @ABA2396 @status102
* 更新雷电截图增强文档和 Issue 模板 (#10638) @Rbqwow @Constrat @HX3N
* tasks.json 十四章内容补充 (#10644) @Daydreamer514
* 截图测试使用GetFreshImage替代添加冗余任务 @status102
* 优化可清除内容的下拉框显示效果 @ABA2396 @Alan-Charred
* switch to StageSideStory.png for OD YostarKR @HX3N
* switch to StageSideStory.png for OD YostarEN @Constrat
* InvokeAsync 前判断是否已经在 ui 线程 @ABA2396
* ZT關卡導引更新 @XuQingTW
* perf!: Full multithread implementation for ResourceUpdater.exe + method refactoring @Constrat
* 更新 bug-issue 模板 (#10675) @Daydreamer114

### 修复 | Fix

* 修复漏了的初始化 @MistEO
* Dormitory theme may block OCR sequence in EN fix #10645 @Constrat
* 下干员操作不显示干员组名字 @ABA2396
* 肉鸽重新编队应在首页执行 (#10465) @YueLengM
* 自动编队偶现无法匹配职业图标 @ABA2396
* 强制定时启动前显示窗口 导致程序崩溃 @ABA2396
* ResourceUpdater always returning 0 even on error @Constrat
* ResourceUpdater git diff not finding pngs @Constrat
* YostarKR switch OD navigation to template @HX3N
* switch OD navigation to template @Constrat
* renamed input directory @Constrat
* uncommented update stages section. @Constrat
* 调整肉鸽 StageTraderRefreshWithDice 任务的 roi 参数 (#10686) @Alan-Charred
* resupd clone data script wrong location @Constrat
* txwy gamedata + various refactoring for resource updater @Constrat
* 强制定时启动报错 @ABA2396
* 降低 Tales@RA@LeaveCurrentZone 任务的 templThreshold (#10658) @Alan-Charred

### 文档 | Docs

* Auto Update Changelogs of v5.7.0-beta.1 (#10657) @github-actions[bot] @status102
* 文档错别字 @ABA2396
* Auto Update Changelogs of v5.7.0-beta.2 (#10709) @github-actions[bot] @status102 @ABA2396
* LD screenshot JP translate (#10659) @Manicsteiner @ABA2396

### 其他 | Other

* 将生息演算的 `策略` 选项移至常规设置 @ABA2396
* 截图延迟过高时自动降低目标帧率 @ABA2396
* Release v5.7.0-beta.2 (#10708) @ABA2396
* Release v5.7.0-beta.1 (#10635) @ABA2396
* YostarKR ocr fix @HX3N
* ss-open 使用 StageSideStory.png @ABA2396
* OD tasks for navigation (from MaaRelease) @Constrat
* 启动后对已有配置排序 @ABA2396
* 喜报！ (#10578) @ABA2396 @pre-commit-ci[bot] @SherkeyXD
* SSS log EN tweak @Constrat
* zh任务链完成耗时 @status102
* 移除 ConnectSettingsUserControl 设计时高度限制 @ABA2396
* 掉落统计显示当前关卡名称 @ABA2396
* removed wrong tooltip from AllowUseStoneSave @Constrat
* EN tweak for Use Originium option @Constrat
* EN tweak @Constrat
* 执行完成后动作前输出当前选择的动作 @ABA2396
* 调整 bug report @ABA2396
* Update ResourceUpdater script and clone_data_repo.ps1 @Constrat
* cd to starting dir for script tools @Constrat
* ResourceUpdater tweaks and code optimizations (#10689) @Constrat
* more threading, namespace refactoring for ResourceUpdater @Constrat
* Revert "feat(perf): multithreaded resource updater" (#10684) @Constrat
* TimerSettings 改用 NumericUpDown 验证输入 @ABA2396
* 定时启动添加日志 @ABA2396
* 主定时器改用 Timer @ABA2396
* 在 ui 线程清除日志 @ABA2396
