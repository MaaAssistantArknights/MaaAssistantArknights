## v6.3.1

### 拉电线不知天地为何物 | Highlight

本次版本更新真的是千呼万唤始出来，也恰逢新年版本和 PC 端的发布，我们在这个版本提供了对 PC 端的初步支持，也合并了 SideStory ｢辞岁行｣ 的数据。

#### Windows 端一键长草任务配置重构

本次更新我们对 Windows 端 ｢一键长草｣ 任务配置进行了重构，解决了一些痛点。

现在牛牛支持添加多个重复类型任务，并支持重命名。现在你可以通过 ｢一键长草｣ 任务左下角的加号按钮添加多个同类型任务，并可以通过拖拽任务来调整任务执行顺序，以及右键点击任务右侧的齿轮图标重命名和删除任务，这样你就可以更灵活地管理任务。

牛牛也新增了理智作战任务的周计划设置，可按星期数（如星期三、星期二、星期五等）设置任务是否执行。这样你就可以结合上面提到的添加任务，<u>在理智作战任务前添加另一个理智作战任务，并将其设置为仅打当期剿灭、仅在星期一运行</u>，更多用法等你探索~

另外，在牛牛执行一键长草流程时，各个任务的勾选框会根据其运行状态以不同颜色进行区分，直观反馈任务被跳过、已完成、运行中或执行失败等状态。

注：由于牛牛现已支持 ｢理智作战｣ 周计划并可添加多个同类型任务，本次更新**移除了原有 ｢剿灭作战｣ 任务失败后自动尝试下一个已开放的备选逻辑**，如有需要请参照上方的示例添加对应任务。

#### PC 端初步支持

本次更新，牛牛已支持控制 PC 端明日方舟了~不过有以下限制：

* 由于 Windows 的限制，PC 端在被控制时**不可以处于最小化窗口状态**，否则游戏画面不会被渲染，也就无法被牛牛识别；
* 由于 Win32 API 和某反作弊软件的限制，牛牛在控制 PC 端时**必须要直接控制鼠标（不能模拟点击）**，期间不建议你使用鼠标。

另外，由于维护人手有限，PC 端的适配可能长期处于不稳定状态，部分功能可能出现异常或暂时无法使用。

我们非常欢迎社区开发者协助适配并提交改进，共同完善对 PC 端的支持。

#### 其他方面

* 我们继续对界园肉鸽进行适配，包括 DLC 2 的更新支持；
* 我们优化了自动编队的识别，现在支持对作业要求的精英化、等级和模组的识别，如果发现你的干员不满足要求将会有对应提示；
* 相应的，干员识别功能也支持显示精英化和等级了。

----

#### Restructuring of *Farming* Configuration for Windows

In this update, we have restructured the *Farming* configuration for Windows, addressing several pain points.

MAA now supports adding multiple tasks of the same type and renaming them. You can add multiple tasks of the same type using the plus button at the bottom left of the *Farming* panel, adjust the execution order by dragging tasks, and rename or delete tasks by right-clicking the gear icon on the right side of each task. This allows for more flexible task management.

MAA also supports weekly schedule settings for *Combat* tasks, allowing you to specify on which days of the week (e.g., Monday, Thursday) a task should run. For example, you can add another *Combat* task before your existing one and set it to only run Current Annihilation on Mondays. More usage scenarios await your exploration!

Additionally, when MAA executes the *Farming* process, the checkboxes for each task will be color-coded to indicate their status: skipped, completed, running, or failed, providing intuitive feedback.

Note: Since MAA now supports weekly schedules for *Combat* tasks and allows adding multiple tasks of the same type, this update **removes the previous logic that automatically attempted the next available alternative stage after a "Current Annihilation" task failure**. If needed, please refer to the example above to add corresponding tasks.

#### Preliminary PC Client Support (**[CN ONLY]**)

In this update, MAA now supports controlling the PC client of Arknights.

However, due to limited maintenance resources, PC client adaptation may remain unstable for a long period, and some features may experience issues or be temporarily unavailable.

We warmly welcome community developers to assist with adaptation and submit improvements to collectively enhance PC client support.

#### Other Updates

* Continued adaptation for *Sui's Garden of Grotesqueries*, including support for DLC 2 updates (**[CN ONLY]**)
* Optimized recognition in *Auto Squad*, which now supports identifying elite, level, and module requirements. If your operators do not meet the requirements, you will receive corresponding prompts.
* The operator recognition feature now also supports displaying elite and level information.

----

以下是详细内容：

### 新增 | New

