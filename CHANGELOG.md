## v4.16.0-beta.1

### 新增

- 牛牛抽卡！ @MistEO @hguandl
- 新增肉鸽刷指路鳞（请在设置中打开） @MistEO @hguandl
- 新增内置保全作业 雷神工业测试平台 浊蒂版，更稳定 (#4528) @junyihan233 @MistEO
- Mac UI 支持启动 iOS 客户端 @hguandl
- Mac UI 仓库结果导出 JSON @hguandl
- 干员 BOX 识别增加等级和精英度 (#4438) @GD-GK @MistEO

### 改进

- 优化资源加载速度 @MistEO @hguandl
- 大幅优化视频识别切片准确率 @MistEO
- 优化干员 BOX 识别准确率 @MistEO
- Vision 相关代码 超级大重构 @MistEO
- 优化保全雷神工业第五层部署逻辑 (#4503) @AnnoyingFlowers
- Win UI 添加模拟器检测错误日志输出、启动等待结束提示 @ABA2396
- Win UI Add InstanceManager (#4422) (#4486) @moomiji
- Win UI 脚本运行不阻塞 UI (#4422) (#4486) @moomiji
- Win UI 优化主界面布局 @ABA2396
- Win UI Do not save config on startup @MistEO
- Win UI 减慢弹出速率 @moomiji
- Mac UI 运行时防止系统进入睡眠 @hguandl
- Mac UI 启动前加载外服资源 @hguandl
- Mac UI 添加视频识别说明 @hguandl
- Auto Update Game Resources - 2023-04-27 @MistEO

### 修复

- 修复 Win UI Changelog 弹窗 (#4422) (#4486) @moomiji
- 修复剿灭模式下任务失败时备选关卡为未在列表的主线关卡被跳过的问题 @ABA2396
- 修复主线导航 ocrReplace 错误 @ABA2396
- Mac UI 修复自定义基建文件权限 @hguandl
- 修复任务还未结束时就显示 Link Start @ABA2396
- 部分肉鸽地图无法正确部署 (#4522) @LingXii
- 再次修复肉鸽“瞻前顾后”关卡名识别错误 @MistEO
- 修复识别工具失去焦点重置界面问题 @ABA2396
- 修复启动时自动开启模拟器后无法自动停止 @ABA2396
- 修复只自动开启模拟器时停不下来的情况 @moomiji
- 修复二次点击 StartButton1 时误触 StartButton2 @ABA2396
- Mac UI 修复干员识别适配 @hguandl
- 修复路径检查错误 @ABA2396
- Win UI SettingsView displayed incompletely @moomiji
- 修复开启 `启动MAA后自动开启模拟器` 报错 @ABA2396
- perf: TryToStartEmulator 执行不更改按钮状态 #4533 @ABA2396
- 修复开始唤醒任务提前结束，导致基建等任务点进商店的问题 @MistEO
- 修复 mac 上访问好友任务出错

### 其他

- Win UI 同时加载 API @ABA2396
- 更新一图流上传接口 @MistEO
- 从 maa-website 转移文档 (#4545) @horror-proton
- add r2 release mirror @GalvinGao @MistEO
- Auto Tag Release PR (#4531) @MistEO
- 升级 macOS 编译环境，清理多余依赖  @hguandl
- 代码清理 @MistEO @hguandl @ABA2396
- 修复 Win UI 绑定错误 @ABA2396

### For overseas

#### Common

- Added CI that automatically updates game resources, including all overseas clients resources (but needs to be manually triggered). If you want to create a pull request to update game resources, you can use \<your forked repo\> -> `Actions` -> `res-update-game` -> `Run workflow` to update resources instead of manual update. @MistEO
- Fix the recognition error of recruiting senior operators @MistEO
- fix #4471: Add loading instructions for overseas clients resource files @hguandl

#### YostarJP

- Doc : Add EPISODE11 Depot (#4506) @wallsman
- Doc : JP Update EPISODE11「淬火煙塵」 (#4504) @wallsman

#### YostarEN

- Resolve recruit failure for Supporter @hguandl

#### txwy

- 繁中服 傀影肉鴿道具 (#4491) @vonnoq
- 繁中服 保全物件/傀影關卡識別 (#4489) @vonnoq

### For develops

**由于我们的文档正在迁移，为避免造成更多的 conflicts，近期新接口还请参考 Win UI 或 Mac UI 集成逻辑**

- 新增抽卡功能，[集成参考](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/f644f8c548568220b61955c0369c9ac6c7bceec4/src/MaaWpfGui/Main/AsstProxy.cs#L1662)  @MistEO
- 肉鸽新增刷 `指路鳞` 选项，[集成参考](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/f644f8c548568220b61955c0369c9ac6c7bceec4/src/MaaWpfGui/Main/AsstProxy.cs#L1588), [回调参考](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/f644f8c548568220b61955c0369c9ac6c7bceec4/src/MaaWpfGui/Main/AsstProxy.cs#L774) @MistEO
- 干员 BOX 识别，潜能 改成 1-6 @MistEO
- 新增自动更新游戏资源 CI: `res-update-game` @MistEO
- Add Debug configuration (#4474) @dantmnf @MistEO
