## v5.15.0

### Highlight

本次更新我们带来了全新的导航系统与视觉体验升级，并持续优化错误反馈机制，让牛牛更聪明更贴心！

#### 主线导航系统更新

国服新版 UI 导航系统已实装适配（注：第 15 章与 OF-1 因关卡结构特殊暂未支持导航）。现在牛牛能更精准识别主界面入口逻辑，自动作战时遇到路径变动也不会迷路啦~

#### 视觉体验升级

全新【背景设置】面板现已加入豪华套餐！你可以：

- 自定义壁纸并实时显示效果
- 调节背景透明度打造朦胧美
- 开启高斯模糊获得毛玻璃质感

（悄悄说：设置路径在【设置】→【背景设置】，快去试试吧！）

#### 信息反馈优化

新增 MirrorChyan 镜像服务的错误代码提示。SMTP 邮件通知也已支持 HTML 彩色标记，可以在邮件里显示和 MAA 中一样的信息咯~

#### 关键问题修复

- 修复了第15章自动战斗失效的小 bug
- 会客室现在可以愉快地赠送线索啦~
- 肉鸽难度选择界面显示逻辑更加清晰直观

----

以下是详细内容：

### 新增 | New

* 支持新 UI 主线导航 (#12278) @ABA2396
* 添加控件半透明背景色，新增自定义背景 (#12204) @ABA2396 @status102
* 新增 MirrorChyan ErrorCode 提示 (#12202) @ABA2396
* 调整下载来源提示 (#12156) @MistEO
* Mac 任务列表支持多个开始唤醒，以实现多账号 (#12144) @hguandl
* Telegram Topic notification (#12188) @Constrat
* YoStarEN Sarkaz theme + JP and KR template optimization @Constrat
* SSS#6 for EN (#12168) @dragonheart107
* EN Siege Alter gamedata @Constrat
* YostarEN/JP/KR GO navigation @Constrat @Manicsteiner @HX3N

### 改进 | Improved

* 资源更新换一个解压目录 (#12153) @MistEO
* 自动战斗开始前仅移除相似的召唤物头像缓存 @status102
* 优化肉鸽难度选择显示 @ABA2396

### 修复 | Fix

* 自动战斗-战斗列表 突袭难度切换失效 @status102
* wpf 外服资源加载错误 @status102
* 修复资源/肉鸽导航 (#12266) @ABA2396
* 保全开始部署 点击过快导致点击无效 (#12185) @Daydreamer114
* 自动战斗在开启战斗列表时导入作业后，关闭战斗列表开始任务时作业使用错误 @status102
* 15章 自动战斗 @ABA2396
* 钼铅识别错误 @ABA2396
* 线索翻页限制 10 次，超出次数或未识别到下一页强制给当页第一个好友送线索 @ABA2396
* mac下选择难度 999 导致卡死 (#12235) @Alan-Charred
* 修复宿舍填充干员时不经过检测盲点首位干员的问题 (#12234) @Alan-Charred
* 繁中服生息演算無法讀檔 (#12230) @momomochi987
* CheckLevelMax ocr target "-" -> digits (#12223) @BxFS
* YostarKR lower StartToVisit templThreshold (#12193) @HX3N
* SSS#6 directional EC and branches @Constrat
* 繁中服_生息演算組裝道具後卡住 (#12182) @momomochi987 @Daydreamer114

### 文档 | Docs

* 添加ci文档 (#11924) @SherkeyXD @Rbqwow
* 调整新手上路步骤顺序 (#12228) @Rbqwow
* add space @MistEO

### 其他 | Other

* handle leak @dantmnf
* 开始作战 @ABA2396
* 自动更新 @hguandl
* 控制中枢模板mask (#12177) @Daydreamer114
* warning SA1413 in Telegram notifications @Constrat
* xaml requires &#160; @Constrat
* update version.json on Arknights-Tile-Pos changes fix 43c672df541bae77b484c2faeb23c4522230e3ca @Constrat
* roguelike already left encounter check and return from map screen (#12088) @BxFS
* prettier formatting @Constrat
* revert infrast.json temporary @MistEO
* 外部通知 SMTP 与 Qmsg 输入框样式错误 @ABA2396
* 落了个 ResetRecruitVariables @ABA2396
* 其他地图数据修复 @status102
* 主线 15 章地图修复 @status102
* 反转右键效果时更新 tooltip 悬浮提示 @ABA2396
* check nightly version with mirrorchyan (#12173) @MistEO
* overseas (JP & KR) data (#12274) @Constrat
* 添加自定义任务，仅在 debug 模式下可见入口 @ABA2396
* 调整主任务勾选框显示 (#12283) @ABA2396
* Wpf CustomTask 序列化 (#12264) @status102
* 繁中服「生路」活動導航 (#12224) @momomochi987
* gpu 选项禁用滚轮切换 @ABA2396
* 移除过时的重连代码 @ABA2396
* 繁中服_聲浪安保派駐_定向導能元件 (#12174) @momomochi987
* Wpf肉鸽任务序列化 (#12124) @status102
* 添加一个临时定时 (#12243) @ABA2396
* 将密码框样式独立出来 @ABA2396
* 提前适应新 UI 的资源关卡和肉鸽图标 @ABA2396
* 给反转 CheckBox 加个样式 @ABA2396
* 添加背景模糊半径滑块 @ABA2396
* 添加反转主任务右键单击效果勾选项 @ABA2396
* 调整背景模糊默认半径 @ABA2396
* 可修改 gui.json 自定义 Background 模糊半径 @ABA2396
* YostarKR ocr and UI updates (#12284) @HX3N
* YostarJP clue and GUI updates (#12276) @Manicsteiner
* fix version time @MistEO
* update en-bug-report template @Daydreamer114
* YostarKR Sarkaz Theme (#12220) @HX3N
* YostarJP Sarkaz Theme (#12216) @Manicsteiner
* YostarJP 琳琅诗怀雅 ocr fix (#12219) @Manicsteiner
* update wording about mirrorchyan (#12208) @MistEO
* add delay pre UnlockClues @Constrat
* YostarKR SSS#6 BuffChoose (#12191) @HX3N
* update issue_template to remove new lines for EN @Constrat
* tweak SSS files for global @Constrat
* tweaked SSS copilot operators output @Constrat
* add space after `:` @Constrat
* YostarJP EnterInfrastDelicious (#12176) @Manicsteiner
* set output to UTF_8 for ResourceUpdater @Constrat
* manual update version @MistEO
* bump maa-cli to 0.5.4 (#12091) @wangl-cc
