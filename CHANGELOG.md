## v6.1.0-beta.4

### 新增 | New

* 更新期间退出应用添加二次确认 (#14964) @Hakuin123 @Hakuin123 @ABA2396
* YostarJP SecretFront edit (#15191) @Manicsteiner

### 改进 | Improved

* 优化悬浮窗布局 @ABA2396
* en reoptimize ALL templates from scratch @Constrat

### 修复 | Fix

* 部分情况下无法进入借助战界面 @ABA2396
* 前往上一次作战增加重试 @ABA2396
* 修复通宝拾取时卡死并删除m_retry_times=1 (#15197) @travellerse
* 基建自定义配置迁移加个try @status102
* 保全自动战斗日志悬浮窗 @ABA2396
* 保全派驻因网络波动可能无法点击阵容确认 @ABA2396
* 未设置自定义基建排班路径时第一次启动会报错 @ABA2396
* 自定义基建计划指定某个计划后会在1分钟后被重置为定时计划 (#14649) @status102 @Constrat
* LoadingText 结束后 UI 延迟变化导致误匹配 (#15198) @status102
* roi out of range (#15204) @178619
* sendclue reception standardize templates (#15205) @Constrat

### 其他 | Other

* 更新 pre-commit 配置 @SherkeyXD
* 移除 isort/black 配置 @SherkeyXD
* devcontainer 迁移至 ruff @SherkeyXD
* 修复 roi updater 工具的问题 @SherkeyXD
* 悬浮窗移动时不隐藏，避免某些窗口点击就会触发闪烁（说的就是 QQ） @ABA2396
* update pre-commit toolchain (#15179) @SherkeyXD
