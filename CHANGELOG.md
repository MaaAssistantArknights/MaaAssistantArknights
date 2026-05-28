## v6.11.0-beta.1

### 新增 | New

* 记录 MaaMacGui 子仓库更新 (#16870) @ColdSpellhere
* 为设置添加了搜索支持 (#16833) @H2O-MERO @ABA2396
* 新增 MarkdownDataHelper @ABA2396
* 对一键长草列表进行了交互优化，并添加了复制按钮 (#16733) @H2O-MERO @status102 @ABA2396
* 公招保留指定词条 (#16586) @ABA2396 @status102
* 理智作战支持设定目标材料最大库存 (#16487) @ABA2396 @status102
* 支持部分任务间通过导航栏切换 (#16869) @ABA2396
* TouchMode ToolTip 添加视频演示 (#16812) @ABA2396

### 改进 | Improved

* 优化 FightTaskStageResetModeConverter 与 RecruitTaskHoldTagsConverter 嵌套逻辑 @ABA2396
* NotificationImplWpf当Toast不可用时提示原因 (#16877) @status102 @status102 @momomochi987
* 统一使用重载的 GetValue 替换 Convert.To (#16866) @ABA2396
* 界园肉鸽弹窗Next关闭 @status102
* TaskQueue CheckBox与添加任务按钮对齐 @status102
* 优化TaskQueue选中任务时设置按钮表现以突出当前选中的选项 @status102

### 修复 | Fix

* 信用收支卡在商店里 @ABA2396
* 使用小房子转跳后无法切换基建设施 @ABA2396
* 嘗試修復繁中服界園肉鴿無法放棄探索 (#16887) @momomochi987
* Roi.height 越界 @status102
* 避免多次correct_rect后返回全图rect @status102
* 避免多次correct_rect后返回全图rect @status102
* LogWarn 等宏在 Release 下依旧输出scope导致额外间隔 @status102
* 界园见钱问柳事件选择逻辑 @Saratoga-Official
* filenum_ctrl SIGABRT (#16233) + PosixIO fork _exit + CI action SHA pin (#16502) @FireflySentinel
* 更新检查失败时优先尝试 api2 再使用缓存 (#16873) @glimmertouch
* 降低 RA4 和 RA15 二倍速识别阈值 (#16860) @ColdSpellhere
* 任务匹配进入onErrorNext时, cur_task_ptr错误置空 @status102
* 处理界园司岁台分队招募券 NEXT 关闭 (#16806) @ZiyinLin @status102

### 文档 | Docs

* changelog @ABA2396
* Auto Update Changelogs of v6.11.0-beta.1 (#16898) @github-actions[bot] @github-actions[bot] @ABA2396
* 补全 指定天数内的理智药 相关文档 @ABA2396
* skip_tags -> preserve_tags @ABA2396

### 其他 | Other

* Release v6.11.0-beta.1 (#16895) @ABA2396
* 删除多余标题 @ABA2396
* 调整 changelog skill @ABA2396
* 公告也改用 MarkdownDataHelper @ABA2396
* 添加任务复制失败的本地化翻译 @ABA2396
* 调整静态视频路径的绑定 @ABA2396
* 在 OnCustomToolTipChanged 中增加对 PART_Border 为空的防护，以避免在模板应用前出现空引用异常 @ABA2396
* 调整特定平台下的 IsNotificationAvailable 返回 @ABA2396
* 增加基建小房子切换兜底 @ABA2396
* 添加主任务右键效果切换提示 @ABA2396
* 加回右键菜单和单次运行 @ABA2396
* 繁中服「天想」主題 (#16893) @momomochi987
* MT 入口 @ABA2396
* Revert "fix: 任务匹配进入onErrorNext时, cur_task_ptr错误置空" @status102
