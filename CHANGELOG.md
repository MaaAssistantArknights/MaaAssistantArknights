## v5.11.2

### 新增 | New

* 点刺、后勤种子存钱 & ProcessTask 添加 Input 方法 (#11521) @Daydreamer114 @ABA2396
* 肉鸽满级自动停止选项 (#11466) @BxFS @Constrat @momomochi987 @status102
* 为肉鸽开始探索添加 cd 识别 (#11443) @Daydreamer114
* 萨卡兹肉鸽冰川期作战策略 @Daydreamer114
* 萨卡兹内容拓展II点刺进入商店获得构想 (#11509) @Daydreamer114
* 不自动招募1/5/6星干员时，不计入最大确认招募次数 (#11380) @Roland125 @horror-proton
* 干员识别排除当前客户端未出干员 @ABA2396
* 肉鸽开局干员列表排除当前客户端未出干员 @ABA2396

### 改进 | Improved

* 新增投掷手干员组并调整优先级 @Daydreamer114
* 优化傀影肉鸽雪山上的来客ew部署 (#11195) @Daydreamer114
* manual recursion + robocopy for smoke-testing (#11458) @Constrat
* implement cache for smoke-test (#11457) @Constrat

### 修复 | Fix

* 肉鸽烧热水没烧出来会从预设难度开始，而不是返回n0 @ABA2396
* 删除傀影肉鸽远方来客意义不明的撤退 (#11194) @Daydreamer114
* delay and retry downloads on resource updater (#11504) @Constrat
* use read/write secret to delete cache on pr merge @Constrat
* 博朗台计算等待时间失败数据处理 @status102
* increase fetch depth for release nightly-ota to generate tags (might need successive increases) @Constrat
* 修正nothing to select情况下的判断逻辑 @Roland125
* update Collect Rewards template for EN fix #11485 @Constrat
* tw OcrReplace 肉鸽招募助战 (#11487) @Saratoga-Official
* 繁中服作戰失敗畫面卡住 (#11479) @momomochi987
* InitialDrop.png更新 @Constrat @BxFS
* txwy duplicates in tasks.json @Constrat
* checkout depth for nightly ota @Constrat
* 更新 “视相“ 主题后未关闭退出基建弹窗时无法回到主界面 @ABA2396
* `手动输入关卡名` 与  `使用剩余理智` 选项无法保存 @ABA2396

### 文档 | Docs

* 肉鸽辅助协议文档翻译 (#11360) @Windsland52

### 其他 | Other

* Release 模式下，如文件夹中包含 DEBUG.txt 也会输出 DBG 日志 (#11496) @ABA2396
* 移动企鹅物流及一图流上报设置 至 运行设置 @status102
* Translations update from MAA Weblate (#11524) @AlisaAkiron
* ignore blame for e3d63894b28b2ef5e2405e144a32a6981de5e1b2 oxipng optimization @Constrat
* disable link checker in issues and PRs (#11506) @Constrat
* use API for cache-deletion @Constrat
* 移除不再使用的代码 for 最小化启动模拟器 @status102
* move `push tag` later in the workflow in case or errors (#11480) @Constrat
* 上报添加 User-Agent @ABA2396
* 修改上报抬头 @ABA2396
* Use %B to consider header for skip changelog @Constrat
* try setup dotnet cache @Constrat
* EN duplicates in tasks.json + SSS Buffs @Constrat
* YostarJP phantom roguelike game pass, SSS#6 (#11473) @Manicsteiner
* 繁中服「源石塵行動」復刻活動導航 @momomochi987
* battle_data 未实装干员添加字段提示 @ABA2396
* 别用 1234567890ABCDEF 去连模拟器了 @ABA2396
* Revert "refactor: move resource copy to test script" @Constrat
* `启动 MAA 后直接运行` 和 `启动 MAA 后自动开启模拟器` 改为独立配置 @ABA2396
* 只有一个配置的时候不显示 `此选项页为全局配置` @ABA2396
* 当前配置不存在时尝试读取全局配置 @ABA2396
* Config序列化参数不转义中文 @status102
