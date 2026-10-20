## v6.20.1

### Highlights

#### 基建换班支持训练室与回收站

新增基建新设施 ｢回收站｣ 的换班支持，常规模式按技能评分选择进驻干员；自定义基建排班可配置回收站与训练室，训练室分别指定训练位与协助者的候选干员；候选干员或编组不可用时房间保持原样，任务日志附原因提示。

#### 黑流树海 DLC 初步适配

难度选择新增 N16，分队选择新增团结阵线分队与踏足遍历分队两个新分队。

#### 黑流树海完整刷等级模式

黑流树海刷等级新增目标结局选项（现开放一结局），按所选结局推进；快速飞三层为独立勾选项（需满天赋机械师），勾选后临时以特勤分队、稳扎稳打与自己的机械师开局。

<details>
<summary><b>English</b></summary>

#### Infrast Shift Support for the Training Room and the Recycling Station

Add shift support for the new facility "Recycling Station", where operators are selected by skill score in Normal Mode; the Custom Base Mode schedule can now configure the Recycling Station and the Training Room, specifying candidates for the training slot and the assistant separately; when candidates or groups are unavailable, the room is kept unchanged, with the reason shown in the task log.

#### Preliminary DLC Support for BlackFlow

N16 difficulty is added to the difficulty list, and two new squads, 团结阵线分队 and 踏足遍历分队, are added to the squad list.

#### Full Level-Farming Mode for BlackFlow

A target ending can now be selected for BlackFlow level farming (the first ending for now), and the run pushes toward it; quick flight to Floor 3 remains available as a standalone checkbox (requires the maxed Mechanic talent), temporarily overriding the squad, roles and starting operator when checked.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.20.1 (2026-10-20)</b></summary>

### 新增 | New

