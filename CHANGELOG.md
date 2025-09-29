## v5.26.0

### 刺身鱼案，启动！ | Highlight

这个版本我们上线了「次生预案」的初步支持，并且再次对自动肉鸽进行了大量优化。

#### 次生预案部分

你现在可以在【小工具】的“小游戏”部分找到“RM-次生预案”选项，在游戏的「次生预案」活动的“前哨支点”关卡列表里，找到最开始的教学关卡 RM-TR-1 荒地前哨，点击 MAA 的 Link Start! 按钮即可开始自动刷取。

由于是~~十里坡剑神~~初版，功能还比较有限，目前仅支持自动通关教学关卡，只能刷取少量奖励代币，后续不排除牛牛不出新手村的可能，建议你还是在刷取足够的启动材料 ~~（把 ew 精 2 后）~~ 后往后面的关卡前进。

**提示：由于「次生预案」是类“循环勇士”玩法，也就是有种田要素，所以越早将关卡的生产产线布置好，就能越早开始挂机产出资源。不要等到最后一天再开始刷，会来不及的。**

#### 自动肉鸽部分

本次我们对自动肉鸽的招募策略进行了较大幅度的调整，主要是为了提升肉鸽的通关率和稳定性。同时我们也修复了部分问题，比如傀影肉鸽无法识别四结局的问题。

#### 其他方面

我们重置了 MAA 的新手指引部分，现在应该不会再出现没看新手指引瞎设置的问题了吧。

我们也对文档站进行了较大幅度的改版，增加了不少新功能，提升了阅读体验，欢迎你前往查看。

----

In this version, we've added initial support for the *Rebuilding Mandate* and made significant improvements to the *Auto I.S.*.

#### [CN ONLY] *Rebuilding Mandate*

You can now find the *RM-Rebuilding Mandate* option in the *Minigames* section of the *Toolbox* menu. Within the *Rebuilding Mandate* event, locate the tutorial stage *RM-TR-1* in the *Outpost Support Point* stage list and click the *Link Start!* button to begin automated gameplay.

Since this is the initial version, the functionality is limited. Currently, it only supports automatically completing the tutorial stage and can only obtain a small amount of reward tokens. We can't guarantee that you'll be able to progress beyond the tutorial stage via MAA, so we recommend that after gathering enough resources (e.g., upgraded Wiš'adel to Elite II) you can try to attempt advanced stages.

**Tip: Since *Rebuilding Mandate* is a game similar to *Loop Hero* (with resource-gathering gameplay), the sooner you set up your resource production system in the stage, the sooner you can start passively generating resources. Don't wait until the last day to start farming; it will be too late.**

#### *Auto I.S.*

We've made significant adjustments to the *Auto I.S.* recruitment strategy, primarily to improve the success rate and stability. We've also fixed some issues, such as the problem where the game couldn't recognize the Ending 4 in the *Phantom* Theme.

#### Other Improvements

We've revamped the Settings Guide. Hopefully, this will prevent users from making incorrect settings without reading the tutorial.

We've also redesigned our documentation website, adding many new features and improving the user experience. Please check it out!

----

以下是详细内容：

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
