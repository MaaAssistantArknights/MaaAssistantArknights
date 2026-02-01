## v6.3.0-beta.6

### 新增 | New

* 干员识别支持显示精英化等级与潜能 @ABA2396
* 追加自定干员名称无效时的错误处理及本地化支持 (#15556) @yali-hzy @HX3N
* 界园肉鸽通宝数据更新 @SherkeyXD
* 界园肉鸽 dlc2 分队更新 @SherkeyXD
* YostarJP Dreamland and JieGarden themes @Constrat @Manicsteiner
* YostarKR Dreamland and JieGarden themes @Constrat @HX3N
* YostarEN Dreamland and JieGarden themes @Constrat
* 自动编队识别精英化及等级 (#15161) @status102 @Manicsteiner @Constrat @HX3N
* 新增注入弹窗不再提醒的勾选框，勾选后使用软件渲染 @ABA2396

### 改进 | Improved

* 关卡候选列表刷新 & 关卡选择下拉列表刷新 (#15562) @status102 @HX3N @Constrat @Manicsteiner
* 优化自动战斗界面布局 (#15512) @yali-hzy
* TaskQueue重命名与移除时显示任务序号 @status102
* use field @status102
* 提取任务启用状态 @status102
* 刷理智任务高级设置UI调整选项顺序 @status102
* cv::Mat const ref @status102

### 修复 | Fix

* 自动战斗自动编队补充自定干员名非法字段名调整 @status102
* ; @status102
* 禁用自动战斗-自动编队在无需检查技能等级时的快速选中, 以修复外服干员技能描述过长的错误选中 @status102
* 追加信赖时重复计算助战 @status102
* OR 关卡掉落界面关卡名识别问题 @ABA2396
* 刷理智任务运行时不允许添加关卡 @status102
* 自动战斗-自动编队 干员等级不足i18n未启用 @status102
* 保证通宝优先级未定义时不会加载崩溃 fallback到默认值 @SherkeyXD
* 选中 完成后动作 时添加新任务未能隐藏 完成后动作 设置UI @status102
* 切换刷理智时, 候选掉落物反复添加到下拉列表 @status102
* 信用收取任务访问好友及OF-1战斗最后执行时间未能保存 @status102
* YostarKR use ' ' in ocrReplace to preserve '\n' for InfrastTrainingTask @HX3N
* 日服酒神识别 @Saratoga-Official
* 未勾选访问好友时若未勾选每日仅访问一次, 则依旧会访问好友 @status102
* 手动输入关卡名时, 不移除过期关卡 @status102
* 基建计划选择计数增加后未刷新UI @status102
* 涤火杰西卡识别 @ABA2396

### 文档 | Docs

* 修正开发文档中的格式错误及笔误 (#15516) @yali-hzy
* Auto Update Changelogs of v6.3.0-beta.6 (#15551) @github-actions[bot] @github-actions[bot]

### 其他 | Other

* KR AnnouncementNotFinishedConfirm @HX3N
* 调整按钮不透明度 @ABA2396
* Revert "fix: 追加信赖时重复计算助战" @status102
* 调整正则 @ABA2396
* 修改`当前剿灭`的key, 以免与剿灭模式混淆 @status102
* EN minigame honeyfruit @Constrat
* JP MiniGame HoneyFruit @Manicsteiner
* KR MiniGame ConversationRoom and HoneyFruit @HX3N
* KR ReceptionOptionsRequireInfrast @HX3N
* KR CreditFightWhenOF-1Warning @HX3N
* manual data for txwy @Constrat
* optimize templates @Constrat
* Revert "fix: Yostaren IS invest template update" @Constrat
* 补全缺少的翻译 @ABA2396
