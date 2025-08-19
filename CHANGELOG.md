## v5.23.2

### 夏活第二弹 | Highlight

本次更新带来了大家期盼已久的多项适配，一起来看看吧：

#### 集成战略 #6「岁的界园志异」

本次更新我们对界园肉鸽做了多项适配：

1. 现在牛牛支持刷开局策略了，博士可以用来自动刷热水壶等等~
2. 刷等级策略下，牛牛支持进入道中的树洞层，而不是直接跳过。

#### 其他方面

本次更新，牛牛支持对新的动态主题「梦乡」进行识别，博士们可以尽情享受两年前的那个夏天啦~

以及~~虽然不知道有什么作用，但~~支持对设置的各个小节进行手动排序，拖动设置页左侧目录即可。

----

#### [CN ONLY] Integrated Strategies #6 "Sui's Garden of Grotesqueries"

In this update, we've made several adaptations for the Garden I.S.:

1. MAA now supports "Grind to obtain Hot Water Kettle or Elite II Operators" strategies, allowing Doctors to automatically obtain Hot Water Kettle~
2. Under Gaining Experience Points strategies, MAA can now enter the Wonderland Encounter(误入奇境) during the run instead of skipping them directly.

#### Other Aspects

In this update, MAA now supports recognition of the new dynamic theme "Dreamland", allowing Doctors to fully relive that summer from two years ago~

Additionally~~though NiuNiu isn’t quite sure what purpose it serves, but~~ manual sorting of various sections in the settings is now supported. Simply drag the directory on the left side of the settings page.

----

以下是详细内容：

## v5.23.2

### 新增 | New

* 肉鸽难度括号里显示一下对应难度 @ABA2396

### 改进 | Improved

