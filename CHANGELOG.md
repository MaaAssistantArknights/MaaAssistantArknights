## v5.21.0

### 新增 | New

* 添加自定义剿灭关卡勾选框 @ABA2396
* 适配长期剿灭 (#13241) @dikxingmengya @HX3N @Constrat @momomochi987 @Manicsteiner
* 界园肉鸽开始探索CD任务ocr追加 @status102
* 禁用弹出成就气泡 (#13345) @Sakurapainting @Constrat @status102 @ABA2396
* 公招识别结果根据当前潜能排序 @ABA2396
* 成就通知自动关闭选项 @Sakurapainting
* 公招结果支持彩色显示 @ABA2396
* 查看公告按钮添加检查时禁用 @ABA2396
* 移除肉鸽默认分队与默认职业组 @ABA2396
* 新增是否启动客户端勾选框 @ABA2396

### 改进 | Improved

* 简化长期剿灭任务名 @status102
* wpf剿灭关卡选择 @status102
* 优化肉鸽难度选择逻辑 @ABA2396
* 肉鸽指定干员开局招募进入干员选择页状态监测追加首次进入的指导的监测 @status102
* 重构仓库识别逻辑，大幅提高识别速度 @ABA2396
* PerformanceUseGpu 读取替换 @status102
* 更新繁中服部分幹員 ocr + 補充截圖 (#13338) @momomochi987 @pre-commit-ci[bot]
* wpf不再允许通过Gui直接启用`允许使用不推荐的GPU`选项 @status102
* 优化客户端切换逻辑，官服B服切换无需重载 @ABA2396
* 添加自动编队选项 忽略干员属性要求 (#13250) @travellerse @status102 @HX3N @Constrat
* 优化资源损坏弹窗描述，提供解决方案 @ABA2396
* 雷电注册表优先使用原版路径 @ABA2396
* 繁中服「追跡日落以西」活動導航 (#13336) @momomochi987
* 肉鸽局内外招募流程拆分 (#13342) @status102
* 简中繁中截图耗时过长提示增加截图增强提醒 (#13309) @status102 @Manicsteiner @Constrat @HX3N

### 修复 | Fix

* 移除未使用的AnnihilationWarning @status102
* 加回来删掉的提示 @status102
* 肉鸽开局招募状态 @status102
* 界园肉鸽结算 @status102
* 事件名错误 @ABA2396
* 卡招募 卡电弧 @ABA2396
* 投钱 @ABA2396
* StageEncounterOptionUnknown 点到源石锭 @ABA2396
* 换用strategyChange @Alan-Charred
* 饕餮廊事件识别错误 @ABA2396
* 我就不明白了，为什么之前主题没有暴露出来问题？ @Alan-Charred
* 我的问题 @Alan-Charred
* 尝试修复卡进场 @Alan-Charred
* 血量识别 @ABA2396
* 尝试修复 investment_with_more_score @Alan-Charred
* 怎么多了个票卷生命值位置都变了 @ABA2396
* 无法点击树洞 @ABA2396
* 事件选择 @ABA2396
* 卡退出战斗 @ABA2396
* 卡招募（？ @ABA2396
* 截图保存报错 @ABA2396
* 修修补补 @Alan-Charred
* 修复换层后 StrategyChange 功能 @Alan-Charred
* 修复换层识别 roi @Alan-Charred
* 无法连接主站时无法通过备站更新 @ABA2396
* 肉鸽招募任务计数未能正确归0 @status102
* uuid 与 version 返回错误格式时崩溃 @ABA2396
* YostarJP rogue OCR tweak for Mushikazu no Taki (#13346) @RadioNoiseE
* 公招识别按钮超出界面范围 @ABA2396
* SA1136, SA1133 file header @status102
* 分支条件判断错误 @ABA2396
* 仓库识别 x万 识别 @ABA2396
* 嵌套翻译 @ABA2396
* 公开招募前三个槽均未招募时不会查看第四个槽 @ABA2396
* 勾选框状态错误显示 @ABA2396
* 移除 `investment_enter_second_floor` 支持 @status102
* fix AppImage build toolkit (#13324) @Manicsteiner
* 萨卡兹肉鸽结算页识别错误 @ABA2396
* YostarKR InfrastReward ocrReplace @HX3N
* 移除MaaCore的StartUp任务的client_type空值支持 (#13318) @status102 @HX3N
* 使用理智药时恢复理智Ocr包含意外字符 @status102
* wpf TaskApi加载cache时缓存文件不存在 @status102
* missing OR-5 in main repository @Constrat
* 水月肉鸽卡商店刷新 @ABA2396
* 更新日志与公告滚动条过于贴近窗口边缘 @ABA2396

### 文档 | Docs

* EN issue simplification @Constrat

### 其他 | Other

* 置顶按钮鼠标悬浮背景色 @ABA2396
* 调整模拟器分辨率不支持的描述 @ABA2396
* 添加开机自启与系统通知相关注册表提示 @ABA2396
* 调整更新提示 @ABA2396
* 扩大公告宽度，更容易滑到底（了吗？） @ABA2396
* 调整界面描述 @ABA2396
* 标题栏显示版本改为 ui 版本 @ABA2396
* 调整 gpu 提示 @ABA2396
* cdk 边框圆角 @ABA2396
* range::copy替换 @status102
* MirrocCdk复制按钮接缝美化 @status102
* CN issue模板简化及Bot优化 (#13297) @status102
* 长草任务序列化重构已完成部分合并 (#13275) @status102
* 进出洞 @ABA2396
* ocr 任务暴露灰度阈值接口 @ABA2396
* 添加选项 fallback @ABA2396
* ui 添加刷等级入口 @ABA2396
* 界园屏蔽进二层 @ABA2396
* 增加快速刷钱时路线全是战斗的回调 @ABA2396
* 先复制一下之前的导航代码，能跑就行；以后再重构 @Alan-Charred
* 补充相关任务链 @Alan-Charred
* fast investment for JieGarden @Alan-Charred
* update map @Alan-Charred
* node templates @Alan-Charred
* skip recruitment @Alan-Charred
* add new routing strategy -- FastInvestment_JieGarden @Alan-Charred
* 关闭自定义剿灭时默认当期剿灭 @ABA2396
* import @status102
* 替换 JieGarden StageEncounterOptionUnknown模板图 @ABA2396
* 护鸭金刚 选项 @ABA2396
* Revert "feat: 界园屏蔽进二层" @ABA2396
* 多余的 ocrReplace @ABA2396
* 防止没进 StrategyChange 的情况 @ABA2396
* polish code @Alan-Charred
* 仓库识别全局缓存，去除开方运算 @ABA2396
* SA1122 Use string.Empty for empty strings @status102
* wpf`干员`统一使用指定字符串资源 (#13333) @status102 @Constrat @Manicsteiner @HX3N
* vs code jsonc @status102
* make GUI UserAgent simple @MistEO
* 替换表合并使用 insert，保证顺序并提升可读性 @ABA2396
* 添加 gpu 日志 @ABA2396
* 添加 Tencent EdgeOne 备用 CDN @ABA2396
* 肉鸽加点注释 @status102
* 添加请求耗时记录 @ABA2396
