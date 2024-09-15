## v5.6.1

### 新增 | New

* 移除在非windows上locale的实现 (#9680) @soundofautumn
* time diff log output and notification (#10562) @Constrat @status102
* new theme for txwy (#10566) @Manicsteiner
* YostarEN Rainbow6Siege theme @Constrat

### 改进 | Improved

* 傀影肉鸽优化雕匠与石像作战策略 (#10613) @Daydreamer514
* 使用 devices 判断是否需要连接 (#10606) @ABA2396
* 优化萨米肉鸽半吊子之旅部署策略 (#10576) @Daydreamer514
* 优化 connect 逻辑 (#10571) @ABA2396
* 增强任务和基建列表健壮性，部分异常配置下可自动还原 @ABA2396
* 更新 google play beta 开发者模拟器相关文档 (#10572) @Rbqwow
* 优化肉鸽文档对 key 干员和 0 希望干员的描述 @Daydreamer514
* 优化傀影肉鸽覆水难收撤退策略 (#10541) @Daydreamer514
* 公招识别得分较低 @ABA2396
* Wpf版本号显示最小宽度调整 @status102
* 重构 ProcessTask (#10001) @zzyyyl
* 调整`MumuExtras未生效`->`Mumu截图增强未生效` @status102
* 拆分技能识别与使用 (#10536) @ABA2396
* 允许 TimerCanceledAsync 提前返回 @ABA2396
* 优化傀影肉鸽雕匠与石像部署策略 (#10441) @Daydreamer514
* 优化傀影肉鸽 ew, logos 招募策略 @Daydreamer514
* 优化傀影肉鸽从众效应部署策略 @Daydreamer514
* resource updater validator in native powershell (#10493) @Constrat

### 修复 | Fix

* try to fix gen-changelog @status102
* 重置战斗参数状态错误 @ABA2396
* EN R6S collab regex @Constrat
* missing parameter for system notification fix https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/10562#issuecomment-2342509182 @Constrat
* EN EnterInfrastSiege Apparently reverting was not a fix (don't know how it worked). @Constrat
* 阻止休眠时保持屏幕常亮功能无效 @ABA2396
* 将肉鸽招募的等级限制改为精一55级 @Daydreamer514
* Reclamation scroll down (#10554) @Manicsteiner
* 尝试修复日服理智药使用数量识别 @status102
* 未输入完整开局干员时点选会清空已输入内容 @ABA2396
* xaml escape character fix ee96efe @Constrat
* YostarEN EnterInfrast Siege theme without this the score is MistCity: 0.62. No idea how they are different @Constrat
* Reclamation for Yostar servers (#10498) @Manicsteiner

### 文档 | Docs

* 修正容器的默认标题介绍 (#10593) @Daydreamer514
* 补充战斗流程协议对 delay 的描述 @Daydreamer514
* 肉鸽文档招募 offset 添加默认值描述 (#10552) @Daydreamer514

### 其他 | Other

* RA2 replace brush with farm (#10610) @dragonheart107
* Update EN CR navigation EX stages @Constrat
* EN reduced copilot output @Constrat
* international english date format @Constrat
* 添加移动 MAA.exe 的解决方案 (#10565) @ABA2396 @HX3N
* KR tweaked translations (#10560) @HX3N
* Revert 02dc16d Somehow now it works with the original CN template. No idea of what changed. @Constrat
* Shutdown() 增加输出调用者信息 @ABA2396
* 关机倒计时前显示窗口 @ABA2396
* Add logging for timer cancellation status @ABA2396
* 增加未取消关机的日志 @ABA2396
* 加点关机日志 @ABA2396
* en-us Expiring potion hour consistency @status102 should give a better idea @Constrat
* 调整使用技能间隔日志输出等级，调整间隔时间 @ABA2396
* modify files to allow changes @Constrat
* Add JP「オペレーション ルーセントアローヘッド」 (#10501) @wallsman
* 修改存储的构建日期 @ABA2396
