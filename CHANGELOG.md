## v5.22.1

### 博士五连冠（x | Highlight

本次版本我们修复了大量 bug，为本次夏活提供了小游戏支持。

#### SideStory「墟」

在这个版本，我们为本次夏活的【相谈室】小游戏提供了支持，但请注意：

牛牛**只会**选择增加最多“兴味”的选项，丝毫不考虑“信任”，也不会考虑客人类型所决定的最大回合数（即“情绪”），

牛牛只会帮你解放双手，如果想要追求最大收益，请关注其他博士总结的攻略吧~

#### 集成战略 #6「岁的界园志异」

我们修复了大量界园肉鸽和其他肉鸽相关的 bug，并且优化了刷钱流程。

现在你可以在 MAA 选择指挥分队开局，并选择难度 3（当然游戏内得先解锁难度 3），获得最大的刷钱效率。

刷等级功能也作了初步适配（但尚未测算效率），不过刷热水壶开局还是需要再等等，牛牛在努力啦~

----

#### [CN ONLY] SideStory "Ato"

In this version, we've added support for the [Conversation Room] minigame in this summer event. Please note:

NiuNiu will **ONLY** select options that maximize "Interest", completely disregarding "Trust" and the maximum turn limit determined by guest types (i.e., "Mood").

NiuNiu is designed to automate the process. For maximizing rewards, please refer to strategy guides curated by other Doctors.

#### [CN ONLY] Integrated Strategies #6 "Sui's Garden of Grotesqueries"

We've fixed numerous bugs related to this and other I.S., while optimizing the coin investing process.

You may now select Leader Squad as your starting and choose Difficulty 3 (requires prior in-game unlock) in MAA for maximum coin investing efficiency.

Preliminary adaptation has been made for gaining Experience Points (efficiency metrics pending). The "Grind to obtain Hot Water Kettle or Elite II Operators" strategy requires further development - NiuNiu is working hard!

----

以下是详细信息：

## v5.22.1

### 新增 | New

