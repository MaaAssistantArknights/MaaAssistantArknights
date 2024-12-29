## v5.11.1

### 新增 | New

* 适配界面主题「大荒」与「视相」 (#11449) @SherkeyXD
* 支持 XX-EX1 类型关卡名列表添加 @ABA2396
* 添加运行库安装脚本 @ABA2396

### 修复 | Fix

* StartButton1 ocrReplace syntax error in JP tasks.json @Constrat

### 文档 | Docs

* 更新文档 (#11430) @Rbqwow @SherkeyXD

### 其他 | Other

* MAA 启动相关改为全局配置 @ABA2396
* 界面设置改为全局配置 @ABA2396
* 热键改为全局配置 @ABA2396

## v5.11.0

### 没有摸鱼 | Highlight

本次更新，我们将精力放在优化上面，Windows 和 MacOS 版的用户应该能感觉到<del>一丢丢的</del>流畅度提升，其他方面的改动如下：

#### 其他

1. 由于近期游戏版本更新，在当前理智不足 45 点时，碎石后不会超过理智上线，因此葛朗台模式在检测到碎石理智不溢出时不再等待下一跳理智；
2. 这个版本我们更新了内置的保全派驻的荒地群兽音乐厅和多索雷斯在建地块的新作业；
3. 这个版本随附的游戏资源数据更新至【跨年欢庆·中坚#1225】。

### 新增 | New

* RA2 multi-crafting @Constrat
* RS-5 导航 @ABA2396
* add discord webhook external notification (#11373) @liang36176381
* 重新适配 DepotEnter 多界面主题 @ABA2396
* 更新 243 极限效率一天四换排班表（20241216 修订） @bodayw
* Mac GUI重构&多语言 (#11295) @hguandl
* SideStory「出苍白海」导航 (#11294) @SherkeyXD
* YostarJP manually add BP new operators (#11286) @Manicsteiner
* add BP new operators @Constrat
* YostarEN BP navigation @Constrat
* SSS#5 ocr replace for conductive elements @Constrat
* SSS#5 for global @Constrat
* 为选择线索添加判定 (#11323) @Alan-Charred @pre-commit-ci[bot]
* enhance MAA website animation for a more elegant appearance (#11303) @chisato2233

### 改进 | Improved

* 更新繁中服進入肉鴿的方式 (#11386) @momomochi987
* use API instead of gh cache @Constrat
* Revert "chore: 改进Python脚本中incremental_path参数说明" @ABA2396
* WpfGui重构 拆分`一键长草-刷理智任务` (#11223) @status102
* 账号切换退出账号增加OCR (#11258) @Daydreamer114
* 优化葛朗台，碎石在理智不会溢出时不再等待 @status102
* 优化 StageManager 代码结构 (#11233) @ABA2396 @status102
* WpfGui重构 拆分`一键长草-领取奖励任务` (#11228) @status102
* WpfGui重构 拆分`一键长草-基建任务`任务 (#11219) @status102
* WpfGui重构 拆分`一键长草-获取信用及购物任务` (#11218) @status102
* 简化绑定 @status102
* WpfGui重构 拆分`一键长草-自动肉鸽` (#11201) @status102
* 优化参数解析 @ABA2396
* WpfGui重构 拆分`设置-远程控制` (#11202) @status102
* WpfGui重构 拆分`设置-定时执行` (#11192) @status102
* WpfGui重构 部分拆分`设置-关于` (#11330) @status102
* WpfGui重构 拆分`设置-性能设置` (#11329) @status102
* 更新保全派驻荒地群兽音乐厅，多索雷斯在建地块新作业 (#11318) @Saratoga-Official @pre-commit-ci[bot]
* WpfGui重构 拆分`一键长草-公招任务` (#11311) @status102
* 优化仓库识别显示，减少读取文件次数，优化文件占用 @ABA2396

### 修复 | Fix

* “启动 MAA 后直接最小化”选项功能失效 @ABA2396
* RA2 crafting list fix e1177db34895254ab068f1b8462445466836b93b @Constrat
* Mac GUI 界面改进和修复 (#11422) @hguandl
* crop ClueSelected template to make sure it works under long clue names (#11409) @Alan-Charred @pre-commit-ci[bot]
* RA删除存档时重复识别 (#11406) @Daydreamer114
* git init to allow gh commands @Constrat
* GH_TOKEN for smoke testing + fail_fast @Constrat
* 卡达，玛露西尔基建技能效率错误 @ABA2396
* resource updater txwy second error from intern-kun @Constrat
* txwy gamedata xaml typo intern-kun fucked up [again] @Constrat
* 修复肉鸽选难度偶现识别错误 @ABA2396
* 生息演算无存档刷点模式，允许在确认离开当前区块后没有 Loading Text 的情况下等待 (#11381) @Alan-Charred
* Try to fix the compilation failure on aarch64 (#11378) @Roland125
* powershell instead of bash @Constrat
* precommit for blame-ignore + bypass on artifact missing @Constrat
* ci release nightly ota if condition @Constrat
* 萨卡兹肉鸽点刺成锭分队开局思维阻塞 (#11276) @Daydreamer114
* missing battle_data BP event @Constrat
* 双击托盘图标可能导致窗口位置错误 @ABA2396
* Mac GUI日志&肉鸽难度设置 (#11273) @hguandl
* 为 ios端降低 Sarkaz@Roguelike@DecreaseBurdenAbandonThought 任务模版匹配的 threshold (#11266) @Alan-Charred
* 干员识别中出现卫戍中的 4★ 预备干员 及 特有 6★ @ABA2396
* Mac GUI 优化作业列表 (#11250) @hguandl
* 新下载的 MAA 会在加载关卡时报错 @ABA2396
* 繁中服無法訪問好友 (#11248) @momomochi987 @pre-commit-ci[bot]
* macOS 肉鸽参数&客户端包名 (#11245) @hguandl
* UI 更新后自动填充有概率拉取其他设施内干员 @ABA2396
* 避免非预期的任务参数设置检查 (#11232) @status102
* 肉鸽构想不足过高阈值导致 mac 版无法识别 @Daydreamer114
* 切换配置后不会自动启动 @ABA2396
* API 中被删除的已关闭活动关卡被误判为常驻关卡 @ABA2396
* 刷理智失败次数限制未生效 @ABA2396
* 修改邮件通知中特殊字符的处理方式 (#11169) @LogicDX342 @status102
* 访问好友在部分背景下无法进入 @ABA2396
* 命令行切换配置后弹出公告 @ABA2396
* 加载顺序 @ABA2396
* 使用命令行切换配置时有可能使关卡选择被覆盖 @ABA2396
* IS2 missing templates for recruiting and obtaining relics (#11196) @nnnn1111 @pre-commit-ci[bot]
* not entering recruitment when in mall with recruitment permit item on screen @Constrat
* 荒芜拉普兰德 OcrReplace (#11184) @Saratoga-Official
* 设置Xcode版本为15.3 (#11298) @hguandl
* Shu EN regex @Constrat
* YostarKR improved StageBoskyPassageEnter recognition score (#11351) @HX3N
* CheckComboBox 初始化报错 @ABA2396
* SSS recruitment flag EN @Constrat
* YostarKR improved StageMysteriousPresageEnter recognition score (#11336) @HX3N @pre-commit-ci[bot]
* 网络卡顿导致领邮件失败 (#11333) @Daydreamer114
* 进入去伪存真后识别太快导致错过 close collection 识别 @Daydreamer114
* 小工具-公招识别 绑定失效 @status102
* 增加 Tales@RA@ResourceGained 任务的 roi (#11322) @Alan-Charred
* YostarJP MedicineExpiringTime ocr fix (#11307) @Manicsteiner
* 将萨卡兹肉鸽 CloseCollection 的 Continue 和 Close 改为 RGBCount (#11302) @Daydreamer114
* 仓库识别无法识别 ”万“ @ABA2396
* 移除多余的阻止休眠设置选项 (#11301) @Rbqwow

### 文档 | Docs

* 更新 README (#11398) @ABA2396 @pre-commit-ci[bot]
* 任务流程协议 管理员权限FAQ 牛牛监控 (#11375) @Rbqwow @ABA2396
* Auto Update Changelogs of v5.11.0-beta.1 (#11297) @github-actions[bot]
* 修改常见问题中的下载/安装问题的描述 (#11229) @ClozyA
* Auto Update Changelogs of v5.11.0-beta.2 (#11367) @github-actions[bot]
* 新手上路增加非 Windows 指引；macOS 和 Android 实体设备添加说明 (#11227) @Daydreamer114 @ABA2396 @Rbqwow
* 增加Mac模拟器：蓝叠模拟器air版 (#11349) @MikeQu223

### 其他 | Other

* tw OcrReplace 添加关卡内道具 (#11431) @Daydreamer114
* bump maa-cli to 0.5.2 (#11428) @wangl-cc
* YostarKR HE rerun navigation @HX3N
* smoke-testing bash -> powershell @Constrat
* Translated using Weblate (Chinese (Traditional)) @momomochi987
* 適配繁中服「圍攻行動」主題介面 @momomochi987
* 繁中服「水晶箭行動」活動導航 @momomochi987
* suppress false positive warning of gcc12 @horror-proton
* 添加 python 资源加载示例 @ABA2396
* Revert "feat: DepotEnter适配界面主题 (#7145)" @ABA2396
* Revert "chore: 在Python脚本中添加活动关导航下载与加载的示例 (#3916)" @ABA2396
* 移除设置指引的 skip @ABA2396
* revert: changes from blame-ignore test @Constrat
* Revert "fix: ci release nightly ota if condition" @Constrat
* Release v5.11.0-beta.2 (#11366) @ABA2396
* Release v5.11.0-beta.1 (#11299) @ABA2396
* Release v5.11.0-beta.1 (#11291) @ABA2396
* remove Yostar servers reclamation algorithm temp edits @Manicsteiner
* 管理员权限启动无法使用开机自启 (#11290) @ABA2396 @momomochi987
* 因为资源版本上报失败时添加提示 @ABA2396
* YostarKR add BP new operators (#11285) @HX3N
* 移除多余代码 @ABA2396
* YostarKR BP navigation (#11279) @HX3N
* YostarJP BP navigation @Manicsteiner
* Translations update from MAA Weblate (#11255) @AlisaAkiron
* WpfGui`游戏设置`修改为`运行设置` (#11209) @status102 @Constrat
* EN tweaks [skip ci] @Constrat
* 通过控制台退出模拟器时不再kill端口 @ABA2396
* YostarJP ocr replace 魔王 (#11222) @Manicsteiner
* 成功代理不再减少任务失败计数 @ABA2396
* 移除过渡代码 @ABA2396
* YostarKR SSSBuffChoose ocr fix @HX3N
* automatically remove spaces for YoStarKR in CharsNameOcrReplace (#11205) @Constrat
* YostarKR SSS#5 ocr added (#11200) @HX3N
* 拆分 设置-界面设置 (#11188) @status102
* 繁中服保全派駐的新定向導能元件 @momomochi987
* 拆分 设置-游戏设置 (#11185) @status102
* rename `ExternalNotificationDataContext` to `ExternalNotificationSettings` @status102
* rename `VersionUpdateDataContext` to `VersionUpdateSettings` @status102
* add YostarJP ocrReplace 黑角 (#11350) @liang36176381 @Manicsteiner
* ci/nightly-schedule (#11317) @Constrat @pre-commit-ci[bot]
* 简化完成后动作UI绑定 (#11316) @status102
