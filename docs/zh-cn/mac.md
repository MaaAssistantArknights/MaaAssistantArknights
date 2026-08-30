---
title: MAA Mac GUI 公告
comments: false
editLink: false
readingTime: false
---

::: note 过渡期说明
在 Mac GUI 内置公告功能完成前，版本动态、重要提醒和临时解决方案将在本页发布。
:::

<!--
维护约定：
1. 当前公告按重要程度排列，不要把更新日志完整复制到这里。
2. 需要用户立即处理的内容使用 danger，影响部分用户的内容使用 warning，一般消息使用 tip。
3. 每条公告注明发布日期和适用版本；失效后移入“历史公告”。
4. 安装、连接等长期有效的说明应更新到用户手册，本页只保留链接。
-->

## 当前公告

::: warning 2026 年 8 月 26 日资源更新异常
2026 年 8 月 26 日凌晨发布的资源更新存在异常，可能导致 Mac GUI 报告“初始化失败”。

如遇此错误，请更新至 `v6.17.0-beta.6` 或更高版本。你可以在 Mac GUI 的更新设置中开启“**接收开发版更新**”后使用内置更新，也可以参考下方的 [更新与下载](#更新与下载) 手动下载。
:::

<!-- 公告模板
::: warning 公告标题
**发布日期：** YYYY-MM-DD
**适用版本：** vX.Y.Z（如不限制版本，可写“全部版本”）

用一两句话说明发生了什么、影响哪些用户。

**你需要做什么：**

1. 给出最短、明确的处理步骤。
2. 如有详细说明，链接到用户手册或对应 Issue。
:::
-->

## 更新与下载

- 建议优先使用 Mac GUI 和 PlayCover 各自内置的“检查更新”功能。若无法正常更新，可分别前往 [MAA Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) 和 [PlayCover Releases](https://github.com/hguandl/PlayCover/releases) 手动下载。
- Releases 中的 Mac GUI 安装包命名格式为 `MAA-<版本号>-macos-universal.dmg`。

::: tip 推荐使用 Beta 版 Mac GUI
Mac GUI 的开发进度可能滞后于 Windows GUI。建议在 Releases 页面选择标记为 **Pre-release** 的 Beta 版，或在 Mac GUI 的更新设置中开启“**接收开发版更新**”，以便及时获取 Mac 端的最新修复。
:::

## 已知问题与临时解决方案

### “连接失败”不一定是连接问题

Mac GUI 目前可能会将部分配置错误也显示为“连接失败”。遇到该提示时，除检查设备连接外，也请确认任务配置和资源是否正确。常见原因包括关卡名称填写错误，或资源未及时更新、缺少对应关卡的数据文件。

### 其他连接与设备兼容问题

遇到其他常规连接或设备兼容问题时，请先查阅 [Mac 模拟器支持](./manual/device/macos.md)。

<!-- 已知问题模板
### 问题简述

- **影响范围：** 版本、系统或设备范围
- **现象：** 用户能够观察到的表现
- **临时方案：** 可立即执行的规避步骤
- **跟踪进度：** 对应 Issue 或 PR 链接
-->

## 问题反馈

如果公告和文档均未覆盖你遇到的问题，请前往 [GitHub Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) 反馈。提交前请准备：

- Mac GUI 版本与 macOS 版本
- 使用的模拟器或 PlayCover 版本
- 问题发生前后的操作步骤
- 日志、报错信息和必要的截图

## 历史公告

<!--
旧公告按时间倒序放入 details 容器，避免拉长页面。例如：

::: details YYYY-MM-DD · 公告标题
公告正文或处理结果。
:::
-->

暂无历史公告。
