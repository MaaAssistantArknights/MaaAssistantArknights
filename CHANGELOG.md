## v5.20.0-beta.3

### 新增 | New

* macos 版本添加界园入口 @hguandl
* maa-cli 版本初步适配界园肉鸽 (#13278) @wangl-cc

### 改进 | Improved

* 界园肉鸽更新战斗逻辑、事件选择 @DavidWang19 @ABA2396 @Saratoga-Official @SherkeyXD
* 新Config任务参数读写, 界面刷新, 任务添改base @status102
* 优化加载顺序, 简化StageManager变量 (#13277) @status102
* 肉鸽任务参数初始化简化 @status102
* 先筛选未进驻再清空队列 @ABA2396
* 优化日志输出 @ABA2396
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

* YostarJP ocr edits (#13276) @Manicsteiner
* TaskSettingVisibilityInfo.Current -> Instance @status102
* YostarKR OR ocr edits @HX3N
* 长草任务当前选中index @status102
