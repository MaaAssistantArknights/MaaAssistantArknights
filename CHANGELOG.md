## v5.28.0-beta.1

### 新增 | New

* 卫戍协议：盟约小游戏 (#14763) @ABA2396 @HX3N @Constrat
* 任务错误日志鼠标悬浮可以查看出错时截图 @ABA2396
* 日志汇报打包 debug 截图，支持分卷，修改存储路径 @ABA2396

### 改进 | Improved

* std::signal 自动舍弃 abort / terminate 上方栈 @status102

### 修复 | Fix

* 修复、改进自动借助战功能，优化助战选项显示布局和错误提示 (#11105) @Alan-Charred @ABA2396
* 干员识别包含盟约·辅助干员和领主·Sharp @ABA2396
* 简化萨米肉鸽"诡秘的预感"相关任务链 (#14749) @Alan-Charred
* 修复"入暂亭"事件 (#14684) @Alan-Charred
* 防止生息演算重置地图视图时因截图延迟卡死 (#14721) @Alan-Charred
* 肉鸽可能卡进剧目获取界面和收藏品界面 @Saratoga-Official
* 肉鸽允许跳过招募组合直接开始初始招募 (#14713) @Alan-Charred
* 投资入口不知道为啥没点进去 @Saratoga-Official
* 关卡复核会把 8 识别成 g @Saratoga-Official
* 无法区分界园肉鸽第 3 层与第 5 层 boss、界园肉鸽第 5 层 boss 前暂停设置无效 @ABA2396
* 给 open_series_list 加上失败截图机制 (#14693) @Alan-Charred
* KR AD navigation (#14742) @HX3N
* JP paradox @Manicsteiner
* ocrReplace for EN (#14740) @Alan-Charred @Saratoga-Official
* build WpfGui for ARM architecture (#14722) @Alan-Charred

### 文档 | Docs

* integration zh 可读性增强 (#14756) @status102
* 自动战斗文档添加 copilot_list, is_raid, is_paradox, loop_times 注释 @Alan-Charred
* 向 MaaCore 工程增加 tasks 文件以便检索 (#14731) @status102

### 其他 | Other

* KR tweak SupportUnitUsage translation @HX3N
* JP ocr edits (#14748) @Manicsteiner