* 自动编队干员组显示模组需求 (#13796) @travellerse @ABA2396
* copilot useFormation ui @status102

### 修复 | Fix

* tw 服缺少薩米 mode 1 对应层名翻译 @ABA2396
* 支持新建关闭状态的节点，并修正日志输出 (#13766) @travellerse
* 界园树洞烛火数量过少时无法匹配退出 @ABA2396
* Y 模组模板匹配阈值要求过高 @ABA2396
* 退出商店时如果没有识别到下一层的文字会卡在遇见密文板 @ABA2396
* 因模组约束取消干员时删除编队信息 (#13780) @travellerse
* 萨米无法离开深埋迷境 @Saratoga-Official
* 因动画或卡顿导致无法退出商店 @ABA2396
* use Official template for Global (#13801) @Constrat @momomochi987 @HX3N @Manicsteiner
* EN Phantom increase ROI for StrategyChange @Constrat
* EN Sami big typo. @Constrat

### 其他 | Other

* 修改保全作业站位和工具人数量 (#13797) @Saratoga-Official
* 調整繁中服水月、薩卡茲肉鴿相關內容 (#13806) @momomochi987
* 自动战斗日志本地化 (#13777) @soundofautumn @ABA2396
* 修改并添加保全派驻作业 (#13783) @Saratoga-Official
* 自动战斗中的 `使用编队` 改为勾选框 @ABA2396
* missing tasks for JP/KR/EN IS @Manicsteiner @HX3N @Constrat

## v5.23.1

### 修复 | Fix

* 商店没钱了不会停止投资 @ABA2396

## v5.23.0

### 新增 | New

* 新保全作业 (#13754) @Saratoga-Official
* 界园肉鸽烧水 (#13690) @SherkeyXD @ABA2396
* 支持 `梦乡` 主题 (#13693) @ABA2396
* 优化标题栏显示效果 @ABA2396
* 版本后面也跟个日期 @ABA2396
* 幸运掉落用家具零件代替显示 @ABA2396
* `ADB 连接失败时尝试启动模拟器` 勾选框在 `连接设置` 与 `启动设置` 中同时显示 @ABA2396
* 支持 MuMu 5555 端口使用控制台退出与截图增强 @ABA2396
* 反转网络桥接提示框的按钮位置，避免误勾选 @ABA2396
* 自动编队支持指定使用编队 @ABA2396
* 调整自动战斗页面布局 @ABA2396
* 调整编队界面翻译 @ABA2396
* 适配界园树洞节点 (#13552) @travellerse
* 设置排序 (#13699) @ABA2396
* 调整自动战斗页面布局 @ABA2396
* 使用编队宽度自适应 @ABA2396

### 改进 | Improved

* 合并 regex @status102
* 仓库识别改用二值化后的非 char 模型识别 (#13667) @ABA2396
* 繁中服基建 + 保全派駐 7 相關更新 (#13643) @momomochi987
* 更新 nuke 版本与构建 @SherkeyXD
* 更新 MaaBuilder 中使用的过时方法 @SherkeyXD
* 代理倍率、肉鸽难度 使用 `不切换` 代替 `当前`、`不使用` 以符合实际逻辑 (#13612) @status102 @HX3N @Constrat
* wpf 肉鸽难度初始化优化 @status102
* 不同更新渠道加个描述 (#13734) @status102 @momomochi987 @Constrat @HX3N
* 自动战斗使用编队选择框 UI @status102
* 一键长草任务列表翻转按钮下拉框样式优化 @status102

### 修复 | Fix

* 萨米肉鸽刷坍缩范式闪退 @ABA2396
* 修复界园树洞无可用节点时任务失败 (#13741) @travellerse
* 修复萨卡兹肉鸽部分字段无法正确识别的问题 (#13691) @THSLP13
* 无法区分界园 3/5 层 boss @ABA2396
* 因招募动画卡预见密文板 (#13680) @ABA2396
* 修修 ocr bin threshold 被 task info 错误覆盖 @status102
* 获得排异反应的干员无法选择技能 @ABA2396
* 移除使用 empty.png 当 JustReturn 屎 (#13652) @status102
* 识别错误等待 +200ms @status102
* 肉鸽投资可能由于存款被长期遮挡导致提前退出 @status102
* 未启用备选关卡也会输出剿灭提醒 @status102
* 合成密文板未进入合成页面导致卡死 @ABA2396
* 修复 fody 一直报 warning @SherkeyXD
* 商店因为动画卡在源石锭不足 @ABA2396
* 理智药背景干扰使用数量识别 @status102
* 第一层没出商店时会在第三层才退出 @ABA2396
* 肉鸽任务高级设置界面 xaml 错误修复 @status102
* 肉鸽种子选择框 visibility 判断更新 @status102
* 每次截图测试前断开模拟器连接以获取最新的最快截图耗时 @ABA2396
* 锻冶旧迹 导航识别错误 @ABA2396
* 追加自定干员职业与最后编入干员职业相同时, 误展开子职业列表 @status102
* 肉鸽烧水选择当前难度时禁用 0 难烧水切换 @status102
* 移除未使用 @status102
* 肉鸽投资在单次投资达到 999 时可能无法退出 @status102
* 修复掉落上报消息类型没有在 Python 层定义导致外部脚本报错的问题 (#13723) @husy8
* debug version 拿不到活动关卡 @ABA2396
* 干员识别输出遗漏干员持有情况、潜能 @status102
* 剿灭失败不报错 @status102
* 傀影卡在无法进行初始招募的深入探索月度小队 @ABA2396
* schema error @ABA2396
* add slight delay before `InfrastEnteredFlag' in case of stutters in base rendering @Constrat
* EN Phantom IS font change nextlevel fix @Constrat
* start plugin task not enough delay (#13623) @Constrat

### 文档 | Docs

* 修正 `刷理智` 文档表述错误 (#13745) @vcfch843875
* MuMu 5.0 文档介绍 (#13658) @Rbqwow
* 文档站小修复 (#13659) @Rbqwow
* 文档日语化翻译 (#13713) @vcfch843875

### 其他 | Other

* 迁移 UpdateStageList @status102
* _stageManager 迁移 @status102
* binThreshold, useRaw 直接写入 task @status102
* 繁中服「出蒼白海」導航入口文字調整 (#13681) @momomochi987
* 调整 LDExtras 文件位置 @ABA2396
* 自动战斗存图阈值改为0.75 @ABA2396
* 刷理智任务指定材料不再使用双属性保存，仅保存物品 id @status102
* 密文板加一个进入后检查 @ABA2396
* 繁中服「出蒼白海」活動導航 (#13644) @momomochi987
* 添加 null 检测 @SherkeyXD
* 合并 ocrBinThreshold (#13635) @status102
* gitignore 添加 MaaBuilder 相关内容 @SherkeyXD
* TestLinkAndGetImage 不允许同时运行 @ABA2396
* 模板轮换 (#13625) @status102
* 基建设施排序初始化 @status102
* wpf 任务名字段统一 @status102
* 基建设施种类 @status102
* CustomTask 存储 @status102
* 浅调一下初始化顺序 @status102
* KR tweak copilot option translation @HX3N
* catch 一下企鹅物流的上报错误请求，避免影响重试逻辑 @ABA2396
* YostarKR CharsNameOcrReplace ocr edit @HX3N
* rename MinimumFeatureLevel to minimumFeatureLevel @SherkeyXD
* add copyright header @SherkeyXD
* add nullable flags @SherkeyXD
* add missing png suffix to templates @SherkeyXD
