## v6.8.0

### 新增 | New

* updater 增加移除和移动文件列表打印 @ABA2396
* updater 日志增加大小上限 @ABA2396
* 非法Enum值将使用属性设置的默认值作为替代 (#16138) @status102
* 线索交流时先移除所有放置的线索 (#16054) @travellerse
* 启动时判断版本是否一致 @ABA2396
* 新增吃指定天数过期的理智药 (#13849) @soundofautumn @status102
* 添加单元测试框架和验证角色分配算法的测试用例 (#16245) @lhhxxxxx
* AVD截图增强的售后（文档、CI变更等） (#16031) @satgo1546 @Rbqwow @MistEO
* V0.2 新构建跨平台前端界面MAAUnified，合并代码请求 (#16048) @Halo5082 @MistEO
* 配置存储支持条件优化 (#15850) @status102
* 界园肉鸽月度小队和深入调查 (#16271) @SherkeyXD

### 改进 | Improved

* 自动编队预编队后检查选中情况 @status102
* 优化提示元素展示效果 @ABA2396
* 作业版本号需求允许省略patch @status102
* 使用SemaphoreSlim替换Lock @status102
* 分辨率不支持时打印当前分辨率 @ABA2396
* 移除 NotificationImplWinRT 中二次进入 UI 线程 (#16196) @EzraRT
* 提升 Algorithm.hpp 算法性能及其鲁棒性 (#16235) @lhhxxxxx
* 干员数据重构, 支持跨职业重名干员 (#16084) @status102
* 外部更新使用分离的 updater (#16326) @ABA2396
* 外部更新不再读配置 @ABA2396
* 简化更新代码 @ABA2396
* 涉及 dll 的更新使用外部更新 @ABA2396
* 重构更新逻辑，允许拖入指定名称的压缩包进行更新 (#16308) @ABA2396

### 修复 | Fix

* 修复截图延迟极低时，可能会随机出现，更换产物/订单失败的问题 (#16330) @Roland125
* local-install 找不到 Artifact @ABA2396
* 内测版与其他类型版本对比时 removelist 会错误添加所有目录 @ABA2396
* 临时修复rect完全超限时的超限返回值 @status102
* KR OSChapterToOS OCR @Daydreamer114
* yj 怎么还暗改老主题 @ABA2396
* 自动战斗鼠标长按分页时, 可能会反复触发切换 @status102
* EN IS6 DLC1 regexes @Constrat
* 在定时任务触发时, 固定等待UpdateStageList @status102
* 修复LinkStart期间UpdateStageList内进入SetFightParams导致死锁 @status102
* prts.plus改为zoot.plus @status102
* updater utf8 解析 @ABA2396
* warnings @ABA2396
* 描述误导 @status102
* 基建开启设施无法保存 @ABA2396
* index越界 @status102
* macOS PlayTools/SCK 几处小修正 (#16276) @FireflySentinel
* 干员库存识别返回错误id @status102
* baseList 无法编译的问题 (#16293) @Yi-Zh17

### 文档 | Docs

* Auto Update Changelogs of v6.8.0-beta.1 (#16285) @github-actions[bot] @github-actions[bot] @Constrat
* i18n for install.md (#16214) @JasonHuang79 @HX3N @Manicsteiner @momomochi987 @Constrat
* Auto Update Changelogs of v6.8.0-beta.2 (#16324) @github-actions[bot] @github-actions[bot] @ABA2396
* add FAQ guidance for Windows Defender false positives (#16145) @Leo91314

### 其他 | Other

* 简化公告日志记录 @ABA2396
* tool 更新 @ABA2396
* updater 额外保留 cache 文件夹 @ABA2396
* 更新保全作业 @Saratoga-Official
* Release v6.8.0-beta.2 (#16318) @ABA2396
* Release v6.8.0-beta.1 (#16284) @Constrat
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
* 注释推错了 @status102
* 调整手动更新方法描述 @ABA2396
* KR UseExpireMedicineForActivity @HX3N
* 繁中服宿舍截圖 & 部分 OCR 內容 (#16298) @momomochi987
