## v5.15.1

### Highlight

本次更新我们并带来多项体验优化与问题修复。

#### 基建排班

更新了243/153排班表（20250412修订版）

#### 运行稳定性提升

- 修复雷电模拟器重连误判问题
- 优化网络波动时的操作容错（特别是碎石/吃药场景）
- 高延迟下进入战斗判断错误
- 线索选择逻辑更加智能精准

#### 细节体验优化

- 热更新资源改为异步加载，启动速度更快
- 日志包现在会一并打包 GUI 配置，问题排查更全面
- 官网 mirror酱 下载链接现在会根据操作系统自动生成
- 背景图默认路径改为相对路径，迁移配置更方便

----

以下是详细内容：

### 新增 | New

* 新增更新源 tooltip @ABA2396
* CustomTasks 支持逗号、分号切割任务列表 (#12342) @ABA2396 @Constrat @status102
* 配置增加全局键移除 & bool自动转换 @status102
* 更新 243/153 极限效率一天四换排班表（20250412 修订） (#12337) @bodayw
* 支持新 UI 的 OF-1 导航 (#12335) @Szzrain @pre-commit-ci[bot] @ABA2396
* Guide enable ADB (#12329) @Constrat @HX3N @ABA2396 @Rbqwow
* 自动战斗15章突袭难度适配 @status102
* mirror酱链接按照操作系统自动生成 @ChingCdesu
* 日志包一并打包gui配置文件 (#12310) @status102 @pre-commit-ci[bot]

### 改进 | Improved

* 优化自定义任务显示 @ABA2396
* 优化线索选择逻辑 @ABA2396
* wpf自动战斗增加启动失败时的log @status102
* 移除无效调用 @status102
* 使用 MirrorChyan 进行软件更新时未处理 errorCode 显示 @ABA2396
* 背景图默认值修改为相对路径 @status102
* 热更新资源改为异步加载避免启动阻塞 @ABA2396

### 修复 | Fix

* Connection lost 提前返回 false @ABA2396
* 无法选中线索 @ABA2396
* 雷电模拟器重新连接时误认为断开连接 @ABA2396
* 自动战斗网络请求返回失败时，错误提示异常 @status102
* FightSeries for global (#12332) @Manicsteiner
* 非自定义关卡名情况下，未能移除不显示的关卡 @status102
* 点击开始行动三秒未进入备战界面导致二次点击 @ABA2396
* trade post rewards post base update @Constrat
* YostarKR InfrastReward ocr fix (#12317) @HX3N
* Wpf一键长草任务栏设置按钮绑定报错 @status102
* new Support GUI for EN fix #12285#issuecomment-2788133440 @Constrat
* 修复手动关闭模拟器导致的连接状态不一致问题 @ABA2396
* 连战次数 @ABA2396
* update deployment (#11959) @Alan-Charred @ABA2396
* Wpf刷理智连战次数UI为空 @status102
* 刷理智碎石时网络卡顿造成重复点击 @status102
* task type @status102
* 修复刷理智错误识别突袭关3星结算 @status102
* 主线关卡未选择默认进度时无法导航 @ABA2396
* 吃药/碎石 时网络波动延迟导致二次确认时点到取消代理 @ABA2396
* 15 章战斗列表 @ABA2396

### 文档 | Docs

* fix punctuation for en-us @Constrat

### 其他 | Other

* 修改描述 @ABA2396
* 支持实体机检测 @ABA2396
* miss brackets @ABA2396
* FightSeries use the same ROI for all clients @ABA2396
* 给剩余理智加个兜底 @ABA2396
* manual stages resources @Constrat
* file header & warning @status102
* YostarKR InfrastReward ocr replace operator to trust @HX3N
* 顺序检查 @ABA2396
* 弹窗加个文件名 @ABA2396
* 自动战斗 战斗列表 关卡导航名允许覆盖 @status102
* 调整热更提示 @ABA2396
* Wpf刷理智任务、基建任务序列化 (#12281) @status102
