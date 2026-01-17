## v6.2.2

### Highlights

随着米诺斯 SS 的到来，我们迎来了一位全新的萨尔贡六星干员，~~那么牛牛的米诺斯大C在哪呢？~~

#### 日志侧边栏重构

本次更新我们对主页日志侧边栏进行了整体重构，统一并优化了布局与视觉样式。

现在你可以在「设置 → 界面设置」中控制【使用卡片样式日志】设置来切换日志样式，控制【日志缩略图最大数量】设置来决定日志侧边栏的缩略图数量上限（这些缩略图都是临时保存在内存里的，**内存较小的用户可能需要调整数量**）

#### 自动战斗优化

我们优化了自动战斗功能的作业路径下拉选择栏，现在同一个任务的本地作业将会折叠在一起，大幅度提升了下拉选择栏的可读性。

另外，本次游戏更新 yj 修改了暂停按钮的位置，我们已经通过热更新修复该问题，本次更新 MAA 也会合并此项修复。

#### 今日关卡提示优化

今日关卡提示中的掉落信息现已支持同步显示对应的库存数量，便于在刷图规划时快速判断材料缺口与优先级。

你可通过「小工具 → 仓库识别」更新库存数量，今日关卡提示中的掉落信息会自动同步更新。

#### 基建与线索管理优化

我们在「一键长草 → 基建换班 → 高级设置」里新增了【进行线索交流】【赠送线索】两项独立开关设置，提升基建管理的灵活性。

我们也优化了一键赠送线索等的逻辑，减少误操作发生的可能性。

同时，在基建进入设施失败时牛牛将自动保留测试截图，便于后续问题的定位与排查。

#### 成就系统 DLC

我们新增了一些成就，覆盖多个场景，等你探索哦~

----

#### Log Sidebar Redesign

In this update, we've completely redesigned the main page log sidebar, unifying and optimizing its layout and visual style.

You can now switch log styles by controlling the *Use card style log* setting in *Settings → GUI*, and control the *Maximum number of log thumbnails* setting to determine the maximum number of thumbnails in the log sidebar (these thumbnails are temporarily stored in memory, **users with limited memory may need to adjust the number**).

#### *Copilot* Optimization

We've optimized the task path dropdown selection bar for the *Copilot*. Local tasks for the same stage will now be collapsed together, significantly improving the readability of the dropdown selection bar.

Additionally, the game update changed the position of the pause button. We have fixed this issue through a hot update, and this fix has been merged into the MAA update.

#### *Today's open stages* Hint Optimization

The drop information in the *Today's open stages* hints now supports synchronous display of the corresponding depot quantity, making it easier to quickly determine material shortages and priorities when planning your stage runs.

You can update your depot info through *Toolbox → Depot*, and the daily drop information in the *Today's open stages* hints will be automatically updated.

#### *Base* and Clue Management Optimizations

We've added two independent toggle settings for *Conduct Clue Exchange* and *Send Clues* in *Farming → Base → Advanced*, improving the flexibility of *Base* management.

We've also optimized the logic for one-click clue sending, reducing the possibility of accidental operations.

Additionally, when *Base* entry fails, MAA will automatically save a test screenshot for easier troubleshooting and identification of subsequent issues.

#### Achievement System DLC

We've added some achievements covering multiple scenarios, waiting for you to explore!

----

以下是详细内容：

## v6.2.2

### 新增 | New

