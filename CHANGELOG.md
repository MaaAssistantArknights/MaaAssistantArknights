## v5.13.0-beta.4

### 修复 | Fix
* v5.13.0-beta.3 引入的自动公招识别问题 @horror-proton

## v5.13.0-beta.3

### 改进 | Improved
* 优化存亡之战部署策略 (#11706) @Black1312

### 修复 | Fix

* 公招勾选使用加急许可时任务失败 (fix return value of RecruitImageAnalyzer) @horror-proton
* 肉鸽开局干员使用助战失效 @status102
* 肉鸽烧水使用分队失效 @status102
* 自动战斗开始战斗时使用理智药检测失效 @status102

### 其他 | Other

* 更新日志支持惯性运动 @ABA2396

## v5.13.0-beta.2

### 新增 | New

* 繁中服「懷黍離」導航入口更動 (#11662) @momomochi987
* 添加 MirrorChyan 资源更新方式 (#11669) @ABA2396 @MistEO
* 适配新 ui 未进驻选项 @ABA2396
* 添加不期而遇新事件空无前兆 (#11573) @DavidWang19

### 改进 | Improved

* 萨卡兹肉鸽不期而遇统一使用默认策略 (#11512) @Daydreamer114
* 小工具-仓库识别 隐藏黑边 @ABA2396

### 修复 | Fix

* MaaCore日志(asst.log)滚动修复，并上调阈值至64MiB (#11670) @status102
* Wpf肉鸽烧水时使用分队UI为空 @status102
* 肉鸽临时招募预备干员时, 不额外提升权重 (#11442) @Daydreamer114
* 公告窗口触控板滚动异常 (#11684) @Rbqwow
* 肉鸽作战编队截图过快导致截图与实际不符 (#11527) @Daydreamer114
* 肉鸽烧水未获得目标奖励逻辑补漏 @status102
* 幸运墙领取奖励界面识别过早 @status102
* JP 干员识别 艾雅法拉 (#11685) @Saratoga-Official @status102 @Daydreamer114

### 文档 | Docs

* 推荐分辨率720P或1080P (#11651) @Rbqwow
* 替换.NET8 桌面运行时下载链接为直链 (#11693) @wryx166
* 肉鸽推荐开局策略 (#11570) @Rbqwow @Constrat

### 其他 | Other

* Attempt retry once screencap for MumuExtras (#11550) @teldd1
* 临时处理肉鸽烧水flag异常 @status102
* mirror-chyan notify error @MistEO
* missing `user_agent` param for mirrorchyan query @MistEO
* styling @Constrat
* mirrorchyan toast @MistEO
* 简化肉鸽任务使用助战参数内部存储流程 @status102
* 添加资源更新提示翻译 @ABA2396
* 将Sarkaz开局添加负荷干员的进入任务改为范围点击 (#11100) @Daydreamer114
* discord link for website docs (#11687) @Constrat @momomochi987 @Rbqwow
* Discord link in About Us @Constrat
* en announcement wpf logic @Constrat
* Mirror酱说明调整 @status102
* NoSkland 放到 wpf 内部 @ABA2396
* 密钥改成 PasswordBox @ABA2396
* remove chinese punctuation from en @Constrat
* 调整肉鸽选择烧水奖励任务链，重新将Roguelike@LastRewardConfirm并入主任务链 (#11689) @status102
* add discord link to main readme @Constrat
* 修改划火柴设置界面布局 (#11682) @Rbqwow
* WpfGui划火柴相关说明调整 @status102
* Revert "perf: 肉鸽优先拿美愿 (#11558)" (#11565) @Daydreamer114
* 调整基建办公室阈值 @ABA2396
* 调整 InfrastBottomLeftTab 的 specificRect @ABA2396
