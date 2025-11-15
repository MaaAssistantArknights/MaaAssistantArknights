## v5.28.0-beta.1

### 新增 | New

* 卫戍协议：盟约 小游戏 (#14763) @ABA2396 @HX3N
* 去掉勾选框 @ABA2396
* 任务错误日志鼠标悬浮可以查看出错时截图 @ABA2396

### 改进 | Improved

* std::signal 自动舍弃 abort / terminate 上方栈 @status102

### 修复 | Fix

* 进一步调整 WpfGui 助战选项显示布局 @Alan-Charred
* 自动借助战错误显示 @ABA2396
* 干员识别包含 盟约·辅助干员 和 领主·Sharp @ABA2396
* JP paradox @Manicsteiner
* 修复、改进自动借助战功能 (#11105) @Alan-Charred
* 简化萨米肉鸽 "诡秘的预感" 相关任务链 (#14749) @Alan-Charred
* 给 open_series_list 加上失败截图机制 (#14693) @Alan-Charred
* 修复 "入暂亭" 事件 (#14684) @Alan-Charred
* 防止生息演算重置地图视图时因截图延迟卡死 (#14721) @Alan-Charred
* KR AD navigation (#14742) @HX3N
* ocrReplace for EN (#14740) @Alan-Charred @Saratoga-Official
* 肉鸽可能卡进剧目获取界面和收藏品界面 @Saratoga-Official
* build WpfGui for ARM architecture (#14722) @Alan-Charred
* 关卡复核会把8识别成g @Saratoga-Official
* 肉鸽允许跳过招募组合直接开始初始招募 (#14713) @Alan-Charred
* 投资入口不知道为啥没点进去 @Saratoga-Official
* 无法区分界园肉鸽第三层与第五层 boss、界园肉鸽第五层 boss 前暂停设置无效 @ABA2396

### 文档 | Docs

* integration zh可读性增强 (#14756) @status102
* 自动战斗文档添加 copilot_list, is_raid, is_paradox, loop_times 注释 @Alan-Charred

### 其他 | Other

* KR tweak SupportUnitUsage translation @HX3N
* 向MaaCore工程增加tasks文件以便检索 (#14731) @status102
* JP ocr edits (#14748) @Manicsteiner
* 日志汇报打包 debug 截图，支持分卷，修改存储路径 @ABA2396
