## v4.17.0

### 改进

- 设置布局大重构 ✿✿ヽ(°▽°)ノ✿ @moomiji @ABA2396 @MistEO
- 版本更新大优化 ✿✿ヽ(°▽°)ノ✿ @MistEO
- 调整 Hyper-V 蓝叠 地址检测逻辑，需勾选 `自动检测` + `每次检测` @MistEO @ABA2396 @SherkeyXD
- ADB 替换失败则临时用本地的 @MistEO
- 添加 MuMu12 检测连接与控制台退出模拟器 @ABA2396 @MistEO
- 标题栏显示卡池信息 @MistEO @ABA2396
- UI improvements (#4582) @dantmnf
- 优化保全派驻内置作业 SSS_约翰老妈在建地块_浊蒂水陈版.json (#4686) @junyihan233

### 修复

- 修正 Android 12 上无法结束游戏进程的问题 (#4685) @AnnAngela
- 修复碎石后加载时间过长导致任务提前中止的问题 @zzyyyl
- 修复关卡 `消息传递`、`窒息` 识别错误 (#4781) @cenfusheng
- 解决抄作业guard和supporter字段不识别的问题 (#4735) @AnnoyingFlowers
- XAML Intellisense unable to load resources stored in ResourceDictionary @moomiji
- avatar saved at wrong path in set_avatar() (#4724) @lengyanyu258
- 更新自动战斗点赞反馈提示语 (#4776) @UniMars
- 为 Resource loader 添加重入锁 @MistEO
- 修复 CI release 不更新 version api 的问题 @MistEO

### 其他

- ~~小水管爆了~~ 不再从 OTA 站下载更新包 @MistEO
- 优化并修复一些 CI 的错误 @MistEO @horror-proton
- 冒烟测试添加所有外服资源检查 @MistEO
- 冒烟测试的 nuget 添加重试 @MistEO
- 更新文档、界面文字等 @MistEO
- 文档补充 maadeps-download.py 位置 (#4672) @niuden
- 添加关于 MuMu 模拟器 12 的说明 (#4793) @AnnAngela
- Doc : JP シラクザーノ (#4782) @wallsman

### For overseas

#### YostarKR

- Fix getting stuck in RIIC for KR (#4749) @178619

### For develops

- update linux tutorial @horror-proton
- 让VS可以补全头文件 @MistEO
- 新增 [version api](https://ota.maa.plus/MaaAssistantArknights/api/version/) @MistEO
