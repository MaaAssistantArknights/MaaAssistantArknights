## v5.20.0-beta.3

### 新增 | New

* 肉鸽支持多级事件选择 @ABA2396
* 界园肉鸽部分关卡自动战斗逻辑 (#13219) @Saratoga-Official
* 新Config任务参数读写, 界面刷新, 任务添改base @status102
* 新的添加任务 @status102
* 添加界园入口 @hguandl

### 改进 | Improved

* 优化加载顺序, 简化StageManager变量 (#13277) @status102
* 肉鸽任务参数初始化简化 @status102
* 不期而遇子事件可读性优化 @SherkeyXD
* 先筛选未进驻再清空队列 @ABA2396
* 优化日志输出 @ABA2396
* 调整干员罗比菈塔在肉鸽使用的技能 @Saratoga-Official
* 界园肉鸽中停止招募锡人 @DavidWang19
* 优化界园不期而遇默认逻辑 @DavidWang19
* 优化指点迷津节点task的复用 @DavidWang19
* 重构部分预载 @status102

### 修复 | Fix

* 手动改时间或睡眠跨日后关卡列表与提示未更新 @ABA2396
* v5.20.0-beta.2.d037 重启后概率闪退 @ABA2396
* 刷理智代理倍率移除1000适配 @status102
* YostarEN OR navigation @Constrat
* YostarEN Eye for an Eye Sarkaz Encounter @Constrat
* YostarKR 以血还血 ocr tweak @HX3N
* 尝试修复点击过头导致选到月度小队 @SherkeyXD
* yostaren sarkaz encouter ocr tweaks @Constrat
* 缺 dll 和 resource 的时候没弹喜报 @ABA2396
* 勾选｢反转主任务右键单击效果｣时，半选主任务在重启后无法还原选中状态 @ABA2396
* 最小化会在桌角留下一个小窗口 @ABA2396
* 勾选启动后直接最小化后无法拖动窗口 @ABA2396
* 更新关卡ocr替换 @DavidWang19
* 肉鸽导航无法选择傀影肉鸽 (#13255) @Lemon-miaow
* 修复在得偿所愿或其他位置选择投钱后卡住的问题 @DavidWang19
* 修复检测不到next的问题 @DavidWang19
* typo @status102
* 调整指点迷津节点阈值 @DavidWang19

### 其他 | Other

* 界园肉鸽没有二希望开（？） @ABA2396
* YostarJP ocr edits (#13276) @Manicsteiner
* bump maa-cli to 0.5.7 (#13278) @wangl-cc
* 肉鸽烧水参数rename @status102
* 肉鸽开局奖励选择 @status102
* TaskSettingVisibilityInfo.Current -> Instance @status102
* YostarKR OR ocr edits @HX3N
* 长草任务当前选中index @status102
