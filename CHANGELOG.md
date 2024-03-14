## v5.2.0

### 新增 | New

- 刷开局模式增加萨米肉鸽生活队凹密文板 (#8408) @Lancarus @status102
- 萨米肉鸽增加凹远见密文板 (#8539) @Lancarus
- 「围攻」适配 | Templates for new GUI (Siege) (#8510) @Lemon-miaow
- 水晶箭行动活动导航 @ABA2396
- 访问好友开关可关闭 | Visit Friends can be toggled off (#8488) @Constrat
- 肉鸽适配新干员用法 (#8565) @Lancarus

### 改进 | Improved

- 战斗列表添加作业时自动滚动到添加位置 (#8455) @status102
- 增加肉鸽商店中招募券的购买优先度 (#8469) @Lancarus
- 抽卡风险提示默认选择取消 @ABA2396
- 碎石时锁定理智药使用量为999 (#8415) @status102
- 运行期允许滚动战斗列表 @status102
- 肉鸽开局干员 隐藏未实装干员 (#8436) @Manicsteiner
- 自动编队识别不到干员的重试次数下调为1 @status102
- 优化自动编队逻辑 (#8393) @KevinT3Hu
- 肉鸽高级设置-只凹精二，不可用时隐藏 @status102
- 优化傀影肉鸽部分关卡策略 (#8401) @Lancarus
- 结束后动作`仅一次`在执行后不恢复为`无动作`，而是上一次的选择项 (#8387) @status102
- 将肉鸽开局干员组和开局干员移动至常规设置，未勾选`使用助战`时隐藏`可选非好友助战` (#8389) @status102
- 当自动战斗选择不支持的关卡时，输出作业中的关卡名 @status102
- 如果待部署区未识别干员全部为冷却中，则跳过暂停识别避免反复暂停 (#8337) @status102
- 调整访问好友的模板图 @ABA2396
- 训练室干员名识别优化 @ABA2396
- 水月肉鸽添加缺失干员用法 (#8574) @Manicsteiner

### 修复 | Fix

- 傀影肉鸽选项造成的卡死 (#8468) @Lancarus
- 生息演算卡在战斗结算 @ABA2396
- 助战勾选框不出现bug (#8459) @Lancarus
- 修复肉鸽重复部署一个干员后不会开启技能 (#8447) @Lancarus
- 修复使用干员组切换技能用法失效 (#8410) @status102
- 修复引航者试炼S4地图的部分数据错误 @status102
- 修复肉鸽切换模式时，投资可能在禁用的情况下显示已启用 @status102
- 肉鸽闪退 @ABA2396
- 基建无法选中干员(fix oper rect in InfrastOperImageAnalyzer) @horror-proton
- 修复迷城主题无法进入仓库进行识别(update template DepotEnterMistCity) @horror-proton
- 修复未识别到幸运墙时报错的问题 @SherkeyXD
- 修复部分角色冷却识别错误 (#8366) @status102
- 战斗列表突袭标志点击后暂时不可见 @status102
- 自动战斗-战斗列表的突袭关描述从`Adverse`修正为`Raid` (#8082) @status102
- 修复部分情况下待部署区干员无法正确识别为冷却中 (#8336) @status102
- 修复投资系统出错时，余额ocr可能出错 (#8326) @status102
- 更正同时使用理智药+碎石的短路说明 (#8452) @Rbqwow @status102
- 「围攻」部分透明度高的模板图片无法识别 @ABA2396
- 「银淞」无法进入基建 @ABA2396
- 蓝叠模拟器多开，在正确配置关键词时，两个MAA实例会指向同一个模拟器实例 @ABA2396
- 肉鸽投资无法禁用 @status102
- 基建专精干员名切割错误 @status102
- 基建換班 Lancet-2/Castle-3 识别异常 @ABA2396

### 其他 | Other

- 肉鸽开局奖励template删除文字部分 (#7991) @Manicsteiner
- 基建专精干员名增加ocrReplace @status102
- Toast改为win10API @status102
- 调整CopilotViewModel.AddLog @status102
- 调整基建专精输出文本 @status102
- 格式化 @ABA2396
- 调整账号切换功能说明 @status102
- 锁定Xcode SDK版本 (#8533) @hguandl
- update clang-format @MistEO
- 更新&重构文档 (#8316) @Rbqwow
- asst.log中的输出使用相对路径取代绝对路径 (#8385) @status102
- 移除自动战斗设置参数时不必要的警告输出 @status102
- 支持肉鸽事件choice_require解析 (#7904) @SherkeyXD @zzyyyl @horror-proton @ABA2396 @status102
- 删除continue @ABA2396
- 增加连接配置项 CompatPOSIXShellWithoutScreencapErr (#8359) @aur3l14no
- 修正发版时触发的文档部署workflow @SherkeyXD
- 根据灰度二值化关卡名后再识别 (#7848) @zzyyyl
- 减少了自动资源更新中的项目数量 | reduced items count in Auto Resource Updater @Constrat
- 指定Wpf目标操作系统版本为win10 (#8409) @status102
- 添加截图过长解决方案 @ABA2396 @status102
- 略微上调干员部署滑动 @status102
- 下调截图用时>800ms时的警告等级 @status102
- removed reclamation from module others @Constrat
- update yituliu report url @horror-proton
- update glossary (#8437) @Manicsteiner
- await Async @ABA2396
- update meojson to v4.0.1 @MistEO
- build error from meojson updates @MistEO
- wrong std::move of meojson @MistEO
- Update operators.md @wallsman
- removed parenthesis for OCR @Constrat

### For Overseas

- update ocr replace for 亚梅塔 and 「弦驚」 @horror-proton
- Translations update from MAA Weblate (#8339) @LiamSho @Constrat @ABA2396 @status102 @Manicsteiner
- "Retrospection" needed for "Rerun" events @Constrat
- changed Sami theme name to reduce issues @Constrat
- greyy alter ocr in battle @Constrat
- DV navigation @Constrat

#### txwy

- [台服] 自定義基建換班辨認 麒麟R夜刀 失敗 @ABA2396
- 繁中服「愚人號」復刻活動導航 (#8505) @momomochi987

#### YostarEN
- updated YostarEN infrast templates fix #8439 @Constrat
- Temporary Rhodes Island Employee regex YostarEN @Constrat
- IS4 EN higher floors NextLevel text @Constrat
- ignoring templates Overseas @Constrat
- Feat: IS4 - YostarEN early adaptation (#8524) @Constrat
- updated Friends Terminal templates for EN after 27-02-2024 update @Constrat
- custom ROI for StartToVisit YostarEN @Constrat
- ocr for trader shopping @Constrat
- increased roi for RoguelikeFormationOcr EN (fix long names) @Constrat
- IS4 EN drop recruit selection option @Constrat
- IS4 EN higher floors NextLevel text @Constrat

#### YostarKR

- Update ko-kr.xaml @178619
- updated ko-kr.xaml for 探索者的银凇止境 (#8433) @HX3N
- update resources for YostarKR (#8450) @178619
- YoStarKR modified ocrReplace and template image (#8418) @HX3N
- YoStarKR modified BattleStageOcr for 引航者试炼#03 (#8325) @HX3N
- YoStarKR update tasks for Sami roguelike  (#8495) @HX3N
- update template images (YostarKR) @178619
- add missing resource for YoStarKR @178619
- update YostarKR resources @178619
- YoStarKR Sami roguelike templates and tasks (#8487) @HX3N
- YoStarKR update tasks (#8568) @HX3N
- YoStarKR ROI for StartToVisit (#8558) @HX3N

#### YostarJP

- YoStarJP task for 翠玉の夢 and sami rogue (#8475) @Manicsteiner
- update ja-jp.xaml for sami (#8435) @Manicsteiner
- YoStarJP modified template image (#8423) @Manicsteiner
- refactor JP docs (#8298) @Manicsteiner
- YoStarJP training template (#8351) @Manicsteiner
- YoStarJP roguelike data remove duplicate and fixes (#8521) @Manicsteiner
- YoStarJP Sami rogue templates and tasks (#8486) @Manicsteiner
- YoStarJP ROI for StartToVisit, sami template StageFerociousPre… (#8555) @Manicsteiner
