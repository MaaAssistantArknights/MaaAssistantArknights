## v4.19.0-beta.2

### 改进

- 支持连接失败重启 ADB @ABA2396
- 优化关卡列表相关逻辑 @ABA2396

### 修复

- 修复截图方式判断错误导致的操作慢问题 @horror-proton @MistEO
- 修复切换日服后再切回后OCR配置不清空的问题 @MistEO

### 其他

- 修复自动翻译工具 python3.11无法安装依赖的问题 & 重构项目结构，方便后续打包，并调整部分模块 (#4951) @UniMars
- 优化调整一些 CI @AnnAngela @MistEO
- 翻译优化 @ABA2396 @Alisa

### For overseas

#### Common

- i18n: Translations update from MAA Weblate (#4971) @weblate
- update overseas json/template tool @liuyifan-eric

#### YostarJP

- try to fix JP IS StageEncounter Stuck @liuyifan-eric

#### YostarEN

- Optimize and fix some issues of Mizuki @peter1997546 @Pitiedwzr
- Fix threshold issue with `UsePrts` for YostarEN @MistEO

#### YostarKR

- Update resources for KR @178619
- Update 1.3-EMULATOR_SUPPORTS.md (KR) @178619
