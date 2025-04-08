## v5.15.0

### 新增 | New

* EN Siege Alter gamedata @Constrat
* 支持新 UI 主线导航 (#12278) @ABA2396
* overseas (JP & KR) data (#12274) @Constrat
* 手动更新资源 @MistEO
* 添加控件半透明背景色，新增自定义背景 (#12204) @ABA2396 @status102
* YoStarEN Sarkaz theme + JP and KR template optimization @Constrat
* 新增 MirrorChyan ErrorCode 提示 (#12202) @ABA2396
* Telegram Topic notification (#12188) @Constrat
* 任务列表支持多个开始唤醒，以实现多账号 (#12144) @hguandl
* SSS#6 for EN (#12168) @dragonheart107
* check nightly version with mirrorchyan (#12173) @MistEO
* 调整下载来源提示 (#12156) @MistEO
* 将背景设置独立出来 (#12254) @ABA2396
* 添加自定义任务，仅在 debug 模式下可见入口 @ABA2396
* YostarEN GO navigation preload @Constrat
* YostarJP GO navigation (#12244) @Manicsteiner
* YostarKR GO navigation (#12242) @HX3N

### 改进 | Improved

* 孤星搓玉关 @Daydreamer114
* 资源更新换一个解压目录 (#12153) @MistEO
* 自动战斗开始前仅移除相似的召唤物头像缓存 @status102
* 反转右键效果时更新 tooltip 悬浮提示 @ABA2396
* 优化肉鸽难度选择显示 @ABA2396

### 修复 | Fix

* 自动战斗-战斗列表 突袭难度切换失效 @status102
* 15章自动战斗 @ABA2396
* 15章 自动战斗 @ABA2396
* 开始作战 @ABA2396
* 其他地图数据修复 @status102
* 主线15章地图修复 @status102
* 修复资源/肉鸽导航 (#12266) @ABA2396
* 自动更新 @hguandl
* resource @MistEO
* wpf外服资源加载错误 @status102
* prettier formatting @Constrat
* revert infrast.json temporary @MistEO
* warning SA1413 in Telegram notifications @Constrat
* Debug TaskName 忘保存了 @ABA2396
* CheckLevelMax ocr target "-" -> digits (#12223) @BxFS
* handle leak @dantmnf
* YostarKR lower StartToVisit templThreshold (#12193) @HX3N
* 保全开始部署 点击过快导致点击无效 (#12185) @Daydreamer114
* 自动战斗在开启战斗列表时导入作业后，关闭战斗列表开始任务时作业使用错误 @status102
* SSS#6 directional EC and branches @Constrat
* 控制中枢模板mask (#12177) @Daydreamer114
* 繁中服_生息演算組裝道具後卡住 (#12182) @momomochi987 @Daydreamer114
* xaml requires &#160; @Constrat
* 钼铅识别错误 @ABA2396
* update version.json on Arknights-Tile-Pos changes fix 43c672df541bae77b484c2faeb23c4522230e3ca @Constrat
* roguelike already left encounter check and return from map screen (#12088) @BxFS
* 线索翻页限制 10 次，超出次数或未识别到下一页强制给当页第一个好友送线索 @ABA2396
* 外部通知 SMTP 与 Qmsg 输入框样式错误 @ABA2396
* 落了个 ResetRecruitVariables @ABA2396
* mac下选择难度 999 导致卡死 (#12235) @Alan-Charred
* 修复宿舍填充干员时不经过检测盲点首位干员的问题 (#12234) @Alan-Charred
* 繁中服生息演算無法讀檔 (#12230) @momomochi987

### 文档 | Docs

* Auto Update Changelogs of v5.15.0-beta.1 (#12222) @github-actions[bot] @ABA2396
* add space @MistEO
* add space @MistEO
* Auto Update Changelogs of v5.15.0-beta.2 (#12262) @github-actions[bot] @ABA2396
* 添加ci文档 (#11924) @SherkeyXD @Rbqwow
* 调整新手上路步骤顺序 (#12228) @Rbqwow

### 其他 | Other

* 调整主任务勾选框显示 (#12283) @ABA2396
* YostarKR ocr and UI updates (#12284) @HX3N
* YostarJP clue and GUI updates (#12276) @Manicsteiner
* Revert "fix: 15章 自动战斗" @ABA2396
* fix version time @MistEO
* Wpf CustomTask序列化 (#12264) @status102
* Release v5.15.0-beta.2 (#12261) @ABA2396
* Release v5.15.0-beta.1 (#12213) @ABA2396
* 繁中服「生路」活動導航 (#12224) @momomochi987
* update en-bug-report template @Daydreamer114
* YostarKR Sarkaz Theme (#12220) @HX3N
* gpu 选项禁用滚轮切换 @ABA2396
* YostarJP Sarkaz Theme (#12216) @Manicsteiner
* YostarJP 琳琅诗怀雅 ocr fix (#12219) @Manicsteiner
* update wording about mirrorchyan (#12208) @MistEO
* add delay pre UnlockClues @Constrat
* YostarKR SSS#6 BuffChoose (#12191) @HX3N
* update issue_template to remove new lines for EN @Constrat
* 移除过时的重连代码 @ABA2396
* tweak SSS files for global @Constrat
* tweaked SSS copilot operators output @Constrat
* add space after `:` @Constrat
* YostarJP EnterInfrastDelicious (#12176) @Manicsteiner
* 繁中服_聲浪安保派駐_定向導能元件 (#12174) @momomochi987
* set output to UTF_8 for ResourceUpdater @Constrat
* manual update version @MistEO
* Revert "fix: roguelike already left encounter check and return from map screen (#12088)" @status102
* Wpf肉鸽任务序列化 (#12124) @status102
* 添加一个临时定时 (#12243) @ABA2396
* 将密码框样式独立出来 @ABA2396
* bump maa-cli to 0.5.4 (#12091) @wangl-cc
* 提前适应新 UI 的资源关卡和肉鸽图标 @ABA2396
* 给反转 CheckBox 加个样式 @ABA2396
* 添加背景模糊半径滑块 @ABA2396
* 添加反转主任务右键单击效果勾选项 @ABA2396
* 调整背景模糊默认半径 @ABA2396
* 可修改 gui.json 自定义 Background 模糊半径 @ABA2396
