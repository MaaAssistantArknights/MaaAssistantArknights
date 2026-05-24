## v6.10.6

### Highlights

生息演算第二期来了！我们适配了重启锚点模式，同时优化了更新流程，让挂机更省心、版本升级更顺畅。

#### 生息演算：重启锚点

第二期上线，我们适配了重启锚点模式，支持 RA-1、RA-15 关卡，并提供了无需手动过关即可挂机刷代币与科技点的策略。其中 RA-15 策略尚未经充分完善，运行过程中可能遇到问题，将在后续版本逐步优化。

#### Mirror酱 增量包智能等待

优化了 Mirror酱 更新流程，现在新版本发布后若增量包尚未生成，MAA 会自动等待其就绪后重试，省去手动下载完整包的麻烦。

#### 干员识别与仓库识别升级

干员识别和仓库识别新增 Json/Markdown/CSV 导出格式，界面布局统一优化，同时引入虚拟化大幅提高首次加载速度。

#### 小游戏界面重构

小游戏界面全面重构，新增分类支持与日志显示，优化选择逻辑，体验更加直观。  
~~以及更名成了评论区中点赞数最多的名称~~

----

The second season of Reclamation Algorithm is here! We've adapted the Relaunch Anchor mode while also improving the update flow for a smoother experience.

#### Reclamation Algorithm: Relaunch Anchor

The second season is live. We've adapted support for the Relaunch Anchor mode covering RA-1 and RA-15 stages, along with strategies that let you farm tokens and tech points without manual stage clears. The RA-15 strategy is still a work in progress and may encounter issues during execution; it will be progressively refined in future releases.

#### MirrorChyan OTA Smart Retry

The MirrorChyan update flow has been improved. If the OTA package isn't ready right after a new release, MAA will wait and retry automatically — no need to manually grab the full package.

#### Operator & Depot Recognition Upgrade

Operator and depot recognition now support Json/Markdown/CSV export with a unified layout. Virtualization has been introduced to significantly improve initial load times.

#### Minigame UI Overhaul

The minigame interface has been renamed to Useful Tasks, now featuring category support, improved selection logic, and log display for a more intuitive experience.

----

以下是详细内容：

## v6.10.6

### 改进 | Improved

