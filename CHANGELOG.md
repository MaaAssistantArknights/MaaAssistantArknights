## v6.3.0-beta.8

### 新增 | New

* 芯片本支持显示库存数量 @ABA2396
* 发布时minify resource文件下的json (#15567) @soundofautumn
* WpfGui清空缓存按钮 (#15582) @soundofautumn @Constrat @HX3N
* 界园肉鸽指定种子开局 (#15588) @status102 @HX3N @Manicsteiner

### 改进 | Improved

* 自动战斗不支持技能重置说明中, 干员名遵循干员名语言设置 @status102
* 移除过时的参数兼容 @status102
* 追加自定干员允许不切换技能 @status102
* 清空缓存移动到界面设置 @status102
* 自动战斗作业列表使用相对路径代替绝对路径 @status102
* TaskQueue 任务开始&完成显示修改后的任务名 @status102

### 修复 | Fix

* NumberOcrReplace移除`|`和`/` (#15625) @status102
* EN IS ShoppingConfirm text font change. @Constrat
* 过期关卡重置模式补充自动迁移 @status102
* UI绑定 @status102
* 无效关卡未能显示i18n文本 @status102
* 修复生息演算商店无法正常购买皮肤的问题 (#15585) @drway
* 先兼容旧作业中不合理的技能选择 @status102
* 干员组干员未解析精英化及等级属性 @status102
* 肉鸽烧水分队切换界面后错误重置 @status102
* 初始化 StartEnabled 属性为 true (#15596) @yali-hzy
* 自动战斗切换活动类型未清空解析缓存 @status102
* 刷理智取消勾选`下拉框中隐藏非当日关卡`后关卡选择框不显示内容 @status102
* 退出MAA时重置变量不再刷新UI @status102
* egg 炸了 @ABA2396

### 文档 | Docs

* 自动战斗作业文档干员技能值范围补上0 @status102

### 其他 | Other

* 自动战斗-自动编队干员不支持技能说明 (#15609) @status102 @Constrat @HX3N @Manicsteiner
* YostarJP AveMujica preload @Manicsteiner
* YostarKR preload AveMujica @HX3N
* preload AveMujica EN @Constrat
* NumberOcrReplace 新增规则 (#14186) @Manicsteiner
* devcontainer适配CMakePresets.json (#15606) @lucienshawls
* Revert "ci: temp fix for txwy gamedat" @Constrat
* 优化 emoji @ABA2396
