## v6.2.2

### 新增 | New

* 只有一个配置时标题栏隐藏配置名 @ABA2396
* 检查更新失败时吐司通知 @ABA2396
* 一图流上报 id 单独实现 @ABA2396
* 新增截图增强模拟器路径的选择窗口 @ABA2396
* 自动战斗-自动编队支持技能等级要求 (#15355) @status102
* 设置指引中添加使用指引页 @ABA2396

### 改进 | Improved

* 悬浮窗改static @status102
* 调整 ViewModels 目录结构，规范 ViewModels 命名 (#15389) @ABA2396
* 优化动画效果 @ABA2396

### 修复 | Fix

* phantom puppet not deploying @Constrat
* AT ClickStage t -> T @Constrat
* 牛牛抽卡点击停止之后文字提示不会停止变化 @ABA2396
* 萨米肉鸽StartAction @Saratoga-Official
* 矿石“杀手”识别 @Saratoga-Official
* 肉鸽增加黑屏等待时间 @Saratoga-Official
* 悖论模拟-作业列表 额外等待 @status102
* build warring @ABA2396
* 仓库数据 parse 失败时返回空数据 @ABA2396
* 肉鸽快速编队按钮边缘无法点击导致出错 @Saratoga-Official
* 修复了特殊路径下执行adb命令可能失败的问题(#15381) (#15382) @sylw114
* add AsstGetImageBgr to wine shim @dantmnf
* 修复 TaskDialog 按钮文本赋值时的语法错误及空引用保护 (#15377) @hcl55

### 文档 | Docs

* 更新开发指南 @ABA2396
* Update cli usage.md with Windows installation note (#15369) @JasonHuang79

### 其他 | Other

* 力竭了 @ABA2396
* EN tweak to inventory display @Constrat
* AT updates @Constrat
* 抽卡任务结束后提示还原为默认 @ABA2396
* YostarJP AT updates @Manicsteiner
* YostarKR AT updates @HX3N
* Preload AT + new alter operators regex for EN @Constrat
* YostarJP AT preload and ocr edit @Manicsteiner
* 下调一点仓库识别二值化阈值 @ABA2396
* update OrundumActivities for EN @Constrat
* 制造站加入苍苔组，避免苍苔与阿罗玛冲突 (#15343) @drway
* 地块类型标注 TileType (#15373) @status102
* 收到关卡名识别失败回调时更新卡片 @ABA2396
