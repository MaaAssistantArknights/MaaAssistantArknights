## v4.23.0-beta.1

### 新增

- 新增新保全关卡内置作业 @AnnoyingFlowers @junyihan233
- 增加烧开水功能，优化萨米肉鸽策略 (#6053) @Lancarus
- 抄作业新增 "skill_times" 字段 (#6007) @AnnoyingFlowers
- 肉鸽开局干员支持点选 is_start 中的干员 @ABA2396
- 增加关卡数据获取成功提示 @ABA2396
- add config `CompatPOSIXShell` (#5945) @wangl-cc

### 改进

- 添加刷理智任务报错提示 @ABA2396
- [Roguelike] 优化刷源石锭模式的逻辑 (#5987) @lassnuttun
- 优化萨米肉鸽策略 (#5990) @Lancarus
- 优化了萨米肉鸽第三层的部分关卡打法,更新了肉鸽文档 (#5986) @Lancarus @ABA2396
- 优化萨米肉鸽招募逻辑，优化前两层作战逻辑 (#5966) @Lancarus
- optimized ROI for EPISODE navigation @Constrat
- 更新简中OCR识别模型 | Re-training OCR rec model of CN client (#5895) @Plumess

### 修复

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

### For Overseas

#### YostarKR

- add templates for YoStarKR @178619
- SSS copilot support for YostarKR @178619
- make the option to not recruit selectable in SSS for YoStarKR @178619

#### YostarJP

- Invitation to Wine rerun implemen. for JP @Constrat
- add roguelike encounter ocrReplace for JP and EN (#5903) @momomochi987
- [JP] Docs Update (#6070) @wallsman
- add ocr replace for YostarJP (#5977) @jie17
- Removed useless .png from JP I.S. @Constrat

#### txwy

- 適配繁中服保全派駐 (#5926) @momomochi987
- remake StageSK & StagePR-C picture for txwy (#5999) (#6018) @momomochi987
- 添加繁中服对于鸿雪的OCR等价规则 #5931 (#5933) @Roland125 @Constrat @MistEO

#### YoStarEN

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
