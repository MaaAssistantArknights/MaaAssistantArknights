## v5.0.0

### 新增 | New

- 资源更新完毕后添加自动重启（需勾选 设置-软件更新-自动安装更新包） @ABA2396
- SideStory「怀黍离」导航 (#8124) @AnnAngela
- CLI 支持多天排班 @wangl-cc
- 增加肉鸽刷开局模式下只凹精二选项 (#7705) @s-yh-china
- 萨米肉鸽新板子 @ABA2396
- ui 和 core 版本不一致时开始任务添加 waring 提示 @ABA2396
- 肉鸽开局干员支持输入任意客户端对应名称 (#7726) @ABA2396
- 萨米肉鸽适配新干员用法 (#8033) @Lancarus
- 连战次数选择 (#8159) @ABA2396

### 改进 | Improved

- 基建排班工具链接更新 @ABA2396
- 捕获启动模拟器时产生的权限不足异常 (#7908) @Chiichen
- 允许直接复制干员识别的结果 @ABA2396
- 优化萨米事不过四策略 (#8059) @Lancarus
- 优化连续战斗次数列表展开图片 @status102
- 优化萨米锏精二优先度 (#7939) @Lancarus
- 优化萨米肉鸽部分关卡逻辑 (#7925、#7935、#7936) @Lancarus
- 初步适配萨米肉鸽dlc关卡等 (#7924) @Lancarus
- 优化肉鸽作战逻辑，使不能空放的干员技能不影响后续部署指令的执行 (#7934) @Lancarus
- 更新繁中服生息演算最後一天的日期 (#7903) @momomochi987
- 优化刷理智-已开始战斗x次时临期理智药使用输出以增加可读性 (#7846) @status102
- 优化截图耗时额外提示文本触发条件，改为由平均耗时触发 (#7868) @status102
- 优化自动战斗部署栏识别条件，保全增加节能模式 (#8044) @ABA2396
- 优化wpf肉鸽结算输出格式 for i18n (#8011) @status102 @Constrat
- more scalable solution to 7bbb5f3 thanks to @ABA2396 @Constrat
- 限制理智识别重试次数 fix #7998 @status102
- 修改 前行的林地 默认选择 @ABA2396

### 修复 | Fix

- 减少重复截图 (#7945) @zzyyyl
- 更新导航格式 @ABA2396
- 修复OCR识别干员黍错误的问题 @SherkeyXD
- 点击 `复制错误信息` 报错 @ABA2396
- 更新吐司通知报错 @ABA2396
- 修复更新重启后热键失效 @ABA2396
- SL-8 not recognized @Constrat
- 修复刷开局模式下强制启用投资造成与UI显示不一致的错误 @status102
- 自动战斗群组中多名干员在编组中时不会选择 @ABA2396
- 修复基建排班的 JSON schema (#7788) @wangl-cc
- 修复肉鸽烧水选择难度可能不对的问题 (#7946) @zzyyyl
- 一键长草期间锁定自动肉鸽设置以符合执行逻辑 @status102
- 修复刷理智调整战斗次数时可能错误点击关卡报酬 (#7942) @status102
- 勾选启动后直接最小化时无法使用右键强制显示 @ABA2396
- 使用启动旧版.cmd启动后下次更新报错 @ABA2396
- 下调检测阈值以尝试修复刷理智卡在次数选择 fix #7919 @status102
- reduced Mizuki@Roguelike@MissionFailedFlag threshold for iOS fix #7864 temporary fix as #7872 exists @Constrat
- 修复部分情况下肉鸽结算点击错误 @status102
- 修复肉鸽-刷钱模式下仍然会在商店进行招募、购物的错误 (#7863) @status102
- 配置修改后无法保存 @ABA2396
- 修复刷理智结束时理智输出错误 @status102
- 清剩余理智时卡在选择次数 @ABA2396
- 修复README的一处链接错误 (#7905) @Chiichen
- changed template YostarEN I.S. enter @zzyyyl
- 移除 StageEncounterClickToLeave 任务 (#7887) @zzyyyl
- 修复肉鸽刷源石锭模式卡住的问题 (#7882) @zzyyyl
- EN服 无法自动吃大理智药 @ABA2396
- 提供打不开MAA_win7.exe的处理方法 @ABA2396
- 干员识别报错 @ABA2396
- subprocess handle leak @dantmnf
- 修复部分情况下，关卡掉落识别关卡名出错 @status102
- ClickedCorrectStage more ocrReplace regex @Constrat
- 不存在tmp时不保存version @ABA2396
- 剩余理智关卡在限定了关卡选择刷取条件（如次数）时会吃临期理智药 @ABA2396
- Eyjafjalla Alter regex [skip ci] @Constrat
- So Long Adele navigation @Constrat
- So Long Adele ROI change (pre Trials for Navigator) @Constrat
- Executor the Ex Foedere regex OCR @Constrat
- 测试版显示错误的ui版本号 @ABA2396
- 博朗台模式识别理智回复时间大于6分钟则返回失败 @ABA2396

### 其他 | Other

- 谁把我fody删了 @Cryolitia
- Logger析构时不滚动日志 (#7975) @status102
- update skill_ready_cls model (#7733) @MistEO
- 版本兼容 @ABA2396
- 修复SSS_自走机械搏斗场_浊蒂重岳塞雷娅泥岩_速冷脚本没有文件名后缀的错误 (#7879) @junyihan233
- 修复并无损压缩模板图片 (#7831) @zzyyyl
- 未在字典中的连接配置在使用自动检测时报错 @ABA2396
- 开机自启功能异常 .net 8 的 string.GetHashCode() 重启之后不一样了） @ABA2396
- 解决冲突 @ABA2396
- 兼容旧版本语言特性 @ABA2396
- 修复#7733文件路径错误 (#7980) @status102
- 优化ConfigurationHelper @ABA2396
- optimize template in 5e4002b058bc928a5e4b7c1dbd43f164682bbaf7 @zzyyyl
- 优化RoguelikeSetting格式 @status102
- 优化部分插件判断 @status102
- 删除没有用到的 task @zzyyyl
- Update qodana.yaml @ABA2396
- moved version requirement for faster triaging (#8121) @Constrat @ABA2396
- Update cn-bug-report.yaml @ABA2396
- Update en-bug-report.yaml @ABA2396
- 移除Sample本地运行时重复加载外服资源，避免产生潜在的错误 (#7930) @status102
- 简化部分初始化 @status102
- 添加词典 [skip ci] @ABA2396
- 简化集合 [skip ci] @ABA2396
- typo: 修改插件名以符合项目格式 @status102
- 调整剩余理智关卡的说明 @status102
- translated some infrast log lines (#7889) @Constrat @status102
- 肉鸽设置追加部分注释 @status102
- gitignore FodyWeavers.xsd @ABA2396
- 修改公招多选tag选项描述 @broken-paint
- added ps1 for cloning ResourceUpdater repo for local developing @Constrat
- update instruction for waydroid @horror-proton
- net8后不进行MAA_win7.exe备份 @ABA2396
- 修改readme中最大分辨率为2k @status102
- fix SyncRes build error @dantmnf
- dotnet8.0 (#7554) @hxdnshx @SherkeyXD @LiamSho @ABA2396 @moomiji
- update build tutorial @horror-proton
- bump `maa-cli` to `v0.4.0` (#7996) @wangl-cc

### For Developers

- vs设计器无法正常显示窗体 @ABA2396
- 修正CopilotTask参数名`is_adverse` -> `is_raid` (#8126) @status102
- 允许插件停用ProcessTask (#7954) @status102

### For Overseas

#### twxy

- 繁中服「吾導先路」復刻活動導航 (#8071) @momomochi987

#### YoStarEN

- EN Challenge mode battle list @Constrat
- changed template YostarEN I.S. enter fix #7766 fix #7886 @Constrat
- YosrarEN Lingering Echoes stage navigation @Constrat
- reformatted CharsNameOcr for YostarEN (removed duplicate + moved) [skip changelo] @Constrat
- YostarEN So Long Adele event navigation @Constrat

#### YoStarJP

- YoStarJP roguelike nextlevel ocr (#7859) @Manicsteiner @Constrat
- YoStarJP AnnihilationReturnFlag.png (#7964) @Manicsteiner
- YoStarJP ocr fix Chestnut, Bena (#8077) @Manicsteiner
- skipping recruit YostarJP, until further fix (#8028) @Constrat
- Replacing strange space characters in ResourceUpdater for JP Recruit fix #8027 @Constrat
- YoStarJP 塵影に交わる残響 復刻 navigation (#7888) @Manicsteiner
- [Doc]Operator JP name add for Adele @wallsman
- YoStarJP ocr fix Swire & Eyjafjalla Alter (#8038) @Manicsteiner
- YoStarJP operator name ocr fix (#7968) @Manicsteiner
- removed YostarJP recruit data skip @Constrat
- YoStarJP 火山と雲と夢色の旅路 navigation (#8015) @Manicsteiner
- YoStarJP ReceiveMail.png (#8004) @Manicsteiner
- YoStarJP ocr fix (#8086) @Manicsteiner

#### YoStarKR

- YoStarKR modified ocr text (#7960) @HX3N
- YoStarKR "CharsNameOcrReplace" modification (#8070) @HX3N
- YoStarKR added some missing task (#8069) @HX3N
- YoStarKR added template image (#7974) @HX3N
- YoStarKR ocrReplace regex fix (#7901) @HX3N
- YoStarKR ocrReplace text replacement (#7897) @HX3N
- YoStarKR LE stage navigation (#7896) @HX3N
- YoStarKR template image replacement (#7895) @HX3N
- YoStarKR LE Stage navigation (#7894) @HX3N
- YoStarKR ocr text replacement (#7845) @HX3N
- YoStarKR translation modify (#8047) @HX3N
- YoStarKR ocr text update for SO LONG ADELE (#8024) @HX3N
- YoStarKR modifiy tasks.json (#8014) @HX3N
- YoStarKR add ReceiveMail.png (#8009) @HX3N
- YoStarKR fixed ocr '紧急调遣' and '龙腾.(A/F/L)' (#8074) @HX3N
- YoStarKR ocrReplace modification (Infrast, Recruit, Roguelike) (#8072) @HX3N
