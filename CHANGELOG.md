## v5.15.3

### 主要更新 | Highlight

#### 博士，新罗德岛已经准备好了！

随着第 15 章加入【终端 - 曲谱 - 乐章】，牛牛也支持对其的关卡导航了。

#### 少一次点击，多一分速度

牛牛优化了自动战斗时的自动编队功能，现在会在取消编队后立刻选择视野里已出现的需编队干员。

这样在部分场合——尤其是使用作业集对 SS 的 EX 和 S 关使用同一套作业进行普通、突袭连打时——能有效减少编队耗时。

#### 反馈问题前请更新到最新版本哦~

牛牛现在会在检测到新版本时，在反馈问题按钮处显示更新提示，希望你能先更新到最新版本，确认问题依然存在后再来反馈呀~

#### 其他方面

官网进行了一波更新，添加了亮色模式的支持，并修复了截图部分多次失焦后会变模糊的 bug。

作业站也进行了优化，现在对于绝大部分省份的你都能有良好的访问体验。

牛牛建议你选择直接连接作业站而不走其他渠道，因为近期作业站检测到部分机房 IP 存在异常高频访问或恶意自动化行为，为保障服务稳定将会进行拉黑处理~

----

以下是详细内容：

### 新增 | New

* 添加任务超时提示 (#12430) @ABA2396
* 新增 15 章导航 @ABA2396
* web cache (#12403) @MistEO
* LoadResourceAndCheckRet 额外读取对应文件名_custom.json (#12382) @ABA2396
* 官网调整 @MistEO @Rbqwow @Aliothmoon
* 官网亮色模式 (#12396) @Aliothmoon
* MaaApiService 添加 etag 支持 @ABA2396

### 改进 | Improved

* 优化运行库下载脚本 @ABA2396
* 任务列表查副本 @status102
* 修改存在更新时的问题反馈界面描述 @ABA2396
* wpf反馈压缩包分离日志和配置 @status102
* 修改版本更新提示逻辑 @ABA2396
* image size @MistEO
* enumerable @status102
* 添加资源更新提示 @ABA2396
* 添加更新提示 @ABA2396
* 自动战斗待部署识别减少一次预识别 @status102

### 修复 | Fix

* 仅公招识别配置(#12161) @hguandl
* 修正官网域名 @MistEO
* 刷理智参数更新崩溃 @status102
* 移除官网的PerformanceMonitor (#12411) @Aliothmoon @Rbqwow
* 下载状态文本适配浅色模式 (#12410) @Aliothmoon
* 从其他界面进入仓库识别任务时可能卡在主界面 @ABA2396
* SelectClue 任务 roi 与模板图片不匹配 @ABA2396
* discord logo on website @Constrat
* KR Recruit Support Unit regex @HX3N
* 在关闭系统通知后无法收到通知信息 @ABA2396
* 二次点击部署栏取消选中时，点击 y 坐标较小的 rect @ABA2396
* 主线菜单导致刷理智无法吃药 @status102
* 自动战斗 保全水泥替换 @status102

### 文档 | Docs

* 移除 MaaX 相关说明 @MistEO
* 自动战斗-自动编队 `特别关注`标记影响提示 中英文提示调整 @status102

### 其他 | Other

* YostarEN PV preload @Constrat
* 员工守则 @ABA2396
* YostarKR PV preload (#12431) @HX3N
* YostarJP PV preload (#12429) @Manicsteiner
* 拆分任务完成后通知 @ABA2396
* 添加保全派驻作业 (#12423) @Saratoga-Official
* MaaCore日志文件占用缓解, 取消大小限制 @status102
* 提取 helper 方法 @ABA2396
* 添加模拟器路径为空的提示 @ABA2396
* 调整友链弹窗最大高度 @MistEO
* 官网使用Mirror酱新链接 @MistEO
* KR remove ocrReplace in RoguelikeChooseSupportBtnOcr @HX3N
* lastUpdateTime -> stageAndTasksUpdateTime (#12393) @ABA2396
* 官网友情链接更小一点 @MistEO
* 自动战斗待部署区识别未知干员已知问题标记 @status102
* rename @status102
