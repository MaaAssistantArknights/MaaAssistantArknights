
## v4.23.1

### 新增

- 活动资源更新 @MistEO @ABA2396
- 支持woolang绑定 (#6142) @mrcino

## v4.23.0

### 新增

- 支持带参数启动切换配置 @ABA2396
- 新增新保全关卡内置作业 @AnnoyingFlowers @junyihan233
- 增加烧开水功能，优化萨米肉鸽策略 (#6053) @Lancarus
- 抄作业新增 "skill_times" 字段 (#6007) @AnnoyingFlowers
- 肉鸽开局干员支持点选 is_start 中的干员 @ABA2396
- 增加关卡数据获取成功提示 @ABA2396
- add config `CompatPOSIXShell` (#5945) @wangl-cc
- 关卡导航 - SideStory「理想城：长夏狂欢季」复刻 @ABA2396
- 繁中服風雪過境復刻導航 (#6130) @momomochi987

### 改进

- 修复 PlayCover 下识别差异问题 @MistEO @hguandl
- 修复肉鸽中干员 山 2 技能反复释放的问题 (#6033) @AnnoyingFlowers
- 尝试修复日志被占用的问题 @MistEO
- 修复干员识别错误 (#6022) @Black1312
- 尝试修复mac上阈值差异 @MistEO
- 修复自动编队部分干员文字识别错误 (#6020) @cenfusheng
- 重写自动战斗编队干员检测 @MistEO
- all operators are marked as new for overseas clients (Toolbox-Recruitment) (#6009) @178619
- 修复SSS技能用法判断错误的问题 @MistEO
- 修复编队时“全部”识别错误 @MistEO
- 仓库识别界面未在最左侧时，识别不全的问题 (#6000) @H1MSK
- Prevents possible forced retreat (Amiya S3) @178619
- 修复一些文字识别问题（txwy） (#5993) @cenfusheng
- 紧急修复 dlx 算法工具类的越界问题 @lhhxxxxx
- Mizuki IS DropBox recog. and Shopping regex @Constrat
- SL event navigation @Constrat
- 定时执行逻辑错误 @ABA2396
- "Gummy" with skin undetected with new ROI @Constrat
- increased ROI in fight task @Constrat
- changed award ROI and template  EN @Constrat
- 修复关卡“干酪封锁”与装置““甜蜜狂搅””的识别 (#5975) @cenfusheng
- link in mac-runtime release (#5954) @wangl-cc
- 肉鸽在招募逻辑内可以将同一干员纳入多个群组,修复由于拆分文件造成的bug (#5946) @Lancarus
- 新版萨米肉鸽商店文件&水月肉鸽商店文件提交 (#5936) @Yumi0606
- 错误的类型转换 @ABA2396
- 修复模拟器关闭逻辑错误，增加模拟器退出错误日志 @ABA2396
- `CloseDown` not works on macOS (#5920) @wangl-cc
- 关闭模拟器时索引超出了数组界限 @ABA2396
- 添加繁中服对于鸿雪的OCR等价规则 #5931 #5933 @Constrat
- 肉鸽打折商品识别错误 @ABA2396
- 修复特别关注干员的识别问题 (#5924) @cenfusheng
- wrong template for reception room detection @Constrat
- 修复添加了具有相同键的项 @MistEO
- 添加刷理智任务报错提示 @ABA2396
- [Roguelike] 优化刷源石锭模式的逻辑 (#5987) @lassnuttun
- 优化萨米肉鸽策略 (#5990) @Lancarus
- 优化了萨米肉鸽第三层的部分关卡打法,更新了肉鸽文档 (#5986) @Lancarus @ABA2396
- 优化萨米肉鸽招募逻辑，优化前两层作战逻辑 (#5966) @Lancarus
- optimized ROI for EPISODE navigation @Constrat
- 更新简中OCR识别模型 | Re-training OCR rec model of CN client (#5895) @Plumess
- 更新游戏资源 @yuanyan3060 @MistEO
- 拆分所有肉鸽资源文件 (#5856) @MistEO @Lancarus
- 优化萨米肉鸽不期而遇 (#5890) @cenfusheng
- 傀影肉鸽适配更多干员 (#5817) @cenfusheng
- 适配萨米肉鸽关卡“豪华车队” (#5798) @cenfusheng
- 肉鸽开局助战or自选干员后不再要求选用start属性干员 (#5824) @LingXii
- 增加强制替换adb的信息提示，增加异常处理 (#5838) @ABA2396
- 优化烧开水逻辑，现在能在低难度烧水，高难度开始 (#6094) @Lancarus
- 更新3.4任务流程协议文档 @ABA2396

### 修复

- 强制替换adb报错 @ABA2396
- 修复纯烬艾雅法拉识别 (#5791) @Black1312
- 修复了傀影肉鸽的一些错误 (#5859) @Lancarus
- 修正 zh-cn.xaml 文本错误 (#5861) @javilak
- 尝试修复TN-4地图view错误 (#6097) @status102
- txwy stage navigation, similar to 4c2c644 @Constrat

### 其他

- 更新文档内容 (#5901) (#5885) @SherkeyXD @AnnAngela
- Events and Banners displayed according to timetable (#6050) @Constrat
- 添加用户词典，减少拼写错误提示 @ABA2396
- merge and re-structure stage.json (#5944) @MistEO
- golang 接口从io/ioutil包更新为os包 (#5988) @javilak
- 重构 TaskQueueViewModel (#5753) @ABA2396
- 重构 CopilotViewModel.cs (#6003) @ABA2396
- 重构 RecognizerViewModel.cs @ABA2396
- 重构 CopilotViewModel.cs @ABA2396
- tweaked english tasks json schema @Constrat
- 3.1-Integration.md (#6039) @NtskwK
- Update 3.1-集成文档.md (#6036) @hzxjy1
- 修正偶尔OCR错误造成的吃48小时理智药失效问题 (#6004) @bigqiao
- gui.log 启动时记录目录 @ABA2396
- ML translation @Constrat
- tools: global content update @Constrat
- ormatting + output log cleanup / clearup @Constrat
- Feat: Auto Update now orders Global tasks.json like Official @Constrat
- localization update @178619
- 调整肉鸽商品识别roi范围 @ABA2396
- Useless png in roguelike JP @Constrat
- tools: added impossible to translate task @Constrat
- 为文档添加docsearch和sitemap支持 (#5785) @SherkeyXD
- CI-在静态分析中增加基线 (#5757)(#5867) @hxdnshx
- 修改了部分肉鸽文档 (#5873) @Lancarus
- typo of 1.3-模拟器支持.md (#5802) @BlueandwhiteXD
- fix rpath for macos @horror-proton
- add log for #6006 @MistEO
- roguelike: 适配刷源石锭模式不期而遇选择策略 (#6102) @Yumi0606
- 启动时检测是否为管理员启动 @ABA2396
- 添加报错信息 #6089 @ABA2396
- 修改格式为适配更多启动参数 @ABA2396
- 合并关闭+重启方法 @ABA2396

### For overseas

#### txwy

- 更新繁中服肉鴿不期而遇 ocrReplace (#5882) @momomochi987
- 修正繁中服水月肉鴿會卡在 "緊急運輸" 的問題 (#5790) @momomochi987
- 繁中服 快捷編隊 部署費用 (#6131) @vonnoq
- 適配繁中服保全派駐 (#5926) @momomochi987
- remake StageSK & StagePR-C picture for txwy (#5999) (#6018) @momomochi987
- 添加繁中服对于鸿雪的OCR等价规则 #5931 (#5933) @Roland125 @Constrat @MistEO

#### YostarEN

- d0dc294 auto update version issue @Constrat
- more long names regex for OperBox @Constrat
- ROI for BattleStartOCR, new IMG+ROI for Oper @Constrat
- OperBox regex, BattleMatcher regex, longname and IS @Constrat
- modified text for EX stages (WB event) @Constrat
- SSS Identification issue with NON-1080 emulators @Constrat
- DepotAllTab for EN from 1126db9 @Constrat
- Adapted EN regex for new AutoSquad algorithm (#6028) @Constrat
- tweaked SSS copilot descriptions for EN @Constrat
- PRTS3 and LevelOFDifficulty i18n EN @Constrat
- Senior Operator tag EN @Constrat
- Infrast Task stuck Max Trust notification EN @Constrat
- Reception sends clue when full in YostarEN @Constrat
- battlequickformationocr modified roi for EN @Constrat
- optimized: regex for I.S. trader shopping in EN @Constrat
- Invitation to Wine rerun implementation for EN @Constrat
- Oper Regex Tweaked for EN (BattleMatcher) @Constrat
- Adapted EndOfActionAnnihilation for EN @Constrat
- Paradox Simulation for YostarEN + BattleMatcher regex @Constrat

#### YostarKR

- add templates for YoStarKR @178619
- SSS copilot support for YostarKR @178619
- make the option to not recruit selectable in SSS for YoStarKR @178619
- Goldenglow(澄闪) & Surtr(史尔特尔) ocrReplace (#5852) @Large-Japchae
- nbsp character from OCR (YoStarKR) @178619
- ocr replace for YostarJP (#6093) @jie17

#### YostarJP

- Invitation to Wine rerun implemen. for JP @Constrat
- add roguelike encounter ocrReplace for JP and EN (#5903) @momomochi987
- [JP] Docs Update (#6070) @wallsman
- add ocr replace for YostarJP (#5977) @jie17
- Removed useless .png from JP I.S. @Constrat
