## v5.9.0

### 新增 | New

* 可自定义3~5星招募时间 (#11019) @ABA2396
* 点刺成锭分队，刷钱 (#10901) @Alan-Charred @status102
* 支持肉鸽选难度 (#10918) @ABA2396 @Alan-Charred @status102
* SideStory「揭幕者们」 @ABA2396
* 支持最新的幸运墙任务 (#11008) @SherkeyXD
* Mac GUI 支持手动资源更新 (#11036) @hguandl
* new tag for recruitment @Constrat
* MiningActivity for YostarEN @Constrat
* Special Access YostarEN @Constrat

### 改进 | Improved

* 适配萨卡兹内容拓展Ⅰ (#10976) @Daydreamer114
* 优化模拟器最小化启动 @ABA2396
* 资源更新失败时弹出吐司通知 @ABA2396
* 调整关卡选择更新逻辑 (#10913) @ABA2396
* 优化全主线关卡导航 @ABA2396
* 自动公招时间属性设置优化 (#11025) @status102
* 优化关卡列表更新 (#11022) @ABA2396

### 修复 | Fix

* 跨日更新崩溃 @ABA2396
* 修复手动关闭或闪退后，无法截图到新窗口 @ABA2396
* 为生息演算无存档刷点数模式最后的删除存档任务添加重试机制 (#10965) @Daydreamer114
* 快速编队向左滑动后等待可能的画面回弹 @Alan-Charred
* MuMu 网络桥接模式无法使用截图增强 (#10937) @Alan-Charred @ABA2396
* 肉鸽偏后的分队选错 @ABA2396
* 无法按信赖排序基建 @ABA2396
* 肉鸽难度默认值错误 @ABA2396
* 公招时间默认值错误 @ABA2396
* 修改基建 InfrastOperListTab 部分的 roi (#10999) (#11001) @Windsland52
* 基建换班宿舍任务期间重进宿舍后，重置过滤状态记录 (#11007) @status102
* 替換繁中服「跳過」圖片 (#10991) @vonnoq
* OCR for Wisadel EN @Constrat
* change roi for MiningActivities @Constrat
* YostarEN regex for new operators @Constrat
* Special Access not claiming @Constrat

### 文档 | Docs

* 更新 Feature request 模板 (#10971) @Saratoga-Official
* 补充连战次数文档 (#11028) @Rbqwow
* 运行库问题 (#10950) @Rbqwow @ABA2396
* MuMu 网络桥接 (#10953) @Rbqwow @ABA2396
* 移除赘字 (#10977) @1lyvianis

### 其他 | Other

* 肉鸽难度不可用 / 不切换时禁用插件 (#10993) @status102
* 优化互斥锁 @ABA2396
* 导航错误 @ABA2396
* MaaCore RoguelikeTask 补充未选择刷开局模式下启用凹精二的禁用判定 @status102
* 优化互斥锁描述 @ABA2396
* 修复对 expected_collapsal_paradigms 参数支持 (#10978) @Alan-Charred @ABA2396
* 1234567890ABCDEF 给爷爬 @ABA2396
* 理智识别的理智上限校验修改 @status102
* revert ede078f to fix b28ffcc for EN @Constrat
* revert fd959ae34154b6a1847f09b2778a9e9f7b800cf3 @Constrat
* 为生息演算无存档刷点模式，点击区域节点任务加上 1 秒的 postDelay (#11035) @Alan-Charred
* UpdateStageList 只在 ui 线程执行 @ABA2396
* typo @ABA2396
* 添加一键长草任务进度条 @ABA2396
* 支持状态栏显示进度条 @ABA2396
* 添加直接显示吐司内容函数，固定Show执行线程 @ABA2396
* AbstractTask新增附加已有插件、插件查找 @status102
* 繁中服「銀心湖列車」活動導航 (#10987) @momomochi987
* YostarJP EP14 ocr update (#10986) @Manicsteiner
* 对 gui.json 排序 @ABA2396
* 修改版本不一致描述，提前 return 位置 @ABA2396
* 设置代理时提高github优先度 @ABA2396
* 移除多余变量 @ABA2396
* remove test @ABA2396
* YostarJP EP14 preload (#10954) @Manicsteiner
* Translations update from MAA Weblate (#10944) @AlisaAkiron
* 部分中文与英文中追加间隔；修正理智识别的理智上限注释 (#10945) @weinibuliu @status102
* 调整切换语言显示 @ABA2396
* changelog_generator @status102
* ChangelogGenerator检查commit message中的skip changelog是否为自动追加 (#11016) @status102
* tools: update validator.ps1 to return to starting directory after ending call @Constrat
* remove duplicates inside CharsNameOcrReplace for Yostar & txwy @Constrat
* 修改延迟时间 [skip changelog] @ABA2396
* Revert "chore: PV 关卡地图数据 (#11003)" @status102
* YostarKR EP14 update (#10988) @HX3N
* PV 关卡地图数据 (#11003) @status102
* 分钟取整 @ABA2396
* 修改幸运墙逻辑 @ABA2396
