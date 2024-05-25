## v5.3.0

### 新增 | New

* Sami theme for YoStarJP (#9194) @Manicsteiner
* add cmake option `USE_RANGE_V3` (#9169) @wangl-cc @horror-proton
* Sami theme for YostarKR @HX3N
* Sami theme for YostarEN might need tweaks when it comes out @Constrat
* DepotEnter for YostarEN themes @Constrat
* 连战次数选项可隐藏 @ABA2396
* 自动公招公招手动确认支援机械改为手动确认1星 (#9047) @Toby-Shi-cloud
* auto retry start on app startup crash (#8966) @Constrat
* 添加状态监测站 (#9001) @AnnAngela
* 肉鸽适配新干员用法 (#8967) @Lancarus
* mumu extras (#8939) @MistEO @status102 @ABA2396
* 「健将」名片识别 @ABA2396
* add compile def to disable emulator extras @horror-proton
* Wine 支持 (#8960) @dantmnf
* YostarEN Siracusano navigation EnterChapterIS might need future regex. Wait for event @Constrat
* add workaround to limit roguelike framerate @horror-proton
* 萨米肉鸽刷等级增加商店刷新 (#9142) @Lancarus @Constrat
* Improved Recruitment tab readability @Constrat

### 改进 | Improved

* 优化萨米肉鸽策略 (#9185) @Lancarus
* 更新文档 (#8981) @Rbqwow
* 更新 actions/checkout@v4 (#9042) @AnnAngela
* 优化肉鸽ew优先度 (#9020) @Lancarus
* mumu12模拟器路径默认值置空 @status102
* Mumu12模拟器截图增强模式禁用时隐藏配置项输入框 @status102
* 增加Mumu12模拟器截图增强配置的模拟器路径存在检查 @status102
* 更新文档 MuMu 运行库 蓝叠 (#8964) @Rbqwow @horror-proton
* auto start optimization + MAC fix  (#9083) @Constrat
* 优化萨米肉鸽策略 (#9114) @Lancarus
* 更新 mumu12 截图增强模式使用说明 @ABA2396
* 优化萨米肉鸽策略 (#9149) @Lancarus

### 修复 | Fix

* 部分立绘使用「银凇」主题无法进入刷理智任务 @ABA2396
* updated infrastructure EN ocr InfrastEnterOperList @Constrat
* 理智药数量识别错误 @ABA2396
* 干员识别修正: 崖心8 -> 崖心 (#9048) @kuttakke @Constrat
* 修复受损引用 (#9041) @AnnAngela
* 关卡导航无法识别部分关卡 @ABA2396
* 重复延迟 @ABA2396
* 自动编队选错排在末尾的干员 @ABA2396
* 暂停下干员卡在结束部署后 @ABA2396
* increase delay post "Mission Start" button (#8950) @Constrat
* delay after enter floor affected by fast screenshots ref afbe5e2 https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/afbe5e2e034018a33169e044d7091579867445e2#commitcomment-141687499 @Constrat
* 修复LT14-04地图参数 @status102
* 修复截图增强模式禁用时，模拟器路径无效时无法启动 @status102
* mumu模拟器连接失败 @ABA2396
* MuMu极速模式关卡导航切换关卡难度报错 @ABA2396
* unexpected change @status102
* 开关 MuMu 极速操控需要重启生效 @ABA2396
* 修复肉鸽关卡识别受截图速度影响的问题 @MistEO
* fallback to normal for empty m_first_tags @horror-proton
* 一图流 243 一天四换换班结束时的日志信息 (#8838) @bodayw
* 再次修正赠送月卡选项卡位移导致的领取失败问题 @SherkeyXD
* 部署失败后不清除已占用格子 (#9111) @Lancarus
* remove incorrect use of __cpp_lib_ranges @horror-proton
* 触控模式不可用时仍继续任务 @ABA2396
* increased roi + foldartal regex for EN @Constrat
* typo @ABA2396
* handling KR-specific announcements (#9177) @HX3N
* update roi of RecruitNoPermit @horror-proton
* change swipe position in StageDropsTaskPlugin @aur3l14no
* attempt to fix winrt notification crash @dantmnf
* adding delay between swipe and click @Constrat
* delay between FriendsList analyze and click @Constrat
* 尝试修复肉鸽编队不选干员 (#9148) @Lancarus
* StartUpConnectingFlag not detecting @Constrat

### 其他 | Other

* include path of onnxruntime>=1.16 @horror-proton
* cmake fastdeploy_ppocr, windows only option @horror-proton
* update CMakeLists.txt @horror-proton
* updated missingtemplates py for overseas @Constrat
* 修改 mumu12 支持版本提示 @ABA2396
* config (#7683) @Cryolitia @ABA2396 @SherkeyXD
* Release v5.3.0-beta.3 (#9182) @ABA2396
* Release v5.3.0-beta.2 (#9132) @ABA2396
* Release v5.3.0-beta.2 (#9127) @ABA2396
* Release v5.3.0-beta.1 (#9063) @ABA2396
* keep challenge consistency @Constrat
* Auto Update Changelogs of v5.3.0-beta.1 (#9067) @github-actions[bot] @ABA2396
* Revert "perf: mumu12模拟器路径默认值置空" @ABA2396
* Translations update from MAA Weblate (#9066) @LiamSho @ABA2396
* add favourite tip for i18n (#9051) @Constrat @Manicsteiner @ABA2396 @HX3N
* remove unused directives @ABA2396
* 添加 `MuMu 截图增强模式` 启用方法提示 @ABA2396
* 补上另一个actions/checkout@v4 (#9044) @AnnAngela
* YoStarJP ocr fix (#9033) @Manicsteiner
* update glossary @HX3N
* format TaskQueueViewModel @ABA2396
* add JP ツウィリングトゥルムの黄金 @wallsman
* YoStarJP ocr fix (#9022) @Manicsteiner
* Update bug-report (#9015) @ABA2396
* YoStarJP roguelike fix (#8991) @Manicsteiner
* 移动旧保全作业 @status102
* 更正Wpf中的`MuMu 独家极速操控模式`为`MuMu 截图增强模式` @status102
* 路径不存在时弹窗提示 @ABA2396
* 连接时判断截图方式是否生效 @ABA2396
* 修改注释 @ABA2396
* format AsstProxy @ABA2396
* fix markdown format @horror-proton
* Update CHANGELOG.md @ABA2396
* Auto Update Changelogs of v5.3.0-beta.2 (#9128) @ABA2396
* 繁中服「塵影餘音」復刻活動導航 (#9108) @momomochi987
* 添加超链接未设置默认浏览器的报错解决方案 @ABA2396
* run MaaWpfGui under Wine ref 1f9aa4c7a79f7527c2e9d22df6f6aa9e4b31b97a #8960) @Constrat
* fix a bad link (#9085) @lizy14
* YoStarKR translation fix @HX3N
* YoStarJP ocr fix (#9068) @Manicsteiner
* macOS 版本使用运行于 M1 芯片的 macOS 14 系统编译 (#9071) @AnnAngela
* 无需手动验证 doc (#9070) @AnnAngela
* Auto Update Changelogs of v5.3.0-beta.3 (#9184) @github-actions[bot] @ABA2396
* Revert "chore: force Aero2 theme" @ABA2396
* YoStarKR IS navigation @HX3N
* YoStarJP IS rerun navigation (#9164) @Manicsteiner
* StartGameTaskPlugin.cpp for improved readability @Constrat
