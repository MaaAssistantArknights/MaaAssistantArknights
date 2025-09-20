## v5.25.0

### 依\~赖\~大\~更\~新\~ | Highlights

在这个版本，我们大幅更新了项目的依赖，**并为官网和 API 节点更换了新的 cdn 提供商，大幅优化了访问速度**。

#### 自动战斗方面

我们对【自动战斗】功能进行了一些改动，主要包括：

* 牛牛现在会自动识别作业集内各个作业的干员编组情况，如果上一份作业的干员编组与当前作业的一致，牛牛就不再重复进行干员编组了，从而节省时间
* 战斗列表部分，对各个作业的设置按键的鼠标按键逻辑进行了调整：点击左键将可以查看该作业的说明和干员配置需求，点击右键将直接提取该作业并关闭战斗列表功能（即只打该作业）

#### 基建换班方面

我们优化了基建换班中与基建技能有关的功能：

* 我们为所有没有设置效率数据的基建技能都设置了效率数据，这样牛牛在【基建换班】任务中就能更好地为干员不全的玩家选择合适的基建技能了（但还是把 U 酱拿掉了，~~U 酱那么可爱为什么要工作~~）
* 如果【基建换班】任务处于「自定义基建配置」模式，且牛牛正在工作中，那么班次将不再随时间切换，避免牛牛工作到一半班次被切了而产生的各种问题
* 如果【基建换班】任务处于「队列轮换」模式，那么牛牛将不再进入未勾选的设施
* 我们调整了会客室技能的排序逻辑，现在会优先选择更容易获得尚未拥有的线索的技能

#### 其他方面

我们增加了一些小的改动，值得注意的有：

* 我们在【自动肉鸽】任务中优化了丰川祥子相关招募策略 ~~saki酱、saki酱、saki酱~~
* 牛牛在【生息演算】任务中将会自动识别当前的存档状态与任务的设置是否一致，如果不一致将会提示你检查存档状态
* 当你设置 Mirror 酱的 cdk 后，MAA 将会显示 cdk 的剩余有效时间，方便你掌握有效期

----

In this version, we have significantly updated the project dependencies and **switched to a new CDN provider for the official website and API nodes, greatly optimizing access speed**.

#### *Copilot* Improvements

We have made some adjustments to the *Copilot* feature, which mainly include:

* MAA will now automatically recognize the operator formation in each job within the job set. If the operator formation of the previous job matches that of the current job, MAA will no longer repeat the operator squading process, thereby saving time.
* In the Battle list section, the mouse button logic for each job's settings button has been adjusted: left-clicking will allow you to view the job's description and operator formation requirements, while right-clicking will directly extract that job and disable the combat list function (i.e., only run that specific operation).

#### *Base* Improvements

We have optimized the base skill-related functions in *Base* task:

* We have set efficiency data for all base skills that previously lacked that data, allowing MAA to better select suitable base skills for doctors with incomplete operator rosters during the *Base* task (though we did remove U-Offcial, ~~U-Offcial is so cute, why should she work~~)
* If the *Base* task is in Custom Base Mod and the MAA is currently running, Base Plan will no longer switch over time to avoid various issues caused by interrupting the MAA's work by switching the Base Plan when MAA is running.
* If the *Base* task is in One-click Rotation Mode, the MAA will no longer enter unchecked facilities.
* We adjusted the sorting logic for reception room skills, now prioritizing skills that are more likely to obtain clues not yet owned.

#### Other Aspects

We've made some minor changes worth noting:

* [CN ONLY] In the *Auto I.S.* task, we've optimized the recruitment strategy related to Togawa Sakiko ~~Saki-chan, Saki-chan, Saki-chan~~
* In the *Reclamation Algorithm* task, MAA will automatically detect whether the current save status matches the mission settings. If they don't match, it will prompt you to check your save status.
* After setting MirrorChyan's CDK, MAA will display the remaining validity period of the CDK, making it easier for you to track its expiration

----

以下是详细内容：
                
### 新增 | New

