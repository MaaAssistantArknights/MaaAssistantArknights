## v5.20.0

### 界园导游，启动！ | Highlight

我们为你带来了界园肉鸽实装以来的第一个稳定版，如果有遇到任何问题请及时反馈~

#### 自动肉鸽模式支持界园主题

这个版本我们初步支持了界园肉鸽的刷源石锭存钱策略（牛牛使用维什戴尔+狙医分队/指挥分队最稳），其他功能（比如速刷等级、速烧开水等）还在开发中哦~

#### 这个问号是什么，这个箭头又是什么？

对于部分难懂的设置项，我们添加了显眼的问号图标，将鼠标悬浮上去即可查看详细说明~

另外，日志中部分内容支持扩展显示，鼠标悬浮在该行的箭头处即可查看扩展内容（如截图延迟、自动公招推测的干员列表等）

#### 自动战斗功能支持设定模组啦

在这个版本开始，自动战斗功能将支持作业里设置干员的模组（zoot.plus 的作业编辑器 v2 版支持设置模组），这样，对于部分有特定模组要求的作业，牛牛也能自动设置啦~

不过目前自定干员部分还不支持设置模组哦，所以如果要带深海队挂件的话还是得手动设置下模组~

#### 模拟器支持

这个版本开始，牛牛能自动检测雷电模拟器明日方舟专版并设置端口啦~

----

#### [CN ONLY] Auto I.S. Supports the Sui's Garden of Grotesqueries

This version initially supports investing coins in Sui's Garden of Grotesqueries (the most stable setup is using Wiš'adel in High Ground Breakthrough Squad (高台突破分队, new squad in this I.S.), Tactical Ranged Squad or Leader Squad). Other features (such as quick gaining experience points, grinding to obtain Hot Water Kettle or Elite II operators, etc.) are still under development~

#### What Is This Question Mark? And What About This Arrow?

For some confusing settings, we've added prominent question mark icons, hover your mouse over them to see detailed explanations~

Additionally, some log content supports expanded display. Hover the mouse over the arrow in the corresponding row to view extended details (such as screenshot delays, automatically inferred operator lists from recruitment, etc.)

#### Copilot now supports module configuration

