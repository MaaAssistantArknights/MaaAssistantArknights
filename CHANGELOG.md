## v6.10.5

### 新增 | New

* RA-4 (#16749) @Saratoga-Official @ABA2396
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

### 改进 | Improved

* 刷理智关卡选择提示当前任务将执行的关卡 (#16797) @status102 @Constrat @HX3N
* 移除FightTask不再使用的关卡设置 @status102
* RA-1 增加迷迭香部署方向失败重试，增加过场动画过长时等待 @ABA2396
* 支持禁用日志停滞检测，优化数据绑定 @ABA2396
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

### 修复 | Fix

* 修复未开启线索交流时不计算会客室效率 @ABA2396
* 移除dft路径下的缓存 & 补充缓存驱逐机制 (#16800) @Aliothmoon
* TooltipBlock无法使用Binding进行绑定 (#16796) @status102
* SS复刻任务导航超时临时修复 @status102
* 删多了 @status102
* RA-15 导致无法启动 @ABA2396
* 生息演算RA15 bug fix (#16770) @walkerljy
* 为 PlayCover 生息演算 RA1 二倍速识别添加 iOS 模板和降低识别阈值 (#16779) @ColdSpellhere
* OF-1战斗失败错误结束 @status102
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

### 文档 | Docs

* Release notes for version 6.10.5 @ABA2396
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

### 其他 | Other

* 删除 （大型） 兽栏的描述 @ABA2396
* 肉鸽分队添加 代理人分队 不支持描述 @ABA2396
* 优化界面布局 @ABA2396
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
