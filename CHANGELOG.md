## v5.14.0-beta.4

### 新增 | New

* SideStory「挽歌燃烧殆尽」导航 (#12094) @SherkeyXD
* YostarEN DT navigation @Constrat
* 萨卡兹肉鸽深入调查实装 (#12098) @BxFS
* parse release note for resource updater (#12092) @MistEO @ABA2396 @Constrat

### 改进 | Improved

* 肉鸽投资ocr封装 @status102
* 手动更新下活动名称 @ABA2396
* 填写完cdk后自动检查一次更新 @ABA2396
* 企鹅物流上报失败-不支持的关卡，输出降级为警告，移除提示中的更新提醒 @status102
* 自动战斗在作业地图不支持时，自动检测资源更新 @status102
* 调整干员识别及仓库识别开始流程 @status102
* 修改初次设置引导中的项目，移除账号切换相关 @status102
* 移除无用的彩蛋控件 @status102
* 基建会客室信息板无新访客时，不再进入领取页 @status102

### 修复 | Fix

* wpf自动战斗-追加自定干员失效 @status102
* wpf领取奖励任务序列化错误 @status102
* 修改彩蛋gif默认路径为string.Empty @status102
* sarkaz MS count 1->8 (#12085) @BxFS
* 修复截图速度过快导致不能切换贸易站订单的问题 (#12090) @Roland125
* 基建设置UI绑定失效 @status102
* burden/toil tasks threshold 0.9 -> 0.8 (#12087) @BxFS @Constrat
* wpf开始唤醒任务在未选择官服/B服时，不再提供账号切换输入 @status102
* wpf自动战斗-自动编队追加自定干员意外激活 @status102
* log rotate (#12025) @status102 @pre-commit-ci[bot]
* remove duplicate strings (#12065) @Constrat
* Sarkaz MS unable to progress from post StartExplore (#12039) @BxFS @HX3N @Constrat
* stuck on StageEnterBattleAgain while in team selection screen (#12059) @BxFS
* smtp 格式 @ABA2396
* 「滋味」主题在有多个 badge 时无法识别基建入口 (#12057) @Alan-Charred
* 修复公招的几个问题 (#12056) @Roland125 @pre-commit-ci[bot]
* 修复不自动招募4星干员时，会执行空招募的问题 (#12054) @Roland125 @pre-commit-ci[bot]
* 自动战斗追加自定义干员序列化 @status102

### 文档 | Docs

* 枯朽祭坛 维什戴尔 逻各斯 OCR辨識 (#12096) @XuQingTW @pre-commit-ci[bot]
* 更新开发前须知 (#12043) @Rbqwow @pre-commit-ci[bot]

### 其他 | Other

* wpf基建任务序列化默认值补全 @status102
* 删除多余判断 @ABA2396
* YostarKR DT navigation (#12105) @HX3N
* wpf信用任务序列化 @status102
* YostarJP DT テラ飯 preload (#12101) @Manicsteiner
* 提取 ResourceReload 方法 @ABA2396
* wpf领取奖励任务序列化 @status102
* 倒计时添加日志 @ABA2396
* 添加 spid @ABA2396
* 调整参数名 @ABA2396
* Wpf基建任务序列化调整 (#12053) @status102
* Wpf唤醒任务序列化 @status102
* Wpf关闭明日方舟任务序列化 @status102
* 外部通知邮件 html 支持颜色输出 @ABA2396
* wpf生息演算任务序列化 @status102
* smtp 通知使用 html @ABA2396
* add sami investigation equipment text and temporary retreat template (#12046) @momomochi987 @pre-commit-ci[bot] @HX3N
* 猫猫改为全局 @ABA2396
