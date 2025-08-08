## v5.22.2

### 新增 | New

* StartUp识别开始唤醒时最开始的完整性检测 (#13576) @status102 @HX3N @Constrat @Manicsteiner
* 首次使用肉鸽选难度 @ABA2396
* 更新 243 极限效率一天四换排班表（20250806 修订） (#13561) @bodayw
* u16 OCR replace (#13533) @MistEO @pre-commit-ci[bot]
* 调整翻译 @ABA2396

### 改进 | Improved

* 肉鸽难度在首次进入后，仅在通关后才再次尝试调整 @status102
* UseStoneDisplay绑定优化 @status102
* Use default config key @status102
* 优化成就列表滚动条 @ABA2396

### 修复 | Fix

* 招募助战的确认多试几次 @ABA2396
* 最后一个节点开到招募藏品 @ABA2396
* 漏了 @status102
* using @status102
* 诡谲断章有概率识别错误 @Saratoga-Official
* 抗干扰值识别错误时卡树洞 @ABA2396
* 统一使用NotChooseLevel1 @status102
* 肉鸽招募完干员点击右上角时点到了动画结束后的开始探索 @ABA2396
* 关闭骰子页面时点进藏品页 @ABA2396
* 干员识别时因特殊排序时的遮挡而导致识别异常 (#13538) @Burnside999 @pre-commit-ci[bot] @HX3N
* EN Phantom IS updated Drop templates @Constrat
* 使 python 脚本支持从 github 获取文件长度并更新 (#13457) @cibimo
* EN IS5 missing template GetDrop4 @Constrat

### 文档 | Docs

* KR update and overhaul entire documentation (#13550) @HX3N @pre-commit-ci[bot]
* cache defaults to true in task-schema.md @Constrat

### 其他 | Other

* 调整 指点迷津 与 思维边界 的颜色范围 @ABA2396
* 调整节点识别范围 @ABA2396
* stages 识别失败时放弃当前探索 @ABA2396
* 沉睡石像-战斗 @ABA2396
* fix EN task schema @Constrat
* 扩大 ChooseDifficulty base 范围 @ABA2396
* update comment @Constrat
* wpf拆分Enum @status102
* YostarJP ocr fix (#13571) @Manicsteiner
* nullable update @status102
* KR minigame tweak @HX3N
* YostarKR tweak Roguelike@StartExploreCD text @HX3N
* wpf刷理智UI变量rename @status102
* wpf刷理智UI默认值类型 @status102
* wpf移除不必要的null移除转换 @status102
* 移除肉鸽非开局时的EnterAfterRecruit，完成开始探索状态分离 @status102
* 肉鸽月度小队模式和深入探索开局招募流程状态与局内分离 @status102
* EN AT minigame tweak @Constrat
*  fix: YostarKR update ocrReplace and ocr_config (#13562) @HX3N
* 调整 schema (#13554) @ABA2396 @pre-commit-ci[bot]
* EN main readme @Constrat
* EN overhaul entire documentation! @Constrat
* EN manual updates/fixes @Constrat
* EN device updates/fixes @Constrat
* EN manual\cli updates/fixes @Constrat
* EN develop updates/fixes @Constrat
* ci tutorial for EN @Constrat
* EN I.S. manual update @Constrat
