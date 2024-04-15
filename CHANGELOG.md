## v5.2.1

### 新增 | New

* 巴别塔二、三阶段导航预载 (#8829) @AnnAngela
* 巴别塔导航数据 (#8827) @AnnAngela
* Add Bark notification provider (#8701) @KawashiroNitori
* add multi external notifications (#8628) @Sieluna
* calculate position of skill & retreat button @horror-proton

### 改进 | Improved

* 战斗列表处于激活时，浏览带有难度标识的作业会自动添加对应难度到战斗列表 (#8491) @status102
* 肉鸽投资模式`进入第二层`拓展为`投资模式启用购物、招募` (#8620) @status102
* 调整编队干员识别 roi @status102
* 移除过多的剧情检查 @status102
* 专精干员名称按ui语言显示 @ABA2396
* 优化changelog (#8608) @AnnAngela
* 自动战斗-自动编队存储编队信息 (#8542) @status102
* 优化 LocalizationHelper @ABA2396
* Add missing headers & optimize the code @ABA2396
* Improved InfrastFilterMenuNotStationedButton recognition score @Constrat

### 修复 | Fix

* 部分场景下无法执行访问好友 @ABA2396
* 假日场景下，银淞主题UI无法进入基建 (#8795) @HoshinoAyako
* 肉鸽解析Vision失败 @ABA2396
* 刷理智愚人号任务出错 @ABA2396
* 无奖励关卡无法匹配愚人号部分关卡 @ABA2396
* 泡普卡识别错误 @ABA2396
* 尝试修复肉鸽导航 (#8764) @horror-proton
* pallas 无法获取 DynamicResource @ABA2396
* 找不到语言资源文件 @ABA2396
* 无法进入设置 @ABA2396
* 修复gen_changelog的错误操作 (#8756) @AnnAngela
* 修复changelog_generator编码问题，以及部分重构 (#8739) @SherkeyXD
* 信用购买卡在购买完成界面 @ABA2396
* macos 生息演算无法退出关卡 @ABA2396
* 训练室读取干员名失效 @status102
* 干员名ocrReplace追加`一煌`->`煌` & 夹子 @Saratoga-Official
* 干员名识别移除前缀c @status102
* 补充低信赖筛选错误 @status102
* 战斗中教程剧情导致错误的倍速切换 @status102
* 自动战斗-自动编队-补充低信赖时自动关闭特关 @status102
* 干员名识别`一山`->`山` @status102
* 所有任务都添加失败时清空Core任务列表 @ABA2396
* 修复nightly的changelog tag选取 @status102
* 自动战斗作业自动点赞检测 @status102
* 尝试修复换行混乱 @status102
* 外服小车名称识别 @ABA2396
* 特米米 in battle ocr detection (#8788) @Constrat
* train regex @status102
* match the correct file suffix @Amsterwolf

### 其他 | Other

* 修改文档图片透明度 @ABA2396
* 修改抗干扰值日志输出 @ABA2396
* 生息演算介绍Update zh-cn.xaml @ABA2396
* 加点不期而遇的日志 @ABA2396
* LastBuyWineTime 改为全局配置 @ABA2396
* 修改nightly changelog忽略多余抬头 @status102
* 修正nightly使用生成文件 @status102
* 一天只喝一次酒 @ABA2396
* 修改不支持的关卡提示 @ABA2396
* 去掉太慢的clang-format配置 @MistEO
* 添加蓝叠核心版本推荐 (#8688) @Rbqwow
* Update 肉鸽辅助协议.md (#8703) @guguji12
* 修改changelog_generator的无序列表格式，从`- `改为`* ` @status102
* 移除空格转义 @status102
* 修改自动生成changelog与上个tag对比，而非与上个公版 @status102
* rename Linux模拟器.md to Linux模拟器&容器.md (#8513) @zayn7lie
* pr checker @status102
* var name @status102
* ResourceUpdater @ABA2396
* change image transparency (#8802) @Manicsteiner
* use relative md instead of html links @wangl-cc
* split and update CLI documents @wangl-cc
* update maa-cli to 0.4.5 and build with vendored-openssl @wangl-cc
* Remote Control spacing @Constrat
* LocalizationHelper & SettingsViewModel @ABA2396
* sync with upstream, re-arrange for linux users (#8733) @Constrat @Cryolitia
* fix breakline @Cryolitia
* declare license AGPL-3.0-only @Cryolitia
* generate changelog with python script for nightly (#8728) @wangl-cc
* output format @status102
* Update .clang-format @MistEO
* nightly auto changelog (#8614) @status102 @AnnAngela

### For Overseas

#### txwy

* 台服保全開場選元件ocr替换 (#8641) @vonnoq
* 替換繁中服 StartToVisit 圖片 (#8817) @momomochi987
* 繁中服幹員識別修正 (#8807) @vonnoq
* custom ROI and notes for StartToVisit txwy (#8682) @Manicsteiner
* txwy ocrreplace (#8655) @EROTCZ
* 繁中服「孤星」活動導航 (#8662) @momomochi987

#### YostarEN

* Skip the story before a battle for EN @Constrat
* optimized roi for EN infrast training @Constrat
* IS4 EN foldartal declare text @Constrat
* Foldartal use for IS4 - overseas (#8612) @Constrat
* IS4 Rain! encounter regex fix #8709 @Constrat
* OCR replace for Auto Battle (CV) @Constrat
* CV roi for navigation @Constrat
* Eyja Alter EN regex for SSS @Constrat
* Eyjafjalla Alter ocr @Constrat
* en transparency ref: 0c5a79f @Constrat
* EN CV navigation @Constrat
* YostarEN SSS#3 (#8645) @Constrat

#### YostarKR

* YoStarKR SSS#3 (#8643) @HX3N
* YoStarKR Executor alter regex (#8723) @HX3N
* YoStarKR SkipThePreBattlePlot (#8671) @HX3N
* YoStarKR ocrReplace (#8665) @HX3N
* YoStarKR update operators.md @HX3N
*  docs: update and refactor KR docs (#8793) @HX3N
* YoStarKR CV stage navigation and add Conductive Unit Upgrade (#8649) @HX3N
* YoStarKR update operator.md (#8626) @HX3N

#### YostarJP

* YoStarJP SSS#4 (#8630) @Manicsteiner
* Global IS navigation (#8769) @Manicsteiner
* YoStarJP ocrreplace (#8663) @Manicsteiner
* YoStarJP ocr fix (#8768) @Manicsteiner @Constrat
* YoStarJP ダーティマネー navigation (#8647) @Manicsteiner
* Update operators.md for JP ダーティマネー @wallsman
