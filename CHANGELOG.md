## v5.16.3

在这个版本，我们优化了自适应调整连战次数功能，每次开始前均调整为可用的最大次数，避免了理智溢出的风险.

> 典型流程示例:  
> 当前20理智, 上限150, 关卡消耗21理智 (6连战需126理智)  
> 
> 情况1: 有2瓶80理智药, 设置吃2瓶, 吃完两瓶理智药超出上限 (20 + 160 = 180) → 先吃一瓶 (20 + 80 = 100) 并调整次数为4连战 (4 × 21 = 84), 完成后再吃一瓶并消耗完剩下的理智  
> 情况2: 有2瓶80理智药, 设置只吃1瓶吃1瓶 (+80) 达到吃药上限，关闭窗口触发调整 → 自动调整为4连战  
> 情况3: 有1瓶80理智药, 自动吃掉1瓶 (+80) 无药可吃触发调整 → 自动调整为4连战

并且指定次数将联动连战次数

> 示例: 假设当前 100 理智, 关卡消耗 6 理智  
> 
> 旧版本: 指定次数设为 3, 连战次数设为 5, 将作战 5 * 3 = 15 次, 消耗 15 x 6 = 90 理智  
> 新版本: 指定次数设为 10, 连战次数设为 4, 将作战 4 * floor(10 / 4) = 8 次, 消耗 8 x 6 = 48 理智  
> 新版本: 指定次数设为 10, 连战次数设为 AUTO, 将作战 6 * floor(10 / 6) + (10 % 6) = 10 次, 消耗 10 x 6 = 60 理智  

如果您遇到任何意外行为，请通过【设置】→【问题反馈】生成日志并提交给我们~

----

In this version, we've optimized the adaptive Serie count adjustment feature. Before each operation, the system will now automatically adjust to the maximum available Serie count, eliminating the risk of sanity overflow.

> Typical workflow examples:  
> Current sanity: 20, Max sanity: 150, Stage cost: 21 sanity (6-Serie requires 126 sanity)  
> 
> Case 1: Having 2×80 sanity potions, set to consume 2 potions. Full consumption would exceed max sanity (20 + 160 = 180) → First consume one potion (20 + 80 = 100) and adjust to 4-Serie (4 × 21 = 84). After completion, consume the second potion and expend remaining sanity.  
> Case 2: Having 2×80 sanity potions, set to consume only 1 potion (+80) reaches consumption limit. Closing window triggers adjustment → Automatically adjusts to 4-Serie.  
> Case 3: Having 1×80 sanity potion, auto-consume 1 potion (+80). With no remaining potions, triggers adjustment → Automatically adjusts to 4-Serie.

Additionally, specified operation count will now coordinate with Serie count:

> Example: Current 100 sanity, stage cost 6 sanity  
> 
> Old version: Specified count set to 3, Serie count set to 5 → Would complete 5 * 3 = 15 operations, consuming 15 x 6 = 90 sanity.  
> New version: Specified count set to 10, Serie count set to 4 → Will complete 4 * floor(10 / 4) = 8 operations, consuming 8 x 6 = 48 sanity.  
> New version: Specified count set to 10, Serie count set to AUTO → Will complete 6 * floor(10 / 6) + (10 % 6) = 10 operations, consuming 10 x 6 = 60 sanity.  

If you encounter any unexpected behavior, please generate logs via [Settings] → [Issue Report] and submit them to us~

----

以下是详细内容： Changelog below:

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
