## v6.20.0-beta.1

### Highlights

#### 基建换班支持训练室与回收站

新增基建新设施 ｢回收站｣ 的换班支持，常规模式按技能评分选择进驻干员；自定义基建排班可配置回收站与训练室，训练室分别指定训练位与协助者的候选干员；候选干员或编组不可用时房间保持原样，任务日志附原因提示。

<details>
<summary><b>English</b></summary>

#### Infrast Shift Support for the Training Room and the Recycling Station

Adds shift support for the new facility "Recycling Station", where operators are selected by skill score in Normal Mode; the Custom Base Mode schedule can now configure the Recycling Station and the Training Room, specifying candidates for the training slot and the assistant separately; when candidates or groups are unavailable, the room is kept unchanged, with the reason shown in the task log.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.20.0-beta.1 (2026-10-16)</b></summary>

### 新增 | New

* 基建自定义排班新增训练室与回收站设施支持，训练室可分别配置训练位与协助者的候选干员（训练位需已精二且仍有可专精的技能，协助者不限）；候选干员或编组不可用时房间保持原样，任务日志以设施卡片附原因提示；房间数组留空时整设施保持原样，未覆盖到的房间不换班并提示；候选选人排除正在工作中的干员，宿舍在干员心情回满后才让位；一键收获识别回收站 ｢材料产出｣ ([#18632](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18632)) @ABA2396
* 基建常规换班支持回收站设施，按技能评分选择进驻干员 ([#18601](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18601)) @Lancarus @youzibigg @ABA2396

### 改进 | Improved

* 完善黑流树海 ｢秘境行商｣ 交易流程，第一层进入行商时只培育补货不出售，第二层起出售闲置自然物并优先回购增值物品，交易异常时停止交易并离店 ([#18667](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18667)) @ZiyinLin
* 调整黑流树海路线偏好权重，第二层偏向 ｢不期而遇｣ 节点，带线人角标的不期而遇额外加分 ([#18664](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18664)) @ZiyinLin
* 优化黑流树海阶段目标的界面上报，按访问次数计分的路线偏好不再刷屏，界面按已完成、已错过或无法完成着色显示；节点外语名称与既有肉鸽主题译名统一 ([#18663](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18663)) @ZiyinLin
* 材料合成与回收站换班选人时排除已进驻其他生产设施与训练室的干员 ([#18654](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18654)) @youzibigg @ABA2396
* 会客室入驻优先级新增克莱门莎 ([#18675](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18675)) @Reverse0xCC
* 优化基建设施设置页布局，默认基建选择合并为组合控件 @ABA2396
* YostarJP update OCR replacements, add Zelgrad to OrundumActivities and add Reconstruct theme navigation templates ([#18669](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18669)) @Manicsteiner
* YostarKR add EP17-related OCR entries and the OrundumActivities notice board entrance @HX3N
* YostarKR recognize the unexecuted production plan popup when switching facilities @HX3N
* YostarEN add Zelgrad to OrundumActivities @Constrat

### 修复 | Fix

* 修复基建换班切换设施时被 ｢尚未执行新的制造计划，确认离开设施｣ 弹窗卡住、后续任务全部失败的问题；源石碎片补货改为等待更改确认完成，换产物失败时同样处理该弹窗 ([#18655](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18655)) @djx-trans-n @ABA2396
* 修复水月肉鸽结算后卡在数据回溯界面的问题 ([#18656](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18656)) @chanshengbinying
* 修复快速编队职业筛选 ｢ALL｣ 按钮被 OCR 误读为 ｢ALI｣ ｢AL1｣ ｢AL[｣ 等导致筛选失效的问题 ([#18619](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18619) [#18625](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18625)) @youzibigg @Aliothmoon
* 放宽加载提示的 OCR 匹配以容错误读，修复高延迟下等待提前结束打断流程的问题（国服） ([#18650](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18650)) @djx-trans-n
* 修正肉鸽招募配置中 ｢骋风｣ ｢艾丝黛尔｣ 等干员名错别字，修复对应干员招募优先级不生效的问题 ([#18677](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18677)) @chanshengbinying
* 修复肉鸽第二、三位开局干员头像显示异常的问题 @ABA2396
* 修复可搜索下拉框在输入法组字期间误过滤并意外选中干员的问题，影响肉鸽开局干员、关卡选择、干员培养与库存保持的输入框 ([#18631](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18631)) @Aliothmoon
* 修复干员培养任务页面计划列表显示不全的问题 @status102

### 文档 | Docs

* 明确黑流树海购物配置的适用范围与校验规则，命名买卖表与价格字段仅对黑流树海生效 ([#18670](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18670)) @ZiyinLin
* 修复文档中 33 处失效的站内锚点链接（目标小节更名 10 处、Unicode 规范化差异 23 处） ([#18620](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18620) [#18629](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18629)) @chanshengbinying

### 其他 | Other

* CI 对 PR 标题自动补齐中英文间空格 ([#18657](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18657)) @satgo1546
* 修复 clang-formatter 忽略目录时误伤同前缀目录的问题 ([#18613](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18613)) @FanWu-ai
* 统一调试截图与录像导出文件的落盘位置到用户目录 @ABA2396

</details>
