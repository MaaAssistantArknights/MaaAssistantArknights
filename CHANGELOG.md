## v5.6.0-beta.2

### 新增 | New

* 外部通知支持多选 (#10395) @ABA2396
* 使用变换后的图像进行技能按钮识别 (#10293) @horror-proton
* 生息演算2刷开局清空编队干员 (#10359) @Daydreamer114
* add Qmsg notification (#10358) @octopusYan
* GetLocalizedNames for Infrast and Copilot output (#10335) @Constrat
* FC rerun navigation fix EN @Constrat
* 允许手动指定WPFGUI中干员名称显示语言 (#10310) @ABA2396 @Manicsteiner
* 公招错误时保存截图 @zzyyyl
* 保全作战保留core不过牌且放置core后回到待部署则重放工具人和core @Daydreamer114
* 保全作战相同location以json写的策略顺序执行 @Daydreamer114

### 改进 | Improved

* 全肉鸽招募适配娜仁图亚、艾拉 (#10385) @Daydreamer114
* 重构 FightSettingsUserControl (#10407) @ABA2396
* Mumu截图增强路径清空时不再检查路径是否存在 @status102
* duplicates templates from I.S. (#10376) @Constrat
* 优化外部通知界面显示 (#10363) @ABA2396
* 更新 bug issue 模板 (#10357) @Rbqwow
* 重构 OperBox 输出与显示 (#10320) @ABA2396
* 重构定时器和重启询问 (#10078) @ABA2396
* CopilotViewModel (#10099) @Manicsteiner
* Win10以上系统在退出时Wpf不再清除Toast (#10307) @status102
* 优化界面显示 @ABA2396
* smoking-test中肉鸽参数更新 @SherkeyXD

### 修复 | Fix

* OTA打包时对跳过的版本做删除处理 (#10020) @SherkeyXD
* Reclamation for YostarJP (#10414) @Manicsteiner
* 生息演算主题读取配置错误 @ABA2396
* 萨卡兹肉鸽多选招募券模板错误 @ABA2396
* 肉鸽编队检测在未触底时返回 true (#10396) @Alan-Charred
* DoDragDrop 拖动操作已在进行中 (#10368) @ABA2396
* insert delay after SquadConfirm @Constrat
* 使用匹配后偏移代替每日任务 @status102
* add ocrReplace for JP "Reclamation2CopiousCoppice" (#10362) @Daydreamer114
* 勾选启动MAA后直接最小化后点击隐藏托盘图标后无法显示MAA @ABA2396
* EN needs templates for clue exchange the number font is different, score too low @Constrat
* add delay after selecting clue @Constrat
* remove "" in nightly fix #10308 @Constrat
* roi 错误 @ABA2396
* SL 导航错误 @ABA2396
* 修复调试版本判断条件 @SherkeyXD
* `std::ranges::views::join` with LLVM clang 16 on darwin  (#10309) @Cryolitia
* 多配置下公告和更新日志显示异常 @ABA2396
* 修复保全战斗在core干员重复时只会放1次bug (#10306) @status102
* ProxyType 重启不生效 @ABA2396
* rev-list instead of rev-parse @Constrat
* revert to simple if @Constrat
* fetching depth 0 @Constrat
* impossiblity of fetch-depth modification. reverting + generic perfs @Constrat

### 文档 | Docs

* 贡献者头像添加 105 上限 (#10351) @MistEO
* git blame ignore @Constrat

### 其他 | Other

* 调用PowerManagement.Shutdown();后再次调用Bootstrapper.Shutdown(); @ABA2396
* 关机前尝试保存配置 @ABA2396
* 调整令牌关闭强度 @ABA2396
* 生息演算添加沙中之火选择项 @ABA2396
* 迁移公告相关配置 (#10399) @status102
* 第一次启动时默认不勾选肉鸽和生息演算 @ABA2396
* bump maa-cli to 0.4.12 (#10390) @wangl-cc
* 调整 check link 提示样式 @ABA2396
* 对comment中的未知链接进行提醒 (#10379) @IzakyL @ABA2396
* update ignore templates @Constrat
* 获取任务端口无效时不进行轮询 (#10321) @ABA2396
* use CsWin32 source generator instead of random pinvoke library (#10361) @dantmnf
* 删除子模块 @ABA2396
* remove MaaDeps submodule (#10354) @dantmnf
* RoguelikeRoutingTaskPlugin.h missing VS22 filter @Constrat
* bump zzyyyl/issue-checker from 1.8 to 1.9 @zzyyyl
* 公招识别拥有全干员时不显示未拥有干员数量 @ABA2396
* YostarJP ocr fix @Manicsteiner
* JP ZH-TW GPU option & reclamation translation @Manicsteiner
* KR GpuDeprecated translation @HX3N
* fix WPF Warning @SherkeyXD
* 修改过时的Binding方法 @SherkeyXD
* YostarJP FC navigation (#10316) @Manicsteiner
* 整理 tasks.json 中记录的肉鸽插件参数 (#10290) @Alan-Charred
* Revert "feat: 保全作战保留core不过牌且放置core后回到待部署则重放工具人和core" (#10300) @status102
* clearout git blame @Constrat
* MuMu12EmulatorPath Placeholder 添加示例提示 @ABA2396
* remove last checked commit @Constrat
* auto blame ignore @github-actions[bot]
* git blame added styling commits (#10283) @Constrat
* smoking-test添加领取奖励的测试 @SherkeyXD
* 移除tasks中的默认值 @SherkeyXD
