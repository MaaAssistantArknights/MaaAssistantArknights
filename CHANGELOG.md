## v5.3.0

### 新增 | New

* 萨米肉鸽刷等级增加商店刷新 (#9142) @Lancarus @Constrat
* Wine 支持 (#8960) @dantmnf
* 「健将」名片识别 @ABA2396
* 连战次数选项可隐藏 @ABA2396
* 自动公招公招手动确认支援机械改为手动确认1星 (#9047) @Toby-Shi-cloud
* 添加状态监测站 (#9001) @AnnAngela
* 肉鸽适配新干员用法 (#8967) @Lancarus
* mumu12 截图增强 (#8939) @MistEO @status102 @ABA2396
* auto retry start on app startup crash (#8966) @Constrat
* Improved Recruitment tab readability @Constrat
* add workaround to limit roguelike framerate @horror-proton
* add compile def to disable emulator extras @horror-proton

### 改进 | Improved

* 更新 mumu12 截图增强模式使用说明 @ABA2396
* 优化萨米肉鸽策略 (#9185) @Lancarus
* 优化萨米肉鸽策略 (#9149) @Lancarus
* 优化萨米肉鸽策略 (#9114) @Lancarus
* 开始唤醒优化 (#9083) @Constrat
* 更新 actions/checkout@v4 (#9042) @AnnAngela
* 优化肉鸽ew优先度 (#9020) @Lancarus
* Mumu12模拟器截图增强模式禁用时隐藏配置项输入框 @status102
* 增加Mumu12模拟器截图增强配置的模拟器路径存在检查 @status102
* 更新文档(#8981)(#8964) @Rbqwow @horror-proton
* refactor(WpfGui): config (#7683) @Cryolitia @ABA2396 @moomiji @SherkeyXD @LiamSho

### 修复 | Fix

* 部分立绘使用「银凇」主题无法进入刷理智任务 @ABA2396
* 尝试修复肉鸽编队不选干员 (#9148) @Lancarus
* 修复仓库识别界面显示异常 @ABA2396
* mac 开始唤醒报错 (#9083) @Constrat
* 修正赠送月卡选项卡位移导致的领取失败问题 @SherkeyXD
* 部署失败后不清除已占用格子 (#9111) @Lancarus
* 触控模式不可用时仍继续任务 @ABA2396
* 理智药数量识别错误 @ABA2396
* 干员识别修正: 崖心8 -> 崖心 (#9048) @kuttakke @Constrat
* 修复受损引用 (#9041) @AnnAngela
* 关卡导航无法识别部分关卡 @ABA2396
* 重复延迟 @ABA2396
* 自动编队选错排在末尾的干员 @ABA2396
* 暂停下干员卡在结束部署后 @ABA2396
* 修复LT14-04地图参数 @status102
* 修复截图增强模式禁用时，模拟器路径无效时无法启动 @status102
* MuMu模拟器连接失败 @ABA2396
* 关卡导航切换关卡难度报错 @ABA2396
* 开关 MuMu 极速操控需要重启生效 @ABA2396
* 修复肉鸽关卡识别受截图速度影响的问题 @MistEO
* 一图流 243 一天四换换班结束时的日志信息 (#8838) @bodayw
* increase delay post "Mission Start" button (#8950) @Constrat
* delay after enter floor affected by fast screenshots @Constrat
* unexpected change @status102
* fallback to normal for empty m_first_tags @horror-proton
* update roi of RecruitNoPermit @horror-proton
* change swipe position in StageDropsTaskPlugin @aur3l14no
* attempt to fix winrt notification crash @dantmnf
* adding delay between swipe and click @Constrat
* delay between FriendsList analyze and click @Constrat
* StartUpConnectingFlag not detecting @Constrat
* remove incorrect use of __cpp_lib_ranges @horror-proton

### 其他 | Other

* 添加超链接未设置默认浏览器的报错解决方案 @ABA2396
* macOS 版本使用运行于 M1 芯片的 macOS 14 系统编译 (#9071) @AnnAngela
* 无需手动验证 doc (#9070) @AnnAngela
* 添加 `MuMu 截图增强模式` 启用方法提示 @ABA2396
* 移动旧保全作业 @status102
* 路径不存在时弹窗提示 @ABA2396
* 连接时判断截图方式是否生效 @ABA2396
* 修改注释 @ABA2396
* update glossary @HX3N
* format TaskQueueViewModel @ABA2396
* Update bug-report (#9015) @ABA2396
* format AsstProxy @ABA2396
* fix markdown format @horror-proton
* Translations update from MAA Weblate (#9066) @LiamSho @ABA2396
* add favourite tip for i18n (#9051) @Constrat @Manicsteiner @HX3N @ABA2396
* remove unused directives @ABA2396
* StartGameTaskPlugin.cpp for improved readability @Constrat
* run MaaWpfGui under Wine (#8960) @Constrat
* fix a bad link (#9085) @lizy14
* updated missingtemplates py for overseas @Constrat
* update CMakeLists.txt @horror-proton
* cmake fastdeploy_ppocr, windows only option @horror-proton
* include path of onnxruntime>=1.16 @horror-proton
* add cmake option USE_RANGE_V3 (#9169) @horror-proton @wangl-cc

### For Overseas

#### txwy

* 繁中服「塵影餘音」復刻活動導航 (#9108) @momomochi987

#### YostarEN

* DepotEnter for YostarEN themes @Constrat
* Sami theme for YostarEN @Constrat
* YostarEN Siracusano navigation EnterChapterIS might need future regex. Wait for event @Constrat
* increased roi + foldartal regex for EN @Constrat

#### YostarJP

* Sami theme for YoStarJP (#9194) @Manicsteiner
* YoStarJP IS rerun navigation (#9164) @Manicsteiner
* YoStarJP ocr fix (#9068) @Manicsteiner
* YoStarJP ocr fix (#9033) @Manicsteiner
* add JP ツウィリングトゥルムの黄金 @wallsman
* YoStarJP ocr fix (#9022) @Manicsteiner
* YoStarJP roguelike fix (#8991) @Manicsteiner

#### YostarKR

* Sami theme for YostarKR @HX3N
* handling KR-specific announcements (#9177) @HX3N
* YoStarKR IS navigation @HX3N
* YoStarKR translation fix @HX3N
