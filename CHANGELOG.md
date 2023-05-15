## v4.17.0-beta.1

### 新增

- 再次微调界面布局 @MistEO
- 调整设置界面布局 @MistEO
- 添加同路径互斥锁 @ABA2396
- 标题显示卡池信息 @ABA2396
- 支持蓝叠hyperV使用自动检测 @ABA2396
- 为ToolTip加上指引 @moomiji
- 添加 MuMuEmulator12 config @ABA2396
- 支持直接读取蓝叠模拟器Hyper-V版本非多开模式下使用的核心 (#4673) @SherkeyXD
   - 支持从bluestacks.conf中读取模拟器核心编号 @SherkeyXD
   - 完善对蓝叠模拟器多开功能的说明 @SherkeyXD
   - 更新README中新UI相关信息 @SherkeyXD
- 添加 MuMu12 检测连接与控制台退出模拟器 @ABA2396

### 改进

- 优化设置选项 @MistEO
- 优化设置界面 @MistEO
- 优化界面布局及引导设置 @MistEO
- 优化一些界面布局 @MistEO
- 改进HandyControl控件显示效果 (#4731) @moomiji
   - 修复点击具有焦点的数值选择控件按钮时文本不变的情况 @moomiji
   - modify messagebox design @moomiji
   - add messagebox design from HandyControl @moomiji
   - tidy up odds and ends of Themes / Styles @moomiji
- 冒烟 CI 尝试使用 nuget 缓存 @MistEO
- release-nightly-ota.yml 早点把 tag 推上去 @MistEO
- res-update-game.yml 别一直报错了（ @MistEO
- 使用 OtaAPI 检查版本更新 @MistEO
- 自动更新器添加版本号 @MistEO
- 优化提示 @ABA2396
- 重构任务设置界面布局 (#4556) @moomiji
   - 调整文档任务设置相关描述 @moomiji
   - 设置指引 @moomiji
   - unexpected action target in task settings @moomiji
   - add advanced task settings @moomiji
   - 移除设置界面中的重复设置 @moomiji
   - 重构任务设置界面布局 @moomiji
   - 任务设置Demo @moomiji
- 资源更新器错误 @MistEO
- 打开自动更新资源，并且只在主仓库跑 (#4661) @MistEO

### 修复

- pr-auto-tag.yml @MistEO
- smoke-testing.yml 缓存 @MistEO
- 尝试修复版本更新后因互斥锁遇到的问题 (#4754) @ABA2396
   - 尝试修复版本更新后因互斥锁遇到的问题 @ABA2396
- 修复一些CI权限问题 @MistEO
- 尝试修复发版不触发 version api update 的问题 @MistEO
- res-update-game.yml 修复步骤名 @MistEO
- 解决guard和supporter字段不识别的问题 (#4735) @AnnoyingFlowers
   - 解决guard和supporter字段不识别的问题 @AnnoyingFlowers
- XAML Intellisense unable to load resources stored in ResourceDictionary @moomiji
- avatar saved at wrong path in set_avatar() (#4724) @lengyanyu258
   - avatar saved at wrong path in set_avatar() @lengyanyu258
- 修复语言资源错误 @ABA2396
- 修复语言资源错误 @ABA2396
- 修复部分模拟器多开时进程获取错误的问题 @MistEO
- res-update-game.yml @MistEO
- 修正 Android 12 上无法结束游戏进程的问题 (#4685) @AnnAngela
   - 修正 Android 12 上无法结束游戏进程的问题 @AnnAngela
- 继续修复碎石后加载时间过长导致任务提前中止的问题 @zzyyyl
- 修复碎石后加载时间过长导致任务提前中止的问题 @zzyyyl
- 修复资源更新器 @MistEO
- 修复繁中服资源更新错误 @MistEO

### 其他

- Auto Update Changelogs of v4.17.0-beta.1 (#4766) @github-actions[bot]
- revert: 暂时删除WPF单例锁 @MistEO
- 调整设置界面 (#4732) @moomiji
   - 调整任务设置界面 @moomiji
   - 还原文档中 任务设置 - 开始唤醒 的描述 @moomiji
   - 调整部分连接设置至双入口 @moomiji
- Auto Update Game Resources - 2023-05-15 @MistEO
- revert: smoke testing @MistEO
- fix workflow triggering issue for resource updater @MistEO
- fix workflow triggering issue @horror-proton
- Fix getting stuck in RIIC for KR (#4749) @178619
   - Fix getting stuck in RIIC for KR @178619
- revert: 暂时删掉进程互斥锁 @MistEO
- 重写记住主界面位置功能 (#4666) @moomiji
   - 兼容旧配置 @moomiji
   - 修复贴靠的窗口位置保存不正确的问题 @moomiji
   - 重写记住主界面位置功能 @moomiji
- 修改部分日志提示 @ABA2396
- Use built-in type alias @ABA2396
- 空白任务设置页里放牛牛.ico @moomiji
- 修改 ToolTip 指引描述 @moomiji
- 修改英语界面翻译 @ABA2396
- 修改替换 adb 报错解决方案描述 @ABA2396
- update linux tutorial @horror-proton
- UI improvements (#4582) @dantmnf
   - close MainWindow instead of shutdown @dantmnf
   - move IoC initialization out of RootViewModel @dantmnf
   - move confirm exit logic to TaskQueueViewModel @dantmnf
   - cleanup MessageBoxHelper @dantmnf
   - confirm exit if task is running @dantmnf
   - use TaskDialog for native MessageBox @dantmnf
   - remove hardcoded language @dantmnf
   - set config language on all FrameworkElement @dantmnf
- Auto Update Game Resources - 2023-05-08 @MistEO
- revert: "fix: res-update-game.yml" @MistEO
- Update SSS_约翰老妈在建地块_浊蒂水陈版.json (#4686) @junyihan233
   - Update SSS_约翰老妈在建地块_浊蒂水陈版.json @junyihan233
- 调整设置界面显示 @moomiji
- 修正战斗流程协议链接 @ABA2396
- 修正战斗流程协议链接 (#4677) @ntgmc
- 修改自动战斗小提示 @ABA2396
- 让VS可以补全头文件 @MistEO
- 文档补充 maadeps-download.py 位置 (#4672) @niuden
- auto gen changelogs 仅在非草稿状态下生成 (#4660) @MistEO
