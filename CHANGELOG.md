## v6.3.0-beta.5

### 新增 | New

* 新Config 加载时移除旧Config中不存在的配置 @status102

### 改进 | Improved

* 信用战斗检查启用的首个刷理智是否选择 当前/上次 @status102
* 优化干员识别、仓库识别显示 @ABA2396
* 存在crash.log时, Wpf尝试获取dumps文件 (#15432) @status102
* 配置迁移检查简化 @status102
* 移除多余的新Config配置删除 @status102
* 新Config 字符序列化 @status102
* 剿灭卡使用到上限时不报错停止 @ABA2396
* 剿灭关卡通过 ends_with 判断 @ABA2396
* FightTask 以剿灭为目标关卡时, 在终端界面找不到周剿灭获取进度图标不再以 False 退出 @status102

### 修复 | Fix

* 剩余理智启用状态迁移后未能从旧配置移除 @status102
* 剩余理智关卡 关卡选择 迁移后错误使用 @status102
* 萨米肉鸽刷开局功能异常 @ABA2396
* 基建计划选中 Index 超出范围 @status102
* SEH 错误终止运行 @status102
* 自动编队识别技能等级匹配失败 @status102
* 信用收取后刷 OF-1 不会在后续刷理智任务选中 `当前/上次` 时禁用 @status102
* 启动MAA后重新读取基建计划 @status102
* 默认配置创建修复 @status102
* 剩余理智勾选且设定关卡为空时, 迁移后直接禁用剩余理智 @status102
* 配置迁移后切换回原配置 @status102
* `源石恢复` -> `使用源石` 遗漏 @status102
* 刷理智使用源石 CheckBox 勾选后不生效 @status102

### 文档 | Docs

* 繁中文件大更新 (#15480) @momomochi987

### 其他 | Other

* 增加借助战 OF-1 在后续刷理智选择 `当前/上次` 导致禁用时的输出 (#15478) @status102 @ABA2396 @momomochi987
* 繁中服不上报企鹅物流 @ABA2396
* 基建找不到对应时间的基建计划 (#15468) @status102 @momomochi987
* Revert "perf: 信用战斗检查启用的首个刷理智是否选择 当前/上次" @ABA2396
* 移除不再使用的 VirtualizingWrapPanel 与 NoAutomationDataGrid @ABA2396
* Revert "perf: 剿灭作战找不到终端内的图标直接结束" @status102
