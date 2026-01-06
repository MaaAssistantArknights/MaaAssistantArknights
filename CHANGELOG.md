## v6.2.0-beta.2

### 新增 | New

* 支持日志样式切换 @ABA2396
* 新增基建设施缩略图展示，并支持仅更新图片而不追加日志内容 @ABA2396

### 改进 | Improved

* 优化基建设施缩略图生成与管理逻辑，调整 ToolTip 延迟及默认数量设置 @ABA2396
* 重构 PropertyDependsOn 相关实现，将 ViewModel 调整为静态工具类以提升可维护性 @ABA2396
* 调整部分命名以更符合当前文件与目录结构 @ABA2396
* 刷理智任务中，指定次数与代理倍率冲突时增加明确提醒 (#15233) @status102

### 修复 | Fix

* 修复肉鸽模式未填写开局干员时，借助战被错误强制为 false 的问题 @ABA2396
* 修复隐秘战线结局识别异常及识别到多余字符时无法进入对应事件的问题 @ABA2396
* 修复 Waydroid 环境下 screencap 原始同步调用的异常情况 (#15196) @commondservice
* 修复在非本地 locale 环境下，掉落次数被误识别为数字字符的问题 (#15306) @aflyhorse
* 修复无法通过清空自动战斗输入框来清除当前作业，以及通过特殊输入直接开始战斗的问题 @ABA2396
* 修复部分代码中的 SA1518 警告 @Constrat

### 文档 | Docs

* 更新 VS Code 扩展中 Quick OCR 相关文档说明 (#15298) @neko-para @Constrat

### 其他 | Other

* 补充 `FightTimesMayNotExhausted` 的英文翻译 @Constrat
* 优化 YoStar JP（SN）设备的 OCR 识别支持 (#15310) @cheriu
