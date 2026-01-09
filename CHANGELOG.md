## v6.2.0

### 新增 | New

* 增加冬时至基建温蒂组并调整温蒂组选人逻辑 (#15294) @drway
* 成就 dlc (#15288) @ABA2396
* 增強自动战斗文件选择功能，支持多级路径和相对目录 (#15174) @momomochi987 @ABA2396
* 通知渠道添加 Gotify (#15284) @2436238575
* 可以点 20 次按钮关闭弹窗 @ABA2396
* 还点？ @ABA2396
* 允许设置是否启用线索交流与赠送线索 (#15278) @ABA2396
* 给 bark 通知添加默认的分类组 (#15244) @Anselyuki
* 重构主页日志侧边栏整体布局和样式 (#15211) @MistEO @ABA2396
* 支持日志样式切换 @ABA2396
* 基建设施缩略图 @ABA2396
* addlog 允许只更新图片不添加内容 @ABA2396
* 自动战斗日志栏添加日志悬浮窗按钮 @ABA2396
* 低于 2 核心电脑使用单线程 OCR @ABA2396
* 更新库存提示移到 ToolTip @ABA2396
* 关卡提示支持显示库存 @ABA2396
* 新增肉鸽主题推荐配置tip并适配多语言 (#15324) @hcl55 @HX3N @Manicsteiner @Constrat @momomochi987 @ABA2396
* Disable CoreML for detection and recognition on macOS @MistEO
* SideStory「雅赛努斯复仇记」导航 @SherkeyXD
* 限制使用 CPU 推理时的线程占用数，优先保证模拟器运行 @ABA2396
* 基建进入设施失败时保留测试截图 @ABA2396
* 将自动重载资源独立出来，在 debug 模式下显示勾选框 @ABA2396
* 尝试增加 b服 开屏活动跳过 @ABA2396
* 检测到自身处于临时路径中时阻止启动 (#14961) @Rbqwow @ABA2396
* 企鹅物流上报 ID 不为空时禁止被上报结果赋值 @ABA2396

### 改进 | Improved

* 优化界面显示效果 @ABA2396
* 优化换行 @ABA2396
* 在禁用时 TooltipBlock 显示特效 (#15260) @yali-hzy @ABA2396
* devcontainer 从 conda 迁移至 mise/uv (#15251) @SherkeyXD
* 优化缩略图逻辑 @ABA2396
* 重命名以符合文件结构 @ABA2396
* 优化 PropertyDependsOnHelper 实现 @ABA2396
* 将 PropertyDependsOnViewModel 改为静态工具类 @ABA2396
* 调整缩略图 ToolTip 延迟 @ABA2396
* 刷理智任务指定次数与代理倍率冲突提醒 (#15233) @status102 @HX3N @momomochi987 @Manicsteiner
* 重构自动战斗标签页逻辑, 拆分悖论模拟任务 (#15327) @ABA2396 @yali-hzy @status102 @HX3N @Constrat
* rename Dps to Ops (#15325) @ABA2396
* 优化线索交流、获取好友线索逻辑 @ABA2396

### 修复 | Fix

* 给缺失干员检查补上 Unavailable (#15296) @Alan-Charred
* 尝试修复基建效率计算中的 out_of_range 异常 @ABA2396
* update DormMini.png for EN (again) @Constrat
* SA1633 warning missing copyright notice @Constrat
* 修复文档站搜索问题 @SherkeyXD
* 远程控制截图无法获取最新图像 (#15276) @Hakuin123
* Debug图片保存目录 (#15250) @hguandl
* 鬼影迷踪 -> 诡影迷踪 @ABA2396
* ai review @ABA2396
* fix SA1518 warnings @Constrat
* 肉鸽未填写开局干员时借助战强制为 false @ABA2396
* 隐秘战线结局识别 @ABA2396
* 隐秘战线识别到多余字符时无法进入对应事件 @ABA2396
* waydroid rawbync screencap 2>/dev/null (#15196) @commondservice
* 避免其他locale下，掉落次数误认数字字符 (#15306) @aflyhorse
* 无法通过删除自动战斗输入框内容清除当前作业，无法通过在输入框输入神秘代码直接开始战斗 @ABA2396
* 撷英调香师 @ABA2396
* 20260109 游戏更新导致自动战斗失效 @status102
* 线索数量识别 @ABA2396
* 线索板上有线索时无法一键放置线索 @ABA2396
* 同时开启 `剿灭模式` 和 `备选关卡` 会导致 `企鹅物流汇报 ID` 被修改 @ABA2396
* Filename too long @Daydreamer114

### 文档 | Docs

* Auto Update Changelogs of v6.2.0-beta.1 (#15301) @github-actions[bot] @github-actions[bot] @ABA2396
* 使用脚本一键安装 maa-cli (#15283) @wangl-cc
* update KR documents (#15282) @HX3N
* 源码链接同步最新行数 @Rbqwow
* 中文集成文档统一格式 @ABA2396
* 战斗协议 移动镜头 (#15261) @Daydreamer114
* Auto Update Changelogs of v6.2.0-beta.2 (#15316) @github-actions[bot] @github-actions[bot] @ABA2396
* update vsc ext docs for quick ocr (#15298) @neko-para @HX3N @Constrat
* Auto Update Changelogs of v6.2.0-beta.3 (#15338) @github-actions[bot] @github-actions[bot] @ABA2396

### 其他 | Other

* 调整文件夹判断逻辑 @ABA2396
* Release v6.2.0-beta.3 (#15337) @ABA2396
* Release v6.2.0-beta.2 (#15315) @ABA2396
* Release v6.2.0-beta.1 (#15299) @ABA2396
* add IsDebugVersion to _forcedReloadResource (#15293) @Constrat
* YostarKR tweak AS-OpenOcr @HX3N
* 调整注释 @ABA2396
* 微调公告确认按钮位置 @SherkeyXD
* YostarJP roguelike JieGarden ocr edit @Manicsteiner
* EN for `FightTimesMayNotExhausted` @Constrat
* 调整 MaxNumberOfLogThumbnails 作用域，调整默认数量 @ABA2396
* YoStarJP SN device ocr (#15310) @cheriu
* H16-4, 引航者#6 TN-1~TN-4 剩余地图 view[1] @status102
* 移除多余关卡 @SherkeyXD
* port changes from api @SherkeyXD
* 添加挂调试器下使用 GPU 的注释 @ABA2396
