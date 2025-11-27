## v5.28.0

### 继续修 BUG | Highlights

在这个版本，我们继续修复了大量 BUG，并且优化了遇到各类错误时的调试体验。

#### 错误处理方面

* 出错时鼠标悬浮在任务错误日志上即可查看出错时的截图，方便快速定位问题。
* ｢生成日志压缩包｣ 功能现在支持同时打包 `debug` 文件夹的截图，并支持分卷压缩，在 GitHub 之类有文件大小限制的地方可以传分卷包（单个分卷最大 20 MB），更加灵活方便。

#### 自动战斗方面

* 自动战斗在只缺一个干员的时候，现在支持自动借缺少的干员，让自动战斗更加智能和便捷。
* 根据 B 站官号投票动态的结果，我们优化了自动战斗界面的显示效果，布局更加合理，操作更加便捷。

#### 肉鸽相关修复

修复了大量集成战略相关问题，包括界园肉鸽 boss 识别、通宝配置解析、不期而遇事件识别、萨米肉鸽任务链等，提升了肉鸽自动化的稳定性。

#### 其他方面

我们发现，牛牛在使用 GPU 推理时，如果 GPU 驱动过旧可能会产生兼容性问题，因此我们添加了一项检测，当发现你的 GPU 驱动超过 2 年没有更新时会给出提示，我们建议你更新驱动以获得更好的体验。

----

In this version, we continued to fix numerous bugs and optimized the debugging experience when encountering various types of errors.

#### Error Handling

* When an error occurs, hovering the mouse over the task error log now displays a screenshot, allowing you to quickly locate the problem.
* The "Generate Support Payload" feature now supports packaging screenshots of the `debug` folder simultaneously and supports multi-part compression. This allows for uploading multi-part packages (each part up to 20 MB) to platforms with file size limits, such as GitHub.

#### *EN Only* I.S. stuck issues.

* The maintainers are aware of MAA getting stuck in the formation before starting an I.S. fighting stage. We still haven't been able to 100% deduct (and reproduce) the issue. Multiple fixes have been attempted. This stable version attemps to fix by completely overwriting the current template `Quick Select`. After various discussions on the Discord support channel, it appears the template really was the issue(?). Comparing with the older template (pre Tragodia update) the position seems slightly changed (and apparently) even the clicking box hence why the issues.

#### *Copilot*

* When only one operator is missing in *Copilot*, MAA can now supports automatically borrow the missing Support Unit, making *Copilot* more intelligent and convenient.
* Based on the results of a poll on the official Bilibili account, we optimized the layout of the *Copilot* interface, making it more intuitive and easier to use.

#### *Auto I.S.* Fixes

We fixed numerous issues related to *Auto I.S.*, including Boss recognition in the *Sui's Garden of Grotesqueries*, Tongbao configuration parsing, Encounter event recognition, and quest chains in the *Expeditioner's Jǫklumarkar*, improving the stability of *Auto I.S.*.

#### Other Aspects

We found that outdated GPU drivers may cause compatibility issues when MAA performs GPU inference. Therefore, we added a detection feature that alerts you when your GPU driver has not been updated for more than 2 years and recommends updating it for a better experience.

----

以下是详细内容：

### 新增 | New

* 任务错误日志鼠标悬浮可以查看出错时截图 @ABA2396
* 自动战斗支持切换技能用法使用坐标 @status102
* 为 GPU 加速添加超过 2 年的驱动版本检测 (#14690) @Rbqwow @status102

### 改进 | Improved

* 改进自动借助战功能 (#11105) @Alan-Charred
* 优化自动战斗界面显示 (#14795) @ABA2396 @HX3N @Constrat @Manicsteiner @status102
* 日志汇报打包 debug 截图，支持分卷，修改存储路径 @ABA2396
* 拆分多主题识别任务 (#14774) @SherkeyXD

### 修复 | Fix

* 部分名片无法进入线索交接 @ABA2396
* 修复放弃通宝的后续任务逻辑 (#14840) @travellerse
* 在未进入缩小的全局总览的时可能进错宿舍 @ABA2396
* 修复 "入暂亭" 事件 (#14684) @Alan-Charred
* 修复界园肉鸽第 3 层与第 5 层 boss 无法区分、第 5 层 boss 前暂停设置无效 @ABA2396
* 修复界园树洞偶发点击到剩余烛火，导致无法进入下一个节点 (#14806) @travellerse
* 修复通宝配置解析逻辑及左侧通宝解析失败的情况 (#14802, #14820) @travellerse
* 修正通宝识别中的坐标 (#14829) @travellerse
* 修复集成战略萨米主题下凹密闻板相关功能 (#14755) @Alan-Charred
* 简化萨米肉鸽 "诡秘的预感" 相关任务链 (#14749) @Alan-Charred
* 修正不期而遇事件名 OCR 以区分禳解的 3 个相近选项，添加不期而遇事件名 OCR 替换 (#14799, #14588) @travellerse
* 肉鸽可能卡进剧目获取界面和收藏品界面 @Saratoga-Official
* 肉鸽允许跳过招募组合直接开始初始招募 (#14713) @Alan-Charred
* 保存招募券结束后等待确认招募按钮消失 (#14773) @travellerse
* 防止生息演算重置地图视图时因截图延迟卡死 (#14721) @Alan-Charred
* 给 open_series_list 加上失败截图机制 (#14693) @Alan-Charred
* 投资入口识别失败 @Saratoga-Official
* 进一步调整 WpfGui 助战选项显示布局 @Alan-Charred
* 上调助战栏位匹配分数阈值 (#14796) @Alan-Charred
* 单文件模式下总是传递借助战 @ABA2396
* 自动借助战错误显示 @ABA2396
* 干员识别包含 盟约·辅助干员 和 领主·Sharp @ABA2396
* 干员识别复制结果到剪贴板时丢失未拥有干员 @status102
* 关卡复核会把 8 识别成 g @Saratoga-Official
* 通关角标识别错误 @ABA2396
* 刷理智指定次数未完全消耗警告在剩余理智也会提示 @status102
* 避免输出空矩阵 @ABA2396
* RA 导航错误 @ABA2396
* build WpfGui for ARM architecture (#14722) @Alan-Charred
* copilot file name (#14821) @Manicsteiner
* JP paradox @Manicsteiner
* KR 引航任务识别 @Daydreamer114
* KR AD navigation (#14742) @HX3N
* ocrReplace for EN (#14740) @Alan-Charred @Saratoga-Official
* reduce template size for QuickFormation RL @Constrat
* Sami IS EN 3rd floor regex @Constrat
* regex AD navigation EN @Constrat

### 文档 | Docs

* 自动战斗文档添加 copilot_list, is_raid, is_paradox, loop_times 注释 @Alan-Charred

### 其他 | Other

* 移除过旧的 `add_user_additional` 参数弃用提示 @status102
* 简化 ROI 修正逻辑 @ABA2396
* 调整 ExportOperBox 内的变量命名与结构 @ABA2396
* 清日志时手动清除 ToolTip @ABA2396
* std::signal 自动舍弃 abort / terminate 上方栈 @status102
* 分卷压缩包大小改成 20 MB @ABA2396
* 向 MaaCore 工程增加 tasks 文件以便检索 (#14731) @status102
* 更新多模板截图工具 @SherkeyXD
* 移除部分无需的 include @status102
* 路径迁移 @status102
* 修复 UI 细节 @hguandl
* optimize templates @Constrat
* JP ocr edits (#14748) @Manicsteiner
* KR tweak SupportUnitUsage translation @HX3N
* KR @hguandl
* SSS#8 for global (#14803) @Manicsteiner
