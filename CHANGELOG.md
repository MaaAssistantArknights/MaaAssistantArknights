## v6.1.0-beta.3

### Highlights

#### 日志悬浮窗与肉鸽优化 | Log Floating Window & Roguelike Fixes

新增日志悬浮窗功能，可在右键菜单切换，实时查看自动战斗日志。
修复肉鸽快速编队点击过快和卡顿导致的误操作问题。
新增隐秘战线支持，修复 EN 服务器关卡名识别和模板问题。

----

#### Log Floating Window & Roguelike Stability

New log floating window accessible from right-click menu for real-time battle log viewing.
Fixed roguelike quick squad formation click timing and lag-induced misclick issues.
Added SecretFront support, fixed EN stage name recognition and template issues.

----

### 新增 | New

* 支持点击标题栏左上角的检测到新版本提示触发更新 @ABA2396
* 右键图标菜单栏中增加日志悬浮窗切换 @ABA2396
* 悬浮窗支持自动战斗日志源 @ABA2396
* YostarEN SecretFront (Hidden Front) @Constrat
* 日志悬浮窗 (#15185) @ABA2396
* 在任务开始后的禁用组件内也能查看tooltip (#15186) @yali-hzy
* ProcessTask任务命中缓存结果 (#12651) @status102

### 改进 | Improved

* RefreshCustomInfrastPlanIndexByPeriod 支持传入当前时间 @ABA2396
* optimize templates for secrentfront @Constrat

### 修复 | Fix

* 避免肉鸽快速编队点太快可能点不上 @Saratoga-Official
* 肉鸽因模拟器卡顿可能点进招募界面 @Saratoga-Official
* allow usage of CLI build instead of only VS (#15190) @Constrat
* 萨米积冰岩骸识别 @Saratoga-Official
* 保全关卡名识别 @ABA2396
* 两个检测更新的按钮在更新时禁用 @ABA2396
* 未勾选自动下载更新包且无可用的 OTA 增量更新包时仍然提示“将下载完整包xxx” @ABA2396
* more tongbaso regex en @Constrat
* 单切换账号时，任务运行计时错误 @status102
* 招募助战后补充低信赖干员数量不足 (#15184) @yali-hzy
* BattleStageName requiring ^$  for EN @Constrat
* wrong template for EN @Constrat

### 其他 | Other

* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* EN typo @Constrat
* remove codeql workflow @SherkeyXD
* 资源更新忽略 .gitignore 文件 @ABA2396
* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* Update C# EditorConfig for c# 13 & 14 (#15146) @status102
* KR improve client display names @HX3N