* CustomTask增加任务存在检测 @status102
* custom clang (#14102) @neko-para
* core 崩溃后下次启动时 ui 输出提示 (#14022) @ABA2396 @momomochi987
* allow single copilot task execution @status102
* 给自定义任务添加 ScreenshotTaskPlugin 插件 @ABA2396
* 战斗列表点击作业时自动关闭列表, 原逻辑迁移至鼠标右键 @status102
* convert ResourceUpdater to ubuntu-latest (#14076) @Constrat
* 添加基建计划 ToolTip 提示 @ABA2396
* support build resource updater @neko-para
* 添加信息获取失败时的提示 @ABA2396
* CDK 倒计时显示 @ABA2396
* 成就设置中的图标颜色可以随着主题色动态更新 @ABA2396
* 添加 CDK 到期时间显示 @ABA2396
* 添加 ResourceReferenceHelper，允许绑定 前后景色 key @ABA2396
* 自动战斗多作业支持保存干员组内编入的干员 (#14095) @status102
* cli 支持新版战斗列表以及悖论模拟 (#14154) @wangl-cc
* 为官网首页添加多语言支持 (#13943) @lucienshawls @momomochi987 @Manicsteiner @pre-commit-ci[bot] @Constrat
* RunningState 统一状态变更事件 (#14141) @ABA2396
* 生息盐酸任务添加对错误模式 (有/无存档) 的提示 (#14131) @Alan-Charred
* use coreml OCR for apple (#14108) @MistEO

### 改进 | Improved

* 更新基建技能效率 @ABA2396
* wpf自动战斗列表中作业设置按钮左键单击还原为解析指定作业，新增行为修改至右键，解析作业并关闭列表 @status102
* 优化更新设置中的徽标显示和逻辑 @ABA2396
* remove resource from smoke testing use the default location, do not duplicate @Constrat
* add --parallel to cmake workflows @Constrat
* 更新 markdownlint 规则 @SherkeyXD
* 依赖大更新 (#13908) @MistEO @status102 @pre-commit-ci[bot]
* 优化干员识别动画显示效果 @ABA2396
* 加个try先 @status102

### 修复 | Fix

* typo @Constrat
* type number for nightly ota @Constrat
* 修复 cmake copy_and_add_rpath_library 的失败逻辑 (#14097) @litwak913
* 刷开局奖励只选票券时行为异常 @ABA2396
* smoke testing upload logs @Constrat
* move smoke testing working dir @Constrat
* 自动编队缺少干员输出的额外换行 @status102
* Copilot多任务编队时编队设置残留 @status102
* 自定义基建换班的班次不会随时间切换 @ABA2396
* "统一 WpfGui 工作目录"后，配置为外服无法运行 MAA @ABA2396
* 移除 debug_demo (原Sample) 的对应 resource 额外查找逻辑 (#14081) @status102 @MistEO
* 统一 WpfGui 工作目录 (#14072) @MistEO @pre-commit-ci[bot] @ABA2396 @status102
* add component for install libcxx only @neko-para
* 重构依赖后 MaaWpfGui 的 resource 路径丢失 (#14077) @status102
* 停止中或者空闲状态不允许点击停止按钮 @ABA2396
* 鲍勃杂货店拥有四叶草化石且无抗干扰值时卡住 @ABA2396
* prevent caching in non-dev branches @Constrat
* stuck on max trust EN @Constrat
* pthread linking problem @neko-para
* 无法收取太鼓达人联动主题会客室信息板信用 @ABA2396
* cdk 过期时间超过 2038 年会爆炸（但 10000 年还是会爆炸 @ABA2396
* nightly package zip @MistEO
* global resource path @MistEO
* linux cross compiling (#14048) @neko-para @pre-commit-ci[bot]
* maa 变成前文明产物了 @ABA2396
* 糊一下 debug_demo 资源路径读取 @MistEO
* 修复 wpf gui 一些 resource path 读取 @MistEO
* remove opencv highgui again @MistEO
* remove opencv highgui @MistEO
* build error of ASST_DEBUG @MistEO
* 入暂亭特殊选项 @Saratoga-Official
* Sami floor detection 3 Thanks again @lin4289 on discord @Constrat
* 特里蒙旅行社特派团多选项 @Saratoga-Official
* IS5 同心 @Daydreamer114
* EN Roguelike@ChooseOperConfirm (#14033) @Daydreamer114
* 选择技能时可能会点出描述文字 @ABA2396
* Sami floor detection 2 Thanks again @lin4289 on discord @Constrat
* 基建使用队列轮换时如果未勾选对应设施则不进入 @ABA2396
* 重构后无法使用刷开局刷仅精二 @ABA2396
* 部分背景下 mujica 主题无法进入对应功能 @ABA2396
* 萨米肉鸽刷源石锭可能卡在预见的密文板 @ABA2396
* Sami floor detection Thanks @lin4289 on discord @Constrat
* 允许生息演算任务反复点击存档位置直到画面改变 (#14005) @Alan-Charred
* 肉鸽助战无法刷新 @Saratoga-Official
* smoke-testing script @Constrat
* minitouch 触控 wait ms @ABA2396
* 复核自定义干员时等待游戏动画 @ABA2396
* wrong default on type number @Constrat
* 自动战斗无法读取下拉列表中的作业 @ABA2396
* 成就列表解锁时间显示颜色错误 @ABA2396
* GO 导航 @ABA2396
* 勾选启动后直接运行时无法自动切换基建排班表 @ABA2396
* 网络连接错误的情况下读取了本地缓存也提示了获取热更成功 @ABA2396
* 萨米肉鸽未通关结局时探寻前路卡住 @Saratoga-Official
* 文档站首页在窄屏下的响应式设计 (#14166) @lucienshawls
* 暂时修复无法读取过去的评论的问题 (#14163) @lucienshawls
* Eye For an Eye encounter IS2 EN @Constrat
* 为什么 mujikca 的图黑的不够纯粹 @ABA2396
* 肉鸽在结算界面卡死 (#14145) @Saratoga-Official @pre-commit-ci[bot]
* CopilotTask参数可选项存在性检查 @status102
* SettingsViewModel Idle @ABA2396
* 异格推王的基建加成错误 @Saratoga-Official
* 调低 MacOS PlayCover 下对 InfrastControl 识别的阈值 (#14139) @Alan-Charred
* CurrentConfig未刷新 @status102
* CurrentConfig悬空 @status102
* 拥有全干员的情况下重启后进入干员识别界面不会自动选中选项卡 @ABA2396
* 修复 DataGrid 虚拟化下 Tab 导航异常 @ABA2396
* download to v5 with necessary fix (#14122) @Constrat
* 萨卡兹肉鸽未通关结局时原初异途卡住 @Saratoga-Official
* 肉鸽深入探索无法退出结算界面 (#14123) @Saratoga-Official

### 文档 | Docs

* Auto Update Changelogs of v5.25.0-beta.1 (#14101) @github-actions[bot] @ABA2396 @status102
* 维护信用作战相关功能及其文档 (#14013) @Alan-Charred
* 修复序号问题 @SherkeyXD
* typo @MistEO
* 更新文档cmake配置 @MistEO
* 微调文档 @MistEO
* reapply eebe0a9 (#14036) @wangl-cc
* remove weblate @MistEO
* Auto Update Changelogs of v5.25.0-beta.2 (#14112) @github-actions[bot] @MistEO
* changelog version @MistEO
* changelog @MistEO
* Update CHANGELOG for v5.25.0-beta.4 @ABA2396
* Auto Update Changelogs of v5.25.0-beta.5 (#14181) @github-actions[bot] @ABA2396
* 完善文档站的代码检查和涉及的文本替换 (#14156) @lucienshawls @pre-commit-ci[bot]
* 完善自动肉鸽和其它文档中的部分内容 (#13924) @lucienshawls @Constrat @Manicsteiner
* 切换文档主题至 vuepress-theme-plume (#13821) @SherkeyXD @lucienshawls @MistEO

### 其他 | Other

* 调整 锏 基建技能效率 @ABA2396
* 自定义基建配置时间仍在有效期内时不检查其他时间段 @ABA2396
* 界园萨卡兹肉鸽招募休谟斯优先级 @Saratoga-Official
* remove -DINSTALL_RESOURCES @Constrat
* 调整 y 存图时间间隔 @ABA2396
* 调整定时器触发逻辑 @ABA2396
* 调整未检测到模拟器的输出 @ABA2396
* gitignore @status102
* where and how did this come from? @Constrat
* tweak workflow + remove .sln (#14074) @Constrat @neko-para
* 按时间切换基建配置不在任务运行中或任务开始前切换 @ABA2396
* 调整刷新助战日志 @ABA2396
* 调整界面颜色 @ABA2396
* 肉鸽丰川祥子招募策略 (#14035) @Saratoga-Official
* 繁中服「大荒」介面主題 (#14064) @momomochi987 @pre-commit-ci[bot]
* remove ninja from ci.yml @Constrat
* restore older cache utilization for ci.yml (#14068) @Constrat
* update interface.json @neko-para
* update linux tutorial (en) @neko-para
* EN @Constrat
* 删除一些遗留文件 @MistEO
* 再糊点注释 @MistEO
* 糊点注释 @MistEO
* remove MaaDeps @MistEO
* bump maa-cli to 0.5.8 (#14032) @wangl-cc
* BrightPointAnalyzer -> PixelAnalyzer (#13915) @Alan-Charred
* 管理员权限启动额外判断是否开启 UAC @ABA2396
* mujika -> mujica (#14000) @weinibuliu
* remove SyncRes @MistEO
* use std::format instead of sprintf (#14107) @MistEO @pre-commit-ci[bot]
* fix build warning (#14176) @soundofautumn
* rename RecognizerViewModel -> ToolboxViewModel (#14177) @soundofautumn
* YostarJP ocr fix (#14134) @Manicsteiner
* 调整会客室中更容易获得线索板上尚未拥有的线索的技能优先级 @ABA2396
* 基建把 u酱 ban 了 @ABA2396
* file header @status102
* 特征匹配过程函数拆入analyze() @status102
* 繁中服「相見歡」活動導航 (#14137) @momomochi987
* fix build warning (#14120) @soundofautumn
