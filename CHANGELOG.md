## v5.28.0

### 新增 | New

* 去掉勾选框 @ABA2396
* 任务错误日志鼠标悬浮可以查看出错时截图 @ABA2396
* 为GPU加速添加超过两年的驱动版本检测 (#14690) @Rbqwow @status102
* 清日志的时候把 ToolTip 手动清除一下 @ABA2396

### 改进 | Improved

* 简化 ROI 修正逻辑 @ABA2396
* 修复、改进自动借助战功能 (#11105) @Alan-Charred
* 修复 "入暂亭" 事件 (#14684) @Alan-Charred
* std::signal 自动舍弃 abort / terminate 上方栈 @status102
* 日志汇报打包 debug 截图，支持分卷，修改存储路径 @ABA2396
* 优化自动战斗界面显示 (#14795) @ABA2396 @HX3N @Constrat @Manicsteiner @status102
* 拆分多主题识别任务 (#14774) @SherkeyXD @pre-commit-ci[bot]
* optimize templates @Constrat
* 调整 ExportOperBox 内的变量命名与结构 @ABA2396

### 修复 | Fix

* 避免输出空矩阵 @ABA2396
* unused @status102
* KR 引航 @Daydreamer114
* 擦屁股 @ABA2396
* StartUp 为什么要进终端 @ABA2396
* 进一步调整 WpfGui 助战选项显示布局 @Alan-Charred
* 自动借助战错误显示 @ABA2396
* 干员识别包含 盟约·辅助干员 和 领主·Sharp @ABA2396
* JP paradox @Manicsteiner
* 简化萨米肉鸽 "诡秘的预感" 相关任务链 (#14749) @Alan-Charred
* 给 open_series_list 加上失败截图机制 (#14693) @Alan-Charred
* 防止生息演算重置地图视图时因截图延迟卡死 (#14721) @Alan-Charred
* KR AD navigation (#14742) @HX3N
* ocrReplace for EN (#14740) @Alan-Charred @Saratoga-Official
* 肉鸽可能卡进剧目获取界面和收藏品界面 @Saratoga-Official
* build WpfGui for ARM architecture (#14722) @Alan-Charred
* 关卡复核会把8识别成g @Saratoga-Official
* 肉鸽允许跳过招募组合直接开始初始招募 (#14713) @Alan-Charred
* 投资入口不知道为啥没点进去 @Saratoga-Official
* 无法区分界园肉鸽第三层与第五层 boss、界园肉鸽第五层 boss 前暂停设置无效 @ABA2396
* copilot file name (#14821) @Manicsteiner
* 单文件模式下总是传递借助战 @ABA2396
* 修复界园树洞偶发点击到剩余烛火，导致无法进入下一个节点 (#14806) @travellerse
* reduce template size for QuickFormation RL @Constrat
* RA 导航错误 @ABA2396
* 修复通宝配置解析逻辑 (#14802) @travellerse
* 修复集成战略萨米主题下凹密闻板相关功能 (#14755) @Alan-Charred
* 保存招募券结束后等待确认招募按钮消失 (#14773) @travellerse
* 修正不期而遇事件名ocr以区分禳解的三个相近选项 (#14799) @travellerse
* 刷理智指定次数未完全消耗警告在剩余理智也会提示 @status102
* 干员识别复制结果到剪贴板时丢失未拥有干员 @status102
* 移除过旧的`add_user_additional`参数弃用提示 @status102
* 上调助战栏位匹配分数阈值 (#14796) @Alan-Charred
* 通关角标 @ABA2396
* 添加不期而遇事件名ocr替换 (#14588) @travellerse
* Sami IS EN 3rd floor regex @Constrat
* regex AD navigation EN @Constrat
* 修正通宝识别中的坐标 (#14829) @travellerse
* 处理左侧通宝解析失败的情况 (#14820) @travellerse

### 文档 | Docs

* Auto Update Changelogs of v5.28.0-beta.1 (#14768) @github-actions[bot] @ABA2396 @ABA2396 @github-actions[bot] @ABA2396
* 自动战斗文档添加 copilot_list, is_raid, is_paradox, loop_times 注释 @Alan-Charred
* Auto Update Changelogs of v5.28.0-beta.2 (#14801) @github-actions[bot] @ABA2396 @ABA2396 @github-actions[bot] @ABA2396

### 其他 | Other

* 移除部分无需的include @status102
* Release v5.28.0-beta.2 (#14831) @ABA2396
* Release v5.28.0-beta.2 (#14827) @ABA2396
* Release v5.28.0-beta.2 (#14800) @ABA2396
* Release v5.28.0-beta.1 (#14767) @ABA2396
* KR tweak SupportUnitUsage translation @HX3N
* 向MaaCore工程增加tasks文件以便检索 (#14731) @status102
* JP ocr edits (#14748) @Manicsteiner
* 分卷压缩包大小改成 20 MB @ABA2396
* 更新多模板截图工具 @SherkeyXD
* KR @hguandl
* 修复UI细节 @hguandl
* SSS#8 for global (#14803) @Manicsteiner
* 路径迁移 @status102
