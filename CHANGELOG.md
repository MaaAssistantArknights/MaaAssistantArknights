## v5.28.0-beta.2

### 新增 | New

* 为 GPU 加速添加超过两年的驱动版本检测 (#14690) @Rbqwow @status102
* 清日志的时候把 ToolTip 手动清除一下 @ABA2396

### 改进 | Improved

* 统一存截图 @ABA2396 @status102
* 优化自动战斗界面显示 (#14795) @ABA2396 @Constrat @Manicsteiner @status102
* 拆分多主题识别任务 (#14774) @SherkeyXD
* 调整 ExportOperBox 内的变量命名与结构 @ABA2396
* 优化模板图 @Constrat

### 修复 | Fix

* 界园肉鸽可能在战斗失败后进入领取掉落界面，萨卡兹肉鸽部分深入探索存在进事件给思绪 @Saratoga-Official
* 修复界园树洞偶发点击到剩余烛火，导致无法进入下一个节点 (#14806) @travellerse
* 修复通宝配置解析逻辑 (#14802) @travellerse
* 修复集成战略萨米主题下凹密闻板相关功能 (#14755) @Alan-Charred
* 修复不期而遇事件名 OCR 以区分禳解的三个相近选项 (#14799, #14588) @travellerse
* 保存招募券结束后等待确认招募按钮消失 (#14773) @travellerse
* 刷理智指定次数未完全消耗警告在剩余理智也会提示 @status102
* 干员识别复制结果到剪贴板时丢失未拥有干员 @status102
* 移除过旧的 `add_user_additional` 参数弃用提示 @status102
* 上调助战栏位匹配分数阈值 (#14796) @Alan-Charred
* 通关角标 @ABA2396
* RA 导航错误 @ABA2396
* reduce template size for QuickFormation RL @Constrat
* Sami IS EN 3rd floor regex @Constrat
* regex AD navigation EN @Constrat

### 其他 | Other

* 分卷压缩包大小改成 20 MB @ABA2396
* 更新多模板截图工具 @SherkeyXD
* 修复 UI 细节 @hguandl
* 路径迁移 @status102
* KR @hguandl
* SSS#8 for global (#14803) @Manicsteiner
