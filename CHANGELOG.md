## v5.24.0

### 功能进化 | Highlight

本次更新继续扩展自动化能力，一起来看看吧

#### 自动战斗优化

这个版本我们优化了自动战斗功能，现在你可以在更多的场景下使用自动战斗功能。目前 MAA 支持以下场景：

* `主线/故事里/SideStory`：基础模式，注意只能在当前章节/页面内导航（即 SS 里不能用于切换普通关、EX 关和 S 关）；
* `保全派驻`：用于保全派驻模式，可以点击神秘代码输入框右侧的向下小箭头来选择内置作业，并且可以设置循环次数；
* `悖论模拟`：新增的专用于干员悖论模拟的场景，分为以下两种使用方式：
  * 直接使用单次战斗：输入神秘代码后，在干员的悖论模拟入口，<u>选择好技能，出现【开始模拟】按钮后，点击 MAA 的开始按钮</u>；
  * 多个干员连续作战：勾选战斗列表，依次输入每个干员的作业的神秘代码，然后<u>在干员列表界面，点击 MAA 的开始按钮</u>。
* `其他活动`：兜底模式，不可使用战斗列表。

现在，博士们可以直接使用悖论模拟的战斗列表功能完成所有悖论模拟作业，支持按职业找人、自动跳过未拥有的干员，并在战斗列表中直接显示干员名，真正实现“点一下，全都打完”~

#### 新内容支持

- **争锋频道：蜜果城** 入口适配
- 外服也同步适配了 **新版终端界面**

#### 其他优化

- 自动战斗在关卡结束后增加了黑色返回按钮的检测，更加稳健
- 界面与交互细节继续优化，深色模式显示效果更自然

----

#### Copilot Optimization

In this version, we've optimized the Copilot, allowing you to use it in more scenarios. Currently, MAA supports the following scenarios:

* `Main Theme/Intermezzi/SideStory`: Basic mode. Note that navigation is only possible within the current chapter/page (i.e., in SideStory, you cannot switch between normal stages, EX stages, and S stages);
* `SSS`: For SSS mode. You can click the small downward arrow on the right side of the mysterious code input box to select built-in files, also you can set the number of loop times;
* `Paradox Simulation`: A newly added scenario specifically designed for operator paradox simulations, available in the following two usage methods:
  * Direct single battle usage: After entering the secret code, you need to navigate to the operator's paradox simulation entry point, <u>select the skills, wait for the [Start Simulation] button to appear, then click MAA's start button</u> to start;
  * Continuous battles for multiple operators: You need to check the battle list checkbox, sequentially enter each operator's secret code for the operation, then <u>click MAA's start button on the operator list interface</u>.
* `Other Events`: Fallback mode, battle list cannot be used.

Now, Doctors can directly use the combat list feature in Paradox Simulation to complete all Paradox Simulation assignments. It supports filtering by profession, automatically skipping unowned Operators, and displaying Operator names directly in the combat list — truly achieving "one click, all done"~

#### New Content Support

- [CN ONLY] **Arena Channel: Honey Fruit City** entry  
- **New Terminal interface** adaptation for global servers  

#### Other Improvements

- Added detection for the black return button after auto-battle ends, making runs more robust  
- UI and interaction polish — dark mode looks more natural

----

以下是详细内容：

### 新增 | New

