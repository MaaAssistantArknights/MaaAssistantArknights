## v5.4.0-beta.1

### 新增 | New

* 在理智恢复满时推送信息 (#9285) @ABA2396
* 增加设置开机自启失败提示 @ABA2396
* MacGui 自定义填写招募次数 (#6182) @hguandl
* 增加保全、肉鸽、抄作业可配置截图最小间隔 @ABA2396
* 生息演算支持制造物品刷点数 (#9268) @ABA2396
* Mac Gui 支持连战次数、指定掉落 @hguandl
* 在特定环节执行启动前/后脚本 (#9153) @Linisdjxm

### 改进 | Improved

* 在掉落不超出一页时，不额外滑动掉落列表及截图拼接 (#9260) @status102
* 优化萨米肉鸽自然之怒ew部署 (#9280) @Daydreamer114
* 更新macOS SDK版本 @hguandl
* 垂直滑动优化 @ABA2396
* 优化萨米ew部署策略 (#9249) @Daydreamer114
* 优化萨米肉鸽策略 (#9234)(#9232)(#9224) @Lancarus

### 修复 | Fix

* #7266: 公招没有使用足量的加急许可 @Toby-Shi-cloud
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
* 尝试修复萨米卡0干扰并优化密文板使用策略 (#9223) @Lancarus
* 肉鸽特殊情况下第三层boss门口退出 (#9222) @Lancarus
* call_command timeout时提前返回 std::nullopt @ABA2396
* type cast @status102
* unable to get rewards because ReceiveAward is too fast @Constrat

### 其他 | Other

* issue模板添加mumu反馈 (#8993) @Rbqwow @ABA2396
* chroe: 增加开技能最小间隔时间 @ABA2396
* 界面添加支援道具名称提示，传入内容为空白时自动切换为”荧光棒“ @ABA2396
* Update 肉鸽辅助协议.md @Lancarus
* 忽略括号周围resharper格式检查 @ABA2396
* 统一使用OnUIThread，AddLog默认在ui线程执行 @ABA2396
* 增加「实例已销毁」 (#9201) @aleck099
* fix wrong link maa_cli (#9264) @wlwxj
* fix license typo @Cryolitia
* reduced filename check description for more info @Constrat
* Add trace log for scaled coordinates in ControlScaleProxy click & swipe @ABA2396
* no need to checkout sumodule after https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9168 @Cryolitia
* add CITATION @Cryolitia
* add LICENSE.SPDX @Cryolitia

### For Overseas

#### YostarJP

* YoStarJP roguelike ocr fix (#9203) @Manicsteiner
