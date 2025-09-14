## v5.25.0-beta.1

### 新增 | New

* core 崩溃后下次启动时 ui 输出提示 (#14022) @ABA2396 @momomochi987
* 给自定义任务添加 ScreenshotTaskPlugin 插件 @ABA2396
* 战斗列表点击作业时自动关闭列表, 原逻辑迁移至鼠标右键 @status102
* 添加基建计划 ToolTip 提示 @ABA2396
* 添加信息获取失败时的提示 @ABA2396
* CDK 倒计时显示 @ABA2396
* 添加 CDK 到期时间显示 @ABA2396
* 成就设置中的图标颜色可以随着主题色动态更新 @ABA2396
* 添加 ResourceReferenceHelper，允许绑定前后景色 key @ABA2396
* allow single copilot task execution @status102
* convert ResourceUpdater to ubuntu-latest (#14076) @Constrat
* support build resource updater @neko-para

### 改进 | Improved

* 更新基建技能效率 @ABA2396
* wpf 自动战斗列表中作业设置按钮左键单击还原为解析指定作业，新增行为修改至右键，解析作业并关闭列表 @status102
* 优化更新设置中的徽标显示和逻辑 @ABA2396
* 优化干员识别动画显示效果 @ABA2396
* 更新 markdownlint 规则 @SherkeyXD
* 依赖大更新 (#13908) @MistEO @status102
* remove resource from smoke testing use the default location, do not duplicate @Constrat
* add --parallel to cmake workflows @Constrat

### 修复 | Fix

* 修复 cmake copy_and_add_rpath_library 的失败逻辑 (#14097) @litwak913
* 刷开局奖励只选票券时行为异常 @ABA2396
* 自动编队缺少干员输出的额外换行 @status102
* Copilot多任务编队时编队设置残留 @status102
* 自定义基建换班的班次不会随时间切换 @ABA2396
* "统一 WpfGui 工作目录"后，配置为外服无法运行 MAA @ABA2396
* 停止中或者空闲状态不允许点击停止按钮 @ABA2396
* 鲍勃杂货店拥有四叶草化石且无抗干扰值时卡住 @ABA2396
* 无法收取太鼓达人联动主题会客室信息板信用 @ABA2396
* cdk 过期时间超过 2038 年会爆炸（但 10000 年还是会爆炸 @ABA2396
* 入暂亭特殊选项 @Saratoga-Official
* 特里蒙旅行社特派团多选项 @Saratoga-Official
* IS5 同心 @Daydreamer114
* 选择技能时可能会点出描述文字 @ABA2396
* 基建使用队列轮换时如果未勾选对应设施则不进入 @ABA2396
* 重构后无法使用刷开局刷仅精二 @ABA2396
* 部分背景下 mujica 主题无法进入对应功能 @ABA2396
* 萨米肉鸽刷源石锭可能卡在预见的密文板 @ABA2396
* 允许生息演算任务反复点击存档位置直到画面改变 (#14005) @Alan-Charred
* 肉鸽助战无法刷新 @Saratoga-Official
* 移除 debug_demo (原Sample) 的对应 resource 额外查找逻辑 (#14081) @status102 @MistEO
* 统一 WpfGui 工作目录 (#14072) @MistEO @ABA2396 @status102
* 重构依赖后 MaaWpfGui 的 resource 路径丢失 (#14077) @status102
* maa 变成前文明产物了 @ABA2396
* 糊一下 debug_demo 资源路径读取 @MistEO
* 修复 wpf gui 一些 resource path 读取 @MistEO
* smoke testing upload logs @Constrat
* move smoke testing working dir @Constrat
* prevent caching in non-dev branches @Constrat
* stuck on max trust EN @Constrat
* pthread linking problem @neko-para
* nightly package zip @MistEO
* global resource path @MistEO
* linux cross compiling (#14048) @neko-para
* remove opencv highgui @MistEO
* build error of ASST_DEBUG @MistEO
* EN Roguelike@ChooseOperConfirm (#14033) @Daydreamer114
* Sami floor detection @lin4289 @Constrat

### 文档 | Docs

* 维护信用作战相关功能及其文档 (#14013) @Alan-Charred
* 修复序号问题 @SherkeyXD
* 更新文档 cmake 配置 @MistEO
* 微调文档 @MistEO
* reapply eebe0a9 (#14036) @wangl-cc
* remove weblate @MistEO
* typo @MistEO

### 其他 | Other

* 调整 y 存图时间间隔 @ABA2396
* 调整定时器触发逻辑 @ABA2396
* 调整未检测到模拟器的输出 @ABA2396
* 按时间切换基建配置不在任务运行中或任务开始前切换 @ABA2396
* 调整刷新助战日志 @ABA2396
* 调整界面颜色 @ABA2396
* 肉鸽丰川祥子招募策略 (#14035) @Saratoga-Official
* 繁中服「大荒」介面主題 (#14064) @momomochi987
* 删除一些遗留文件 @MistEO
* 再糊点注释 @MistEO
* 糊点注释 @MistEO
* 管理员权限启动额外判断是否开启 UAC @ABA2396
* mujika -> mujica (#14000) @weinibuliu
* remove -DINSTALL_RESOURCES @Constrat
* gitignore @status102
* where and how did this come from? @Constrat
* tweak workflow + remove .sln (#14074) @Constrat @neko-para
* remove ninja from ci.yml @Constrat
* restore older cache utilization for ci.yml (#14068) @Constrat
* update interface.json @neko-para
* update linux tutorial (en) @neko-para
* remove MaaDeps @MistEO
* bump maa-cli to 0.5.8 (#14032) @wangl-cc
* BrightPointAnalyzer -> PixelAnalyzer (#13915) @Alan-Charred
* EN @Constrat
