## v4.24.0-beta.5

### 新增

- 肉鸽烧开水模式增加凹开局直升 (#6443) @Lancarus
- 肉鸽适配高规格分队开局 (#6436) @Lancarus
- 开始唤醒-账号切换新增手动切换，可单独执行开始唤醒流程 (#6433) @status102
- 如果任务流程中勾选了刷理智，则在任务链结束时提供一个理智回满的预测时间 (#6456) @status102
- macOS 国际服允许添加生息演算任务 @hguandl

### 改进

- 投资达到上限后退出到肉鸽主界面 (#6435) @zzyyyl
- 优化傀影招募策略 (#6476) @Lancarus
- 激活自动战斗-战斗列表时将同时激活自动编队，调整自动战斗UI (#6448) @status102
- 优化傀影肉鸽招募和商店逻辑 (#6447) @Lancarus
- 稍微优化下自动战斗界面 ui @moomiji
- 优化 自动战斗-战斗列表 跳过战斗后剧情的逻辑 (#6430) @status102
- 优化萨米招募逻辑 (#6422) @Lancarus
- 稍微优化下自动战斗界面 ui @ABA2396
- 水月肉鸽：增加支援道具，新干员分组，优化大君遗脉策略 (#6372) @Yumi0606

### 修复

- 增加延迟以提高刷理智-当前理智识别成功率 (#6402) @status102
- 修复 sanity_report == null 的问题 @zzyyyl
- 加工站不设置干员时，直接报错并返回 @MistEO
- 修复水月肉鸽卡在藏品 (#6464) @mole828
- 修复 debug 模式下 tasks.json 检查未通过时的访问越界问题 @zzyyyl
- 修复网络波动造成的使用理智药不清理智问题 @zzyyyl
- 修复自动战斗-战斗列表的导航失效问题 (#6413) @status102
- 修复Status缓存跨任务清理在任务链正常完成情况下失效的错误 (#6428) @status102
- 修复外部通知错误 (#6414) @LiamSho
- 修复资源读取错误，修复错误的界面提示 @ABA2396
- 修复资源更新toast笔误 @MistEO
- 通过进程名关闭蓝叠时如果模拟器个数超过1个则放弃关闭 @ABA2396

### 其他

- 基建换班-编组缓存 使用value_or替代empty检查 (#6404) @status102
- 更新qodana配置 (#6465) @hxdnshx
- gui.log 在启动时记录当前版本 @ABA2396
- 在Python脚本中添加活动关导航下载与加载的示例 (#3916) (#6451) @ZhaoZuohong
- 删除多余的空任务 @zzyyyl
- debug: 增加一项多余空任务的检查 @zzyyyl
- fix algorithm error warning @Constrat
- reordered objects, following zh-cn @Constrat
- removed duplicate operator @Constrat
- Alter Operators ^$ regex fix Yato alter, Noir alter, Terra Research ident. @Constrat
- 无配置字段时储存默认值 @ABA2396
- 全局选项设置减少嵌套，增加日志 @ABA2396

### For overseas

#### YostarEN

- EN SSReopen-XX adaptation @Constrat
- EN Guide Ahead navigation @Constrat
- 修复 EN 任务 `BattleQuickFormationFilter-Cost` @zzyyyl
- 修复 EN 生息演算任务 `Reclamation@ManufactureStatus` roi 越界的问题 @zzyyyl
