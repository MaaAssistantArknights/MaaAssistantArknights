## v6.9.0

### 许愿墙前挂满愿签，牛牛顺手把更新器、作战流程和海外服体验都打磨了一遍 | Highlights

这一版的重点仍然是把日常使用体验打磨得更顺手、更稳定。我们一边补齐周年活动与海外服适配，一边继续清理那些会打断长时间挂机流程的细碎问题。

#### 桌面体验与更新可视化

MAA 现在会在重复启动时直接唤起已经打开的主窗口，不再额外弹出警告打断操作。与此同时，更新器补上了暗色模式、进度条与控制台输出，配合 PC 端连接方式下更清晰的完成后操作限制，整个更新和收尾流程都更直观了。

#### 自动作战更稳，也更懂得何时停下

这一版为关卡未解锁、剿灭未启用全权代理等场景补上了提前停止检查，也支持保存代理指挥记录，并在合成玉 0 掉落时自动结束任务。再加上技能就绪识别、任务状态逻辑和多处界面判定的修正，升级结算、生息演算等此前容易卡住或闪退的流程也稳定了不少。

#### 七周年与海外服内容继续补齐

本次更新适配了七周年许愿墙、“重构”界面主题与争锋频道「绿藤城」，也补齐了 Yostar JP、EN、KR 小游戏相关的 SPA、资源与模板优化，让活动期间和海外服环境下的使用体验更完整。

----

### Highlights

This release continues to focus on making everyday use smoother and more reliable. Alongside support for anniversary content and overseas servers, we kept cleaning up the small but disruptive issues that can break long unattended runs.

#### Better Desktop Flow and Update Visibility

MAA now re-activates the existing main window on repeated launches instead of interrupting users with an extra warning dialog. The updater also gains dark mode, a progress bar, and console output, while the PC client connection flow now makes post-run action limits clearer, making the whole update and wrap-up process easier to follow.

#### More Reliable Automation, with Smarter Stop Conditions

This version adds early-stop checks for locked stages and annihilation runs without proxy enabled, and it can now save auto-command records while ending runs automatically when 0 Orundum is detected. Together with more accurate skill-ready recognition, cleaner task-state logic, and multiple UI-flow fixes, previously fragile scenarios such as level-up settlement and Reclamation Algorithm runs are now much more stable.

#### Continued Anniversary and Overseas Server Coverage

This update adapts to the 7th anniversary wishing wall, the “Reclamation” interface theme, and the Green Vine channel in SSS. It also rounds out Yostar JP, EN, and KR minigame SPA support together with related resource and template optimizations, making event-period and overseas-server usage more complete.

----

以下是详细内容：

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
* 适配七周年许愿墙 @Copilot
* YostarJP/EN/KR MiniGame SPA  @Manicsteiner @Constrat @HX3N

### 改进 | Improved

* 统一 CheckComboBox 样式 @ABA2396
* 大幅提升技能就绪识别准确率，优化技能截图保存策略 (#16393) @ABA2396
* 合并并简化任务状态逻辑 @status102
* 统一 ProcessTask 匹配命中状态更新逻辑 @status102
* 优化 1 星词条选项操作描述 @status102
* 理智上限提升至 210 @status102
* 调整完整后 `无其他 MAA` 选项绑定逻辑，不再强制勾选退出模拟器 @ABA2396
* 调整选项 `IsEnabled` 逻辑 @ABA2396
* 使用 PC 端连接方式时，自动禁用不支持的完成后操作 @ABA2396
* optimize templates yostarkr SPA @Constrat

### 修复 | Fix

* 修复 3 星 tag 标题显示异常 @ABA2396
* 修复 CheckComboBox 不显示标题的问题 @ABA2396
* 修复生息演算 5.1 更新后无法使用无存档刷分的问题 (#16402) @ABA2396
* 修复剿灭后出现升级界面导致任务无法继续的问题 (#16255) (#16370) @Roland125
* 修复干员仓储识别中升变阿米娅的识别问题 @status102
* 修复若干正确性问题（含 null check / race / clamp / retry 等 9 处） (#16332) @FireflySentinel
* 修复干员仓储识别未跳过数据查找失败干员的问题 @status102
* 修复 EN IS6 trail 正则匹配问题 @Constrat
* 修复月度小队隐藏分队选择的问题 @SherkeyXD
* 修复小游戏列表中下拉框偶现无法展开的问题 @ABA2396
* 修复蓝叠模拟器关闭失败的问题 (#16388) @lengyanyu258
* 升级结算时闪退 @ABA2396
* lower YoStarJP office mini threshold (#16390) @Rememorio

### 文档 | Docs

* 更新部分代码注释 (#16215) @JasonHuang79

### 其他 | Other

* 任务排序支持 JSONC @ABA2396
* 调整资源更新环境 @ABA2396
* 修改 issue template @ABA2396
* 更新周年月卡相关文本 @SherkeyXD
