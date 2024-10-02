## v5.7.0

### 发生甚么事了 | Highlight

从本次版本开始，我们将会对正式版的更新笔记编写一份简要说明，以帮助你更好地掌握我们更新了啥。

* __对雷电模拟器支持截图增强模式__

  在这个版本，我们新增了对雷电模拟器<del>（黄色 MuMu）</del>的截图增强模式支持，现在你也可以在雷电模拟器上看到两位数甚至一位数的截图延迟了，大幅度提升各项任务，特别是自动战斗、自动肉鸽、仓库和干员识别的效率。

* __将生息演算的 `策略` 选项移至常规设置__

  以前总有用户找不到生息验算任务的策略选项，运行结果与期待不符，以为出 bug 了，实际上是没选对策略。在这个版本，我们将策略选项从高级设置移动到常规设置中，方便用户更直观地选择所需的策略。

* __喜报！MAA 报错了！__

  总有用户不看报错信息（__特别是里面的解决方案部分__），只会用口述表达，这个版本我们让报错信息窗口以喜报的形式呈现，希望你能先读完再按要求反馈问题。

* __修复了一些已知 bug，提升了 MAA 稳定性__

  在这个版本对部分 bug 进行了修复，例如自动编队时偶现无法选择右侧职业筛选的问题。

* __资源与品质级更新__

  这个版本随附游戏资源文件更新至24年10月矢量突破版本，新增了主线 14 章关卡自动导航数据，并优化了部分界面功能。

----

以下是本次更新详细内容。

### 新增 | New

* ZT關卡導引更新 @XuQingTW
* 掉落统计显示当前关卡名称 @ABA2396
* 喜报！(报错界面) (#10578) @ABA2396 @SherkeyXD
* LD 截图 (#10581) @ABA2396

### 改进 | Improved

* 将生息演算的 `策略` 选项移至常规设置 @ABA2396
* 优化编队模板图片，识别不到对应职业图标时在 `全部` 中寻找 @ABA2396
* 截图延迟过高时自动降低目标帧率 @ABA2396
* 优化可清除内容的下拉框显示效果 @ABA2396 @Alan-Charred
* 启动后对已有配置排序 @ABA2396

### 修复 | Fix

* 修复漏了的初始化 @MistEO
* 肉鸽重新编队应在首页执行 @YueLengM
* 自动编队偶现无法匹配职业图标 @ABA2396
* 强制定时启动前显示窗口 导致程序崩溃 @ABA2396
* 强制定时启动报错 @ABA2396
* 下干员操作不显示干员组名字 @ABA2396
* CV-4 navigation @SherkeyXD
* Dormitory theme may block OCR sequence in EN fix #10645 @Constrat
* YostarKR ocr fix @HX3N

### 文档 | Docs

* 更新 bug-issue 模板 (#10675) @Daydreamer114
* LD screenshot JP translate (#10659) @Manicsteiner @ABA2396
* 更新雷电截图增强文档和 Issue 模板 (#10638) @Rbqwow @Constrat @HX3N
* 文档错别字 @ABA2396

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
* TimerSettings 改用 NumericUpDown 验证输入 @ABA2396
* 定时启动添加日志 @ABA2396
* 主定时器改用 Timer @ABA2396
* 在 ui 线程清除日志 @ABA2396
* 优化 RecognizerViewModel (#10641) @ABA2396 @status102
* 截图测试使用GetFreshImage替代添加冗余任务 @status102
* capture fresh image (#10460) @ABA2396
* ss-open 使用 StageSideStory.png @ABA2396
* OD tasks for navigation (from MaaRelease) @Constrat
* SSS log EN tweak @Constrat
* zh任务链完成耗时 @status102
* tasks.json 十四章内容补充 (#10644) @Daydreamer114
