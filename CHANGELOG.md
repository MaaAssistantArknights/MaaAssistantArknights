## v4.19.0-rc.1

### 新增

- Supported Auto I.S. (Mizuki Roguelike) for YostarKR, YostarJP, YostarEN @178619 @su6aru @liuyifan-eric @Pitiedwzr @RiichiTsumo
- 切换配置功能 @MistEO @ABA2396
- 自定义关卡快捷输入 @ABA2396

### 改进

- 支持连接失败重启 ADB @ABA2396
- 优化关卡列表相关逻辑 @ABA2396
- 扩大任务设置可选范围 @moomiji
- 添加刷理智任务设置中显示基建计划功能 @moomiji
- improve ConfirmExit() @moomiji
- 添加隐藏关闭按钮功能 @moomiji
- 添加托盘图标强制显示MAA功能 @moomiji
- 增加一个文件被占用的解决方法 @MistEO
- 加回 gui.json 备份机制 @MistEO
- 勾选`手动输入关卡名`时可勾选`隐藏当日不开放关卡` @moomiji
- 为日志加入简单的rotate @MistEO

### 修复

- 修复截图方式判断错误导致的操作慢问题 @horror-proton @MistEO
- 修复切换日服后再切回后OCR配置不清空的问题 @MistEO
- 修复干员 Box 识别错误 @MistEO
- 拖动任务顺序后无法保存修改后的顺序 @ABA2396
- 关卡名OCR识别大小写出错导致掉落上传失败 #4867 @ABA2396
- 尝试修复关卡选择变为null的情况 #4871 @ABA2396 @moomiji
- 关卡选择为null时仍会执行借助战任务 @ABA2396
- 修复 `火哨` 的基建技能 @MistEO
- 修改LanguageList初始化过程 @ABA2396
- 修复 `夕` 识别错误 @cenfusheng

### 其他

- 创建本地化文档自动翻译工具 & 更新本地化文档 (#4912) @UniMars @wallsman
- 修复资源更新器不更新水月肉鸽的问题 @MistEO
- update meojson (#4789) @MistEO
- 部分 CI 优化 @MistEO
- Update operators.md (#4929) @wallsman
- 修改对于蓝叠的支持说明 @ABA2396 @MistEO
- 优化调整一些 CI @AnnAngela @MistEO
- 翻译优化 @ABA2396 @LiamSho 

### For overseas

#### Common

- i18n: Translations update from MAA Weblate (#4971) @LiamSho 
- update overseas json/template tool @liuyifan-eric

#### YostarJP

- Supported Auto I.S. (Mizuki Roguelike) (#4936) (#4921) @su6aru @liuyifan-eric
- JP navigation to IS event stages @liuyifan-eric
- update JP json for gacha pull @liuyifan-eric
- IS-10 (Yostar ongoing) slowly swipe to stage @liuyifan-eric
- JP oper ocr replace update @liuyifan-eric
- ad hoc fix for JP ocr of 阿 ア @liuyifan-eric
- try to fix JP IS StageEncounter Stuck @liuyifan-eric

#### YostarEN

- Supported Auto I.S. (Mizuki Roguelike) (#4936) @Pitiedwzr @RiichiTsumo
- Update tasks.json (EN, #4978) (#4985) @178619
- Optimize and fix some issues of Mizuki @peter1997546 @Pitiedwzr
- Fix threshold issue with `UsePrts` for YostarEN @MistEO
- Optimize the replacement of some operator names @Constrat

#### YostarKR

- Supported Auto I.S. (Mizuki Roguelike) (#4932) @178619
- Localization fix for ko-KR (#4930) @GyeRyak
- Update resources for KR @178619
- Update 1.3-EMULATOR_SUPPORTS.md (KR) @178619
- stuck at a specific event (KR, #4973) @178619
