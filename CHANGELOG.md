## v6.12.1

### Highlights

泡影苍霆活动关卡暂不支持多作业模式
由于本次活动关卡的字体与界面结构较为特殊，MAA 无法在活动关卡中正常使用多作业模式。
使用自动战斗功能时，请关闭 MAA 的「多作业模式」，并在干员编队界面启动任务。
作业站中的「作业集」仅用于归纳作业，仍可正常导入。导入后可点击关卡名称右侧的文件图标，快速切换至对应关卡的单作业模式使用。

#### 阵地足球锦标赛

新增支持阵地足球锦标赛小游戏，可前往 ｢小工具-牛杂-当期活动｣ 进行选择。

#### 悖论模拟支持跳过战斗失败的关卡

悖论模拟新增跳过战斗失败的作业选项，仅自动取消已完成的作业，未完成的作业将继续保持勾选状态。

#### 支持 MuMu 6.0 版本截图增强

支持 MuMu 6.0 版本安卓 15 的截图增强路径，提升截图稳定性和兼容性。

<details>
<summary><b>English</b></summary>

#### Paradox Simulation Support for Skipping Failed Battle Stages

The Paradox Simulation now includes an option to skip failed battle stages, automatically unchecking completed stages while keeping incomplete stages checked.

#### Support for MuMu 6.0 Screenshot Enhancement

Supports the enhanced screenshot path for MuMu 6.0 Android 15, improving screenshot stability and compatibility.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.12.1 (2026-06-11)</b></summary>

### 修复 | Fix

* 修复多作业模式关卡导航选错关卡无法重选、关卡名复核失效等多项问题 @status102 @ABA2396
* 修复界园事件内通宝交换后无法正确处理事件结束页 (#16936) @ZiyinLin

</details>

<details>
<summary><b>v6.12.0 (2026-06-11)</b></summary>

### 新增 | New

* 繁中服支持「雪山降臨1101」活动导航与「喀蘭貿易技術研發部」小游戏 (#17073) @momomochi987
* 添加阵地足球锦标赛小游戏 @ABA2396
* 理智药使用增加使用中的药品信息 @status102
* 支持 MuMu 6.0 截图增强路径 (#16994) @ABA2396
* 切换主题时保存当前画面截图 (#16993) @ABA2396
* 悖论模拟支持跳过战斗失败的作业，自动战斗作业增加对应结构 (#16985) @status102
* 启动设置添加模拟器启动测试按钮，便于测试是否配置成功 @ABA2396
* 统一 SearchBar 样式 @ABA2396

### 改进 | Improved

* 更新 MuMu 12 关闭命令至 MuMuManager 新版 API (#17067) @Zmjjeff7
* 贸易站切换产物前先校验当前状态，避免已达成目标时的冗余操作 (#16954) @ZiyinLin
* 修正小游戏显示名称与提示文案的本地化优先级 @ABA2396
* 优化阵地足球锦标赛部署坐标与动画时序 @ABA2396
* 更新 OF-1 信用作战作业干员配置 @ABA2396
* 优化部分情况下自动战斗导航 OCR 结果中会出现误识别的前缀 @status102
* 基于灰度阈值预处理的自动战斗导航，适配 H 关及怪猎二期 TD-2 本 (#16990) @status102
* 肉鸽弹窗类事件处理重构 CloseCollectionClose (#17005) @status102
* InvokeProcSubTaskMsg 重构 (#16979) @status102

### 修复 | Fix

* 修复启动模拟器与 ADB 操作时 Process 对象未释放导致句柄泄漏 (#17060) @Zmjjeff7
* 修复配置创建失败时回滚不一致的问题 @ABA2396
* 修复 RainbowAnimation 画刷选择潜在的空引用异常 @ABA2396
* ConfigFactory Save 锁统一 (#17052) @status102
* 错误隐藏开局分队与开局干员选项 @ABA2396
* 修复特克诺干员名 OCR 误识别 (#17030) @ZiyinLin
* 修复绿票商店状态回退错误及二阶段校验问题 @status102 @ZiyinLin
* MaskedCcoeffMatcher 稀疏路径累加器改用 CV_64F 防止大数目相减精度损失 (#16983) @Aliothmoon
* 降低 PlayCover 下肉鸽部分任务的模版匹配分数阈值 (#16968) @Alan-Charred
* 更新 EN 服 IS6 bosky 模板与文字尺寸 @Constrat
* 修复 MaaMacGui changelog 贡献者 mention (#16978) @ColdSpellhere

### 文档 | Docs

* README「自动抄作业」更新为「自动战斗」 @Rbqwow

### 其他 | Other

* 自动战斗视频链接始终显示 @ABA2396
* 补充可露希尔基建数值 @Saratoga-Official
* YostarKR winden colorScale for compatibility @HX3N

### MaaMacGui

#### 新增 | New

* 添加阵地足球锦标赛小游戏入口 @ABA2396

#### 修复 | Fix

* 统一 gui.log 文件日志中的日期与时间格式 ([#93](https://github.com/MaaAssistantArknights/MaaMacGui/pull/93)) @Alan-Charred

</details>