Starting with this version, the copilot feature will support setting operator modules in tasks (zoot.plus's tasks editor v2 supports module configuration). This means we can now automatically configure modules for tasks that have specific module requirements~

However, custom operator configurations currently do not support module settings, so for example if you want to bring Abyssal Hunter operators, you'll still need to manually configure their modules~

#### Emulator Support

Starting from this version, we can automatically detect the LD Emulator Arknights Special Edition and configure the port~

----

以下是详细信息：

### 新增 | New

* 初步适配界园肉鸽并初步实现存钱 (#13183) @DavidWang19 @Daydreamer114 @SherkeyXD @status102 @HYY1116 @Saratoga-Official
* macos 添加界园入口 @hguandl
* maa-cli 初步适配界园肉鸽 (#13150) @wangl-cc
* 临时屏蔽界园中除投资外的策略选项 @ABA2396
* 优化会客室线索处理逻辑 (#13160) @Lemon-miaow @pre-commit-ci[bot]
* 添加任务及基建设施日志翻译 @ABA2396
* 新增公招星级悬浮提示 @ABA2396
* 为截图测试日志添加 Tooltip 展示所有截图方式耗时 @ABA2396
* 支持编队添加干员时选择模组 (#12811) @travellerse @pre-commit-ci[bot] @status102 @ABA2396
* 适配部分已知的不期而遇 (#13233) @HYY1116 @pre-commit-ci[bot] @SherkeyXD
* 界园肉鸽二层作战逻辑 @Saratoga-Official
* 刷开局wpf仅显示当前主题可用选项 @SherkeyXD
* 界园肉鸽烧水支持票券 @SherkeyXD
* 肉鸽支持多级事件选择 @ABA2396
* 界园肉鸽部分关卡自动战斗逻辑 (#13219) @Saratoga-Official

### 改进 | Improved

* 添加萨卡兹肉鸽 机动队 溃乱魔典 盲盒广场 关卡策略 (#12636) @zitzoom
* 肉鸽参数显示简化, 开局干员空时不显示使用助战 @status102
* 训练室继续专精使用特征匹配识别技能 @ABA2396
* 干员因模组不可选中时, 加个输出 (#13230) @status102 @pre-commit-ci[bot]
* 改进水月肉鸽干员部署逻辑 (#12671) @linlucath @Saratoga-Official
* 先筛选未进驻再清空队列 @ABA2396
* 调整干员罗比菈塔在肉鸽使用的技能 @Saratoga-Official
* 界园肉鸽中停止招募锡人 @DavidWang19
* 优化界园不期而遇默认逻辑 @DavidWang19

### 修复 | Fix

* 烧水仅精二在萨米主题被短路启动 @status102
* 切换肉鸽主题时, 烧水奖励选择丢失 @status102
* fix. YostarEN OR navigation fix @Constrat
* 商店功能错误购买商品 @ABA2396
* 小工具-公招识别 手动触发时不会改变显示状态 @ABA2396
* 理智药使用量ocr错误 @status102
* 无法匹配活动商店部分商品 @ABA2396
* 外服训练室技能名可能被部分替换成干员名 @ABA2396
* 更新肉鸽商店刷新按钮模板 (#13172) @Lemon-miaow @pre-commit-ci[bot]
* 一叶扁舟 choices @Daydreamer114
* JP 似是而非 OCR @Daydreamer114
* 魂灵见闻：善恶同道 OCR @Daydreamer114
* EN training room OCR @Constrat
* 获取雷电 pid 失败 @ABA2396
* EN not enough space for SSS selection @Constrat
* EN InfrastTrainingOperatorAndSkill adaptation @Constrat
* 训练室技能名识别 @ABA2396
* 高版本检测到低版本更新时错误提示无更新包 @ABA2396
* 无法继续专精 @ABA2396
* 新版肉鸽商店刷新 @ABA2396
* 移除ClientType未选择, 统一变更为官服 (#13220) @status102
* ComboBox 剪贴板异常 @ABA2396
* 公告及更新日志背景颜色异常 @ABA2396
* 去掉界园没有的刷2希望开局 @DavidWang19
* 修复wpf主题检测错误 @SherkeyXD
* 修复界园主题选择问题 @DavidWang19
* 卡得偿所愿 @ABA2396
* 更新退出事件模板图 @DavidWang19
* 手动改时间或睡眠跨日后关卡列表与提示未更新 @ABA2396
* v5.20.0-beta.2.d037 重启后概率闪退 @ABA2396
* YostarEN OR navigation @Constrat
* YostarEN Eye for an Eye Sarkaz Encounter @Constrat
* YostarKR 以血还血 ocr tweak @HX3N
* 尝试修复点击过头导致选到月度小队 @SherkeyXD
* yostaren sarkaz encouter ocr tweaks @Constrat
* 缺 dll 和 resource 的时候没弹喜报 @ABA2396
* 勾选｢反转主任务右键单击效果｣时，半选主任务在重启后无法还原选中状态 @ABA2396
* 最小化会在桌角留下一个小窗口 @ABA2396
* 勾选启动后直接最小化后无法拖动窗口 @ABA2396
* 更新关卡ocr替换 @DavidWang19
* 肉鸽导航无法选择傀影肉鸽 (#13255) @Lemon-miaow
* 修复在得偿所愿或其他位置选择投钱后卡住的问题 @DavidWang19
* 调整指点迷津节点阈值 @DavidWang19

### 文档 | Docs

* changelog @status102
* update changelog @SherkeyXD
* 更新changelog @DavidWang19
* 修复 readme logo @Rbqwow
* 还漏了两个 @DavidWang19
* 还漏了一个 @SherkeyXD
* 还有一只.png @SherkeyXD
* 有猪.jpg @SherkeyXD
* 更新肉鸽文档 @ABA2396

### 其他 | Other

* 修复检测不到next的问题 @DavidWang19
* RunningState 无法记录日志 @ABA2396
* 刷理智代理倍率移除1000适配 @status102
* typo @status102
* 肉鸽导航 @Lemon-miaow
* 刷理智界面设计器error @status102
* import @status102
* 基建任务枚举引用路径统一, 修复设计器error @status102
* TooltipBlock 无法使用 binding @ABA2396
* 肉鸽, 生息演算 calcBinding枚举修复 @status102
* 移除Roguelike@GetDrop2中重复的Roguelike@DropsFlag任务 (#13180) @status102
* 肉鸽界面设计器error (#13185) @status102
* typo & const ref @status102
* 修复 ExpiringStatus 无法用于日志输出的问题 @ABA2396
* 修复模板匹配遇到 inf 与 nan 时返回 0 的错误结果 @ABA2396
* 优化加载顺序, 简化StageManager变量 (#13277) @status102
* 肉鸽任务参数初始化简化 @status102
* 不期而遇子事件可读性优化 @SherkeyXD
* 优化日志输出 @ABA2396
* 优化指点迷津节点task的复用 @DavidWang19
* 重构部分预载 @status102
* 添加缺失的翻译与文档更新 @ABA2396
* 优化掉落显示 @ABA2396
* 优化物品图显示缓存 @ABA2396
* 优化窗口显示 @ABA2396
* 优化热更逻辑 @ABA2396
* 添加 idle 状态切换记录 @ABA2396
* 新Config任务参数读写, 界面刷新, 任务添改base @status102
* 新的添加任务 @status102
* 新增通过 string 直接构造 Tooltip 方法 @ABA2396
* 官网添加非当前电脑构架提示 @ABA2396
* 支持MaaPipelineSupport插件 (#13189) @neko-para
* 添加悬浮窗标识符 @ABA2396
* cdk 添加复制按钮 @ABA2396
* 新增资源仓库超链接 @ABA2396
* 日志栏新增 ToolTip 提示 @ABA2396
* 新增返回 bgr byte[] 数据接口，大幅降低 peep 性能占用 @ABA2396
* 新增雷电专版注册表自动检测，新增雷电端口自动检测 @ABA2396
* fetch 请求增加 If-Modified-Since @ABA2396
* 整理一下刚糊的ua @MistEO
* yostaren prepare leizi for alter @Constrat
* 抽离 ToolTip 创建方法，统一 ToolTip 格式 @ABA2396
* 日志栏使用 TooltipBlock @ABA2396
* TooltipBlock 支持显示 svg @ABA2396
* YostarKR SS@Store@CheckUnlimited.png @HX3N
* manual resource updater @Constrat
* YostarEN SS@Store@CheckUnlimited.png @Constrat
* YostarJP SS_Store_CheckUnlimited (#13192) @Manicsteiner @pre-commit-ci[bot]
* 移除导航 filters @ABA2396
* 统一颜色 @ABA2396
* 使用自定义悬浮窗样式 @ABA2396
* 使用 svg 代替 >，避免字体影响 @ABA2396
* 添加设计时预览 @ABA2396
* 日志栏 InitialShowDelay 改为 200，避免误触 @ABA2396
* 使用 UsedImplicitly @ABA2396
* YostarKR BattleQuickFormationModulePage @HX3N
* YostarEN BattleQuickFormationModulePage @Constrat
* YostarJP tasks edits (#13187) @Manicsteiner
* style @status102
* 生息演算任务参数枚举 @status102
* tooltip 顶部居中 @ABA2396
* 调整成就描述 @ABA2396
* YostarEN OR Navigation @Constrat
* YostarKR OR navigation (#13179) @HX3N
* style @status102
* 肉鸽模式参数枚举 @status102
* file header @status102
* 新Config长草任务存储已完成部分预载 @status102
* 新Config结构修改 @status102
* 调整提示 @ABA2396
* no overflow @SherkeyXD
* 勾选手动填写实例号后查询一次实例 @ABA2396
* 避免使用 async void @ABA2396
* 省略默认参数 @ABA2396
* 调整成就描述 @ABA2396
* 调整 ocrReplace @ABA2396
* 添加 FeatureMatch schema @ABA2396
* 米奇妙妙 ocr @ABA2396
* 调整描述 @ABA2396
* 添加日志记录 @ABA2396
* 设置剪切板前先清空 @ABA2396
* response.Log 添加 etag 日志 @ABA2396
* 添加模拟器路径与附加命令的提示 @ABA2396
* 启动时成就打包 (#13221) @status102
* 调整肉鸽日志颜色 @ABA2396
* 调整投资描述 @ABA2396
* 移动所有主题的模板图 @SherkeyXD
* 移动遗漏的肉鸽模板图 @SherkeyXD
* bump dependencies (#13121) @SherkeyXD
* KR JieGarden translation @HX3N
* 调整肉鸽日志颜色 @ABA2396
* 刷开局保证传给core的只有本主题可用内容 @SherkeyXD
* 刷开局精二仅为水月和萨米显示 @SherkeyXD
* 移除未被使用的wpf文本 @SherkeyXD
* 删除界园多余的模板图和task @DavidWang19
* EN IS5 squads @Constrat
* 界园肉鸽没有二希望开（？） @ABA2396
* YostarJP ocr edits (#13276) @Manicsteiner
* bump maa-cli to 0.5.7 (#13278) @wangl-cc
* 肉鸽烧水参数rename @status102
* 肉鸽开局奖励选择 @status102
* TaskSettingVisibilityInfo.Current -> Instance @status102
* YostarKR OR ocr edits @HX3N
* 长草任务当前选中index @status102
* Wpf UserAgent with UiVersion @MistEO