* OperNameAnalyzer 支持左对齐检测 (#15682) @ABA2396
* 支持 PC 端明日方舟 (#15407) @MistEO @ABA2396
* SideStory「辞岁行」导航及地图数据更新 @SherkeyXD @status102
* 界园肉鸽 DLC 2 分队更新、通宝数据更新、可选难度提高至 18、支持指定种子开局 (#15588) @SherkeyXD @status102 @HX3N @Manicsteiner
* 繁中服界園肉鴿初步適配 (#15605) @momomochi987
* 干员识别支持显示精英化、等级与潜能 @ABA2396
* 自动编队识别精英化、等级及模组要求 (#15161) @status102 @Manicsteiner @Constrat @HX3N
* 芯片本支持显示库存数量 @ABA2396
* 新增注入弹窗不再提醒的勾选框，勾选后使用软件渲染 @ABA2396
* WpfGui 清空缓存按钮 (#15582) @soundofautumn @Constrat @HX3N @momomochi987
* 自定干员名称无效时的错误处理及本地化支持 (#15556) @yali-hzy @HX3N
* 设置指引增加右键重命名/删除提示 @ABA2396
* 新 Config 加载时移除旧 Config 中不存在的配置 @status102
* 日志中额外记录 TaskChain 与 taskId ~~免得有人把 Fight 改成开始唤醒~~ @ABA2396
* YostarEN/JP/KR Dreamland、JieGarden 和 AveMujica 主题支持 @Constrat @Manicsteiner @HX3N

### 改进 | Improved

* Wpf 一键长草任务配置重构，支持添加多个同类型任务、重命名和周计划设置 (#15385) @status102 @ABA2396 @momomochi987 @HX3N
* 刷理智过期关卡逻辑/样式优化 @status102 @ABA2396
* 关卡候选列表刷新及关卡选择下拉列表刷新 (#15562) @status102 @HX3N @Constrat @Manicsteiner @momomochi987
* 优化自动战斗界面布局 (#15512) @yali-hzy
* 优化任务设置按钮悬浮提示 @ABA2396
* 优化设置右键菜单布局 @ABA2396
* 优化干员识别、仓库识别显示 @ABA2396
* 配置迁移检查优化与简化 @status102 @ABA2396
* 开始唤醒任务未设置账号切换时，禁用手动切换按钮 @status102
* 自动战斗掉线重连、自动肉鸽在战斗结束前延迟｢停止｣动作添加多任务共用提示 @ABA2396 @status102
* 剿灭卡使用到上限时不报错停止 @ABA2396
* 剿灭关卡通过 ends_with 判断 @ABA2396
* 刷理智任务高级设置 UI 调整选项顺序和显示优化 @status102
* 自动战斗自动编队检查`干员等级&精英化`及`技能等级`拆分 @status102
* 自动战斗不支持技能重置说明中，干员名遵循干员名语言设置 @status102
* 自动战斗作业列表使用相对路径代替绝对路径 @status102
* TaskQueue 重命名与移除时显示任务序号 @status102
* TaskQueue 任务开始&完成显示修改后的任务名 @status102
* 追加自定干员允许不切换技能 @status102
* 存在 crash.log 时，Wpf 尝试获取 dumps 文件 (#15432) @status102
* 移除过时的参数兼容 @status102
* 消除部分编译警告 (#15578) @yali-hzy
* NumberOcrReplace 新增规则 (#14186) @Manicsteiner

### 修复 | Fix

* EN IS StageRefresh @Constrat
* i'm kinda stupid @Constrat
* update refresh node EN IS5 @Constrat
* EN IS fix trader store templates AGAIN @Constrat
* EN Yu OCR for Yutenji @Constrat
* 自动编队选择技能时点击到技能描述教程了 @status102
* 自动编队选择技能时点击到技能范围 @status102
* 配置迁移相关问题修复：迁移后切换回原配置、删除配置时未删除 .new、多配置用户删除 Default 配置时迁移异常、剩余理智启用状态和关卡选择迁移问题 @status102 @ABA2396
* 刷理智任务相关修复：切换刷理智任务时读取错误关卡列表、运行时不允许添加关卡 @status102
* 自动编队相关修复：识别技能等级匹配失败、干员技能描述过长时点击位置错误、禁用快速选中以修复外服干员技能描述过长的错误选中、干员等级跨精英化时判断出错、干员等级不足 i18n 未启用、干员组干员未解析精英化及等级属性、先兼容旧作业中不合理的技能选择 @status102
* 公招加速券识别问题修复 @status102
* LoadApiCache 路径拼接错误 @ABA2396
* 主线导航及外服主线导航问题修复 @SherkeyXD @ABA2396
* 在赠送线索时弹出上次线索交流结束的提示时无法返回 @ABA2396
* 粘贴作业集代码后下方的链接未重置为作业站链接 @ABA2396
* 修复移动已打开设置的任务后，当前的设置面板无法继续修改的问题 @ABA2396
* 萨米肉鸽刷开局功能异常 @ABA2396
* 肉鸽烧水分队切换界面后错误重置 @status102
* 保证通宝优先级未定义时不会加载崩溃 fallback 到默认值 @SherkeyXD
* SEH 错误终止运行 @status102
* 启动 MAA 时若没有任何任务，则追加一套默认任务 @status102
* 启动客户端绑定失效问题修复 @status102
* 关卡列表显示不刷新 @status102
* 选中完成后动作时添加新任务未能隐藏完成后动作设置 UI @status102
* 刷理智使用源石 CheckBox 勾选后不生效 @status102
* 退出 MAA 时重置变量不再刷新 UI @status102
* 初始化 StartEnabled 属性为 true (#15596) @yali-hzy
* 自动战斗切换活动类型未清空解析缓存 @status102
* 修复任务出错日志可能晚于任务完成日志显示的问题 @ABA2396
* 开始干员识别前重置潜能状态 @ABA2396
* 修复生息演算商店无法正常购买皮肤的问题 (#15585) @drway
* 手动输入关卡名时，不移除过期关卡 @status102
* 过期关卡重置模式补充自动迁移 @status102
* OR 关卡掉落界面关卡名识别问题 @ABA2396
* EX 关符合时 1 被识别为 | @ABA2396
* NumberOcrReplace 移除`|`和`/` (#15625) @status102
* 移除单字干员 ocr 替换中的 +*？避免误判 @Saratoga-Official
* 涤火杰西卡识别 @ABA2396
* 日服酒神、見字祠识别 @Saratoga-Official
* YostarKR Roguelike@ChooseOperConfirm @HX3N
* YostarKR use ' ' in ocrReplace to preserve '\n' for InfrastTrainingTask @HX3N
* EN Greyy Alter regex @Constrat
* EN IS6 encounter @Constrat
* EN IS TradeInvest templates text font change @Constrat
* EN IS ShoppingConfirm text font change @Constrat
* EN refresh node template @Constrat
* JP AT minigame confirm (#15427) @Manicsteiner
* JP JieGardenStrategyChange @Saratoga-Official
* add MaaWin32ControlUnit to nightly build (#15447) @Manicsteiner

### 文档 | Docs

* 集成文档统一格式，同时显示 field-group 和示例代码 (#15409) @ABA2396 @Manicsteiner @Constrat @momomochi987
* 繁中文件大更新 (#15480) @momomochi987
* 修正开发文档中的格式错误及笔误 (#15516) @yali-hzy
* 自动战斗作业文档干员技能值范围补上 0 @status102

### 其他 | Other

* 調整繁中服界園肉鴿 OCR (2/?) @momomochi987
* color [INF] for smoke testing as well @Constrat
* 调整单字干员正则 @ABA2396
* 調整繁中服界園肉鴿 OCR (#15678) @momomochi987
* 删除干员名开头/末尾的_<> @ABA2396
* 添加响石、赤刃明霄陈基建技能加成 (#15674) @drway
* 保全派驻自动战斗使用二值化结果识别干员名 @ABA2396
* 删除无用的正则替换 @ABA2396
* 自动战斗使用二值化结果识别干员名 @ABA2396
* 仅在VS Code中添加辅助项目 (#15669) @status102
* 作业集 Parse 后不删除历史记录，避免错过作业抛出的错误 @ABA2396
* 辞岁行地图 2026-02-10 Map 更新 @status102
* 繁中服不上报企鹅物流 @ABA2396
* devcontainer 适配 CMakePresets.json (#15606) @lucienshawls
* 移除不再使用的 VirtualizingWrapPanel 与 NoAutomationDataGrid @ABA2396
* 调整清理图片缓存样式，增加提示 @ABA2396
* 补全缺少的翻译 @ABA2396
* 优化 emoji @ABA2396
* 自动战斗-自动编队干员不支持技能说明国际化 (#15609) @status102 @Constrat @HX3N @Manicsteiner
* 自动战斗编队技能等级不足 i18n，CN 使用理智药及碎石文案 (#15435) @status102 @HX3N
* 增加借助战 OF-1 在后续刷理智选择`当前/上次`导致禁用时的输出 (#15478) @status102 @Manicsteiner @Constrat @HX3N @ABA2396 @momomochi987
* remove regex from `text` field in EN Sui IS @Constrat
* KR AnnouncementNotFinishedConfirm、ReceptionOptionsRequireInfrast、CreditFightWhenOF-1Warning、MiniGame ConversationRoom and HoneyFruit 支持 @HX3N
* EN minigame honeyfruit、IS6 tip 支持 @Constrat
* JP MiniGame HoneyFruit 支持 @Manicsteiner
* YostarJP ocr fix for roguelike @Manicsteiner
* manual data for txwy @Constrat
* optimize templates @Constrat
* fix casing typo and related context (#15656) @ittuann @Daydreamer114
