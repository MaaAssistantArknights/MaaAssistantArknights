## v5.13.0-beta.8

### 新增 | New

* 繁中服「巴別塔」活動導航 (#11863) @momomochi987
* Mac支持肉鸽烧水参数配置: 烧水购物、烧水奖励选择、烧水分队选择 (#11866) @hguandl
* YostarJP Sarkaz roguelike preload (#11850) @Manicsteiner

### 改进 | Improved

* 资源更新后无需重启，优化手动更新逻辑 (#11857) @ABA2396
* 肉鸽投资在同一局内投资系统错误后不再进入投资插件 (#11826) @status102
* 自动战斗自动编队在选择职业前编入非干员组干员 (#11830) @status102

### 修复 | Fix

* 自动战斗不再允许使用带TR的导航关卡名禁用自动编队，改为自动检测 (#11868) @status102
* YostarJP OCR mismatched parenthesis (#11877) @Alan-Charred
* 月度小队模式不再试图提前离开肉鸽 (#11872) @BxFS
* 肉鸽开局无法选择指挥分队时放弃探索 (#11847) @status102 @Constrat
* Wpf自动战斗无法连接到模拟器后不能自动停止 @status102
* Wpf公告内容显示错误显示为上次内容 (#11824) @status102

### 文档 | Docs

* 修改 MirrorChyan 官网链接 @ABA2396
* 肉鸽参数注释 @status102

### 其他 | Other

* 优化界面布局与翻译 @ABA2396
* 肉鸽局内参数重构遗留 (#11829) @status102
* 资源更新后重载 BattleData (#11874) @ABA2396
* MaaCore PackageTask中未进入的subtask将不再输出log (#11783) @status102
* 自动战斗BattleStartPre任务合并理智药检测 @status102
* 移除账号切换中不必要的任务 (#11820) @status102
* 自动编队干员选择错误 @status102
* Roguelike InitialDrop: SquadDefault -> Squad-EnterPoint (#11870) @BxFS
* script to update version.json (#11875) @Constrat
* bypass update resources in formatting cases (#11867) @Constrat
* 简化干员名正则 (#11876) @Saratoga-Official
* 修改mirrorc线路 @MistEO
* missing strings from zh-cn @Constrat
* 简化干员名正则 @ABA2396
* WpfGui引入AsstTaskType代替硬编码 (#11856) @status102
* MirrorChyan域名 @hguandl
* issue_template bug-report Version 处添加提示 (#11848) @Daydreamer114
* Wpf肉鸽任务RoguelikeMode参数类型改为int (#11821) @status102
* update mirrorc tips (#11832) @MistEO @ABA2396
* 上调MaaCore Log Rotate阈值为64MB (#11834) @status102
* Wpf公告存储拆分 (#11825) @status102
* 添加翻译 @ABA2396
* 等待延迟前打个日志 @ABA2396