* 只有一个配置时标题栏隐藏配置名 @ABA2396
* 检查更新失败时吐司通知 @ABA2396
* 新增截图增强模拟器路径的选择窗口 @ABA2396
* 自动战斗-自动编队支持技能等级要求 (#15355) @status102
* 设置指引中添加使用指引页 @ABA2396

### 改进 | Improved

* 调整 ViewModels 目录结构，规范 ViewModels 命名 (#15389) @ABA2396
* 优化动画效果 @ABA2396
* 优化仓库识别二值化阈值 @ABA2396
* 悬浮窗改 static @status102

### 修复 | Fix

* 界园指挥避战刷钱增加黑屏等待 @Saratoga-Official
* 有猪把成就注释掉了 @ABA2396
* 修复特殊路径下执行 adb 命令可能失败的问题 (#15381) (#15382) @sylw114
* 修复 TaskDialog 按钮文本赋值时的语法错误及空引用保护 (#15377) @hcl55
* 修复牛牛抽卡点击停止之后文字提示不会停止变化 @ABA2396
* 仓库数据 parse 失败时返回空数据 @ABA2396
* 修复肉鸽快速编队按钮边缘无法点击导致出错 @Saratoga-Official
* 修复矿石"杀手"识别 @Saratoga-Official
* 肉鸽增加黑屏等待时间 @Saratoga-Official
* 悖论模拟-作业列表额外等待 @status102
* 修复构建警告 @ABA2396
* 收到关卡名识别失败回调时更新卡片 @ABA2396
* Fix phantom puppet not deploying @Constrat
* Fix AT ClickStage t -> T @Constrat
* Add AsstGetImageBgr to wine shim @dantmnf

### 文档 | Docs

* 更新开发指南 @ABA2396
* Update cli usage.md with Windows installation note (#15369) @JasonHuang79

### 其他 | Other

* 快捷置入显示加点判断日志 @ABA2396
* 制造站加入苍苔组，避免苍苔与阿罗玛冲突 (#15343) @drway
* 地块类型标注 TileType (#15373) @status102
* 抽卡任务结束后提示还原为默认 @ABA2396
* 一图流上报 id 单独实现 @ABA2396
* EN tweak to inventory display @Constrat
* AT updates @Constrat
* YostarJP AT updates @Manicsteiner
* YostarKR AT updates @HX3N
* Preload AT + new alter operators regex for EN @Constrat
* YostarJP AT preload and ocr edit @Manicsteiner
* Update OrundumActivities for EN @Constrat

----

## v6.2.1

### 新增 | New

* 公告界面触屏滚动 @ABA2396
* 拆分 debug 和 report 文件夹逻辑 @ABA2396

### 改进 | Improved

* 自动检测地图是否为多阶段地图, 判断是否需要使用 view[0].x 修正镜头 (#15371) @status102
* 优化下拉框逻辑 @ABA2396
* 用妖法实现了自动战斗下拉框的失焦和收起按钮 @ABA2396

### 修复 | Fix

* 游戏更新后结算黑屏时长大幅增加，导致肉鸽结算失败 @ABA2396
* 增加理智战斗后点击到掉落列表的重试次数，避免延长的黑屏时间影响 @ABA2396
* 将借助战打 OF-1 的任务结束后最大等待时间翻倍，避免延长的黑屏时间影响 @ABA2396
* 肉鸽推荐设置文本 (#15370) @ABA2396 @Saratoga-Official
* 自动战斗-悖论模拟 中文路径作业 解析作业失败 @status102
* 肉鸽重试时误点进招募界面 @Saratoga-Official
* 移除 ExceptionStacktrace.hpp @status102
* 收取好友线索延迟 @ABA2396

### 文档 | Docs

* 更新 Visual C++ 可再发行程序包链接至 V14 版本 (#15360) @wryx166

### 其他 | Other

* 调整战斗结束后的点击位置 @ABA2396
* 悖论模拟自动战斗任务翻译 @ABA2396

----

## v6.2.0

### 新增 | New

* 重构仓库识别结构，支持根据关卡掉落情况自动更新 (#15358) @ABA2396
* 成就 DLC 功能 (#15288) @ABA2396
* 增强自动战斗文件选择功能，支持多级路径和相对目录 (#15174) @momomochi987 @ABA2396
* 允许设置是否启用线索交流与赠送线索 (#15278) @ABA2396
* 增加冬时至基建温蒂组并调整温蒂组选人逻辑 (#15294) @drway
* 通知渠道添加 Gotify (#15284) @2436238575
* 给 Bark 通知添加默认的分类组 (#15244) @Anselyuki
* 重构主页日志侧边栏整体布局和样式 (#15211) @MistEO @ABA2396
* 支持日志样式切换 @ABA2396
* 基建设施缩略图 @ABA2396
* 自动战斗日志栏添加日志悬浮窗按钮 @ABA2396
* 关卡提示支持显示库存 @ABA2396
* 更新库存提示移到 ToolTip @ABA2396
* 可以点击 20 次按钮关闭公告弹窗 @ABA2396
* 日志允许图片更新时不添加内容 @ABA2396
* 限制使用 CPU 推理时的线程占用数，优先保证模拟器运行 @ABA2396
* 基建进入设施失败时保留测试截图 @ABA2396
* 将自动重载资源独立出来，在 debug 模式下显示勾选框 @ABA2396
* 尝试增加 B 服开屏活动跳过 @ABA2396
* 检测到自身处于临时路径中时阻止启动 (#14961) @Rbqwow @ABA2396
* 企鹅物流上报 ID 不为空时禁止被上报结果赋值 @ABA2396
* 新增肉鸽主题推荐配置 tip 并适配多语言 (#15324) @hcl55 @HX3N @Manicsteiner @Constrat @momomochi987 @ABA2396
* SideStory 「雅赛努斯复仇记」导航 @SherkeyXD
* Disable CoreML for detection and recognition on macOS @MistEO

### 改进 | Improved

* 重构自动战斗标签页逻辑，拆分悖论模拟任务 (#15327) @ABA2396 @yali-hzy @status102 @HX3N @Constrat
* 刷理智任务指定次数与代理倍率冲突提醒 (#15233) @status102 @HX3N @momomochi987 @Manicsteiner
* 优化线索交流、获取好友线索逻辑 @ABA2396
* 优化界面显示效果与换行 @ABA2396
* 优化缩略图逻辑 @ABA2396
* 优化 PropertyDependsOnHelper 实现 @ABA2396
* 将 PropertyDependsOnViewModel 改为静态工具类 @ABA2396
* 调整缩略图 ToolTip 延迟 @ABA2396
* 在禁用时 TooltipBlock 显示特效 (#15260) @yali-hzy @ABA2396
* 重命名以符合文件结构 @ABA2396
* devcontainer 从 conda 迁移至 mise/uv (#15251) @SherkeyXD
* rename Dps to Ops (#15325) @ABA2396

### 修复 | Fix

* 界园肉鸽因点击模板边缘导致关卡进不去 @Saratoga-Official
* 肉鸽暂停按钮更新 @ABA2396
* 20260109 游戏更新导致自动战斗失效 @status102
* 同时开启「剿灭模式」和「备选关卡」会导致「企鹅物流汇报 ID」被修改 @ABA2396
* 无法通过删除自动战斗输入框内容清除当前作业，无法通过在输入框输入神秘代码直接开始战斗 @ABA2396
* 线索数量识别 @ABA2396
* 线索板上有线索时无法一键放置线索 @ABA2396
* 肉鸽未填写开局干员时借助战强制为 false @ABA2396
* 隐秘战线结局识别 @ABA2396
* 隐秘战线识别到多余字符时无法进入对应事件 @ABA2396
* 给缺失干员检查补上 Unavailable (#15296) @Alan-Charred
* 尝试修复基建效率计算中的 out_of_range 异常 @ABA2396
* 远程控制截图无法获取最新图像 (#15276) @Hakuin123
* Debug 图片保存目录 (#15250) @hguandl
* 修复文档站搜索问题 @SherkeyXD
* 鬼影迷踪 -> 诡影迷踪 @ABA2396
* 撷英调香师 @ABA2396
* 避免其他 locale 下，掉落次数误认数字字符 (#15306) @aflyhorse
* ai review @ABA2396
* Filename too long @Daydreamer114
* update DormMini.png for EN (again) @Constrat
* fix SA1518 warnings @Constrat
* SA1633 warning missing copyright notice @Constrat
* waydroid rawbync screencap 2>/dev/null (#15196) @commondservice

### 文档 | Docs

* 使用脚本一键安装 maa-cli (#15283) @wangl-cc
* 战斗协议 移动镜头 (#15261) @Daydreamer114
* 源码链接同步最新行数 @Rbqwow
* 中文集成文档统一格式 @ABA2396
* update KR documents (#15282) @HX3N
* update vsc ext docs for quick ocr (#15298) @neko-para @HX3N @Constrat

### 其他 | Other

* 移除地图未修复期间的临时糊屎, 移除未更正的注释 @status102
* 添加雪猎基建技能加成 @Saratoga-Official
* 调整文件夹判断逻辑 @ABA2396
* 调整注释 @ABA2396
* 调整 MaxNumberOfLogThumbnails 作用域，调整默认数量 @ABA2396
* 微调公告确认按钮位置 @SherkeyXD
* 移除多余关卡 @SherkeyXD
* 添加挂调试器下使用 GPU 的注释 @ABA2396
* add IsDebugVersion to _forcedReloadResource (#15293) @Constrat
* port changes from api @SherkeyXD
* H16-4, 引航者 #6 TN-1~TN-4 剩余地图 view[1] @status102
* YostarKR tweak AS-OpenOcr @HX3N
* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* YoStarJP SN device ocr (#15310) @cheriu
* EN for `FightTimesMayNotExhausted` @Constrat