* 更新技能识别模型，采用更全面的数据集和使用focal loss训练 (#13498) @scyyh11
* Unhandled exception (#13535) @ABA2396
* 不忽略肉鸽的报错 @ABA2396
* 新增小游戏 @hguandl

### 改进 | Improved

* 更新CropRoi的依赖 @SherkeyXD
* wpf Win10+Toast通知使用系统内置通知声而不是独立播放 @status102
* 手动点击查看公告时获取公告更新前先显示本地公告 @ABA2396
* 优化 ComboBox 搜索 @ABA2396

### 修复 | Fix

* 傀影卡历史馈赠 @ABA2396
* 部分策略无法离开诡谲断章 @ABA2396
* wpf肉鸽投资N个源石锭NumericUpDown绑定失效 @status102
* 购买商品时因动画导致点击无效 @ABA2396
* 不开线索交流可能导致技能效率判断崩溃 @ABA2396
* RoutingRefreshNode roi expand (#13515) @Manicsteiner
* 去伪存真卡在灵光乍现 @Saratoga-Official
* update YostarEN IS3 templates fix #13502 @Constrat
* 概率卡在出洞后的投钱 @ABA2396
* 界园卡通关结算页面 @ABA2396
* YostarKR corrected ocrReplace in IS5 StrategyChange_mode1 @HX3N
* 生息演算组装数量识别失败时取消组装 @ABA2396
* 其他肉鸽可能点到月度小队 @ABA2396
* clang编译警告 (#13482) @hguandl
* 模拟器卸载/更新注册表残留可能导致截图增强弹窗死循环 @ABA2396
* 仓库识别概率无法识别部分物品 @ABA2396

### 文档 | Docs

* fix typo (#13503) @Burnside999

### 其他 | Other

* 调整部分UI绑定namespace @status102
* rename FreeRecruit -> FreeGacha @status102
* 繁中服「詞祭」介面主題截圖 (#13521) @momomochi987
* 再度調整繁中服「崔林特爾梅之金·復刻」活動導航 (#13519) @momomochi987
* YostarJP ocr fix (#13500) @Manicsteiner
* 任务出错内容翻译 @ABA2396

## v5.22.0

### 新增 | New

* 相谈室 小游戏 (#13461) @ABA2396
* SideStory「墟」关卡导航 @SherkeyXD
* 界园肉鸽补充战斗逻辑 (#13433) @Saratoga-Official
* wpf支持作业版本限制 @status102
* 界园肉鸽补充事件选择逻辑 (#13400) @Saratoga-Official
* 烧水分队不存在时自动替换为当前分队 @ABA2396
* 刷开局模式禁用源石锭达到上限时停止 @ABA2396
* 填写错误安装路径时重新弹窗提示 @ABA2396
* 肉鸽使用助战新增需先填写开局干员的提示 @ABA2396
* 新增是否进行线索交流选项 (#13368) @Lemon-miaow

### 改进 | Improved

* 优化水月肉鸽招募&傀影战斗 (#13344) @ntgmc @Saratoga-Official
* 肉鸽节点识别 (#13427) @ABA2396
* 更新繁中服「崔林特爾梅之金」活動導航 (#13450) @momomochi987

### 修复 | Fix

* 卡遇见板子 @ABA2396
* 祠堂口事件识别失败 @Saratoga-Official
* 调整飞来横祸 EW 站位 @Saratoga-Official
* 调整往事暗哑离域检查EW站位 @Saratoga-Official
* 拾取关卡掉落后找不到节点时放弃 @ABA2396
* 祠堂口事件识别失败 @ABA2396
* 夕娥忆 关卡名识别错误 @ABA2396
* 重复报关卡难度 @ABA2396
* 禳解事件识别错误 @ABA2396
* 修复层名识别问题 -- "云瓦亭" -> "云(瓦)?亭" @Alan-Charred
* 拥有时光之末战斗失败后无法跳过动画 @ABA2396
* 战斗失败没进结算识别 @ABA2396
* 汝吾门 识别 @ABA2396
* wpf自动战斗干员模组选择默认值错误 @status102
* 修复任务过程中修改剩余理智后导致关卡无法连续执行的问题 @ABA2396
* 掌灯与引烛 事件识别 @ABA2396
* 肉鸽通关难度成就解锁判定错误 @status102
* 狭路相逢善恶同道识别 (#13383) @Saratoga-Official
* 修复放弃招募任务链；避战识别不到得偿所愿 (#13395) @Alan-Charred @ABA2396
* 无法选中 `坚不可摧` 与 `随心所欲` 分队 @ABA2396
* 无法使用连战 @ABA2396
* fix: 装置“斩”识别 (#13459) @AimOwO
* fix: 催债鬼夜叉二次选择 @Saratoga-Official
* YostarKR 海神的信者 ocr edit @HX3N
* YostarKR 黍 ocr edit @HX3N
* YostarKR removes spaces in StrategyChange_mode and NextLevel @HX3N
* JieGarden roguelike stage templates @Alan-Charred
* remove leading digits in IS encounters for YostarEN @Constrat
* typo for BattleStage Sarkaz EN @Constrat
* clang style @Constrat

### 文档 | Docs

* 文~档~大~更~新，开始失败喽—— (#13410) @Rbqwow @Saratoga-Official @ABA2396
* useRaw 仅当 withoutDet 为 true 时生效 @ABA2396
* 更新文档与 maa_tasks_schema @ABA2396

### 其他 | Other

* 优化界园层名识别 @ABA2396
* 更新截图 @ABA2396
* 优化成就排序与描述 @ABA2396
* 优化界面显示效果 @ABA2396
* 优化多层事件逻辑 @ABA2396
* 鲍老板连锁去掉引号 @Saratoga-Official
* 对掉落信息进行排序 与 ToolTip 保持一致 (#13404) @travellerse
* ComboBox 的光标颜色不会随主题色变化 @ABA2396
* 增加 hp 识别标志 @ABA2396
* 死循环 @ABA2396
* 其他肉鸽的屎山爆炸了 @ABA2396
* 野鬃识别 @ABA2396
* 调整界面显示效果 @ABA2396
* 完整显示注入 maa 的 dll 的路径 @ABA2396
* PipelineAnalyzer 支持使用灰度图匹配文字 @ABA2396
* 添加一键轮换说明 @ABA2396
* 统一 TooltipBlock 位置 @ABA2396
* 调整安全屋颜色范围 @ABA2396
* 调整颜色阈值 @ABA2396
* 添加白色部分识别 @ABA2396
* 调整返回主界面点击位置 @ABA2396
* 怎么jb有个0.6的阈值 @ABA2396
* 不期而遇失败时尝试放弃 @ABA2396
* 在事件中卡死时尝试放弃 @ABA2396
* 删除 999 @ABA2396
* 彻底卡住时记录截图后尝试放弃 @ABA2396
* 地镇事件选择 @ABA2396
* 调整翻译 @ABA2396
* 删除多余注释 @ABA2396
* 强制替换 adb 改为使用本地 adb @ABA2396
* 删除无用函数 @ABA2396
* i18n english key @Constrat
* update IS for EN @Constrat
* KR dehardcode string names and tweak translations @HX3N
* JP edits (#13460) @Manicsteiner
* update YostarEN OR navigation @Constrat
* dehardcode string names follows b0a53cd5b07d0ae13a32b781613f908a1d8b9896 @Constrat
* update ignore templates @Constrat
* i18n: zh-tw tweak translations (#13467) @momomochi987
