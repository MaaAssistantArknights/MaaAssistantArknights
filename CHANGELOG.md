## v6.10.0-beta.4

### 新增 | New

* 为自动公招的输出日志增加已公招次数 (#16651) @H2O-MERO @ABA2396
* 干员识别支持导出 Json/Markdown/CSV，优化导出按钮布局与交互 (#16635) @H2O-MERO
* macOS 生息演算：重启锚点 @hguandl

### 改进 | Improved

* 干员识别本地化导出表头，添加类型化枚举 @ABA2396
* 生息演算增加部署费用、木头数识别，提升运行速度与稳定度 @ABA2396

### 修复 | Fix

* 修复 FFT 路径 masked TM_CCOEFF_NORMED 精度损失导致的误匹配 (#16652) @Aliothmoon
* 修复日志输出停滞功能在未开启外部通知时无法生效 @ABA2396
* 界园深入探索模板 (#16626) @ZiyinLin
* Various IS encounter Regex @Constrat
* TimesChange event @Constrat

### 其他 | Other

* AddLog 缺失 param 介绍 @ABA2396
* 统一干员识别与仓库识别界面布局 @ABA2396
* 提高生息演算对话速度 @ABA2396
* 生息演算统一命名 @ABA2396
* 生息演算拖动地图增加重试 @ABA2396
* YostarKR UR stage navigation @HX3N
* gitignore for C# dev kit vsc deo @Constrat

## v6.10.0-beta.3

### 新增 | New

* 点击成就横幅跳转至成就设置，自动打开成就列表并筛选对应成就 (#16537) @H2O-MERO @ABA2396
* 任务日志输出停滞时发送通知, 替换 任务超时通知 (#16511) @H2O-MERO

### 其他 | Other

* 放宽对 RA-1 关卡名的检查 @ABA2396
* 添加拆除设施的描述 @ABA2396
* 删除辅助建设模式的描述 @ABA2396

## v6.10.0-beta.2

### 新增 | New

* 生息演算支持不同分辨率 @ABA2396

### 改进 | Improved

* 减少中间状态 @ABA2396

### 其他 | Other

* 欠费下小猫 @ABA2396
* 快速下小猫 @ABA2396
* 加快下一轮循环 @ABA2396
* 提升对话速度 @ABA2396
* 添加描述 @ABA2396

## v6.10.0-beta.1

### 新增 | New

* 初步支持 `生息演算：重启锚点` 新手刷代币与科技点策略 @ABA2396
* 17 章导航 @ABA2396
* 仓库识别支持导出 Markdown/CSV，优化导出按钮布局与交互 (#16543) @H2O-MERO @ABA2396
* support native android (#16179) @Aliothmoon

### 改进 | Improved

* 优化 masked TM_CCOEFF_NORMED 匹配性能 (#16593) @Aliothmoon
* 调整 小工具-便捷任务 布局，调整日志输出 @ABA2396
* 小游戏界面重构，添加分类并优化选择逻辑，添加日志显示 (#16499) @SherkeyXD @momomochi987
* 干员识别与仓库识别支持虚拟化，大幅提高首次加载速度 (#16486) @ABA2396

### 修复 | Fix

* 15 章之后的难度切换 @ABA2396
* 理智药过期天数识别失败取消确认逻辑未生效 @status102
* 临期理智药天数缺省值 @status102
* 刷理智-理智药过期参数迁移输出 Warning 中参数名错误 @status102
* 繼續調整繁中服部分幹員名稱 OCR (#16600) @momomochi987

### 其他 | Other

* 便捷功能提示文本自适应大小 @ABA2396
* 避免不必要的 new @status102
* RunningState 更新 (#16585) @status102
* OCRer DEBUG 下绘制匹配结果 @status102
* 统一符号 @ABA2396
* 调整干员识别提示换行 @ABA2396
* 调整图标阴影 @ABA2396
* 采用 System.Windows 的剪贴板 @ABA2396
* 调整输出格式 @ABA2396
* 赛博道长 @ABA2396
