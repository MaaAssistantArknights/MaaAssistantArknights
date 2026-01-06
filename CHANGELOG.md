## v6.2.0-beta.2

### 新增 | New

* 支持日志样式切换 @ABA2396
* 基建设施缩略图 @ABA2396
* addlog 允许只更新图片不添加内容 @ABA2396

### 改进 | Improved

* 优化缩略图逻辑 @ABA2396
* 重命名以符合文件结构 @ABA2396
* 优化 PropertyDependsOnHelper 实现 @ABA2396
* 将 PropertyDependsOnViewModel 改为静态工具类 @ABA2396
* 调整缩略图 ToolTip 延迟 @ABA2396
* 刷理智任务指定次数与代理倍率冲突提醒 (#15233) @status102

### 修复 | Fix

* fix SA1518 warnings @Constrat
* 肉鸽未填写开局干员时借助战强制为 false @ABA2396
* 隐秘战线结局识别 @ABA2396
* 隐秘战线识别到多余字符时无法进入对应事件 @ABA2396
* waydroid rawbync screencap 2>/dev/null (#15196) @commondservice
* 避免其他locale下，掉落次数误认数字字符 (#15306) @aflyhorse
* 无法通过删除自动战斗输入框内容清除当前作业，无法通过在输入框输入神秘代码直接开始战斗 @ABA2396

### 文档 | Docs

* update vsc ext docs for quick ocr (#15298) @neko-para @Constrat

### 其他 | Other

* EN for `FightTimesMayNotExhausted` @Constrat
* 调整 MaxNumberOfLogThumbnails 作用域，调整默认数量 @ABA2396
* YoStarJP SN device ocr (#15310) @cheriu