* 优化 ADB 连接处理逻辑，避免不必要的连接尝试，改进错误报告 (#15300) @wangl-cc
* 剿灭关卡使用代理卷后优化网络等待逻辑 @status102

### 修复 | Fix

* 修复傀影肉鸽烧水后无法前往指定难度 (#16843) @ABA2396
* 修复 RegionOCR use_raw = false 时 bounding_rect 扩展失效 @status102
* 修复事件订阅泄露 @status102
* 修复生息演算 RA-1 交付石材识别不到 @Saratoga-Official
* 修复生息演算 RA-4 已知问题 (#16821) @Saratoga-Official
* 基建制造站切换产物流程稳定性提升，产物切换失败时继续执行后续步骤 (#16747) @ZiyinLin @Roland125
* 修复 Bark 与 Gotify 无法使用反代路径 @ABA2396
* 修复自动战斗结束检测 bypass @status102

## v6.10.5

### 新增 | New

* 生息演算新增 RA-4 相关功能 (#16749) @Saratoga-Official @ABA2396

### 改进 | Improved

* 刷理智关卡选择提示当前任务将执行的关卡 (#16797) @status102 @Constrat @HX3N @Manicsteiner
* RA-1 增加迷迭香部署方向失败重试，增加过场动画过长时等待 @ABA2396
* 支持禁用日志停滞检测，优化数据绑定 @ABA2396
* 移除 dft 路径下的缓存并补充缓存驱逐机制 (#16800) @Aliothmoon

### 修复 | Fix

* 修复干员名称截断时前缀完全匹配导致的误选择 @status102
* 修复未开启线索交流时不计算会客室效率 @ABA2396
* 修复 TooltipBlock 无法使用 Binding 进行绑定 (#16796) @status102
* 临时修复 SS 复刻任务导航超时问题 @status102
* 修复 RA-15 导致无法启动的问题 @ABA2396
* 修复生息演算 RA-15 若干问题 (#16770) @walkerljy
* macOS：为 PlayCover 生息演算 RA-1 二倍速识别添加 iOS 模板并降低识别阈值 (#16779) @ColdSpellhere
* 修复 OF-1 战斗失败后错误结束任务 @status102

### 其他 | Other

* 优化任务超时判断 @ABA2396
* 肉鸽分队新增「代理人分队」不支持说明 @ABA2396
* 移除 FightTask 不再使用的关卡设置 @status102
* 删除（大型）兽栏的描述 @ABA2396
* 优化界面布局 @ABA2396

## v6.10.4

### 改进 | Improved

* 优化配置损坏时的提示与记录逻辑，提升异常处理体验 @ABA2396 @status102
* 生息演算 RA-15 OCR 文本匹配改为词组、增加延迟与重试、优化策略流程 (#16725) @walkerljy
* 生息演算 RA-1 部署基地失败时自动重试 @ABA2396
* 生息演算增加基础设施识别，避免误操作进入错误位置 @ABA2396
* Config 检查到缺失时统一使用当前配置进行赋值 @status102
* Yostar EN/JP UR 关卡导航 (#16723) @Manicsteiner

### 修复 | Fix

* 修复潜在的空配置导致异常 @status102
* 修复重复添加同名配置会删除上一个配置 @ABA2396
* 修复远控 LinkStart-* 子任务失败 (#16740) @ABA2396
* 修复剿灭掉落识别不到合成玉时错误停止任务 (#16726) @Roland125
* 补上繁中服漏掉的「拟态学者分队」(#16731) @momomochi987

### 文档 | Docs

* 自动战斗提示移除需要手动借助战的额外操作说明 @status102
* 补充生息演算与小工具相关文档 @ABA2396
* 设备文档添加 steps/details 容器，修复 bat 代码块格式 (#16712) @wryx166
* 更新基建排班文档中过时的链接 (#16700) @H2O-MERO

## v6.10.3

### 改进 | Improved

* 优化 [Flags] 判断 @ABA2396
* RA mode (#16697)  @status102

### 修复 | Fix

* TolerantEnumConverter 支持 Flags @ABA2396
* 遇到无法转换的枚举值转换为带路径信息的 JsonException @ABA2396

## v6.10.2

### 修复 | Fix

* 修复自定义枚举转换器无法处理枚举作为字典键 @ABA2396

## v6.10.1

### 修复 | Fix

* 修复在遇到多个非法配置参数时会直接重置配置 @ABA2396

## v6.10.0

### 新增 | New

* 初步支持生息演算：重启锚点 RA1/RA-15 关卡，支持新手刷代币与科技点 @ABA2396 @walkerljy @Daydreamer114 @SherkeyXD @hguandl
* 支持通过 Mirror酱 下载时若新版本无增量包则等待后重试 (#16656) @ABA2396
* 17 章导航 @ABA2396
* 便捷功能提示文本自适应大小 @ABA2396
* 仓库识别支持导出 Markdown/CSV，优化导出按钮布局与交互 (#16543) @H2O-MERO @ABA2396
* 生息演算支持不同分辨率 @ABA2396
* 点击成就横幅跳转至成就设置，自动打开成就列表并筛选对应成就 (#16537) @H2O-MERO @ABA2396
* 任务日志输出停滞时发送通知，替换任务超时通知 (#16511) @H2O-MERO
* 为自动公招的输出日志增加已公招次数 (#16651) @H2O-MERO @ABA2396
* 干员识别支持导出 Json/Markdown/CSV，优化导出按钮布局与交互 (#16635) @H2O-MERO
* support native android (#16179) @Aliothmoon

### 改进 | Improved

* 优化生息演算木材数量与部署费用识别，支持开局自带木材快速完成任务，提升运行速度与稳定度 @ABA2396 @AnnAngela
* 生息演算策略逻辑重构 (#16680) @ABA2396
* 统一 LocalizationHelper GetStringFormat (#16658) @ABA2396
* 更新基建排班表（243 高配三队简化、333 极限 3 队，20260518 修订）(#16678) (#16679) @ntgmc
* 干员识别与仓库识别支持虚拟化，大幅提高首次加载速度 (#16486) @ABA2396
* 优化 masked TM_CCOEFF_NORMED 匹配性能 (#16593) @Aliothmoon
* 小游戏界面重构，添加分类并优化选择逻辑，添加日志显示 (#16499) @SherkeyXD @Constrat @momomochi987
* WPF 新配置修改日志记录等级提升至 Info @status102
* 自动战斗拆出导航 @status102
* 调整小工具-便捷任务布局，调整日志输出 @ABA2396
* RunningState 更新 (#16585) @status102
* 干员识别本地化导出表头，添加类型化枚举 @ABA2396
* 统一干员识别与仓库识别界面布局 @ABA2396
* 合并输出 @status102
* OCRer DEBUG 下绘制匹配结果 @status102
* 减少中间状态 @ABA2396
* 优化 CI 工作流：升级 actions/upload-artifact 至 v7，设置压缩级别为 0，改进 PR 提交检查评论逻辑 (#16671) @AnnAngela @ABA2396
* Reduce unnecessary allocations @status102
* YostarKR UR stage navigation @HX3N

### 修复 | Fix

* 修复生息交付木材 ROI 错误 @Saratoga-Official
* 修复 PNS 配置项被意外恢复的问题 @ABA2396
* 修复 15 章之后的难度切换 @ABA2396
* 繼續調整繁中服部分幹員名稱 OCR (#16600) @momomochi987
* 修复理智药过期天数识别失败取消确认逻辑未生效 @status102
* 修复临期理智药天数缺省值 @status102
* 修复便捷功能列表滚动 @ABA2396
* 修复便捷功能 GroupStyle @ABA2396
* 修复理智药过期参数迁移输出 Warning 中参数名错误 @status102
* 修复 FFT 路径 masked TM_CCOEFF_NORMED 精度损失导致的误匹配 (#16652) @Aliothmoon
* 修复日志输出停滞功能在未开启外部通知时无法生效 @ABA2396
* 修复 build warning @ABA2396
* 修复 typo @ABA2396
* 界园深入探索模板 (#16626) @ZiyinLin @status102
* Various IS encounter Regex @Constrat
* TimesChange event @Constrat

### 文档 | Docs

* 修正嵌套容器说明 @Rbqwow
* 新增 DWM 相关问题处理知识文档 @ABA2396
* 补充 PC 客户端、多开与账号管理等知识文档 @ABA2396
* 补充 AddLog 缺失 param 介绍 @ABA2396

### 其他 | Other

* 修改 ai-issue-analysis @ABA2396
* 赛博道长 @ABA2396
* 调整图标阴影 @ABA2396
* 采用 System.Windows 的剪贴板 @ABA2396
* 调整输出格式 @ABA2396
* 统一符号 @ABA2396
* 调整干员识别提示换行 @ABA2396
* gitignore for C# dev kit vsc deo @Constrat
