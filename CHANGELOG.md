## v6.8.0-beta.1

## OS 1101 whatever event

Quick beta release to implement the Karlan Trade R&D minigame for all Global servers (EN, KR, JP)
Stable will come soon™ uuh.

Also, implementation of reception all clue fast selection.

Enjoy.


### 新增 | New

* 非法Enum值将使用属性设置的默认值作为替代 (#16138) @status102

### 改进 | Improved

* 自动编队预编队后检查选中情况 @status102
* 优化提示元素展示效果 @ABA2396
* 作业版本号需求允许省略patch @status102
* 使用SemaphoreSlim替换Lock @status102
* 分辨率不支持时打印当前分辨率 @ABA2396
* 移除 NotificationImplWinRT 中二次进入 UI 线程 (#16196) @EzraRT
* 提升 Algorithm.hpp 算法性能及其鲁棒性 (#16235) @lhhxxxxx
* 干员数据重构, 支持跨职业重名干员 (#16084) @status102

### 修复 | Fix

* KR OSChapterToOS OCR @Daydreamer114
* yj 怎么还暗改老主题 @ABA2396
* 自动战斗鼠标长按分页时, 可能会反复触发切换 @status102
* EN IS6 DLC1 regexes @Constrat
* 在定时任务触发时, 固定等待UpdateStageList @status102
* 修复LinkStart期间UpdateStageList内进入SetFightParams导致死锁 @status102
* prts.plus改为zoot.plus @status102

### 文档 | Docs

* i18n for install.md (#16214) @JasonHuang79 @HX3N @Manicsteiner @momomochi987 @Constrat

### 其他 | Other

* implement Quickly Place Clues for Global (#14966) @Constrat
* EN @Constrat
* EN OS minigame (#16283) @Constrat
* YostarJP OS stages and more ocr @Manicsteiner
* YostarKR OS ocr and minigame (#16268) @HX3N
* YostarJP OS ocr and minigame (#16267) @Manicsteiner
* 肉鸽添加怒潮凛冬招募逻辑 (#16217) @Reverse0xCC
* 添加贝洛内、怒潮凛冬基建技能数值 (#16260) @drway
* git ignore 添加 claude code @Daydreamer114
* Revert "ci: issue bot skills 添加 at 符号检测 (#16239)" @MistEO
* Revert "fix: prts.plus改为zoot.plus" @status102
* AnnihilationName @status102
* pc 端禁用完成后退出模拟器 @ABA2396
* 繁中服「次生方案」小活動 (#16216) @momomochi987
* 重新将natvis添加到MaaCore (#16133) @status102
