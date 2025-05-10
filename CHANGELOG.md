## v5.16.3

### 新增 | New

* 添加萨卡兹肉鸽劳作的清晨关卡策略 (#12626) @1286587265 @Daydreamer114 @Saratoga-Official
* 指定次数联动连战, 修复连战导致指定次数失效, Auto模式不再溢出理智 (#12592) @status102 @ABA2396 @Constrat
* 添加 自定义 webhook 功能 (#12602) @KagurazakaIris @ABA2396
* 肉鸽适配新干员 (#12599) @Saratoga-Official
* Mac AUTO连续作战 @hguandl
* 更新 333 搓玉一天三换排班表 (#12604) @E022-23093
* 更新 243 极限效率一天四换排班表（20250507 修订） (#12598) @bodayw

### 改进 | Improved

* 日志压缩包压缩等级提升 (#12622) @BxFS
* 基建使用切换职业栏返回列表最左侧 (#12594) @ABA2396

### 修复 | Fix

* 切换账号失败 @Daydreamer114
* 模拟器路径选择功能弹窗异常 @ABA2396
* 截图增强显示错误 @ABA2396
* 无法进入活动 @ABA2396
* 刷理智结束后卡在“理智药选择”界面 @ABA2396

### 文档 | Docs

* 连战次数文档更新 @status102
* 更正保全派驻协议文档中的拼写错误 (#12578) @lucienshawls
* 更正主文档和中文文档中的两处错别字 (#12577) @lucienshawls

### 其他 | Other

* 调整提示颜色 @ABA2396
* 开始战斗前闪退无限循环识别，关卡名检查放宽至一分钟 (#12580) @ABA2396
* 肉鸽招募 练度不够干员的 priority 可能为正 @Daydreamer114
* 战斗次数识别错误缓解 @status102
* 关闭连战列表点击 @status102
* 关闭连战列表 @status102
* 移除肉鸽难度hard code (#12587) @status102
* 连战次数修复, 减少hard code @status102
* 补上之前漏改的 tasks 路径修改 @ABA2396
* 连战减少不必要的次数调整 @status102
* 战斗次数识别错误处理 @status102
* 关卡理智识别流程通用化 @status102
* 减少一次不必要的连战次数变更 @status102
* 移除过旧的任务参数 @status102
* 优化提示 @ABA2396
* 连战新增 -1 表示禁用切换 @ABA2396
* 修改连接失败提示描述 @ABA2396
* YostarKR BattleStartPre add ocrReplace pattern @HX3N
* YostarJP ocr edits (#12607) @Manicsteiner
* HttpService调整, Post函数追加 (#12605) @status102
* 好像提示了也没人看 @ABA2396
* 加点常用的进来 @status102
* 调整超时提醒输出 @ABA2396
* 理智识别 @status102
* Reapply "feat:实现自适应调整连战次数 (#12555)" @Yoak3n @ABA2396