* 悖论模拟战斗列表通过职业找人 @ABA2396
* 悖论模拟战斗列表支持跳过未拥有的干员 @ABA2396
* 战斗列表悖论模拟关卡名转为干员名 @ABA2396
* 支持一键悖论模拟 (#13893) @status102 @Constrat @Burnside999 @momomochi987 @HX3N @Manicsteiner @ABA2396
* 争锋频道：蜜果城 @ABA2396
* mac 支持蜜果城 @ABA2396
* 调整战斗列表提示描述 @ABA2396
* 远程通知添加 `[MAA]` 前缀 @ABA2396
* 加个启动提醒 @ABA2396
* 在启动时检测额外的 DLL 文件 (#13850) @Rbqwow @ABA2396
* 自动战斗关卡结束后增加识别使用黑色返回按钮的检测 @status102
* 更新源为海外源时隐藏 Mirror酱 CDK 输入框 @ABA2396

### 改进 | Improved

* 覆盖 ThirdlyTextBrush，优化深色模式下的显示效果 @ABA2396
* 任务 baseTask @status102
* 重构自动战斗战斗列表 (#13852) @status102
* 更新 CsWin32 @ABA2396
* ToastNotification 重构 @ABA2396
* 向战斗队列中添加 SSS 作业时追加错误输出 @status102
* limit codeql run only to source code (#13802) @Constrat

### 修复 | Fix

* 悖论模拟单文件无法使用 @ABA2396
* 关卡名下划线被解释成 CheckBox 访问键 @ABA2396
* 勾八导航乱写 @ABA2396
* 自动战斗界面 Idle setter 限制 @status102
* 自动战斗-战斗列表使用错误的作业战斗 @status102
* 信用购物有时候点不掉危机合约支援界面 @ABA2396
* ServerChan 未使用通用 httpService @ABA2396
* 错误清除 _tasksStatus 的时机 @ABA2396
* 肉鸽入口等待 ocr 识别错误 @ABA2396
* 界园肉鸽商店钱包余额ocr roi @status102
* 自动战斗-战斗列表`突袭`提示显示在运行期显示效果 @status102
* 不再点击停止任务时移除存储的 TaskId @status102
* 繁中服薩米肉鴿第四層名稱 (#13846) @momomochi987
* 肉鸽投资存款不变重试 20 次退出 @status102
* required version 显示错误 @ABA2396
* 会客室换班失败（如选人过程中弹出线索交流完成）时重开任务 @ABA2396
* 肉鸽开局招募在无开局干员时, 无法识别两招募的左侧位 @status102
* 千古鸭帝进战斗 @Saratoga-Official
* 日服的萨卡兹第五层怎么全 jb 改成 exit 了 @ABA2396
* 自动战斗干员编队状态; 修复编队中干员属性未达标时反复报错; 缺少干员输出时, 干员组内干员打平输出 (#13795) @status102
* 修正生息演算中 "丰饶灌木林" 的 OCR 识别 (#13825) @Alan-Charred
* 资源更新文件处理 @hguandl
* 萨卡兹肉鸽因动画延时偶发无法领取去伪存真合成物品 @ABA2396
* 萨米投资进二层会卡在预见密文板 @ABA2396
* CrownSlayer -> Crownslayer for EN @Constrat
* RoguelikeDialogSkip for EN @Constrat
* BattleQuickFormationExpandRole text ocr @Constrat
* OfficeMini template for EN @Constrat
* OfficeMini template for EN @Constrat
* ROI and text for EA YostarEN @Constrat
* YostarEN text render template changes @Constrat
* var type @status102
* YostarKR EA navigation @HX3N
* filters @ABA2396

### 其他 | Other

* 整合枚举类 OperProfession 和 Role (#13914) @soundofautumn
* 悖论模拟使用多任务流程 @status102
* 移除MaaCore不再支持的自动战斗作业代码解析 @status102
* 删掉 core 的联网功能 @ABA2396
* core 删除 cpr @ABA2396
* dll 检测排除带 maa 的 dll @ABA2396
* 自动战斗模组错误本地化 & 移除多余的行末空格 (#13800) @soundofautumn @Constrat @HX3N
* 调整小游戏内容顺序 @ABA2396
* 修改 RA 和肉鸽的进入终端等待时间 @ABA2396
* 调整萨卡兹，界园的古米精二优先度 @Saratoga-Official
* 修改备份文件写入时间 @ABA2396
* 关机/休眠/睡眠 统一调用 PowerManagement @ABA2396
* 调整 GetModuleFileName 判断 @ABA2396
* 移除未使用的 container @ABA2396
* mumu 使用 7555 端口时禁用 Index 检测，添加日志警告 @ABA2396
* file header @status102
* 自动编队助战的确实干员读取 @status102
* 加点肉鸽事件 fallback 前延迟，可能减轻点网络波动影响 @ABA2396
* 自动战斗不再使用 `need_navigate` 字段作为导航启用开关, 而是直接使用 `navigate_name` 字段 @status102
* 调整技能截图判断 @ABA2396
* YostarEN updated main stage navigation (#13906) @Constrat
* Give new line for English downloading from @Constrat
* YostarJP remove old navigation (#13905) @Manicsteiner
* YostarJP/KR new main stage navigation and ocr fix @Manicsteiner @HX3N
* A single-line comment within C# code is not preceded by a blank line. @ABA2396
* A C# partial element is missing a documentation header. @ABA2396
* An item within a C# enumeration is missing an Xml documentation header. @ABA2396
* A C# method, constructor, delegate or indexer element is missing documentation for one or more of its parameters. @ABA2396
* The XML header documentation for a C# element is missing a tag. @ABA2396
* Copyright 2025 @ABA2396
