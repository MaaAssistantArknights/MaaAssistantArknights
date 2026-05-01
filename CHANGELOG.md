## v6.9.0-beta.3

### 新增 | New

* 适配七周年许愿墙 @Copilot

### 改进 | Improved

* 大幅提升技能就绪识别准确率，优化技能截图保存策略 (#16393) @ABA2396

### 修复 | Fix

* 修复小游戏列表中下拉框偶现无法展开的问题 @ABA2396
* 修复蓝叠模拟器关闭失败的问题 (#16388) @lengyanyu258
* lower YoStarJP office mini threshold (#16390) @Rememorio

### 其他 | Other

* YostarKR MiniGame SPA (#16364) @HX3N

## v6.9.0-beta.2

### 修复 | Fix

* 升级结算时闪退 @ABA2396

## v6.9.0-beta.1

### 新增 | New

* 重复启动时通过跨进程事件激活主窗口，替代弹窗警告提示 (#16363) @ABA2396
* 新增关卡未解锁代理或剿灭未启用全权代理时的错误停止检查 (#16357) @ABA2396
* 新增保存代理指挥记录功能，并支持合成玉掉落检查，0 掉落时自动结束任务 (#16356) @Roland125
* 掉落物识别额外输出剿灭进度信息 @status102
* 适配“重构”界面主题 (#16349) @SherkeyXD
* 支持腾讯应用宝 5.10.56.xx (#16292) @srdr0p
* 新增 updater 暗色模式支持 @ABA2396
* 新增 updater 进度条与控制台输出支持 @ABA2396
* 支持 PC 端 `完成后退出明日方舟` (#16351) @glimmertouch
* 新增争锋频道「绿藤城」支持 (#16345) @Daydreamer114

### 改进 | Improved

* 合并并简化任务状态逻辑 @status102
* 统一 ProcessTask 匹配命中状态更新逻辑 @status102
* 优化 1 星词条选项操作描述 @status102
* 理智上限提升至 210 @status102
* 调整完整后 `无其他 MAA` 选项绑定逻辑，不再强制勾选退出模拟器 @ABA2396
* 调整选项 `IsEnabled` 逻辑 @ABA2396
* 使用 PC 端连接方式时，自动禁用不支持的完成后操作 @ABA2396

### 修复 | Fix

* 修复剿灭后出现升级界面导致任务无法继续的问题 (#16255) (#16370) @Roland125
* 修复干员仓储识别中升变阿米娅的识别问题 @status102
* 修复若干正确性问题（含 null check / race / clamp / retry 等 9 处） (#16332) @FireflySentinel
* 修复干员仓储识别未跳过数据查找失败干员的问题 @status102
* 修复 EN IS6 trail 正则匹配问题 @Constrat
* 修复月度小队隐藏分队选择的问题 @SherkeyXD

### 文档 | Docs

* 更新部分代码注释 (#16215) @JasonHuang79

### 其他 | Other

* 修改 issue template @ABA2396
* 更新周年月卡相关文本 @SherkeyXD
* YostarJP MiniGame SPA (#16372) @Manicsteiner