* 黑流树海加入 DLC1 部分新加工品 ([#18758](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18758)) @ZiyinLin

### 改进 | Improved

* 黑流树海刷开局模式在准备局第一层商店自动加购种荚等物品 ([#18755](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18755)) @bhdtrrr
* YostarJP add 龍門 to OrundumActivities, add JieGarden StageEncounter leave confirm 720p template and update the signin event popup texts @Manicsteiner

### 修复 | Fix

* 修复更新日志窗口在宽度变化触发重排时崩溃的问题 ([#18772](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18772)) @crazysmile-PhD
* 修复黑流树海编队后重新进入战斗时点击位置错误的问题 ([#18731](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18731)) @youzibigg
* 修复黑流树海 ｢险路尽头｣ 选项错配的问题 ([#18757](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18757)) @ZiyinLin
* 修复黑流树海 ｢居民据点｣ 节点识别失败的问题 ([#18754](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18754)) @ZiyinLin
* 修复黑流树海 ｢误入奇境｣ 节点缺少路由处理的问题 @ZiyinLin
* 黑流树海扣款后等待源石锭数字滚动结束再读取余额，关闭加工品面板后等待界面加载完成 @ZiyinLin
* 放宽黑流树海零件箱超载标识的模板匹配阈值 @ZiyinLin
* 放宽界园放弃探索图标的颜色饱和度匹配范围，修复识别失败的问题 ([#18766](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18766)) @youzibigg
* 修复调试环境下程序退出时因 .old 目录不存在而报错的问题 @status102
* YostarJP update the Annihilation entrance template ([#18752](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18752)) @Rememorio
* YostarKR keep line breaks in the operator OCR replacement @HX3N
* YostarKR update the Awards templates @HX3N

### 文档 | Docs

* 补齐五语言协议文档与使用手册的近期行为变更描述，覆盖回调协议、集成接口、CLI 配置、基建排班与黑流树海资源清单 @ABA2396

### 其他 | Other

* CI 升级 apple-actions/download-provisioning-profiles 从 6 到 7 ([#18712](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18712)) @dependabot[bot]
* 任务队列界面的 UserControl 与 ViewModel 文件按 ｢XxxTaskUserControl｣ 命名规范统一重命名 @status102

</details>

<details>
<summary><b>v6.20.0 (2026-10-18)</b></summary>

### 新增 | New

* 基建自定义排班新增训练室与回收站设施支持，训练室可分别配置训练位与协助者的候选干员（训练位需已精二且仍有可专精的技能，协助者不限）；候选干员或编组不可用时房间保持原样，任务日志以设施卡片附原因提示；房间数组留空时整设施保持原样，未覆盖到的房间不换班并提示；候选选人排除正在工作中的干员，宿舍在干员心情回满后才让位；一键收获识别回收站 ｢材料产出｣ ([#18632](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18632)) @ABA2396
* 基建常规换班支持回收站设施，按技能评分选择进驻干员 ([#18601](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18601)) @Lancarus @youzibigg @ABA2396
* 黑流树海初步适配肉鸽 DLC，难度选择新增 N16，分队选择新增团结阵线分队与踏足遍历分队两个新分队；险路尽头出现第四个选项时仍选择说服同伴 ([#18697](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18697)) @ZiyinLin
* 黑流树海新增刷开局模式，先打准备局取得指定襁褓动物并抵达第三层，目标局开局命中期望奖励后不做选择、退出并保留这一局；第一只与第二只襁褓动物可分别指定（羽蛇至多一只），期望奖励与羽蛇奖励任意一项命中即停，奖励识别失败时停止任务并提示；原刷襁褓动物策略并入其中 ([#18696](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18696)) @ZiyinLin
* 黑流树海刷等级新增目标结局选项（现开放一结局），不勾选快速飞三层时按所选结局推进；快速飞三层改为独立勾选项（需满天赋机械师），勾选时开局设置不能修改，任务临时以特勤分队、稳扎稳打与自己的机械师开局；狭路相逢暂时按普通事件处理 ([#18681](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18681)) @ZiyinLin @ABA2396

### 改进 | Improved

* 完善黑流树海 ｢秘境行商｣ 交易流程，第一层进入行商时只培育补货不出售，第二层起出售闲置自然物并优先回购增值物品，交易异常时停止交易并离店 ([#18667](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18667)) @ZiyinLin
* 调整黑流树海路线偏好权重，第二层偏向 ｢不期而遇｣ 节点，带线人角标的不期而遇额外加分 ([#18664](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18664)) @ZiyinLin
* 优化黑流树海阶段目标的界面上报，按访问次数计分的路线偏好不再刷屏，界面按已完成、已错过或无法完成着色显示；节点外语名称与既有肉鸽主题译名统一 ([#18663](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18663)) @ZiyinLin
* 黑流树海任务日志不再附带路线决策原因与规则说明 ([#18680](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18680)) @ZiyinLin
* 材料合成与回收站换班选人时排除已进驻其他生产设施与训练室的干员 ([#18654](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18654)) @youzibigg @ABA2396
* 会客室入驻优先级新增克莱门莎 ([#18675](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18675)) @Reverse0xCC
* 优化基建设施设置页布局，默认基建选择合并为组合控件 @ABA2396
* 干员培养在训练室被占用且计划仅剩专精目标时跳过干员查找 @status102
* 多作业模式启用 ｢使用一图流数据辅助编队｣ 时，任务日志在预匹配各作业前显示其关卡与文件名 ([#18691](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18691)) @yali-hzy
* YostarJP update OCR replacements, add Zelgrad to OrundumActivities and add Reconstruct theme navigation templates ([#18669](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18669)) @Manicsteiner
* YostarKR add EP17-related OCR entries and the OrundumActivities notice board entrance @HX3N
* YostarKR recognize the unexecuted production plan popup when switching facilities @HX3N
* YostarKR recognize the quick formation role filter All button also displayed as ALL, with OCR misread correction @HX3N
* YostarEN add Zelgrad to OrundumActivities @Constrat
* YostarEN add missing GALLUS to GALLUS² battle name OCR replacement @Constrat
* YostarKR match the SpecialAccessActivitiesConfirm button with the full text "수령하기" @HX3N

### 修复 | Fix

* 修复基建换班切换设施时被 ｢尚未执行新的制造计划，确认离开设施｣ 弹窗卡住、后续任务全部失败的问题；源石碎片补货改为等待更改确认完成，换产物失败时同样处理该弹窗 ([#18655](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18655)) @djx-trans-n @ABA2396
* 修复水月肉鸽结算后卡在数据回溯界面的问题 ([#18656](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18656)) @chanshengbinying
* 修复黑流树海选择支援页奖励识别范围未覆盖整排奖励卡片、边缘奖励漏识别的问题 ([#18682](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18682)) @ZiyinLin
* 修复萨卡兹肉鸽设置开局干员后，点刺投资不直接放弃招募、仍按指定干员选招募券的问题 ([#18684](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18684)) @ZiyinLin
* 修复快速编队职业筛选 ｢ALL｣ 按钮被 OCR 误读为 ｢ALI｣ ｢AL1｣ ｢AL[｣ 等导致筛选失效的问题 ([#18619](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18619) [#18625](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18625)) @youzibigg @Aliothmoon
* 放宽加载提示的 OCR 匹配以容错误读，修复高延迟下等待提前结束打断流程的问题（国服） ([#18650](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18650)) @djx-trans-n
* 修正肉鸽招募配置中 ｢骋风｣ ｢艾丝黛尔｣ 等干员名错别字，修复对应干员招募优先级不生效的问题 ([#18677](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18677)) @chanshengbinying
* 修复肉鸽第二、三位开局干员头像显示异常的问题 @ABA2396
* 修复可搜索下拉框在输入法组字期间误过滤并意外选中干员的问题，影响肉鸽开局干员、关卡选择、干员培养与库存保持的输入框 ([#18631](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18631)) @Aliothmoon
* 修复干员培养任务页面计划列表显示不全的问题 @status102
* 修复刷理智战斗在代理倍率与战斗次数上限组合下提前结束时，错误触发战斗次数提示日志的问题 @status102
* 修复自动提升潜能信物不足提示的识别，兼容提示换行显示 ([#18594](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18594)) @Craun718 @HX3N
* YostarJP fix in-battle name recognition for Pramanix when the name is misread by OCR ([#18683](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18683)) @Radekyspec

### 文档 | Docs

* 明确黑流树海购物配置的适用范围与校验规则，命名买卖表与价格字段仅对黑流树海生效 ([#18670](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18670)) @ZiyinLin
* 协议文档新增黑流树海刷开局回调 BlackFlowStartRun 的字段说明，补充集成接口模式 30003 及相关参数说明 ([#18696](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18696)) @ZiyinLin
* 补齐协议文档 en-us/ja-jp/ko-kr 缺失的段落与字段说明，涉及回调协议、集成文档 Fight 字段与关卡清单、任务流程协议特殊任务类型一节 @ABA2396
* 修复文档中 33 处失效的站内锚点链接（目标小节更名 10 处、Unicode 规范化差异 23 处） ([#18620](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18620) [#18629](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18629)) @chanshengbinying
* 修复集成文档中列表后提示容器未正确闭合的显示问题，五语言同步 @ABA2396

### 其他 | Other

* CI 对 PR 标题自动补齐中英文间空格 ([#18657](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18657)) @satgo1546
* 修复 clang-formatter 忽略目录时误伤同前缀目录的问题 ([#18613](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18613)) @FanWu-ai
* 统一调试截图与录像导出文件的落盘位置到用户目录 @ABA2396
* GenericCombinedData 与 LocalizedObservableList 支持条目级悬浮提示并随语言热切换刷新，黑流树海刷开局的动物与奖励选项列表迁移至该组件 @ABA2396

</details>
