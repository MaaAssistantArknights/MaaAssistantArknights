## v6.12.0-beta.1

### Highlights

#### 自动战斗与作业能力增强

悖论模拟新增「跳过战斗失败的作业」选项；自动战斗作业现可携带对应结构信息，并完成对怪猎二期自动战斗多作业模式的适配。

#### 支持 MuMu 6.0 版本截图增强

支持 MuMu 6.0 版本安卓 15 的截图增强路径，提升截图稳定性和兼容性。

<details>
<summary><b>English</b></summary>

#### Copilot Enhancements
    
Paradox Simulation adds a "skip copilots that failed in battle" option; Copilot now carry the corresponding structure info, with adaptations for Monster Hunter Phase 2 Copilot Multi-Job mode.

#### Support for MuMu 6.0 Screenshot Enhancement

Supports the enhanced screenshot path for MuMu 6.0 Android 15, improving screenshot stability and compatibility.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.12.0-beta.1 (2026-06-09)</b></summary>

### 新增 | New

* 支持 mumu 6.0 截图增强路径 (#16994) @ABA2396
* 切换主题时保存当前画面截图 (#16993) @ABA2396
* 悖论模拟支持跳过战斗失败的作业，自动战斗作业增加对应结构 (#16985) @status102
* 启动设置添加模拟器启动测试按钮，便于测试是否配置成功 @ABA2396
* 统一 SearchBar 样式 @ABA2396

### 改进 | Improved

* 优化部分情况下自动战斗导航OCR结果中会出现误识别的前缀 @status102
* 基于灰度阈值预处理的自动战斗导航，适配 H 关及怪猎二期 TD-2 本 (#16990) @status102
* 肉鸽弹窗类事件处理重构 CloseCollectionClose (#17005) @status102
* InvokeProcSubTaskMsg 重构 (#16979) @status102

### 修复 | Fix

* 修复特克诺干员名 OCR 误识别 (#17030) @ZiyinLin
* 修复绿票商店状态回退错误及二阶段校验问题 @status102 @ZiyinLin
* MaskedCcoeffMatcher 稀疏路径累加器改用 CV_64F 防止大数目相减精度损失 (#16983) @Aliothmoon
* 降低 PlayCover 下肉鸽部分任务的模版匹配分数阈值 (#16968) @Alan-Charred
* 更新 EN 服 IS6 bosky 模板与文字尺寸 @Constrat
* 修复 MaaMacGui changelog 贡献者 mention (#16978) @ColdSpellhere

### 文档 | Docs

* README「自动抄作业」更新为「自动战斗」 @Rbqwow

### 其他 | Other

* rename Wpf ProcSubTaskMsg param name @status102
* DEBUG 环境下 Init 时 TaskQueue 状态限制缓解 @status102
* 自动战斗视频链接始终显示 @ABA2396
* 补充可露希尔基建数值 @Saratoga-Official
* YostarKR winden colorScale for compatibility @HX3N

### MaaMacGui

#### 修复 | Fix

* 统一 gui.log 文件日志中的日期与时间格式 ([#93](https://github.com/MaaAssistantArknights/MaaMacGui/pull/93)) @Alan-Charred

</details>
