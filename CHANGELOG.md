## v6.2.0-beta.3

### 新增 | New

* 自动战斗日志栏添加日志悬浮窗按钮 @ABA2396
* 低于 2 核心电脑使用单线程 OCR @ABA2396
* 更新库存提示移到 ToolTip @ABA2396
* 关卡提示支持显示库存 @ABA2396
* 新增肉鸽主题推荐配置tip并适配多语言 (#15324) @hcl55 @ABA2396
* Disable CoreML for detection and recognition on macOS @MistEO
* SideStory「雅赛努斯复仇记」导航 @SherkeyXD
* 限制使用 CPU 推理时的线程占用数，优先保证模拟器运行 @ABA2396
* 基建进入设施失败时保留测试截图 @ABA2396
* 将自动重载资源独立出来，在 debug 模式下显示勾选框 @ABA2396
* 尝试增加 b服 开屏活动跳过 @ABA2396
* 检测到自身处于临时路径中时阻止启动 (#14961) @Rbqwow @ABA2396
* 企鹅物流上报 ID 不为空时禁止被上报结果赋值 @ABA2396

### 改进 | Improved

* 重构自动战斗标签页逻辑, 拆分悖论模拟任务 (#15327) @ABA2396 @status102
* rename Dps to Ops (#15325) @ABA2396
* 优化线索交流、获取好友线索逻辑 @ABA2396

### 修复 | Fix

* 撷英调香师 @ABA2396
* 20260109 游戏更新导致自动战斗失效 @status102
* 线索数量识别 @ABA2396
* 线索板上有线索时无法一键放置线索 @ABA2396
* 同时开启 `剿灭模式` 和 `备选关卡` 会导致 `企鹅物流汇报 ID` 被修改 @ABA2396
* Filename too long @Daydreamer114

### 其他 | Other

* H16-4, 引航者#6 TN-1~TN-4 剩余地图 view[1] @status102
* 移除多余关卡 @SherkeyXD
* port changes from api @SherkeyXD
* 添加挂调试器下使用 GPU 的注释 @ABA2396
