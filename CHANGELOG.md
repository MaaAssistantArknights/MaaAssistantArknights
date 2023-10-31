## v4.26.0

### 新增

- 适配"迷城"界面主题 (#6847) @SherkeyXD
- 新增 WpfGui选项-自动战斗-战斗列表-批量导入 (#6917) @status102
- WpfGui 刷理智开始行动输出新增 每次开始行动时输出理智药和碎石次数（不记录临期药品使用次数） (#6857) @status102
- 显示 ui 版本号 (#6944) @ABA2396
- 肉鸽适配新干员默认技能 (#6948) @Lancarus
- 支持13章关卡导航 @ABA2396
- 远程控制更多功能 (#6896) @hsyhhssyy
- 点击版本号后复制到剪贴板 @ABA2396
- 自动战斗地图链接指向PRTS.Map对应的地图 @ABA2396
- 公招出高星干员时选中多个tag (#6900) @broken-paint
- IS 关卡导航 @ABA2396
- 肉鸽增加不期而遇事件名输出 (#7112) @status102
- 调整理智药使用为按瓶计数，移除了博朗台在使用理智药不溢出情况下的额外等待，调整Wpf理智药使用输出 (#7022) @status102
- 新增获取信用-借助战时使用指定编队 (#7061) @status102 @broken-paint @MistEO
- 萨米肉鸽使用密文板  (#7086) @Lancarus @zzyyyl
- macOS 完成后关闭客户端 @AndersonALAC @hguandl

### 改进

- Only check version.json for Resource OTA (#6901) @MistEO @ABA2396
- 肉鸽投资不进入第二层则不买东西 (#6946) @zzyyyl
- 使用MdXaml来提升Markdown渲染表现 (#6828) @SherkeyXD
- 修改刷理智每次开始战斗时输出：不再显示理智药、碎石的设定上限值，增加临期理智药使用次数 (#6962) @status102
- 重构 changelog_generator.py 部分内容 (#6957) @quyansiyuanwang
- 优化部分基建日志输出 @zzyyyl
- 更新延迟延长到60分钟 @ABA2396
- 优化资源更新相关代码 @ABA2396
- 更新内置243一天四换极限效率 (#6883) @Black1312
- 使用多模板重写 StartButton2；优化 PRTS#next @zzyyyl
- 重写理智识别 (#6885) @zzyyyl
- 优化 OperBox 相关代码 @zzyyyl
- 优化 AllTasksCompleted 相关 WPF 代码 (#6845) @zzyyyl
- 开始任务后强制锁定当前基建配置，强制定时启动时强制刷新基建配置 @ABA2396
- perf 优化萨米肉鸽部分关卡策略 @Lancarus
- 优化水月肉鸽策略 (#7043) @Yumi0606
- perf & typo: removed redundant variable & variable typo (#7032) @Constrat
- perf & refactor: 重构 tools/AutoLocalization (#6995) @quyansiyuanwang @Constrat
- 优化部分肉鸽逻辑 (#7004) @Lancarus
- 当清空自动战斗-战斗列表中的所有任务时，移除缓存中的战斗列表文件夹 (#6994) @status102
- 增加雷电模拟器 nc 截图方式 (#7062) @zzyyyl
- add some mirror for resource updates @MistEO
- 优化部分情况下，单个刷理智任务在多次执行时等待时间过长 (#7108) @status102
- 优化Wpf关卡数据更新提示，改为Growl.Info (#7103) @status102
- 优化萨米肉鸽部分关卡策略 (#7099) (#7075) @Lancarus

### 修复

- 删除多余（且导致水月肉鸽卡住）的 StartExploreConfirm @zzyyyl
- 修复待选干员在列表末尾时点击错位的问题 (#6960) @zzyyyl
- 降低iOS平台中部分模板匹配的阈值 (#6963) @lanhao34
- 修复干员头像保存错误 (#6967) @MistEO
- 修复自动战斗-保全ver在开始行动时第二次点击可能过早导致无法正常开始的bug (#6934) @status102
- 修复账号切换时可能卡在选择账号之后的错误 (#6933) @status102
- 修改错误的绑定逻辑 && 格式化代码 @ABA2396
- 修复干员 Lancet-2 的识别问题 @zzyyyl
- 修复战斗中费用识别错误的问题 @zzyyyl
- 修复肉鸽卡在招募券界面的问题 @zzyyyl
- 进一步优化资源更新逻辑，修复一些问题 @MistEO
- 在CharsNameOcrReplace添加一条规则，将“^一”替换为空 (#6908) @lanhao34
- 注释掉一行代码，消除一个Warning (#6904) @hsyhhssyy
- 当minitouch断开时自动重连 @MistEO
- 修复理智不足时反复点击开始行动而不吃药的问题 @zzyyyl
- 修复WpfGui肉鸽 等待&停止 重复调用AsstStop的错误 (#6878) @status102
- 修复因 ocr_replace 与 m_eq_classes 冲突导致的 ocr 识别失效的问题 (#6867) @zzyyyl
- 修复部分模板图片名错误的问题；删除多余模板图 @zzyyyl
- 修复傀影肉鸽卡在战斗失败界面的问题 @zzyyyl
- 修复肉鸽烧到水不停的问题 @zzyyyl
- Allow `BattleQuickFormation` to retry 3 times @zzyyyl
- 定时执行强制启动无法切换基建计划 @ABA2396
- 修复“迷城”界面公开招募有时出错的问题 @zzyyyl
- 修复肉鸽探索失败后卡在每月委托/解锁收藏品等界面 @zzyyyl
- 剪切板错误 @ABA2396
- fix #7027 (#7039) @Cryolitia
- 修复肉鸽打完退出的问题 @zzyyyl
- 修复Wpf自动战斗-战斗列表-批量导入时，缓存文件夹未存在导致报错的bug；调整关卡名匹配规则 (#6988) @status102
- 修复部分模板图片 @zzyyyl
- 修复 maatouch 滑动 @zzyyyl
- add 1 sec delay for resource OTA @MistEO
- As in my Adumbration operator implementation @Constrat
- Morgan 摩根 ocrreplace [skip ci] @Constrat
- c# warnings @MistEO
- 修复Wpf无法复制版本号至剪贴板 (#7109) @status102
- 优化设置引导界面的视觉效果，并给 UI Theme 添加翻译 (#7047) @SherkeyXD
- Mizuki I.S. encouter regex [skip ci] @Constrat
- Terra Research Commission EN regex [skip ci] @Constrat
- 叙拉古人活动导航 @ABA2396
- Il Siracusano enter screen change @Constrat
- coredump after failing to restart minitouch @horror-proton
- 修复肉鸽编队会漏选干员的问题（比如>=3页） (#7038) @wengzhe
- fix：萨米肉鸽层名ocr错误 @Lancarus
- resource updater + updated data (#7079) @Constrat
- 修复在点击停止后下一次开始任务时可能操作延时很高的问题 (#7052) @zzyyyl
- duplicate update in stage.json (my mistake from 859802071176f9f145dabd6f3c3ecb442cc3663c) @Constrat
- 修复 #7016 可能影响的部分任务 (#7066) @zzyyyl
- SkipThePreBattlePlot lowered threshold @Constrat
- 修复公招多选 Tags 选项不保存的问题 (#7065) @broken-paint
- fix code block in user guide for CLI (#7060) @wangl-cc

### 其他

- register_plugin允许更多传参 (#7122) @status102 @MistEO
- 在 AppImage 中添加 `Terminal=true` (#7126) @Cryolitia
- Update 2.4-PURE_WEB_PR_TUTORIAL.md @ABA2396
- Enlarge contributors' avatars @MistEO
- show avatar for all contributors @MistEO
- Revert "fix: Il Siracusano enter screen change" @Constrat
- 用于快速适配新页面主题的小工具 (#6846) @SherkeyXD
- 删除无用的 Roguelike@Continue 相关任务 @zzyyyl
- update meojson to v3.1.1 (#6956) @MistEO
- ClangFormatter 的 ignore 参数也忽略所有子文件夹内容 @zzyyyl
- use submodule for maa-cli and build it from source @wangl-cc
- removed UnexpectedType error @Constrat
- 删除多余检查 @ABA2396
- 减少点rider的报错 @ABA2396
- 延长 StartUp 失败判定时长 @zzyyyl
- wpfGui: Taggify Version (#6894) @Cryolitia
- Fix README.md (#6928) @martinwang2002
- tools: overseas duplicates + new themes @Constrat
- format @zzyyyl
- 修改 Stop 返回值，增加文档注释 @ABA2396
- 更正变量名 (#6880) @broken-paint
- debug: save_img 默认使用缓存的图片 @zzyyyl
- debug: 增加一些非法情况的截图 @zzyyyl
- TaskData 优化 (#6844) @zzyyyl
- 增加任务截图插件 (#6973) @zzyyyl
- 声明 baseTask 的任务直接覆盖同名任务 (#7016) @zzyyyl
- bump `maa-cli` and update documents (#7050) @wangl-cc
- 增加 TaskSorter 脚本对 tasks.json 进行排序 (#7046) @zzyyyl
- Change to Appimage Package on Linux (#7030) @Cryolitia
- 删除多余引用 @ABA2396
- Revert "feat: 删除无用的 Roguelike@Continue 相关任务" @zzyyyl
- 补充基建排班协议中`period`字段不存在时的计划切换动作 (#6998) @status102
- re-write changelog_generator in algorithm (#6972) @quyansiyuanwang
- update MaaCore.vcxproj.filters @zzyyyl
- fix some warnings from visual studio (#7031) @status102
- 多划几次 @ABA2396
- format @zzyyyl
- manual after res-update fix @Constrat
- Wpf迁移MaaCore引用，统一合并至MaaService (#6945) @status102

### For overseas

#### YostarJP

- 日服保全派驻 (#7012) @Manicsteiner
- Add Operator JP/12章 @wallsman
- 修复手滑导致的日服资源加载失败的问题 @zzyyyl
- 修复日服肉鸽部分不期而遇的识别 @zzyyyl
- 修复日服傀影肉鸽部分不期而遇的识别 @zzyyyl
- 修复日服傀影肉鸽卡在不期而遇 「血には血を」（以血还血）的问题 @zzyyyl
- 日服水月Rogue选择难度功能 (#6950) @Manicsteiner
- 修正日服“选择难度”按钮图片 @Manicsteiner
- 修改日文 OCR 等价集 @zzyyyl
- 修复日文文档中顿号导致的渲染问题 @SherkeyXD
- 修复日服基建换班卡在宿舍 (#7068) @MejiroSilence

#### YostarEN

- EN Phantom I.S. Encounter Ocr regex link to EN傀影不期而遇遇上鸭爵，会选择战斗可是不会进入战斗 #6975 @Constrat
- 修复美服傀影肉鸽不期而遇 "Adventurer Commission"（委托冒险者）的问题 @zzyyyl
- YostarEN Mizuki I.S. get recruit selection @Constrat
- EN InfrastDormDoubleConfirmButton  not recognized [no ci] @Constrat
- YostarEN Mizuki I.S. Trader + encouter regex (#6993) @Constrat
- 修复美服 “A Cold Separation”（寒渊惜别）关卡识别错误 @zzyyyl
- 修复美服肉鸽卡在放弃行动弹窗界面的问题 @Rbqwow @zzyyyl
- changed EN OCR formation parameter [skip ci] @Constrat

#### txwy

- 繁中服 保全派駐 (#7021) @vonnoq
- 繁中服「照我以火」活動導航 (#7002) @momomochi987 @Constrat @zzyyyl
- 繁中服 保全派駐識別修正 (#7041) @vonnoq
- 調整繁中服某些不期而遇的 ocrReplace (#6977) @momomochi987
- 繁中服肉鸽模式无法招募某些好友干员 (#6965) @lirao
- 繁中服无法识别缄默德克萨斯 @ABA2396
- 修复繁中服获取信用及购物出错的问题 @zzyyyl
- 調整繁中服「照我以火」導航任務、替換水月肉鴿 "繼續探索" 圖片 (#7085) @momomochi987
- zh-tw 变量名错误 (#6882) @broken-paint
- 繁中服 訪問好友修正 (#7133) @vonnoq

### For develops

- support GPU OCR (#6872) @MistEO
