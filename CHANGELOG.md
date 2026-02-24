## v6.3.3

### 新增 | New

* 适配maafw新Win32触控方式 (#15755) @MistEO
* PC 提示新增分辨率要求 @ABA2396
* PC端忽略启动客户端设置 @SherkeyXD

### 改进 | Improved

* 被禁用的关卡错误进入关卡名二次识别 @status102
* 长草任务序列化接口重构拆分 (#15773) @status102
* 开始唤醒支持战斗中识别 @status102

### 修复 | Fix

* 刷理智关卡名识别错误替换 @status102
* EN IS6 tongbao wrong regex @Constrat
* 超出范围的Rect使用{0, 0, 0, 0}代替原样返回, x, y为负值时width和height改正错误 (#15695) @status102
* IS6 EN yostar decided to finally fix the tongbaos releasing an official translation + resource updater copper-ability @Constrat
* 修复自定义基建计划在迁移时基建计划选择转换错误 @status102
* EN Operator OCR roi fixes + skadi alter simplification @Constrat
* IS6 EN encounter regex post ROI increase @Constrat
* touch screen support for version updated page (#15747) @undefined-moe
* invert dice refresh with invest system priority in IS3 (#15740) @Constrat
* 小工具=-公招识别3~5星设置时间未遵循设置 @status102

### 其他 | Other

* YostarKR SSS#9 SSSBuffChoose @HX3N
