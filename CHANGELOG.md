## v5.4.0-beta.2

### 新增 | New

* 无法连接设备时尝试断开连接然后重新连接 (#9433) @lingting
* GPU 加速推理 (DX12/DirectML) @dantmnf @SherkeyXD
* SideStory「生路」活动导航 @ABA2396
* 增加暂停放技能和暂停撤退 (#9348) @Lancarus

### 改进 | Improved

* 优化仓库识别结果展示，支持多语言显示 (#9434) @ABA2396
* 继续#7904的重构 (#9409) @IzakyL
* 优化代码 @ABA2396
* 优化数据绑定逻辑，减少 AsstProxy 逻辑处理 @ABA2396
* 优化肉鸽选项显示逻辑 @ABA2396
* 增加更新时解压更新包失败日志，增加解压失败解决方案 @ABA2396
* 优化萨米肉鸽棘刺技能携带 (#9400) @Daydreamer114
* 优化热更新 @ABA2396
* 允许使用“feat!”表示重大更新 @zzyyyl
* 更新 py 回调 @ABA2396
* 优化关卡选择为剿灭时的逻辑判断 @ABA2396
* 优化肉鸽shopping策略 (#9332) @ntgmc
* 优化萨米肉鸽夺树者高台对boss输出策略 (#9294) @Daydreamer114
* 更新关卡列表与提示延迟至空闲时间，避免动态修改关卡列表 @ABA2396

### 修复 | Fix

* intentionally leak onnxruntime objects to avoid crash @dantmnf
* 弹药类技能不能二次关闭的问题 (#9379) @HoshinoAyako
* 自动战斗跳过对话模板更新 (#9380) @HoshinoAyako
* 无法跳过关卡开始的剧情 @ABA2396
* add `/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR` @horror-proton
* 战斗列表禁用包含非法字符的关卡名添加 (#9363) @status102
* add AsstSetConnectionExtras to wine bridge @dantmnf
* check before destroy callback @dantmnf
* 修复时间比较报错的问题 @SherkeyXD
* 刷理智结束时有概率报错 @ChingCdesu
* fix endless loop in InfrastProductionTask @horror-proton
* fix coredump caused by screencap failure @horror-proton
* 保全文件命名错误 (#9322) @ntgmc

### 其他 | Other

* update cn-mumu-report.yaml @Rbqwow
* fix missing includes @horror-proton
* use factory function instead of class @horror-proton
* bump zzyyyl/issue-checker from 1.7 to 1.8 @zzyyyl
* 修正生息演算相关打标签逻辑 @zzyyyl
* changelog生成器repo独立设置 @SherkeyXD
* move functions to TileCalc2 and TileDef @horror-proton
* Amiya new promotion in RoguelikeBattleTaskPlugin (#9377) @Manicsteiner
* use NativeLibrary for wine check @dantmnf
* Amiya new promotion (#9347) @178619
* 修正临时招募优先度描述错误 (#9344) @IzakyL
* StyleCop链接使用cdn @SherkeyXD

### For Overseas

#### txwy

* 初步適配繁中服薩米肉鴿 (#9429) @momomochi987

#### YostarEN

* YostarEN RS navigation @Constrat
* en 服在没有源石和理智药的情况下无法退出刷理智任务 @ABA2396

#### YostarJP

* YoStarJP RS navigation (#9427) @Manicsteiner
* YoStarJP roguelike ocr fix (#9426) @Manicsteiner

#### YostarKR

* YoStarKR RS stage navigation @HX3N
* depot analyzer to detect multipliers for KR (#9343) @178619
* YoStar add translations for new feat @HX3N
