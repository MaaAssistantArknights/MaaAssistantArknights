## v6.2.0

### Highlights

#### 界面与日志体验全面升级

对主页日志侧边栏进行了整体重构，统一并优化了布局与视觉样式，新增日志样式切换与日志悬浮窗按钮，并支持日志缩略图显示，使运行过程中的关键信息更加直观、易读。同时梳理并优化了自动战斗标签页的逻辑与文件选择流程，支持多级路径选择，减少不必要的操作步骤，整体使用体验更加顺畅自然。

#### 每日关卡提示优化

每日关卡提示中的掉落信息现已支持同步显示对应的库存数量，便于在刷图规划时快速判断材料缺口与优先级。库存数据可通过「小工具 → 仓库识别」进行更新，无需额外操作即可保持提示信息的准确性。

#### 基建与线索管理优化

新增线索交流与赠送功能的独立开关设置，提升基建管理的灵活性，并对快捷置入与一键赠送的逻辑进行了进一步优化，减少误操作发生的可能性。同时，在基建进入设施失败时将自动保留测试截图，便于后续问题的定位与排查。

#### 成就系统 DLC

成就内容围绕用户的使用行为、功能探索与长期使用习惯进行设计，通过多样化的触发条件，引导用户逐步体验并深入了解软件的各项功能。

----

#### Comprehensive UI and Log Experience Upgrade

The layout and visual style of the homepage log sidebar have been fully reworked, introducing log style switching and a floating log window, along with support for log thumbnails. This makes key runtime information clearer and easier to follow. In addition, the logic and file selection workflow of the Auto Battle tab have been refined, with support for multi-level path selection, reducing unnecessary steps and providing a smoother overall experience.

#### Daily Stage Hint Improvements

Drop information in daily stage hints now supports displaying the corresponding inventory counts, making it easier to assess material needs and priorities when planning runs. Inventory data can be updated via **Tools → Inventory Recognition**, ensuring the displayed information remains accurate with minimal effort.

#### Base and Clue Management Enhancements

Independent toggles have been added for clue exchange and clue gifting, improving flexibility in base management. The logic behind quick placement and one-click gifting has also been further optimized to reduce the likelihood of misoperations. In addition, when entering a base facility fails, a test screenshot will now be automatically preserved to assist with issue diagnosis and troubleshooting.

#### Achievement System DLC

The achievement content is designed around user behavior, feature exploration, and long-term usage patterns. With diverse trigger conditions, it encourages users to gradually explore and gain deeper familiarity with the software’s full feature set.


----

以下是详细内容：

## v6.2.0

### 新增 | New

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
