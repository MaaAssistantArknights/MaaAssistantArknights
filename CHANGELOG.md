## v6.4.0

### 何忆卫？ | Highlight  

这一版更新不大，但都是提升使用体验的改动~

#### 自动战斗优化

在这个版本，我们优化了自动战斗的多项体验：

* 我们针对没有跳过按钮的对话流程进行了优化，使其能被正确处理，跳过对话后能继续保留二倍速状态；
* 现在其他活动选项卡也支持加载作业集，可通过作业列表快捷切换不同作业（仅支持手动切换，不支持如主线/SideStory 的多作业连续作战）。

我们还优化了其他内容。

#### 功能适配

在这个版本，我们支持了国服的游戏内暂停期间可部署干员的功能。

国际服方面，我们新增了对国际服次生预案自动刷取的支持，还是那句话：「次生预案」玩法类似《循环勇士》，带有种田要素，越早布置生产产线，就能越早挂机产出资源。不要等到最后一天再开始刷，会来不及。

我们新增了对国际服“齐聚”主题的支持。

对于游戏 MAC 端，我们也适配了 PlayCover 的 BGR 优化截图。

对于游戏 PC 端，由于其安全策略限制，如果需要牛牛控制 PC 端，则需要以管理员身份运行 MAA。 ~~（不是终末地那个咕咕嘎嘎小企鹅）~~

> ⚠️ 注意：控制 PC 端时仍需保持游戏窗口**不能**最小化，并且目前仅支持真实鼠标点击（不能模拟，会抢鼠标）。

#### 其他方面

我们优化了包括自动检测连接、每次重新检测、PC 端连接设置、自动编队忽略干员属性要求等多处文本描述，以便你更好地理解相关内容。

---

This update is minor, but all changes improve the user experience.

#### _Copilot_ Optimization

In this version, we've optimized several aspects of the _Copilot_ experience:

* We've optimized dialogue flows without a skip button, ensuring they are handled correctly and that the 2x speed mode is retained after skipping dialogue.

* _Other Activities_ tabs now support loading job sets, allowing quick switching between different jobs via the Job List (manual switching only; continuous multi-job operation like in MainTheme/SideStory is not supported).

We've also optimized other content.

#### Feature Adaptation

**\[CN ONLY\]** In this version, we support the ability to deploy operators during in-game pauses in the Chinese server.

**\[GLOBAL ONLY\]** We've added support for automatic farming of _Rebuilding Mandate_. As mentioned before, _Rebuilding Mandate_ is similar to _Loop Hero_, with resource-gathering elements. The sooner you set up your resource production in the stages, the sooner you can start passively generating resources. Don't wait until the last day to start farming; it will be too late.

**\[GLOBAL ONLY\]** We've added support for the _Together_ theme.

For the Mac version of the game, we've also adapted PlayCover's BGR-optimized screenshots.

For the PC version of the game, due to its security policy restrictions, if you need MAA to control the PC version, you need to run MAA as an administrator. ~~(Not the Endministrator from the Endfield)~~

> ⚠️ Note: When controlling the PC version, the game window **must not** be minimized, and currently only real mouse clicks are supported (simulated clicks are not supported, and MAA will take over mouse control to perform automatic clicks).

#### Other Aspects

We have optimized several text descriptions, including _Auto detect connection_, _Re-detect every times_, _Connection_ setting for PC version of the game, and _Ignore Operator requirements_ for _Auto Squad_, to help you better understand the relevant content.

---

### 以下是详细内容：

### 新增 | New

* 调整 mumu 国际版截图增强支持提示 @ABA2396
* 合并 `处于战斗中` 检测, 增加倍速记忆, 支持战斗中不带跳过按钮的带头像对话教程 (#15706) @status102
* 设置界面选项页允许折叠 @ABA2396
* PC 端运行时要求以管理员身份重启 MAA (#15748) @SherkeyXD
* 自动战斗-其他活动支持使用作业列表加载作业集并从中选择作业 (#15804) @status102
* JP RebuildingMandate and Together theme (#15931) @Manicsteiner
* YostarEN RebuildingMandate + Together @Constrat

### 改进 | Improved

* 优化自动检测连接与每次重新检测连接提示 @ABA2396
* ASST_DEBUG 在 lazy_parse 后不再重新生成任务信息 (#15895) @status102
* scopeLog 记录函数进出时间 @status102
* TaskQueue 任务属性变更记录路径优化 @status102
* 适配国服游戏内暂停期间部署 (#15888) @status102
* 适配 PlayCover 的 BGR 优化截图 (#15810) @hguandl
* EN, JP, KR MiniGame AT「相談室‧御影」小遊戲 (#15870) @Daydreamer114
* optimize png @Constrat

### 修复 | Fix

* 修复今日关卡小提示中奶盾芯片数量顺序错误 @ABA2396
* 移除不必要的配置迁移触发条件 @status102
* 修改任务名过滤换行 @status102
* Fix YoStarJP Sidestory GO OCR error (#15906) @JasonHuang79
* Make Starting Operator searching dropdown case insensitive for en (#15879) @Zenith00

### 文档 | Docs

* 使文档站中侧边栏的功能介绍板块默认展开 (#15927) @lucienshawls

### 其他 | Other

* 调整每次重新检测勾选框选中时的颜色 @ABA2396
* 繁中服「墟」導航文字 (#15925) @momomochi987
* `战斗列表` 选项更名为 `多作业模式`, 以区分于作业容器 (#15913) @status102 @Constrat @HX3N @Manicsteiner
* 更正最小化的描述 @MistEO
* 调整 PC 端输入选项及描述 @MistEO
* ClientType 常量替换 @status102
* 剿灭 string 常量 @status102
* 刷理智筛选候选关卡后在日志中记录 @status102
* 自动战斗协议 `干员技能默认值` 变更为 0 (不指定技能) (#15898) @status102
* 自动战斗跳过干员属性要求提示 增加 `精英化例外` 及 `跳过后不点赞说明` (#15881) @status102 @Constrat @HX3N @momomochi987
* macOS build scripts (#15936) @hguandl
* MaaDeps cache asset (#15933) @hguandl
* KR RebuildingMandate ocr edit @HX3N
* EN Rebuilding Mandate @Constrat
* KR RebuildingMandate and Together UiTheme (#15932) @HX3N
* cleanup warnings - unused variables and unused includes @Constrat
