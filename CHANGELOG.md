## v6.10.0-beta.4

### 新增 | New

* 为自动公招的输出日志增加已公招次数 (#16651) @H2O-MERO @ABA2396
* 干员识别支持导出 Json/Markdown/CSV，优化导出按钮布局与交互 (#16635) @H2O-MERO
* 生息演算：重启锚点 @hguandl

### 改进 | Improved

* 干员识别本地化导出表头，添加类型化枚举 @ABA2396
* Revert "chore: 优化生息演算替换逻辑" @ABA2396
* 生息演算增加部署费用、木头数识别，提升运行速度与稳定度 @ABA2396

### 修复 | Fix

* 修复 FFT 路径 masked TM_CCOEFF_NORMED 精度损失导致的误匹配 (#16652) @Aliothmoon
* 修复日志输出停滞功能在未开启外部通知时无法生效 @ABA2396
* Various IS encounter Regex @Constrat
* 界园深入探索模板 (#16626) @ZiyinLin
* TimesChange event @Constrat

### 其他 | Other

* AddLog 缺失 param 介绍 @ABA2396
* 统一干员识别与仓库识别界面布局 @ABA2396
* 提高生息演算对话速度 @ABA2396
* 优化生息演算替换逻辑 @ABA2396
* 生息演算统一命名 @ABA2396
* 生息演算拖动地图增加重试 @ABA2396
* YostarKR UR stage navigation @HX3N
* gitignore for C# dev kit vsc deo @Constrat
