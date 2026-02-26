## v6.3.6

### 改进 | Improved

* Wpf 增加未找到 CrashDumps 文件夹 Log, correct_rect 增加 rect.empty() log @status102

### 修复 | Fix

* 刷理智-掉落识别rect超限导致崩溃 @status102
* 修复自动战斗中待部署区干员匹配标志rect后处理越界 @status102
* 移除多余检查 @status102

### 其他 | Other

* issue模板添加闪退上传dmp提示 @Saratoga-Official
* Revert "fix: 超出范围的Rect使用{0, 0, 0, 0}代替原样返回, x, y为负值时width和height改正错误 (#15695)" @status102
