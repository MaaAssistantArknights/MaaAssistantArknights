## v5.26.1

### 刺身鱼案，启动！ | Highlight

这个版本我们上线了「次生预案」的初步支持，并且再次对自动肉鸽进行了大量优化。

#### 次生预案部分

你现在可以在 ｢小工具 - 小游戏｣ 部分找到 ｢RM-次生预案｣ 选项，在游戏的「次生预案」活动的 ｢前哨支点｣ 关卡列表里，找到 `RM-TR-1` 或 `RM-1`，点击 MAA 的 ｢Link Start!｣ 按钮即可开始自动刷取。

`RM-1` 支持更多资源产出，但需解锁前置要求，具体参考对应功能页描述。活动即将结束，建议尽快刷取足够启动材料并推进关卡。

**提示：** 「次生预案」玩法类似《循环勇士》，带有种田要素，越早布置生产产线，就能越早挂机产出资源。不要等到最后一天再开始刷，会来不及。

#### 自动肉鸽部分

本次我们对自动肉鸽的招募策略进行了较大幅度的调整，主要是为了提升肉鸽的通关率和稳定性。同时我们也修复了部分问题，比如傀影肉鸽无法识别四结局的问题。

#### 其他方面

我们在设置指引中新增了两个新的引导，不管你以前是否看过老的指引，这次都会再次弹出，帮助你避免因未正确配置而导致的问题。~~这下就不会有人问为什么 Linkstart 是灰的点不了了~~

我们也对文档站进行了较大幅度的改版，增加了不少新功能，提升了阅读体验，欢迎你前往查看。

----

In this version, we've added initial support for the *Rebuilding Mandate* and made significant improvements to the *Auto I.S.*.

#### [CN ONLY] *Rebuilding Mandate*

You can now find the *RM-Rebuilding Mandate* option in the *Minigames* section of the *Toolbox* menu. Within the *Rebuilding Mandate* event, locate `RM-TR-1` or `RM-1` in the *Outpost Support Point* stage list and click the *Link Start!* button to begin automated gameplay.

`RM-1` supports higher resource output but requires unlocking certain prerequisites. Please refer to the corresponding feature page for details. The event is ending soon, so it is recommended to quickly gather enough starting materials and advance through the stages.

**Tip:** *Rebuilding Mandate* is similar to *Loop Hero*, with resource-gathering elements. The sooner you set up your resource production in the stages, the sooner you can start passively generating resources. Don't wait until the last day to start farming; it will be too late.

#### *Auto I.S.*

We've made significant adjustments to the *Auto I.S.* recruitment strategy, primarily to improve the success rate and stability. We've also fixed some issues, such as the problem where the game couldn't recognize the Ending 4 in the *Phantom* Theme.

#### Other Improvements

We've added two new guides to the *Settings Guide*. No matter if you have already seen the old one before, these new guides will still pop up, helping you avoid incorrect configurations.

We've also redesigned our documentation website, adding many new features and improving the user experience. Please check it out!

----

以下是详细内容：

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
