## v5.28.0

### 难道说？ | Highlights

本次更新为 v5.28.0 正式版，包含了大量功能改进和问题修复。

#### 界面与体验优化

我们优化了自动战斗界面的显示效果，改进了自动借助战功能，并增强了日志记录功能。现在任务错误日志支持鼠标悬浮查看出错时的截图，方便问题排查。

#### 肉鸽功能增强

本次更新修复了大量集成战略相关问题，包括：
- 修复界园肉鸽第 3 层与第 5 层 boss 识别问题
- 修复通宝配置解析逻辑
- 优化不期而遇事件识别
- 简化萨米肉鸽任务链

#### GPU 加速与性能优化

新增了 GPU 加速驱动版本检测功能，对超过 2 年的驱动版本进行提示，帮助用户及时更新驱动以获得更好的性能。

#### 多语言支持

继续完善了国际服支持，包括 KR、JP、EN 等服务器的各项功能适配。

----

This update is the official release of v5.28.0, containing numerous feature improvements and bug fixes.

#### Interface and Experience Optimization

We have optimized the auto-battle interface display, improved the auto support unit feature, and enhanced logging functionality. Error logs now support hovering to view screenshots taken when errors occurred, facilitating problem diagnosis.

#### Integrated Strategies Enhancements

This update fixes numerous I.S.-related issues, including:
- Fixed boss recognition issues for Sui's Garden of Grotesqueries floors 3 and 5
- Fixed Tongbao configuration parsing logic
- Optimized Encounter event recognition
- Simplified Sarkaz's Furnaceside Fables task chains

#### GPU Acceleration and Performance Optimization

Added GPU acceleration driver version detection to notify users with drivers older than 2 years, helping them update drivers for better performance.

#### Multi-language Support

Continued to improve support for international servers, including feature adaptation for KR, JP, EN servers.

----

### 新增 | New

* 任务错误日志鼠标悬浮可以查看出错时截图 @ABA2396
* 清日志时手动清除 ToolTip @ABA2396
* 为 GPU 加速添加超过 2 年的驱动版本检测 (#14690) @Rbqwow @status102

### 改进 | Improved

* 简化 ROI 修正逻辑 @ABA2396
* 改进自动借助战功能 (#11105) @Alan-Charred
* 优化自动战斗界面显示 (#14795) @ABA2396 @HX3N @Constrat @Manicsteiner @status102
* 日志汇报打包 debug 截图，支持分卷，修改存储路径 @ABA2396
* 拆分多主题识别任务 (#14774) @SherkeyXD
* 调整 ExportOperBox 内的变量命名与结构 @ABA2396
* std::signal 自动舍弃 abort / terminate 上方栈 @status102
* optimize templates @Constrat

### 修复 | Fix

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
* 移除过旧的 `add_user_additional` 参数弃用提示 @status102
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

* 分卷压缩包大小改成 20 MB @ABA2396
* 向 MaaCore 工程增加 tasks 文件以便检索 (#14731) @status102
* 更新多模板截图工具 @SherkeyXD
* 移除部分无需的 include @status102
* 路径迁移 @status102
* 修复 UI 细节 @hguandl
* JP ocr edits (#14748) @Manicsteiner
* KR tweak SupportUnitUsage translation @HX3N
* KR @hguandl
* SSS#8 for global (#14803) @Manicsteiner
