## v6.9.0

### 新增 | New

* 进入快捷编队页后校验清空状态 @status102
* 重复启动时通过跨进程事件激活主窗口而非弹窗警告 (#16363) @ABA2396
* 在关卡没解锁代理/剿灭没使用全权代理时报错停止 (#16357) @ABA2396
* 增加 保存代理指挥记录 支持; 新增合成玉掉落检查, 0掉落结束 (#16356) @Roland125
* 掉落物识别额外输出剿灭进度 @status102
* 新增 get_last_matched_task_name 方法；成功命中新节点时与 get_last_task_name 一致，在进入下一轮匹配后若未再命中新节点时，仍保留最近一次成功命中的任务名，避免误取空值 @ABA2396
* 适配界面主题「重构」 (#16349) @SherkeyXD
* 支持腾讯应用宝 5.10.56.xx (#16292) @srdr0p
* 争锋频道 绿藤城 (#16345) @Daydreamer114
* updater 支持暗色模式 @ABA2396
* updater 支持显示进度条或控制台输出 @ABA2396
* 适配七周年许愿墙 @Copilot

### 改进 | Improved

* 优化 DEBUG_skill_ready 判断 @ABA2396
* optimize templates yostarkr SPA @Constrat
* 合并简化任务状态 @status102
* ProcessTask匹配命中状态更新统一化 @status102
* 优化1星词条选项操作描述 @status102
* 理智上限增加到210 @status102
* 大幅提升技能就绪识别准确率，优化技能截图保存策略 (#16393) @ABA2396

### 修复 | Fix

* 生息演算 5.1 更新后无法使用无存档刷分 (#16402) @ABA2396
* CheckComboBox 不显示标题 @ABA2396
* 3 星 tag 标题显示异常 @ABA2396
* 修复剿灭后出现升级界面，导致无法继续任务的问题(#16255) (#16370) @Roland125
* 干员使用技能间隔未能生效 @status102
* 干员仓储识别手动指定术兔 @status102
* 若干正确性问题修正（null check / race / clamp / retry 等 9 处） (#16332) @FireflySentinel
* 干员仓储识别未跳过数据查找失败的干员 @status102
* EN IS6 trail regex @Constrat
* 月度小队不再隐藏分队选择 @SherkeyXD
* 升级结算时闪退 @ABA2396
* 修复小游戏列表中下拉框偶现无法展开的问题 @ABA2396
* 修复蓝叠模拟器关闭失败的问题 (#16388) @lengyanyu258
* lower YoStarJP office mini threshold (#16390) @Rememorio

### 文档 | Docs

* changelog @ABA2396
* Auto Update Changelogs of v6.9.0-beta.1 (#16374) @github-actions[bot] @github-actions[bot] @ABA2396
* 修改部分注释 (#16215) @JasonHuang79
* Add v6.9.0-beta.2 to CHANGELOG @ABA2396
* Update CHANGELOG for v6.9.0-beta.3 release @ABA2396

### 其他 | Other

* Revert "perf: 自动编队预编队后检查选中情况" @status102
* 技能得分低也需要最小截图间隔 @ABA2396
* YostarEN SPA @Constrat
* 统一 CheckComboBox 样式 @ABA2396
* Release v6.9.0-beta.3 (#16397) @ABA2396
* Release v6.9.0-beta.2 (#16378) @ABA2396
* Release v6.9.0-beta.1 (#16373) @ABA2396
* YostarJP MiniGame SPA (#16372) @Manicsteiner
* 支持打包日志文件后 GitHub 上传日志不再需要先关闭 MAA @ABA2396
* 调整完整后 `无其他 MAA` 选项绑定逻辑，不再强制勾选退出模拟器 @ABA2396
* PC 端支持 `完成后退出明日方舟` (#16351) @glimmertouch @Copilot @Copilot
* 调整选项 IsEnabled @ABA2396
* 连接方式使用 PC 端时禁用不支持的完成后操作 @ABA2396
* 周年月卡文本更新 @SherkeyXD
* YostarKR MiniGame SPA (#16364) @HX3N
