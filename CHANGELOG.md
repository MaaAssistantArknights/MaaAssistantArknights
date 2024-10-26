## v5.8.0

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
