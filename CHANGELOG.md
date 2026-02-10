## v6.3.0

### 新增 | New

* 繁中服 界園肉鴿 初步適配 (#15605) @momomochi987
* SideStory「辞岁行」导航 @SherkeyXD
* 日志中额外记录 TaskChain 与 taskId ~~免得有人把 Fight 改成开始唤醒~~ @ABA2396
* 支持 PC 端明日方舟 (#15407) @MistEO @ABA2396
* 设置指引增加右键重命名/删除提示 @ABA2396
* 再次微调 PC 端文案 @MistEO
* PC 端说明文案调整 @MistEO
* 新Config 加载时移除旧Config中不存在的配置 @status102
* 干员识别支持显示精英化等级与潜能 @ABA2396
* 追加自定干员名称无效时的错误处理及本地化支持 (#15556) @yali-hzy @HX3N
* 界园肉鸽通宝数据更新 @SherkeyXD
* 界园肉鸽 dlc2 分队更新 @SherkeyXD
* YostarJP Dreamland and JieGarden themes @Constrat @Manicsteiner
* YostarKR Dreamland and JieGarden themes @Constrat @HX3N
* YostarEN Dreamland and JieGarden themes @Constrat
* 自动编队识别精英化及等级 (#15161) @status102 @Manicsteiner @Constrat @HX3N
* 新增注入弹窗不再提醒的勾选框，勾选后使用软件渲染 @ABA2396
* 界园肉鸽可选难度提高至18 @SherkeyXD
* 芯片本支持显示库存数量 @ABA2396
* 发布时minify resource文件下的json (#15567) @soundofautumn
* WpfGui清空缓存按钮 (#15582) @soundofautumn @Constrat @HX3N @momomochi987
* 界园肉鸽指定种子开局 (#15588) @status102 @HX3N @Manicsteiner

### 改进 | Improved

* 过期关卡使用删除线表示 (#15657) @status102 @ABA2396
* Wpf一键长草任务配置重构 (#15385) @status102 @ABA2396 @momomochi987 @HX3N
* 优化任务设置按钮悬浮提示 @ABA2396
* 开始唤醒 多任务共用参数提示 @status102
* 开始唤醒任务未设置账号切换时, 禁用手动切换按钮 @status102
* 优化设置右键菜单布局 @ABA2396
* 更换Config迁移检查 @status102
* 剿灭作战找不到终端内的图标直接结束 @status102
* Config 创建和切换检查 @status102
* 迁移前清空 gui.new 中的所有配置 @status102
* 调整位置 @status102
* 自动战斗掉线重连、自动肉鸽在战斗结束前延迟 ｢停止｣ 动作 添加多任务共用提示 @ABA2396
* 信用战斗检查启用的首个刷理智是否选择 当前/上次 @status102
* 优化干员识别、仓库识别显示 @ABA2396
* 存在crash.log时, Wpf尝试获取dumps文件 (#15432) @status102
* 配置迁移检查简化 @status102
* 移除多余的新Config配置删除 @status102
* 新Config 字符序列化 @status102
* 剿灭卡使用到上限时不报错停止 @ABA2396
* 剿灭关卡通过 ends_with 判断 @ABA2396
* FightTask 以剿灭为目标关卡时, 在终端界面找不到周剿灭获取进度图标不再以 False 退出 @status102
* 关卡候选列表刷新 & 关卡选择下拉列表刷新 (#15562) @status102 @HX3N @Constrat @Manicsteiner @momomochi987
* 优化自动战斗界面布局 (#15512) @yali-hzy
* TaskQueue重命名与移除时显示任务序号 @status102
* use field @status102
* 提取任务启用状态 @status102
* 刷理智任务高级设置UI调整选项顺序 @status102
* cv::Mat const ref @status102
* 刷理智高级设置使用hc:InfoElement.Title显示设置项名 @status102
* 备选关卡读取后检查 @status102
* 自动战斗自动编队检查`干员等级&精英化`及`技能等级`拆分 @status102
* 自动战斗不支持技能重置说明中, 干员名遵循干员名语言设置 @status102
* 移除过时的参数兼容 @status102
* 追加自定干员允许不切换技能 @status102
* 清空缓存移动到界面设置 @status102
* 自动战斗作业列表使用相对路径代替绝对路径 @status102
* TaskQueue 任务开始&完成显示修改后的任务名 @status102

### 修复 | Fix

* 移除单字干员ocr替换中的+*？避免误判 @Saratoga-Official
* LoadApiCache 路径拼接错误 @ABA2396
* 未开放关卡不重置 (#15647) @Daydreamer114
* YostarKR Roguelike@ChooseOperConfirm @HX3N
* 外服主线导航 @ABA2396
* 主线导航出错 @SherkeyXD
* EN Greyy Alter regex @Constrat
* EN IS6 encounter @Constrat
* 移除不必要的sleep @status102
* 移除不必要的sleep @status102
* 有猪不等动画 @ABA2396
* Yostaren IS invest template update @Constrat
* JP AT minigame confirm (#15427) @Manicsteiner
* jp JieGardenStrategyChange @Saratoga-Official
* 日服見字祠识别 @Saratoga-Official
* 粘贴作业集代码后下方的链接未重置为作业站链接 @ABA2396
* 在赠送线索时弹出上次线索交流结束的提示时无法返回 @ABA2396
* 调整判空逻辑 @ABA2396
* 有猪乱写 OF-1 和 当前/上次 的条件 @ABA2396
* OF-1 跳过条件又有猪改错了 @ABA2396
* config task 默认值错误 @ABA2396
* 右键跳过一次 @status102
* 新Config神秘小bug丢失Default @status102
* config最后一个时移除按钮禁用 @status102
* 删除配置的时候不会删除 .new @ABA2396
* 公招加速券 @status102
* 公招加速券 @status102
* 启动客户端绑定失效 @status102
* 关卡列表显示不刷新 @status102
* 启动客户端绑定 @status102
* 收取信用检查 @status102
* 启动MAA时若没有任何任务, 则追加一套默认任务 @status102
* YostarEN refresh node template @Constrat
* rename @status102
* 配置迁移后移除gui.new中多余的config @status102
* 任务序列化Catch @status102
* 基建计划转换期增加检查 @status102
* add MaaWin32ControlUnit to nightly build (#15447) @Manicsteiner
* 多配置用户在删除 Default 配置时迁移异常 @ABA2396
* EX 关符合时 1 被识别为 | @ABA2396
* 修复移动已打开设置的任务后，当前的设置面板无法继续修改的问题 @ABA2396
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
* 开始干员识别前重置潜能状态 @ABA2396
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
* 切换刷理智任务时读取到错误的关卡列表 @status102
* 修复任务出错日志可能晚于任务完成日志显示的问题 @ABA2396
* 自动编队干员技能描述过长时点击位置错误 @status102
* 干员等级跨精英化时判断出错修复 @status102
* EN IS TradeInvest templates text font change. @Constrat
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
* 路径拼接 @ABA2396

### 文档 | Docs

* Auto Update Changelogs of v6.3.0-beta.1 (#15438) @github-actions[bot] @github-actions[bot] @ABA2396
* 集成文档统一格式，同时显示 field-group 和示例代码 (#15409) @ABA2396 @Manicsteiner @Constrat @momomochi987
* 有猪漏改了 @ABA2396
* Update CHANGELOG for v6.3.0-beta.2 fixes @ABA2396
* Update CHANGELOG for v6.3.0-beta.2 @ABA2396
* en Coupon @ABA2396
* 有猪 @ABA2396
* changelog @ABA2396
* Update CHANGELOG for version 6.3.0-beta.4 @ABA2396
* changelog for PC arknights @MistEO
* Update CHANGELOG for v6.3.0-beta.5 @ABA2396
* 繁中文件大更新 (#15480) @momomochi987
* 修正开发文档中的格式错误及笔误 (#15516) @yali-hzy
* Auto Update Changelogs of v6.3.0-beta.6 (#15551) @github-actions[bot] @github-actions[bot]
* cahngelog @status102
* changelog @status102
* Auto Update Changelogs of v6.3.0-beta.7 (#15576) @github-actions[bot] @github-actions[bot] @status102
* Auto Update Changelogs of v6.3.0-beta.6 (#15571) @github-actions[bot] @github-actions[bot] @ABA2396
* format changelog @ABA2396
* changelog @ABA2396
* Auto Update Changelogs of v6.3.0-beta.8 (#15615) @github-actions[bot] @github-actions[bot] @status102
* 自动战斗作业文档干员技能值范围补上0 @status102

### 其他 | Other

* fix casing typo and related context (#15656) @ittuann @Daydreamer114
* YostarJP AveMujica UiTheme and ocr edits @Manicsteiner
* Revert "fix: 未开放关卡不重置 (#15647)" @status102
* 辞岁行 地图 2026-02-10 Map @status102
* YostarKR AveMujica UiTheme and ocr edits @HX3N
* remove regex from `text` field in EN Sui IS @Constrat
* Release v6.3.0-beta.8 (#15639) @ABA2396
* Release v6.3.0-beta.8 (#15613) @ABA2396
* Release v6.3.0-beta.7 (#15574) @ABA2396
* Release v6.3.0-beta.6 (#15492) @ABA2396
* Release v6.3.0-beta.5 (#15481) @ABA2396
* Release v6.3.0-beta.4 (#15462) @ABA2396
* Release v6.3.0-beta.3 (#15452) @ABA2396
* Release v6.3.0-beta.2 (#15441) @ABA2396
* Release v6.3.0-beta.1 (#15437) @ABA2396
* 调整选技能滑动的 postdelay @ABA2396
* EN IS6 tip @Constrat
* 自动战斗编队 技能等级不足i18n, CN 使用理智药及碎石 (#15435) @status102 @HX3N
* YostarJP ocr fix for roguelike @Manicsteiner
* 调整 OF-1 执行条件判断逻辑 @ABA2396
* 合并判断 @ABA2396
* style @status102
* EN fix @Constrat
* 基建计划选择OutOfIndex @status102
* 调整删旧配置时机 @ABA2396
* 调整迁移判断条件 @ABA2396
* 增加借助战 OF-1 在后续刷理智选择 `当前/上次` 导致禁用时的输出 (#15478) @status102 @Manicsteiner @Constrat @HX3N @ABA2396 @momomochi987
* 繁中服不上报企鹅物流 @ABA2396
* 基建找不到对应时间的基建计划 (#15468) @status102 @momomochi987
* Revert "perf: 信用战斗检查启用的首个刷理智是否选择 当前/上次" @ABA2396
* 移除不再使用的 VirtualizingWrapPanel 与 NoAutomationDataGrid @ABA2396
* Revert "perf: 剿灭作战找不到终端内的图标直接结束" @status102
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
* 消除部分编译警告 (#15578) @yali-hzy
* 调整清理图片缓存样式，增加提示 @ABA2396
* ClearCache 改成 static @ABA2396
* Revert "perf: 清空缓存移动到界面设置" @ABA2396
* 自动战斗-自动编队干员不支持技能说明 (#15609) @status102 @Constrat @HX3N @Manicsteiner
* YostarJP AveMujica preload @Manicsteiner
* YostarKR preload AveMujica @HX3N
* preload AveMujica EN @Constrat
* NumberOcrReplace 新增规则 (#14186) @Manicsteiner
* devcontainer适配CMakePresets.json (#15606) @lucienshawls
* Revert "ci: temp fix for txwy gamedat" @Constrat
* 优化 emoji @ABA2396
* "feat: 发布时minify resource文件下的json (#15567)" @ABA2396
