## v4.19.0-beta.2

### 新增

- 支持连接失败重启 ADB @ABA2396
- 关卡选择为null时变为当前/上次 @ABA2396

### 改进

- 优化UpdateStageList @ABA2396

### 修复

- 修复EN服肉鸽不放预备干员，不选非默认分队，添加复用安全屋文本 (#4972) @Pitiedwzr
   - 为异格干员添加正则 @Pitiedwzr
   - 修复EN服肉鸽不放预备干员，不选非默认分队，添加复用安全屋文本 @Pitiedwzr
- Fix threshold issue with `UsePrts` for YostarEN @MistEO
- 修复切换日服后再切回后OCR配置不清空的问题 @MistEO
- 修复资源更新CI取消条件 @MistEO
- fix size check of android screencap @horror-proton
- 修复自动翻译工具 python3.11无法安装依赖的问题 & 重构项目结构，方便后续打包，并调整部分模块 (#4951) @UniMars
   - 修复python3.11无法安装依赖的问题 & 调整部分模块 @UniMars
   - 重构项目结构，方便后续打包 @UniMars
- 解决EN服水月肉鸽 不期而遇卡死的问题 (#4948) @peter1997546
   - 解决EN服水月肉鸽 不期而遇卡死的问题 @peter1997546
- 修复EN服肉鸽不抓预备干员&添加百嘉鸿雪识别容错 (#4945) @Pitiedwzr
   - Add regular expressions 添加正则 @Pitiedwzr
   - 修复EN服肉鸽鸿雪，百嘉，预备术士识别 @Pitiedwzr
   - 修复EN服肉鸽不抓预备干员&添加百嘉鸿雪识别容错 @Pitiedwzr
   - EN服修复肉鸽不抓取预备干员 @Pitiedwzr
- try to fix JP IS StageEncounter Stuck @liuyifan-eric
- res-update-game.yml @MistEO

### 其他

- i18n: Translations update from MAA Weblate (#4971) @weblate
   - Translated using Weblate (Chinese (Traditional)) @weblate
- 现在 release-mirrors 会自行调用 update_version_api @AnnAngela
- Update tasks.json for KR @178619
- Update resource/tasks.json for KR @178619
- 统一改成半角冒号 @ABA2396
- Update resources for KR @178619
- update overseas json/template tool @liuyifan-eric
- 修改自定义输入关卡判断条件 @ABA2396
- 统一 ADB 大小写显示 @ABA2396
- Update 1.3-EMULATOR_SUPPORTS.md (KR) @178619
