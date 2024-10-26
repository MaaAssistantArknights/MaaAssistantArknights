## v5.8.0

### 演进之美，越进越美 | Highlight

本次版本更新可谓是大快所有人心的大好事，我们为你带来了以下新内容：

#### 重大变化 | BREAKING CHANGES

**如果你使用的是 MuMu 12 模拟器**，请检查你的【设置 - 连接设置】中的“连接地址”，如果使用的是 `7555` 端口，请参考 [文档](https://maa.plus/docs/zh-cn/manual/connection.html#%E6%A8%A1%E6%8B%9F%E5%99%A8%E7%9B%B8%E5%85%B3%E6%96%87%E6%A1%A3%E5%8F%8A%E5%8F%82%E8%80%83%E5%9C%B0%E5%9D%80) 修改成正确的端口。

如不这么做，可能会导致你从这个版本开始无法使用 MuMu 截图增强模式。

#### MuMu 截图增强模式优化

现在我们正式支持 MuMu 12 模拟器开启后台保活了，同时得益于此项和上面提到的优化，你也无需手动填写实例编号和屏幕编号了。

#### 自动检测连接设置

在这个版本，我们支持自动检测和填写连接设置：

1. MuMu 12 和雷电 9 模拟器支持自动检测模拟器安装路径；
2. 蓝叠 5、MuMu 12、雷电 9、夜神、逍遥模拟器支持自动检测 ADB 路径、连接地址和连接配置（仅在只有单个模拟器运行时可用，多个模拟器同时运行时不可用）。

若检测失败，请尝试使用 UAC 管理员权限启动 MAA 并再次检测。若仍失败，则请参考[文档](https://maa.plus/docs/zh-cn/manual/connection.html)手动设置。

#### 资源版本可一键自行更新

还记得在不远的过去，我们禁用了自动更新资源版本。现在，我们为你实现了一键更新功能，在【设置 - 软件更新】处新增了“资源更新”按钮，只需一键即可更新资源版本，让你能在不更新 MAA 版本就能使用新活动的自动战斗功能。

至于为什么不恢复成自动更新呢，这绝对和该功能是通过下载 [MaaResource](https://github.com/MaaAssistantArknights/MaaResource) 仓库源代码压缩包再解压替换的本质无关。（对…对吗？）

如果你无法访问上面的仓库，那么这项功能可能与你无缘。如果你需要特殊的手法，建议查看【设置 - 软件更新】处的输入框。

顺带一提，本次版本，自带资源版本更新至 SideStory「追迹日落以西」。

#### 自动肉鸽优化

我们调整了相关逻辑，使得自动肉鸽功能更加好用，例如：适配了维娜 · 维多利亚、乌尔比安，调整了维什戴尔、维娜 · 维多利亚的优先级，优化了相关干员分组，等等。

----

以下是详细内容：

### 新增 | New

* 新增server酱3的推送逻辑 (#10805) @ClozyA
* GO 导航 @ABA2396
* 自动战斗-战斗列表增加单任务作业查看 (#10764) @status102
* 软件更新中添加手动更新资源版本 (#10877) @ABA2396
* WpfGui限制单路径多实例 | restricted multi-instance under same MAA (#10869) @qwedc001
* YostarEN BB navigation @Constrat
* YostarKR BB navigation @HX3N

### 改进 | Improved

* 支持mumu后台保活，通过包名获取display_id @ABA2396
* IS Phantom Update 雕匠与石像.json (#10829) @Daydreamer114
* 更新肉鸽 ew 优先级 (#10830) @Daydreamer114
* 将萨米、萨卡兹肉鸽阵容完备度的 益达、玛恩纳、水陈 组调为2 (#10831) @Daydreamer114
* RA加快开局教程对话 (#10824) @Daydreamer114
* 降低萨米、萨卡兹肉鸽 维娜·维多利亚 招募优先级 (#10792) @Daydreamer114
* 傀影肉鸽招募适配乌尔比安 (#10798) @Daydreamer114
* 萨米、萨卡兹肉鸽招募适配维娜 · 维多利亚 (#10765) @Wangqiusu020820
* 自动战斗自定义添加干员编队时按职业选择 @status102
* 雷电截图增强支持自动检测模拟器安装路径 @ABA2396

### 修复 | Fix

* b服支持多分辨率多 dpi 登录 (#10817) @ABA2396
* 关卡导航失败 @ABA2396
* 无法导航至关卡9-19刷理智 @ABA2396
* 部分情况下信用购物因过度动画购买错误 (#10893) @Alan-Charred
* 部分情况下信用购物因过度动画购买错误 @ABA2396
* semicolon missing (#10892) @Alan-Charred
* 部署/开技能后添加等待过度动画延迟，避免屏幕倾斜遇到的识别错误 @ABA2396
* 获取信用及购物中不勾选访问好友也会执行 @ABA2396
* 更换确认助战 roi 时填写值错误 (#10762) @Daydreamer114
* 为随机借助战功能添加点击确认按钮的步骤 (#10757) @Alan-Charred
* increase ROI size for Tales@RA@Craft fix #10742 @Constrat
* 修复肉鸽阵容完备度中单个干员重复出现于多个干员组的问题 (#10806) @Daydreamer114
* 公开招募检测 Tag 失败时尝试识别 <继续招募> 按钮 (#10846) @Alan-Charred @ABA2396 @Constrat @HX3N
* 降低 肉鸽投资确认 模板匹配阈值 (#10864) @Daydreamer114
* 自动编队无法选中特种标签页 @ABA2396
* 官服切换账号识别问题 @ABA2396
* b 服切换账号无法选择下拉列表中的中文名 @ABA2396
* 维娜·维多利亚 OcrReplace (#10763, #10818) @Daydreamer114 @ABA2396
* 修复维多利亚正则错误 (#10850) @Saratoga-Official
* GO 识别错误 (#10844) @AnnAngela
* 日服偶现无法开始行动 @ABA2396
* update BB navigation for EX YostarEN @Constrat
* YostarKR replaced StageFerociousPresageEnter.png to improve recognition score @HX3N
* YostarJP ocr fix (#10780) @Manicsteiner
* Update JP バベル (#10758) @wallsman
* update YostarEN MallSiege template fix #10777 @Constrat
* fix ROI box for babel EN OCR @swablueme
* remove custom EnterInfrastSiege for YostarEN @Constrat
* YostarKR tweak Sami StageEnterBattleAgain specificRect (#10767) @HX3N
* YostarKR add ocrReplace on RecruitSupportConfirm (#10883) @HX3N

### 文档 | Docs

* typo in[docs] 更新MuMu截图增强模式中端口的文本描述 (#10866) @lcebot
* 更新 bug-issue 模板 (#10811) @Daydreamer114
* 错误分类 changelog (#10887) @ABA2396
* MuMu 截图增强适配后台保活&雷电截图增强文档 (#10783) @Rbqwow @HX3N @ABA2396
* 明确文档中的 MacOS 称呼 (#10734) @Daydreamer114 @Rbqwow @Constrat
* 修改文档匹配算法描述错误 @ABA2396
* 更新文档（自动检测 雷电端口 截图增强） (#10753) @Rbqwow
* 更新肉鸽文档 (#10807) @Daydreamer114
* 更新MuMu截图增强模式中端口的文本描述 (#10851) @ClozyA
* Mumu regex @status102
* Issue 模板添加雷电截图增强相关 (#10754) @Rbqwow
* fix issue-checker macGui regex (#10755) @Alan-Charred
* Translated using Weblate (English (United States)) @weblate

### 其他 | Other

* mumu display id error if startup ark manually (#10787) @MistEO
* mumu display id error when ark not startup (#10786) @MistEO
* 移除无效的try-catch @status102
* 修复错误的`定时 7`分钟最大值 (#10804) @Rbqwow
* 优化设置指引 @ABA2396
* bump maa-cli to 0.5.1 (#10899) @wangl-cc
* 添加日志 @ABA2396
* 调整雷电自动检测支持端口 @ABA2396
* Revert "ci: use runs/job instead of just runs for ResourceUpdater" @Constrat
