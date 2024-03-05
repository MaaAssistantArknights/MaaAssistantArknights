## v5.2.0-beta.1

### 新增 | New

- 刷开局模式增加萨米肉鸽生活队凹密文板 (#8408) @Lancarus @status102
- 领航者S4地图 @status102

### 改进 | Improved

- 战斗列表添加作业时自动滚动到添加位置 (#8455) @status102
- 增加肉鸽商店中招募券的购买优先度 (#8469) @Lancarus
- 抽卡风险提示默认选择取消 @ABA2396
- 碎石时锁定理智药使用量为999 (#8415) @status102
- 运行期允许滚动战斗列表 @status102
- 优化仓库识别速度 (#8113) @FreeSky-X @horror-proton
- 肉鸽开局干员 隐藏未实装干员 (#8436) @Manicsteiner
- 自动编队识别不到干员的重试次数下调为1 @status102
- 优化自动编队逻辑 (#8393) @KevinT3Hu
- 调整基建专精输出文本 @status102
- 移除肉鸽投资相关的无用代码 @status102
- 肉鸽高级设置-只凹精二，不可用时隐藏 @status102
- 优化傀影肉鸽部分关卡策略 (#8401) @Lancarus
- 结束后动作`仅一次`在执行后不恢复为`无动作`，而是上一次的选择项 (#8387) @status102
- 将肉鸽开局干员组和开局干员移动至常规设置，未勾选`使用助战`时隐藏`可选非好友助战` (#8389) @status102
- 当自动战斗选择不支持的关卡时，输出作业中的关卡名 @status102
- 如果待部署区未识别干员全部为冷却中，则跳过暂停识别避免反复暂停 (#8337) @status102

### 修复 | Fix

- 肉鸽开局奖励template删除文字部分 (#7991) @Manicsteiner
- 傀影肉鸽选项造成的卡死 (#8468) @Lancarus
- 生息演算卡在战斗结算 @ABA2396
- 助战勾选框不出现bug (#8459) @Lancarus
- 修复肉鸽重复部署一个干员后不会开启技能 (#8447) @Lancarus
- [台服] 自定義基建換班辨認 麒麟R夜刀 失敗 @ABA2396
- 修复使用干员组切换技能用法失效 (#8410) @status102
- 修复引航者试炼S4地图的部分数据错误 @status102
- 修复肉鸽切换模式时，投资可能在禁用的情况下显示已启用 @status102
- 肉鸽闪退 @ABA2396
- 基建无法选中干员(fix oper rect in InfrastOperImageAnalyzer) @horror-proton
- update template DepotEnterMistCity @horror-proton
- 修复未识别到幸运墙时报错的问题 @SherkeyXD
- 修复部分角色冷却识别错误 (#8366) @status102
- 战斗列表突袭标志点击后暂时不可见 @status102
- 自动战斗-战斗列表的突袭关描述从`Adverse`修正为`Raid` (#8082) @status102
- 修复部分情况下待部署区干员无法正确识别为冷却中 (#8336) @status102
- 修复投资系统出错时，余额ocr可能出错 (#8326) @status102
- 更正同时使用理智药+碎石的短路说明 (#8452) @Rbqwow @status102

### 其他 | Other

- update yituliu report url @horror-proton
- 更新&重构文档 (#8316) @Rbqwow
- asst.log中的输出使用相对路径取代绝对路径 (#8385) @status102
- 移除自动战斗设置参数时不必要的警告输出 @status102
- 支持肉鸽事件choice_require解析 (#7904) @SherkeyXD @zzyyyl @horror-proton @ABA2396 @status102
- DV navigation https://github.com/MaaAssistantArknights/MaaRelease/commit/d65fd353410272af2e478272dfa675b87eb9673c @Constrat
- update glossary (#8437) @Manicsteiner
- updated Friends Terminal templates for EN after 27-02-2024 update [skip ci] @Constrat
- await Async @ABA2396
- update meojson to v4.0.1 @MistEO
- build error from meojson updates @MistEO
- wrong std::move of meojson @MistEO
- Update operators.md @wallsman
- 删除continue @ABA2396
- 增加连接配置项 CompatPOSIXShellWithoutScreencapErr (#8359) @aur3l14no
- removed reclamation from module others @Constrat
- 修正发版时触发的文档部署workflow @SherkeyXD

### For Overseas

- update ocr replace for 亚梅塔 and 「弦驚] @horror-proton
- updated YostarEN infrast templates fix #8439 @Constrat
- Update ko-kr.xaml @178619
- YoStarJP task for 翠玉の夢 and sami rogue (#8475) @Manicsteiner
- update ja-jp.xaml for sami (#8435) @Manicsteiner
- updated ko-kr.xaml for 探索者的银凇止境 (#8433) @HX3N
- update resources for YostarKR (#8450) @178619
- YoStarJP modified template image (#8423) @Manicsteiner
- YoStarKR modified ocrReplace and template image (#8418) @HX3N
- refactor JP docs (#8298) @Manicsteiner
- Translations update from MAA Weblate (#8339) @LiamSho @Constrat @ABA2396 @status102 @Manicsteiner
- YoStarJP training template (#8351) @Manicsteiner
- YoStarKR modified BattleStageOcr for 引航者试炼#03 (#8325) @HX3N
- Temporary Rhodes Island Employee regex YostarEN @Constrat
