## v5.4.0

### 新增 | New

* 减少上报企鹅物流的重试次数和等待时间 (#9527) @soundofautumn
* 更新 243 极限效率一天四换排班表（20240608 修订） (#9496) @bodayw
* 更新模板图片优化工具并优化模板图片 (#9459) @zzyyyl
* 在理智恢复满时推送信息 (#9285) @ABA2396
* 增加设置开机自启失败提示 @ABA2396
* MacGui 自定义填写招募次数 (#6182) @hguandl
* 增加保全、肉鸽、抄作业可配置截图最小间隔 @ABA2396
* 生息演算支持制造物品刷点数 (#9268) @ABA2396
* Mac Gui 支持连战次数、指定掉落 @hguandl
* 在特定环节执行启动前/后脚本 (#9153) @Linisdjxm
* 无法连接设备时尝试断开连接然后重新连接 (#9433) @lingting
* GPU 推理加速 @dantmnf @SherkeyXD
* SideStory「生路」活动导航 @ABA2396
* 增加暂停放技能和暂停撤退 (#9348) @Lancarus

### 改进 | Improved

* 适配PlayCover更新 @hguandl
* 优化日志输出控制 @ABA2396
* 在掉落不超出一页时，不额外滑动掉落列表及截图拼接 (#9260) @status102
* 优化萨米肉鸽自然之怒ew部署 (#9280) @Daydreamer114
* 更新macOS SDK版本 @hguandl
* 垂直滑动优化 @ABA2396
* 优化萨米ew部署策略 (#9249) @Daydreamer114
* 优化萨米肉鸽策略 (#9234)(#9232) @Lancarus
* 优化萨米肉鸽部分关卡策略 (#9224) @Lancarus
* 优化仓库识别结果展示，支持多语言显示 (#9434) @ABA2396
* 继续7904的重构 (#9409) @IzakyL
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

* 五周年活动结束后单选领取月卡功能时报错 fix #9452 @ABA2396
* increase mask range for SkipThePreBattlePlotConfirm (#9446) @Constrat
* arkntools localization (#9438) @Tsuk1ko
* 公招没有使用足量的加急许可 @Toby-Shi-cloud
* “dorm_notstationed_enabled”参数默认值错误 @ABA2396
* 基建换班重试时只找缺少干员，已选干员无法入驻 @ABA2396
* 生息演算更新后无法选择日期 @ABA2396
* 选择休眠动作则结束后脚本没时间运行 @ABA2396
* constexpr 构造错误 @ABA2396
* 尝试修复水月肉鸽卡数据回溯 (#9276) @Lancarus
* 肉鸽部分选项在未显示时仍然生效 @ABA2396
* 尝试修复肉鸽编队错误,max page误判时仍然重置回最左边 (#9262) @Lancarus
* 公招 `m_first_tag` 匹配个数少于 3 个时的非预期情况 (#9258) @70CentsApple
* 尝试修复生息演算不进入下一天 (#9273) @HoshinoAyako
* 部分界面显示不全 @ABA2396
* 自然之怒.json格式错误 @Lancarus
* Mac Gui使用Swift 5.9以下的语法 @hguandl
* 修复Mac Gui在CI的编译错误 @hguandl
* unable to get rewards because ReceiveAward is too fast @Constrat
* 尝试修复萨米卡0干扰并优化密文板使用策略 (#9223) @Lancarus
* 肉鸽特殊情况下第三层boss门口退出 (#9222) @Lancarus
* call_command timeout时提前返回 std::nullopt @ABA2396
* type cast @status102
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

* 简化Release日志输出 @ABA2396
* unlocks update options when UpdateCheck = false @Constrat
* update operator.md @HX3N @wallsman
* Improve error output in recruitment ResourceUpdater @Constrat
* 删除多余 style @ABA2396
* issue模板添加mumu反馈 (#8993) @Rbqwow @ABA2396
* fix wrong link maa_cli (#9264) @wlwxj @ntgmc
* 增加开技能最小间隔时间 @ABA2396
* 界面添加支援道具名称提示，传入内容为空白时自动切换为 荧光棒 @ABA2396
* Update 肉鸽辅助协议.md @Lancarus
* reduced filename check description for more info @Constrat
* Add trace log for scaled coordinates in ControlScaleProxy click & swipe @ABA2396
* no need to checkout sumodule after https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9168 @Cryolitia
* 忽略括号周围resharper格式检查 @ABA2396
* 统一使用OnUIThread，AddLog默认在ui线程执行 @ABA2396
* Revert "fix: adding delay between swipe and click" (#9170) @Constrat
* format @status102
* fix license typo @Cryolitia
* 增加「实例已销毁」 (#9201) @aleck099
* add CITATION @Cryolitia
* add LICENSE.SPDX @Cryolitia
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

* 修正繁中服薩米肉鴿無法開始探索、密文板無法宣告的問題 (#9484) @momomochi987
* 繁中服「火山旅夢」活動導航 (#9497) @momomochi987
* 補充繁中服薩米肉鴿相關內容 (#9450) @momomochi987
* 初步適配繁中服薩米肉鴿 (#9429) @momomochi987

#### YostarEN

* YostarEN RS navigation @Constrat
* priority to Flint 点火石 trap instead of operator @Constrat
* ResourceUpdater temporary fix for trap_138_winstone having same as Flint @Constrat
* en 服在没有源石和理智药的情况下无法退出刷理智任务 @ABA2396

#### YostarJP

* YostarJP ocr fix 锏 (#9447) @Manicsteiner
* YostarJP RS navigation (#9427) @Manicsteiner
* YostarJP roguelike ocr fix (#9426) @Manicsteiner
* YostarJP roguelike ocr fix (#9203) @Manicsteiner

#### YostarKR

* YostarKR RS stage navigation @HX3N
* YostarKR modified CharsNameOcrReplace @HX3N
* Yostar add translations for new feat @HX3N
* depot analyzer to detect multipliers for KR (#9343) @178619
