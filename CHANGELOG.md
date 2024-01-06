## v4.28.8

### 新增 | New

- 萨米肉鸽新板子 @ABA2396
- 更新繁中服生息演算最後一天的日期 (#7903) @momomochi987
- 初步适配萨米肉鸽dlc关卡等 (#7924) @Lancarus

### 改进 | Improved

- 修改 前行的林地 默认选择 @ABA2396
- 优化萨米肉鸽部分关卡逻辑 (#7925) @Lancarus
- 优化刷理智-已开始战斗x次时临期理智药使用输出以增加可读性 (#7846) @status102
- 优化截图耗时额外提示文本触发条件，改为由平均耗时触发 (#7868) @status102
- 捕获启动模拟器时产生的权限不足异常 (#7908) @Chiichen

### 修复 | Fix

- 勾选启动后直接最小化时无法使用右键强制显示 @ABA2396
- 使用启动旧版.cmd启动后下次更新报错 @ABA2396
- 下调检测阈值以尝试修复刷理智卡在次数选择 fix #7919 @status102
- reduced Mizuki@Roguelike@MissionFailedFlag threshold for iOS fix #7864 temporary fix as #7872 exists @Constrat
- 修复部分情况下肉鸽结算点击错误 @status102
- 未在字典中的连接配置在使用自动检测时报错 @ABA2396
- 修复肉鸽-刷钱模式下仍然会在商店进行招募、购物的错误 (#7863) @status102
- 配置修改后无法保存 @ABA2396
- 修复刷理智结束时理智输出错误 @status102
- 清剩余理智时卡在选择次数 @ABA2396
- 修复README的一处链接错误 (#7905) @Chiichen
- 修复肉鸽刷源石锭模式卡住的问题 (#7882) @zzyyyl
- EN服 无法自动吃大理智药 @ABA2396
- 提供打不开MAA_win7.exe的处理方法 @ABA2396
- 干员识别报错 @ABA2396
- 修复部分情况下，关卡掉落识别关卡名出错 @status102
- ClickedCorrectStage more ocrReplace regex @Constrat
- EN Challenge mode battle list @Constrat
- 博朗台模式识别理智回复时间大于6分钟则返回失败 @ABA2396

### 其他 | Other

- subprocess handle leak @dantmnf
- 版本兼容 @ABA2396
- 修复SSS_自走机械搏斗场_浊蒂重岳塞雷娅泥岩_速冷脚本没有文件名后缀的错误 (#7879) @junyihan233
- changed template YostarEN I.S. enter @zzyyyl
- 移除 StageEncounterClickToLeave 任务 (#7887) @zzyyyl
- changed template YostarEN I.S. enter fix #7766 fix #7886 @Constrat
- 修复并无损压缩模板图片 (#7831) @zzyyyl
- 谁把我fody删了 @Cryolitia
- 兼容旧版本语言特性 @ABA2396
- ui 和 core 版本不一致时开始任务添加 waring 提示 @ABA2396
- optimize template in 5e4002b058bc928a5e4b7c1dbd43f164682bbaf7 @zzyyyl
- 删除没有用到的 task @zzyyyl
- gitignore FodyWeavers.xsd @ABA2396
- 修改公招多选tag选项描述 @broken-paint
- added ps1 for cloning ResourceUpdater repo for local developing @Constrat
- YoStarKR ocrReplace regex fix (#7901) @HX3N
- YoStarKR ocrReplace text replacement (#7897) @HX3N
- YoStarKR LE stage navigation (#7896) @HX3N
- YoStarKR template image replacement (#7895) @HX3N
- YoStarKR LE Stage navigation (#7894) @HX3N
- YosrarEN Lingering Echoes stage navigation @Constrat
- YoStarJP 塵影に交わる残響 復刻 navigation (#7888) @Manicsteiner
- update instruction for waydroid @horror-proton
- YoStarJP roguelike nextlevel ocr (#7859) @Manicsteiner @Constrat
- net8后不进行MAA_win7.exe备份 @ABA2396
- 修改readme中最大分辨率为2k @status102
- fix SyncRes build error @dantmnf
- dotnet8.0 (#7554) @hxdnshx @SherkeyXD @LiamSho @ABA2396 @moomiji
- YoStarKR ocr text replacement (#7845) @HX3N
