## v6.12.1

### 新增 | New

* 初步支持 `生息演算：重启锚点` 新手刷代币与科技点策略 @ABA2396
* 17 章导航 @ABA2396
* 便捷功能提示文本自适应大小 @ABA2396
* 仓库识别支持导出 Markdown/CSV，优化导出按钮布局与交互 (#16543) @H2O-MERO @ABA2396
* support native android (#16179) @Aliothmoon
* 生息演算支持不同分辨率 @ABA2396
* 点击成就横幅跳转至成就设置，自动打开成就列表并筛选对应成就 (#16537) @H2O-MERO
* 任务日志输出停滞时发送通知, 替换 任务超时通知 (#16511) @H2O-MERO
* 为自动公招的输出日志增加已公招次数 (#16651) @H2O-MERO @ABA2396
* 干员识别支持导出 Json/Markdown/CSV，优化导出按钮布局与交互 (#16635) @H2O-MERO
* 生息演算：重启锚点 @hguandl
* 生稀盐酸-重启锚点 添加 RA15 支持 (#16667) @walkerljy @Daydreamer114 @SherkeyXD
* 支持通过 Mirror酱 下载时若新版本无增量包则等待后重试 (#16656) @ABA2396
* SwitchConfig 检测到配置缺失尝试从当前配置复制恢复通知用户 @ABA2396
* RA-4 (#16749) @Saratoga-Official @ABA2396
* 记录 MaaMacGui 子仓库更新 (#16870) @ColdSpellhere
* 为设置添加了搜索支持 (#16833) @H2O-MERO @ABA2396
* 新增 MarkdownDataHelper @ABA2396
* 对一键长草列表进行了交互优化，并添加了复制按钮 (#16733) @H2O-MERO @status102 @ABA2396
* 公招保留指定词条 (#16586) @ABA2396 @status102
* 理智作战支持设定目标材料最大库存 (#16487) @ABA2396 @status102
* 支持部分任务间通过导航栏切换 (#16869) @ABA2396
* TouchMode ToolTip 添加视频演示 (#16812) @ABA2396
* 支持 ImageCropper 截取 PC 窗口 (#16969) @DavidWang19
* 界面主题「出猎」 @SherkeyXD
* 增加怪猎联动二期 TD-6/7/8 (#16962) @ZiyinLin @ABA2396
* GUI 啟動設定支援繁中服顯示帳號名欄位 @momomochi987
* 繁中服資源補上帳號切換流程與模板 @momomochi987
* AccountSwitchTask 支援繁中服客戶端 @momomochi987
* CustomWebhook支持自定义Headers @status102
* 理智药使用增加使用中的药品信息 (#17034) @status102
* 支持 mumu 6.0 截图增强路径 (#16994) @ABA2396
* 切换主题时保存当前画面截图 (#16993) @ABA2396
* 统一 SearchBar 样式 @ABA2396
* 悖论模拟支持跳过战斗失败的作业, 自动战斗作业增加对应结构 (#16985) @status102
* 启动设置添加模拟器启动测试按钮，便于测试是否配置成功 @ABA2396
* 添加阵地足球锦标赛小游戏 @ABA2396

### 改进 | Improved

* OCRer DEBUG下m_image_draw绘制增加结果文本 @status102
* Wpf 新配置修改日志记录等级提升至 Info @status102
* 自动战斗拆出导航 @status102
* 优化 masked TM_CCOEFF_NORMED 匹配性能 (#16593) @Aliothmoon
* 调整 小工具-便捷任务 布局，调整日志输出 @ABA2396
* 小游戏界面重构，添加分类并优化选择逻辑，添加日志显示 (#16499) @SherkeyXD @Constrat @momomochi987
* 避免不必要的new @status102
* RunningState更新 (#16585) @status102
* OCRer DEBUG下绘制匹配结果 @status102
* 干员识别与仓库识别支持虚拟化，大幅提高首次加载速度 (#16486) @ABA2396
* 减少中间状态 @ABA2396
* 干员识别本地化导出表头，添加类型化枚举 @ABA2396
* Revert "chore: 优化生息演算替换逻辑" @ABA2396
* 生息演算增加部署费用、木头数识别，提升运行速度与稳定度 @ABA2396
* 生息演算策略逻辑修改 (#16680) @ABA2396
* 更新 243 高配三队简化一天三换排班表（20260518 修订） (#16678) @ntgmc
* 更新 333 极限3队一天三换排班表（20260518 修订） (#16679) @ntgmc
* 优化木材数量正则 @ABA2396
* 优化木材数量识别，支持开局自带木材快速完成任务 @ABA2396
* 合并输出 @status102
* 优化生息演算小猫费用识别 @ABA2396
* 统一 LocalizationHelper GetString Format (#16658) @ABA2396
* RA mode (#16697) @status102
* 优化 [Flags] 判断 @ABA2396
* 优化配置异常弹窗 @ABA2396
* 配置损坏记录 @status102
* 配置部分损坏提示 @status102
* Config检查到缺失时, 统一使用当前config进行赋值 @status102
* 刷理智关卡选择提示当前任务将执行的关卡 (#16797) @status102 @Constrat @HX3N
* 移除FightTask不再使用的关卡设置 @status102
* RA-1 增加迷迭香部署方向失败重试，增加过场动画过长时等待 @ABA2396
* 支持禁用日志停滞检测，优化数据绑定 @ABA2396
* IsRefreshingUI 自动化支持 @status102
* 剿灭关使用代理卷后网络卡顿等待 @status102
* 优化 FightTaskStageResetModeConverter 与 RecruitTaskHoldTagsConverter 嵌套逻辑 @ABA2396
* NotificationImplWpf当Toast不可用时提示原因 (#16877) @status102 @status102 @Manicsteiner @momomochi987 @HX3N
* 统一使用重载的 GetValue 替换 Convert.To (#16866) @ABA2396
* 界园肉鸽弹窗Next关闭 @status102
* TaskQueue CheckBox与添加任务按钮对齐 @status102
* 优化TaskQueue选中任务时设置按钮表现以突出当前选中的选项 @status102
* PostAction清空按钮新增仅一次状态同步 @status102
* 临期药下拉框显示24h x n @status102
* 悖论模拟作业列表变量名与实际不符 @status102
* 仅在LinkStart时执行完成后动作, 避免单次运行时触发 @status102
* 水月萨卡兹肉鸽 关闭next弹窗 (#16918) @status102
* 优化信用商店复核逻辑 (#16932) @ABA2396
* 优化部分情况下自动战斗导航OCR结果中会出现误识别的前缀 @status102
* DEBUG 环境下Init时TaskQueue状态限制缓解 @status102
* 肉鸽弹窗类事件处理重构 CloseCollectionClose (#17005) @status102
* 基于灰度阈值预处理的自动战斗导航, 适配H关及怪猎2期 (#16990) @status102
* InvokeProcSubTaskMsg 重构 (#16979) @status102

### 修复 | Fix

* 有小猪 @status102
* 15 章之后的难度切换 @ABA2396
* 繼續調整繁中服部分幹員名稱 OCR (#16600) @momomochi987
* 理智药过期天数识别失败取消确认逻辑未生效 @status102
* 临期理智药天数缺省值 @status102
* 便捷功能列表滚动 @ABA2396
* 便捷功能 GroupStyle @ABA2396
* 刷理智-理智药过期参数迁移输出Warning中参数名错误 @status102
* typo @ABA2396
* 修复 FFT 路径 masked TM_CCOEFF_NORMED 精度损失导致的误匹配 (#16652) @Aliothmoon
* 修复日志输出停滞功能在未开启外部通知时无法生效 @ABA2396
* Various IS encounter Regex @Constrat
* 界园深入探索模板 (#16626) @ZiyinLin @status102
* TimesChange event @Constrat
* 特意删的 PNS 怎么又给加回来了 @ABA2396
* build warning @ABA2396
* 生息交付木材roi错误 @Saratoga-Official
* 日志顺序 @ABA2396
* 在遇到多个非法配置参数时会直接重置配置 @ABA2396
* 自定义枚举转换器无法处理枚举作为字典键 @ABA2396
* review @ABA2396
* 不会现在还有人选沙中遗火吧 @ABA2396
* TolerantEnumConverter 支持 Flags @ABA2396
* 修复潜在的空config @status102
* 配置修复弹窗后自动退出 @status102
* 修复重复添加同名配置会删除上一个配置 @ABA2396
* 远控 LinkStart-* 子任务失败 @ABA2396
* 修复部分 RA-15 bug，优化逻辑 (#16725) @walkerljy
* 補上繁中服漏掉的「擬態學者分隊」 (#16731) @momomochi987
* 剿灭掉落识别不到合成玉时不停止任务 (#16726) @Roland125
* 当新配置损坏时, 重新补充默认配置 @status102
* 选项显示内容错误 @ABA2396
* 企鹅物流汇报 ID 始终显示 @ABA2396
* 自动战斗快速编队干员名roi @status102
* 修复未开启线索交流时不计算会客室效率 @ABA2396
* 移除dft路径下的缓存 & 补充缓存驱逐机制 (#16800) @Aliothmoon
* TooltipBlock无法使用Binding进行绑定 (#16796) @status102
* SS复刻任务导航超时临时修复 @status102
* 删多了 @status102
* RA-15 导致无法启动 @ABA2396
* 生息演算RA15 bug fix (#16770) @walkerljy
* 为 PlayCover 生息演算 RA1 二倍速识别添加 iOS 模板和降低识别阈值 (#16779) @ColdSpellhere
* OF-1战斗失败错误结束 @status102
* TaskItem事件 @status102
* 傀影肉鸽烧水后无法前往指定难度 @ABA2396
* RegionOCR use_raw = false时bounding_rect的扩展失效 @status102
* 事件订阅泄露 @status102
* RA1交付石材有可能识别不到 @Saratoga-Official
* 基建制造站切换产物流程稳定性提升 (#16747) @ZiyinLin @Roland125
* refactor ADB connection logic to only connect when needed (#15300) @wangl-cc
* 修复 Bark 与 Gotify 无法使用反代路径 @ABA2396
* RA-4已知问题 (#16821) @Saratoga-Official
* 自动战斗结束检测bypass @status102
* Potential fix for pull request finding @ABA2396
* 修复 adb-lite 跳过 adb connect 后未初始化 client (#16850) @wangl-cc
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
* 信用收支卡在商店里 @ABA2396
* 显式声明 adb-lite client 对应的 serial (#16853) @wangl-cc
* 修复开启基建退出提醒时无法退出基建 @ABA2396
* 修复快捷切换在遇到 LoadingText 时无法跳出循环 @ABA2396
* changelog 猪了 @ABA2396
* roi @ABA2396
* 御龙装置识别 @Saratoga-Official
* 优化追加自定干员弹窗删除时的布局 (#16921) @ZiyinLin
* revert OCR changes @Constrat
* 调整基建干员名 OCR 区域 (#16905) @ZiyinLin
* 修复宿舍自定义干员+信赖补位的bug&外服同步修复&提升代码可读性 (#16659) @ZiyinLin @Constrat @Roland125
* 错误隐藏开局分队与开局干员选项 @ABA2396
* EN IS6 bosky updated template @Constrat
* EN IS6 bosky text size changed @Constrat
* 修正特克诺干员名 OCR 误识别 (#17030) @ZiyinLin
* 绿票商店状态回退错误 @status102
* 绿票商店2阶段check @status102 @ZiyinLin
* MaskedCcoeffMatcher 稀疏路径累加器改用 CV_64F 防止大数相减精度损失 (#16983) @Aliothmoon
* 修复 MaaMacGui changelog 贡献者 mention (#16978) @ColdSpellhere
* 降低 PlayCover 下肉鸽部分任务的模版匹配分数阈值 (#16968) @Alan-Charred
* Potential fix for pull request finding @ABA2396
* ConfigFactory Save锁统一 (#17052) @status102
* 增加贸易站订单切换重试和产物确认逻辑 (#16954) @ZiyinLin
* 修复 MuMu 12 任务完成后无法关闭模拟器 (#17067) @Zmjjeff7
* 释放 StartSettings 中未释放的 Process 对象 (#17060) @Zmjjeff7
* build warning @ABA2396

### 文档 | Docs

* Auto Update Changelogs of v6.9.5 (#16576) @github-actions[bot] @github-actions[bot] @status102
* Auto Update Changelogs of v6.10.0-beta.1 (#16612) @github-actions[bot] @github-actions[bot] @ABA2396
* Update CHANGELOG for v6.10.0-beta.2 release @ABA2396
* Update CHANGELOG for v6.10.0-beta.3 @ABA2396
* Update CHANGELOG for v6.10.0-beta.4 @ABA2396
* changelog @ABA2396
* 修正嵌套容器说明 @Rbqwow
* Update CHANGELOG for version 6.10.1 @ABA2396
* Update CHANGELOG for version 6.10.2 @ABA2396
* Bump version to v6.10.3 and update changelog @ABA2396
* changelog @ABA2396
* Update docs/ko-kr/manual/device/macos.md @ABA2396
* 自动战斗右侧提示移除需要手动借助战的额外操作说明 @status102
* 补充生息演算与小工具相关文档 @ABA2396
* 设备文档添加 steps/details容器，修复bat代码块格式，处理文档中demo-wrapper的废弃警告 (#16712) @wryx166
* changelog @ABA2396
* Release notes for version 6.10.5 @ABA2396
* changelog @ABA2396
* changelog @ABA2396
* changelog @ABA2396
* changelog @ABA2396
* Auto Update Changelogs of v6.11.0-beta.1 (#16898) @github-actions[bot] @github-actions[bot] @ABA2396
* 补全 指定天数内的理智药 相关文档 @ABA2396
* skip_tags -> preserve_tags @ABA2396
* changelog @ABA2396
* Auto Update Changelogs of v6.11.0 (#16965) @github-actions[bot] @github-actions[bot] @ABA2396
* 帳號切換手冊與整合協議補上繁中服說明 @momomochi987
* 更新帳號切換小提示，加入繁中服 Email 帳號說明 @momomochi987
* Update CHANGELOG.md for version 6.11.1 @ABA2396
* Update CHANGELOG top version to v6.11.1 (#16972) @github-actions[bot] @github-actions[bot]
* changelog @ABA2396
* changelog @ABA2396
* changelog @ABA2396
* README 自动抄作业 --> 自动战斗 @Rbqwow
* Update CHANGELOG for v6.12.0-beta.2 release @ABA2396
* changelog @ABA2396

### 其他 | Other

* Release v6.12.0-beta.2 (#17054) @ABA2396
* Release v6.12.0-beta.1 (#17029) @ABA2396
* Release v6.11.1 (#16971) @Daydreamer114
* Release v6.11.0 (#16963) @ABA2396
* Release v6.11.0-beta.2 (#16919) @ABA2396
* Release v6.11.0-beta.2 (#16916) @ABA2396
* Release v6.11.0-beta.1 (#16906) @ABA2396
* Release v6.11.0-beta.1 (#16895) @ABA2396
* Release v6.10.7 (#16851) @ABA2396
* Release v6.10.6 (#16846) @ABA2396
* Release v6.10.5 (#16807) @ABA2396
* Release v6.10.4 (#16743) @ABA2396
* Release v6.10.4 (#16741) @ABA2396
* Release v6.10.3 (#16701) @ABA2396
* Release v6.10.2 (#16693) @ABA2396
* Release v6.10.1 (#16689) @ABA2396
* Release v6.10.0 (#16682) @ABA2396
* Release v6.10.0-beta.4 (#16648) @ABA2396
* Release v6.10.0-beta.3 (#16623) @ABA2396
* Release v6.10.0-beta.2 (#16619) @ABA2396
* Release v6.10.0-beta.1 (#16611) @ABA2396
* Release v6.9.5 (#16575) @status102
* 统一符号 @ABA2396
* Update KNOWLEDGE.md with PC mouse rendering note @ABA2396
* Add Arknights PC Client information to KNOWLEDGE.md @ABA2396
* 调整干员识别提示换行 @ABA2396
* Update KNOWLEDGE.md with PC announcement details @ABA2396
* 调整图标阴影 @ABA2396
* 采用 System.Windows 的剪贴板 @ABA2396
* 调整输出格式 @ABA2396
* 修改 ai-issue-analysis @ABA2396
* 赛博道长 @ABA2396
* 调整描述 @ABA2396
* 欠费下小猫 @ABA2396
* 快速下小猫 @ABA2396
* 加快下一轮循环 @ABA2396
* 提升对话速度 @ABA2396
* 添加描述 @ABA2396
* Add section on MAA multi-opening and account management @ABA2396
* 放宽对 RA-1 关卡名的检查 @ABA2396
* 添加拆除设施的描述 @ABA2396
* 删除辅助建设模式的描述 @ABA2396
* AddLog 缺失 param 介绍 @ABA2396
* 统一干员识别与仓库识别界面布局 @ABA2396
* 提高生息演算对话速度 @ABA2396
* 优化生息演算替换逻辑 @ABA2396
* 生息演算统一命名 @ABA2396
* 生息演算拖动地图增加重试 @ABA2396
* YostarKR UR stage navigation @HX3N
* gitignore for C# dev kit vsc deo @Constrat
* 为什么会叫这个名字呢？ @ABA2396
* 调整 RA-15 滑动速度 @ABA2396
* 修改描述 @ABA2396
* Rename artifact uploads in CI workflow @AnnAngela
* 点击开始建设添加重试 @ABA2396
* Add Abort DWM section to KNOWLEDGE.md @ABA2396
* 遇到无法转换的枚举值转换为带路径信息的 JsonException @ABA2396
* Changelog skill @ABA2396
* RA-1 部署基站失败时尝试重新部署 @ABA2396
* 生息演算增加基础设施识别，避免二次点击制造图标进入错误位置 @ABA2396
* Revert "feat: SwitchConfig 检测到配置缺失尝试从当前配置复制恢复通知用户" @status102
* 调整坐标 @ABA2396
* Yostar UR stage navigation (#16723) @Manicsteiner
* Revert "fix: 当新配置损坏时, 重新补充默认配置" @ABA2396
* 更新基建排班文档中过时的链接 (#16700) @H2O-MERO
* 移除不必要的刷新 @ABA2396
* 优化任务超时判断 @ABA2396
* 删除 （大型） 兽栏的描述 @ABA2396
* 肉鸽分队添加 代理人分队 不支持描述 @ABA2396
* 优化界面布局 @ABA2396
* Revert "rft: IsRefreshingUI 自动化支持" @status102
* 全项目代码审查 - only cursor can do (指薅羊毛 @MistEO
* ci：请使用 wingetcreate 代替 winget-releaser (#16813) @Trenly
* inheritdoc @ABA2396
* YostarKR UR ocr edit @HX3N
* Update KNOWLEDGE.md with Connect.TouchMode information @ABA2396
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
* changelog details 增加日期 @ABA2396
* Revert "perf: 仅在LinkStart时执行完成后动作, 避免单次运行时触发" @ABA2396
* 调整吐司通知禁用说明 @ABA2396
* update MaaDeps version to v2.11.0 (#16961) @Aliothmoon
* 修改描述文本 `过期关卡重置为` -> `过期活动关卡重置为` 以更加符合当前重置范围 @status102
* Revert "fix: 调整基建干员名 OCR 区域" (#16937) @ZiyinLin @Constrat
* 延长 TD-Open 延迟 @ABA2396
* TDChapterToTD 添加重试 @ABA2396
* rename Wpf ProcSubTaskMsg param name @status102
* revert all changes to remainingcandleflame IS6 EN @Constrat
* YostarKR winden colorScale for compatibility @HX3N
* 自动战斗-视频链接 始终显示 @ABA2396
* 补充可露希尔基建数值 @Saratoga-Official
* 删除多余验证 @ABA2396
* 小游戏翻译 @ABA2396
* 繁中服「雪山降臨1101」活動導航 & 「喀蘭貿易技術研發部」小遊戲 (#17073) @momomochi987
* OF1把嵯峨换成银灰避免可能打不过 @Saratoga-Official
* 调整部署速度 @ABA2396
* 足球需要更多的 325 @ABA2396
* 阵地足球开始后增加动画延迟，避免点入编队界面 @ABA2396
* 还原牛杂 i18n 实现方式 @ABA2396
* Revert "chore: 删除多余验证" @ABA2396
* Revert "fix: Potential fix for pull request finding" @ABA2396

### MaaMacGui

### 新增 | New

* 阵地足球锦标赛 入口 @ABA2396
* 为生息演算重启锚点添加策略说明 ([#91](https://github.com/MaaAssistantArknights/MaaMacGui/pull/91)) @ColdSpellhere
* Add localized strings for relaunch case @ABA2396
* RelaunchAnchor @hguandl

### 改进 | Improved

* MaaCore @hguandl
* 统一使用 String(localized:) 替代 NSLocalizedString ([#85](https://github.com/MaaAssistantArknights/MaaMacGui/pull/85)) @FireflySentinel

### 修复 | Fix

* 统一 gui.log 文件日志中的日期与时间格式 ([#93](https://github.com/MaaAssistantArknights/MaaMacGui/pull/93)) @Alan-Charred
* 为生息演算重启锚点添加 RA-4 入口 ([#88](https://github.com/MaaAssistantArknights/MaaMacGui/pull/88)) @ColdSpellhere
* Mac 端选择重启锚点后连接死循环，所有任务无法执行 ([#87](https://github.com/MaaAssistantArknights/MaaMacGui/pull/87)) @FireflySentinel

### 其他 | Other

* Update RA @hguandl
* Remove localized string for energy collection @ABA2396
* Localized String @hguandl
