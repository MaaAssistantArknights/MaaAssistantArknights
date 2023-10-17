## v4.26.0-beta.1

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

### 修复

- 修改日文 OCR 等价集 @zzyyyl
- 修复待选干员在列表末尾时点击错位的问题 (#6960) @zzyyyl
- 降低iOS平台中部分模板匹配的阈值 (#6963) @lanhao34
- 修复干员头像保存错误 (#6967) @MistEO
- 台服肉鸽模式无法招募某些好友干员 (#6965) @lirao
- YostarEN Mizuki I.S. get recruit selection @Constrat
- 修复自动战斗-保全ver在开始行动时第二次点击可能过早导致无法正常开始的bug (#6934) @status102
- 修复账号切换时可能卡在选择账号之后的错误 (#6933) @status102
- 日服水月Rogue选择难度功能 (#6950) @Manicsteiner
- 修正日服“选择难度”按钮图片 @Manicsteiner
- 修改错误的绑定逻辑 && 格式化代码 @ABA2396
- 修复干员 Lancet-2 的识别问题 @zzyyyl
- 修复手滑导致的日服资源加载失败的问题 @zzyyyl
- 修复战斗中费用识别错误的问题 @zzyyyl
- 修复日服肉鸽部分不期而遇的识别 @zzyyyl
- 修复日服傀影肉鸽部分不期而遇的识别 @zzyyyl
- 修复日服傀影肉鸽卡在不期而遇 「血には血を」（以血还血）的问题 @zzyyyl
- 修复肉鸽卡在招募券界面的问题 @zzyyyl
- 台服无法识别缄默德克萨斯 @ABA2396
- 进一步优化资源更新逻辑，修复一些问题 @MistEO
- 在CharsNameOcrReplace添加一条规则，将“^一”替换为空 (#6908) @lanhao34
- 注释掉一行代码，消除一个Warning (#6904) @hsyhhssyy
- 当minitouch断开时自动重连 @MistEO
- 修复理智不足时反复点击开始行动而不吃药的问题 @zzyyyl
- 修复WpfGui肉鸽 等待&停止 重复调用AsstStop的错误 (#6878) @status102
- EN InfrastDormDoubleConfirmButton  not recognized [no ci] @Constrat
- 修复因 ocr_replace 与 m_eq_classes 冲突导致的 ocr 识别失效的问题 (#6867) @zzyyyl
- 修复部分模板图片名错误的问题；删除多余模板图 @zzyyyl
- 修复台服获取信用及购物出错的问题 @zzyyyl
- zh-tw 变量名错误 (#6882) @broken-paint
- 修复傀影肉鸽卡在战斗失败界面的问题 @zzyyyl
- 修复肉鸽烧到水不停的问题 @zzyyyl
- Allow `BattleQuickFormation` to retry 3 times @zzyyyl
- 定时执行强制启动无法切换基建计划 @ABA2396
- changed EN OCR formation parameter [skip ci] @Constrat

### 其他

- 用于快速适配新页面主题的小工具 (#6846) @SherkeyXD
- 删除无用的 Roguelike@Continue 相关任务 @zzyyyl
- update meojson to v3.1.1 (#6956) @MistEO
- ClangFormatter 的 ignore 参数也忽略所有子文件夹内容 @zzyyyl
- use submodule for maa-cli and build it from source @wangl-cc
- removed UnexpectedType error @Constrat
- 删除多余检查 @ABA2396
- 修复美服肉鸽卡在放弃行动弹窗界面的问题 @zzyyyl @MistEO @Rbqwow
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

### For develops

- support GPU OCR (#6872) @MistEO
