## v6.7.2-beta.1

### 新增 | New

* 非法Enum值将使用属性设置的默认值作为替代 (#16138) @status102

### 改进 | Improved

* 优化提示元素展示效果 @ABA2396
* 作业版本号需求允许省略patch @status102
* 使用SemaphoreSlim替换Lock @status102
* 分辨率不支持时打印当前分辨率 @ABA2396
* 移除 NotificationImplWinRT 中二次进入 UI 线程 (#16196) @EzraRT
* 提升 Algorithm.hpp 算法性能及其鲁棒性 (#16235) @lhhxxxxx
* 干员数据重构, 支持跨职业重名干员 (#16084) @status102

### 修复 | Fix

* 自动战斗鼠标长按分页时, 可能会反复触发切换 @status102
* EN IS6 DLC1 regexes @Constrat
* 在定时任务触发时, 固定等待UpdateStageList @status102
* 修复LinkStart期间UpdateStageList内进入SetFightParams导致死锁 @status102
* prts.plus改为zoot.plus @status102

### 文档 | Docs

* i18n for install.md (#16214) @JasonHuang79 @momomochi987 @Constrat

### 其他 | Other

* git ignore 添加 claude code @Daydreamer114
* Revert "ci: issue bot skills 添加 at 符号检测 (#16239)" @MistEO
* Revert "fix: prts.plus改为zoot.plus" @status102
* AnnihilationName @status102
* pc 端禁用完成后退出模拟器 @ABA2396
* 繁中服「次生方案」小活動 (#16216) @momomochi987
* 重新将natvis添加到MaaCore (#16133) @status102
