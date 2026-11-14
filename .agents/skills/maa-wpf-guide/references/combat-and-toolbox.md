# 自动战斗、肉鸽与小工具

## 自动战斗（Copilot／抄作业）

从 ｢自动战斗｣ 页面导入本地 JSON，或填 [作业站](https://prts.plus/) 的代码。先确认识别出的关卡、难度和作业要求，核对干员、技能和模组，再点开始。普通单作业从有 ｢开始行动｣ 的编队界面运行。游戏保持稳定 60 FPS，按文档选择支持的触控模式。

- 自动编队会清空当前编队再选人。识别异常时取消相关干员的特别关注。要手动借助战时，可关掉自动编队，按作业先编好队。
- 多作业模式在关卡所在地图开始，作业应在同一区域。可拖拽顺序和勾选；理智不足、失败或非三星结算可能停止队列。
- 悖论模拟单作业先手动选技能，从显示 ｢开始模拟｣ 的画面启动；多作业模式按页内提示从干员列表启动。
- 保全派驻需要先手动做好初始编队等准备，从显示 ｢开始部署｣ 的关卡详情启动。页面还提供单步任务。写作业应以作业编辑器和对应协议为准。
- 自动战斗按作业执行部署、技能和撤退。日常反复刷已解锁代理的关卡请用 ｢理智作战｣。

依据：[自动战斗](https://docs.maa.plus/zh-cn/manual/introduction/copilot.html) · [普通作业格式](https://docs.maa.plus/zh-cn/protocol/copilot-schema.html) · [保全作业格式](https://docs.maa.plus/zh-cn/protocol/sss-schema.html) · [作业解析与开始校验](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UI/CopilotViewModel.cs)。

## 自动肉鸽（集成战略）

从 ｢一键长草 → 自动肉鸽｣ 的齿轮设置游戏主题、策略和相应的开局选项。优先使用手册为该主题列出的开局和干员组合；难度、开局养成、模式以及是否存在未结束探索均可影响流程。查当期主题与策略时以对应版本文档和资源为准，普通刷分、刷开局、刷指定内容各有不同的结束条件。遇到频繁误操作，先检查稳定帧率、截图耗时、触控模式和游戏画面。

依据：[自动肉鸽](https://docs.maa.plus/zh-cn/manual/introduction/integrated-strategy.html) · [WPF 肉鸽配置与下发](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UserControl/TaskQueue/RoguelikeTaskUserControlModel.cs)。

## 小工具

| 页面 | 用法及限制 |
| --- | --- |
| 公招识别 | 手动打开公招标签界面后识别。可以显示已识别干员的潜能。是否自动选词条或设置时间取决于该页开关；要批量自动招募则用队列里的 ｢自动公招｣。 |
| 干员识别 | 识别持有、未持有和潜能，可导出剪贴板、JSON、Markdown、CSV。设置三方服务后可用一图流只读 Token 获取该账号的一图流练度快照，技能、专精和模组数据取决于快照。 |
| 仓库识别 | 可尝试自动进入仓库；若失败，手动打开 ｢养成材料｣ 并滚动到最左侧。结果与更新数据和库存任务共享。支持 ArkPlanner、明日方舟工具箱、Markdown、CSV 格式。 |
| 牛牛抽卡 | 在游戏内真实抽卡，会消耗抽卡资源。操作前确认账号、卡池与单抽/十连按钮。 |
| 牛牛监控 | 以目标帧率捕获并显示设备画面，截图较慢时会主动降低目标帧率。可用于看 MAA 实际收到的画面。 |
| 牛杂 | 选择常驻或当期限时条目。可见条目随区服、资源与活动变化。常驻示例包括隐秘战线、生息演算、商店兑换、提升潜能、缺口材料合成。兑换、潜能与合成会消耗游戏资源。 |

生息演算按 ｢小工具 → 牛杂 → 生息演算｣ 查对应主题。不同主题有不同的干员、前置关卡和起跑画面。沙洲遗闻有通过组装道具刷点与通过进出关卡刷点的选项，后者可能删除已有存档；有存档时先确认所选模式。旧版本入口可不同。

依据：[小工具](https://docs.maa.plus/zh-cn/manual/introduction/tools.html) · [小工具的 WPF 标签与开关](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Views/UI/ToolboxView.xaml) · [生息演算界面](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Views/UserControl/Toolbox/ReclamationUserControl.xaml)。
