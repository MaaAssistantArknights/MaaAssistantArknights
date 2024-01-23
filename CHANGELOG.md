## v5.0.0-beta.2

### 新增 | New

- 肉鸽开局干员支持输入任意客户端对应名称 (#7726) @ABA2396

### 改进 | Improved

- 优化自动战斗部署栏识别条件，保全增加节能模式 (#8044) @ABA2396
- 优化wpf肉鸽结算输出格式 for i18n (#8011) @status102 @Constrat
- 萨米肉鸽适配新干员用法 (#8033) @Lancarus
- more scalable solution to 7bbb5f3 thanks to @ABA2396 @Constrat
- 限制理智识别重试次数 fix #7998 @status102

### 修复 | Fix

- 剩余理智关卡在限定了关卡选择刷取条件（如次数）时会吃临期理智药 @ABA2396
- Replacing strange space characters in ResourceUpdater for JP Recruit fix #8027 @Constrat
- Eyjafjalla Alter regex [skip ci] @Constrat
- So Long Adele navigation @Constrat
- So Long Adele ROI change (pre Trials for Navigator) @Constrat
- Executor the Ex Foedere regex OCR @Constrat
- 测试版显示错误的ui版本号 @ABA2396

### 其他 | Other

- 基建排班工具链接更新 @ABA2396
- skipping recruit YostarJP, until further fix (#8028) @Constrat
- 不存在tmp时不保存version @ABA2396
- Logger析构时不滚动日志 (#7975) @status102
- [Doc]Operator JP name add for Adele @wallsman
- YoStarKR translation modify (#8047) @HX3N
- YoStarJP ocr fix Swire & Eyjafjalla Alter (#8038) @Manicsteiner
- YoStarJP operator name ocr fix (#7968) @Manicsteiner
- removed YostarJP recruit data skip @Constrat
- reformatted CharsNameOcr for YostarEN (removed duplicate + moved) [skip changelo] @Constrat
- YoStarKR ocr text update for SO LONG ADELE (#8024) @HX3N
- update build tutorial @horror-proton
- YoStarKR modifiy tasks.json (#8014) @HX3N
- YostarEN So Long Adele event navigation @Constrat
- YoStarJP 火山と雲と夢色の旅路 navigation (#8015) @Manicsteiner
- bump `maa-cli` to `v0.4.0` (#7996) @wangl-cc
- YoStarKR add ReceiveMail.png (#8009) @HX3N
- YoStarJP ReceiveMail.png (#8004) @Manicsteiner
