## v6.9.3

### 新增 | New

* 优先使用更新包中的 updater @ABA2396
* 添加因为缺少 MAA.Updater.exe 导致更新失败的弹窗提示 @ABA2396
* 更新 153-4 基建作业 @ABA2396

### 改进 | Improved

* 自动战斗结束增加LoadingIcon等待项 @status102

### 修复 | Fix

* Analyzer执行前未检查m_roi是否未空 @status102
* 修复小游戏界面的开始按钮在连接模拟器失败时仍然发送开始信号 @ABA2396
* 自动战斗进入等待过长 @status102
* OF-1战斗后等待过长导致部分后续流程失败 @status102
* 修复部分成就判断条件错误 @ABA2396

### 文档 | Docs

* update JP preview image (#16485) @Manicsteiner
* 更新 README 预览图片 @ABA2396

### 其他 | Other

* local-install 使用 ci 同款处理方法 @ABA2396
