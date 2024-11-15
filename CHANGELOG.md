## v5.10.0

### Highlight

本来想发个 5.9.1 解决下导航和线索问题的，但 bot 到处拉屎，冲突解决不过来了，干脆发个 5.10.0，出了问题再 .1）

### 新增 | New

* Mac GUI 肉鸽改进 & 一键资源更新 (#11121)(#11123) @hguandl
* 远程通知配置加密(重新输入后生效) (#11076) @ABA2396
* 新增 153 4 班排班表 @ABA2396
* 开始前/结束后 脚本用双引号包裹时支持带参数 @ABA2396
* 自动编队允许指定助战单位 (#11075) @Alan-Charred @status102 @Daydreamer114

### 改进 | Improved

* 移除SettingsViewModel不必要的绑定 @status102
* 优化DataContext绑定 @status102
* WpfGui优化任务进度显示 (#11055) @status102
* 更新 153/4 宿舍排班 @ABA2396
* 公招三星优先tag 改为复选框 (#11097) @AlezHibali @status102
* 更新 macos.cmake 中的库版本 (#11101) @Alan-Charred @pre-commit-ci[bot]
* 优化访问好友模板匹配 @ABA2396
* 更新 153/3 换基建配置，宿舍全指定 @ABA2396

### 修复 | Fix

* 无法进入通用图标的活动 @ABA2396
* 外部通知无法进行发送设置 @status102
* 根据roi进行截图的小工具无法识别文件路径 (#11114) @Daydreamer114
* 修复连接设置下 开始前、结束后脚本及运行任务时休眠设置绑定 @status102
* 修复 连接设置 数据绑定 @status102
* 误点一键换宿舍 @ABA2396
* 尝试修复进度条问题 @ABA2396
* 修复肉鸽选难度的若干 Bug (#11131) @Alan-Charred
* 移除了邮件通知中可能的非法字符 (#11110) @LogicDX342
* 从 wpf 层面上不再允许用户在萨卡兹肉鸽用奇怪的分队刷源石锭 (#11125) @Alan-Charred @ABA2396
* 二星干员无法开始悖论模拟 @ABA2396
* 只凹直升的时候不再需要去 N0 打到 3 层 (#11124) @Alan-Charred
* 自动战斗未启用战斗列表时，禁用理智药使用 @status102
* 肉鸽不期而遇闪退 (#11087) @Daydreamer114
* 萨卡兹肉鸽 原初异途 点进一片莲瓣 (#11106) @Daydreamer114
* 调整自动战斗参数默认值 @status102
* 萨卡兹肉鸽降低 Sarkaz@Roguelike@CloseCollectionContinue 阈值 (#11090) @Daydreamer114
* 导航进商店 @ABA2396
* ExternalNotificationTips 未居中 (#11093) @Rbqwow @pre-commit-ci[bot]
* Greyy alter and BattleQuickFormationOCR opencv tweaks @Constrat
* click 泊松分布超出范围 @ABA2396
* 为 PVChapterToPV 添加 1 秒的 postDelay (#11074) @Alan-Charred
* enlarge rectMove offset for PV-10 (#11073) @Alan-Charred
* pv-10 @ABA2396
* 修正繁中服薩米肉鴿坍縮範式的官方名稱 (#11066) @sjdei
* 修改任务/基建列表顺序后未开始任务会在程序重启后失效 @ABA2396
* Server酱通知推送无法正常推送到微信服务号 (#11057) @foreverlong
* UI 更新后无法开趴 @ABA2396

### 文档 | Docs

* update ko-kr docs (#11136) @rosmontisu @pre-commit-ci[bot]
* Fix typo in docs/ko-kr/manual/newbie.md (#11134) @rosmontisu
* tasks doc clarification [skip changelog] @Constrat
* 添加用户群链接 @ABA2396

### 其他 | Other

* YostarJP ocr update (#11155) @Manicsteiner
* 移动统一部分设置路径 @status102
* Translated using Weblate (Korean) @rosmontisu
* 拆分 设置-连接设置 (#11120) @status102
* YostarEN Sami@Roguelike@GamePassSkip1 (#11148) @Manicsteiner
* Translations update from MAA Weblate (#11138) @AlisaAkiron @rosmontisu
* 拆分 设置-外部通知 (#11113) @status102
* YostarJP phantom roguelike ocr update (#11084) @Manicsteiner
* YostarKR fixed the reversed description for ReclamationEarlyTip (#11078) @HX3N
* 移除最小化启动模拟器选项 @ABA2396
* YostarJP clue and ocr update (#11053) @Manicsteiner @pre-commit-ci[bot]
