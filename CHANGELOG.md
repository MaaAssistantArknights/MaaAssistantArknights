## v5.26.2

### Highlight

Support for Masses' Travel event (global)
Support for Duel Channel minigame (global)
Fixes for IS2 Phantom difficulty update (global)
Various fixes for global interface changes (global)

### 新增 | New

* 支持访问好友半透明界面 @ABA2396

### 修复 | Fix

* ReceptionMini for EN @Constrat
* Various store OCR for EN IS @Constrat
* MT minigame EN @Constrat
* operbox, battlequick, infrast expand role EN @Constrat
* Exusiai and Hoshiguma prepare for alters @Constrat
* increase ROI for Orundum and SpecialAccess @Constrat
* OrundumActivities MT event for EN @Constrat
* update ChooseDifficulty for EN phantom @Constrat
* revert IS2 EN StrategyChange changes @Constrat
* add IS2 invest mode to EN, JP and txwy @Constrat
* POSIX compliant @Constrat
* preload Minigame EN @Constrat
* python formatting @Constrat
* KR monthly card reward @HX3N
* task sorter UTF-8 BOM fix @Constrat
* EN monthly card reward @Constrat
* YostarKR update VisitNext @HX3N
* update VisitNext ref #14357 @Constrat

### 文档 | Docs

* update prts.plus repo link @MistEO
* 移除前端公招 @MistEO
* 更新plume主题并适应新版的collections配置 (#14360) @lucienshawls

### 其他 | Other

* revert temp fix 715c2c13b5372dc769aa0b9efe2be551cd200192 for EN / JP and KR @Constrat
* Enable phantom roguelike difficulty for Yostar servers (#14332) @Manicsteiner
* update MT mini event EN @Constrat
* YostarKR MT ocr edit (#14398) @HX3N
* YostarKR MiniGame (#14395) @HX3N
* YostarJP update VisitNext (#14390) @Manicsteiner
* 调整Award模板阈值 @Saratoga-Official
* auto run smoke testing on tools/SmokeTesting modifications @Constrat
* YostarKR preload minigame (#14375) @HX3N
* preload minigame for MT EN @Constrat
* YostarJP MT stages and Duel channel (#14362) @Manicsteiner
* KR MT navigation (#14365) @HX3N
* 16-16地图 @status102
* remove global info for mumu @Constrat
* preload MT navigation EN @Constrat

----
----

## v5.26.1

### 新增 | New

* 新增 cdk 被封禁的提示信息 @ABA2396
* RM-1 (#14271) @Daydreamer114

### 改进 | Improved

* RegionOCRer 中 useRaw=false 时, 使用原图二值蒙版代替直接 OCR 二值图像 (#14276) @status102

### 修复 | Fix

* 游戏更新公招界面后无法确认招募 (#14335) @ABA2396
* 第一次访问 mirror酱 失败时错误提示 cdk 已过期 @ABA2396
* 手动关闭模拟器后未重启 MAA 时 minitouch 可能失效 @ABA2396
* 尝试修复生息演算任务识别并删除编队时卡住的问题 (#14290) @Alan-Charred
* 增强 playtools 关闭连接时的异常处理，确保套接字安全关闭 (#14280) @RainYangty
* EN IS3 encounter ocr fix MAA, EN 服水月肉鸽 事件名识别错误 bug Fixes @Constrat
* 理智药使用数量 ocr 不准确时中断使用 @status102
* 使用理智药 执行减少次数循环在 asst_stop 时缺少中断判断 @status102
* 修复因失败导致次生预算出错 (#14267) @Saratoga-Official

### 文档 | Docs

* 补充 CopilotTask 的文档 (#14319) @Alan-Charred
* 添加目录自动跳转组件并使 locale 自动生成 (#14299) @lucienshawls
* 文档站新增字符画组件 (#14270) @lucienshawls
* 将文档中指向部分文档目录的链接改为指向对应目录下的第一篇文档 (#14292) @JasonHuang79

### 其他 | Other

* 使用 `BeginAnimation` 替代 `新建 Storyboard 并添加动画` @ABA2396
* 将 mac 开发环境下的 cmake_osx 版本改为 13.4 (#14283) @Pylinx171
* 完善容器配置及依赖安装 (#14208) @lucienshawls
* run smoke test in lldb @horror-proton
* YostarJP ocr fix @Saratoga-Official

----
----

## v5.26.0

### 新增 | New

* 萨卡兹肉鸽 `待诉说的故事` 二次选择 (#14246) @Manicsteiner
* mac 支持次生预案 @ABA2396
* 次生预案十里坡剑神 @ABA2396
* 设置指引添加更新设置 @ABA2396
* 统一显示效果 @ABA2396
* 设置指引添加性能设置 @ABA2396

### 改进 | Improved

* 调整界园肉鸽招募策略 (#14255) @Saratoga-Official
* 优化界园肉鸽部分关卡策略 (#14244) @Lancarus
* 优化次生预案执行速度 @ABA2396

### 修复 | Fix

* 商店刷新两步走 (#14201) @Alan-Charred
* 水月肉鸽商店刷新延迟不够 @Saratoga-Official
* 按钮显示文字错误 @ABA2396
* 次生预案模拟器卡了容易点过头 @Saratoga-Official
* 怎么还有人在用 adb input @ABA2396
* 文档首页语言选择按钮的宽度定义方式 (#14199) @lucienshawls
* 傀影肉鸽无法识别四结局 (#14193) @Saratoga-Official
* GamePassSkip2 识别到错误的跳过 @Saratoga-Official
* 萨米肉鸽不期而遇避战 @Saratoga-Official
* clang @Constrat
* Google Play Games Developer shutdown @Constrat
* manual set resource version time @MistEO
* prettier @Constrat

### 文档 | Docs

* 更新文档 (#14236) @Rbqwow @Saratoga-Official
* 补充 vsc 插件繁中文档 @Rbqwow
* 修复文档站 readme 盾换行 @Rbqwow
* 调整文档站的标题和尾注文本显示 (#14213) @lucienshawls
* 更新网页开发相关文档 (#14167) @Rbqwow @Manicsteiner
* 完善任务流程协议文档 (#13232) @zzyyyl
* 回调消息协议文档视觉更新 @SherkeyXD
* 集成文档视觉更新 @SherkeyXD
* 文档启用b站视频播放功能 @SherkeyXD
* 文档添加字段容器功能 @SherkeyXD
* 文档添加功能 @SherkeyXD
* 更新文档编写指南 @SherkeyXD
* 标题 MAA 统一采用缩写 @MistEO
* 再次调整文档站标题（ @MistEO
* 调整文档站标题 @MistEO
* Update JP(#14227) @wallsman
* markdown pre-commit @zzyyyl
* add extension's evaluating feature @neko-para
* add telegram icon @SherkeyXD

### 其他 | Other

* 新图标和界面风格 @hguandl
* 水月萨米肉鸽不期而遇避战 @Saratoga-Official
* 重写完成后动作仅一次的ui字符串 (#14196) @Rbqwow
* 贸易站没其他好用的人再用锏 @ABA2396
* YostarJP roguelike edits (#14252) @Manicsteiner
* YostarJP Sarkaz roguelike StageEncounter (#14223) @Manicsteiner
* YostarJP sami roguelike 720p (#14210) @Manicsteiner
* YostarJP Mizuki StageEncounter (#14206) @Manicsteiner
* devcontainer.json (#14169) @Rbqwow @lucienshawls
