## v5.20.0-beta.1

### 主要更新 | Highlight

#### ⚠️ 重要提醒

本次更新调整了模板图的目录结构，若您正在从 v5.19.x 及以下版本**手动**更新至该版本，那么**请勿直接覆盖旧版本文件夹**，否则可能导致部分旧文件未被正确删除，从而引发识别问题。

推荐的更新方式有两种：
1. 使用自动更新功能（推荐）：自动更新将会正确清理旧文件。
2. 手动更新时：请将新版本解压到一个全新的文件夹，再将原有的 config 与 data 文件夹复制过去，以保留您的配置。

#### 界园肉鸽来咯～(∠・ω< )⌒★ | Highlight

牛牛火速为大家适配了界园肉鸽的刷钱模式！现在大家可以愉快的给坎诺特上供啦

至于刷等级模式，各位博士还请耐心等待 ~~，我们已经在压榨帕鲁干活了（划掉~~

---

以下是详细内容：

### 新增 | New

* 初步适配界园肉鸽 @DavidWang19 @SherkeyXD @status102 @HYY1116 @Saratoga-Official @HYY1116 @SherkeyXD @ABA2396

### 改进 | Improved

* 干员因模组不可选中时, 加个输出 (#13230) @status102
* 改进水月肉鸽干员部署逻辑 (#12671) @linlucath @Saratoga-Official

### 修复 | Fix

* 刷理智界面设计器error @status102
* 基建任务枚举引用路径统一, 修复设计器error @status102
* 移除ClientType未选择, 统一变更为官服 (#13220) @status102
* ComboBox 剪贴板异常 @ABA2396
* 公告及更新日志背景颜色异常 @ABA2396

### 其他 | Other

* 启动时成就打包 (#13221) @status102
* 移动所有主题的模板图 @SherkeyXD
* bump dependencies (#13121) @SherkeyXD
* KR JieGarden translation @HX3N

## v5.19.0-beta.1

### 新增 | New

* 优化会客室线索处理逻辑 (#13160) @Lemon-miaow
* 官网添加非当前电脑构架提示 @ABA2396
* 支持MaaPipelineSupport插件 (#13189) @neko-para
* 添加悬浮窗标识符 @ABA2396
* 添加任务及基建设施日志翻译 @ABA2396
* 新增公招星级悬浮提示 @ABA2396
* 新增通过 string 直接构造 Tooltip 方法 @ABA2396
* cdk 添加复制按钮 @ABA2396
* 新增资源仓库超链接 @ABA2396
* 为截图测试日志添加 Tooltip 展示所有截图方式耗时 @ABA2396
* 支持编队添加干员时选择模组 (#12811) @travellerse @status102 @ABA2396
* 日志栏新增 ToolTip 提示 @ABA2396
* 新增返回 bgr byte[] 数据接口，大幅降低 peep 性能占用 @ABA2396
* 新增雷电专版注册表自动检测，新增雷电端口自动检测 @ABA2396
* fetch 请求增加 If-Modified-Since @ABA2396

### 改进 | Improved

* 添加萨卡兹肉鸽 机动队 溃乱魔典 盲盒广场 关卡策略 (#12636) @zitzoom
* 添加缺失的翻译与文档更新 @ABA2396
* 优化掉落显示 @ABA2396
* 优化物品图显示缓存 @ABA2396
* 肉鸽参数显示简化, 开局干员空时不显示使用助战 @status102
* 优化窗口显示 @ABA2396
* 训练室继续专精使用特征匹配识别技能 @ABA2396
* 优化热更逻辑 @ABA2396

### 修复 | Fix

* TooltipBlock 无法使用 binding @ABA2396
* 肉鸽, 生息演算 calcBinding枚举修复 @status102
* 商店功能错误购买商品 @ABA2396
* 小工具-公招识别 手动触发时不会改变显示状态 @ABA2396
* 移除Roguelike@GetDrop2中重复的Roguelike@DropsFlag任务 (#13180) @status102
* 肉鸽界面设计器error (#13185) @status102
* typo & const ref @status102
* 修复 ExpiringStatus 无法用于日志输出的问题 @ABA2396
* 理智药使用量ocr错误 @status102
* 修复模板匹配遇到 inf 与 nan 时返回 0 的错误结果 @ABA2396
* 无法匹配活动商店部分商品 @ABA2396
* 外服训练室技能名可能被部分替换成干员名 @ABA2396
* import @status102
* 更新肉鸽商店刷新按钮模板 (#13172) @Lemon-miaow
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

### 其他 | Other

* 抽离 ToolTip 创建方法，统一 ToolTip 格式 @ABA2396
* 日志栏使用 TooltipBlock @ABA2396
* TooltipBlock 支持显示 svg @ABA2396
* YostarKR SS@Store@CheckUnlimited.png @HX3N
* manual resource updater @Constrat
* YostarEN SS@Store@CheckUnlimited.png @Constrat
* YostarJP SS_Store_CheckUnlimited (#13192) @Manicsteiner
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
* bump maa-cli to 0.5.6 (#13150) @wangl-cc
* 添加日志记录 @ABA2396
* 设置剪切板前先清空 @ABA2396
* response.Log 添加 etag 日志 @ABA2396
* 添加模拟器路径与附加命令的提示 @ABA2396

