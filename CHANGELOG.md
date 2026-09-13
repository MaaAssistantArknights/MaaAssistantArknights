## v6.18.0-beta.1

### Highlights

#### 一图流练度数据读取

设置新增 ｢三方服务｣ 栏，干员识别支持填写一图流 OpenAPI Token 后直接读取一图流保存的练度快照，小工具的干员识别不再需要连接模拟器。

#### 黑流树海肉鸽增强

零件箱超载时自动丢弃零件，刷襁褓动物策略统一为行进至第三层，未知终止原因时引导前往问题反馈页。

#### 更新失败自动修复

更新失败后会拦截任务启动，并在弹窗中提供自动修复，自动下载完整包后在本地安装；暂不修复也可正常进入主界面，通过设置检查更新或拖入本地完整包完成更新。

<details>
<summary><b>English</b></summary>

#### Operator Recognition via Yituliu OpenAPI

A new "Third-Party Services" settings section lets operator recognition read the proficiency snapshot stored on Yituliu via an OpenAPI token, and the toolbox's operator recognition no longer requires a connected emulator.

#### BlackFlow Roguelike Enhancements

Parts are discarded automatically when the parts box is overloaded, the cultivation strategy now consistently advances to the third floor, and unknown terminations guide users to the issue report page.

#### Auto Repair for Failed Updates

After a failed update, task startup is blocked and the dialog offers automatic repair, which downloads the full package and installs it locally; postponing the repair still opens the main window normally, where you can check for updates in Settings or drop in a local full package to complete the update.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.18.0-beta.1 (2026-09-14)</b></summary>

### 新增 | New

* 基建换班新增副手换人流程，基建设施列表新增 ｢基建副手｣，可自动更换进驻的副手干员 ([#17943](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17943)) @Lancarus
* 任务设置新增运行时长上限，到达后自动停止任务 ([#18167](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18167)) @Aliothmoon @ABA2396
* 设置新增 ｢三方服务｣ 栏（企鹅物流上报设置迁入其中），干员识别支持填写一图流 OpenAPI Token 后改为读取一图流保存的练度快照，小工具的干员识别不再需要连接模拟器，数据更新任务的干员识别在连接模拟器后立即并行拉取，不等待其他任务完成 ([#18187](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18187)) @ABA2396
* 黑流树海肉鸽零件箱超载时自动丢弃零件 ([#18065](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18065)) @ZiyinLin
* 界面设置新增 ｢悬停主任务时隐藏操作按钮｣ 选项，隐藏后运行一次/复制/重命名/删除等功能可通过任务右键菜单使用 @ABA2396
* ProcessTask 新增 override_next 接口，支持运行时覆盖指定任务的 next 列表 ([#18152](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18152)) @status102
* YostarEN/JP/KR add AssistantConfirm template for the infrast assistant swap flow @Constrat @Manicsteiner @HX3N

### 改进 | Improved

* SR 关卡导航支持 ｢殡仪堂｣ 二期入口识别 @ABA2396
* 补充部分干员与技能的基建效率数据 @Lancarus @ABA2396
* 点击任务列表项的空白区域即可切换到对应任务设置 @ABA2396
* 周计划跳过任务时提示跳过原因，精简库存保持的跳过提示 @ABA2396
* 黑流树海肉鸽刷襁褓动物策略统一为行进至第三层 ([#18198](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18198)) @ZiyinLin
* 优化黑流树海肉鸽的终止原因提示，未知终止时引导前往 ｢设置 - 问题反馈｣ @ABA2396
* 推理加速启用时若 GPU 驱动信息不可读则输出警告，便于排查加速不生效的问题 @ABA2396
* 优化任务加载性能，复用 Regex 验证实例避免重复编译 @status102
* YostarEN/KR preload PA navigation @Constrat @HX3N

### 修复 | Fix

* 修复更新失败提示仅首次启动生效、被忽略后不再拦截的问题，现在更新失败后会拦截任务启动，弹窗中提供自动修复，自动下载完整包后在本地安装；暂不修复时可正常进入主界面，通过设置检查更新或拖入本地完整包完成更新 ([#18170](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18170) [#18204](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18204)) @ABA2396
* 资源损坏或上次更新失败期间，检查更新强制改走完整包，已是最新版本时也会重装同版本完整包；拖入 OTA 增量包会被拒绝并提示改用完整安装包，同版本完整包可直接拖入修复 ([#18215](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18215)) @ABA2396
* 修复 RunningState 计时器并发操作可能引发空引用异常的问题 ([#18202](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18202)) @ABA2396
* 修复 Windows 下局域网连接设备时 RawByNc 截图方式不可用的问题 ([#18179](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18179)) @Aliothmoon
* 修复公招未勾选自动确认 3/4 星招募结果时仍会自动确认的问题 ([#18176](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18176)) @Aliothmoon
* 修复切换界面语言后设置页过渡动画下拉选项停留旧语言的问题 @ABA2396
* 外部通知请求日志中的 URL 截断到域名，避免 token 等凭据随日志泄漏 @ABA2396

### 文档 | Docs

* 更新 Linux 编译教程与设备文档 ([#18184](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18184)) @satgo1546
* 添加基建排班工具 RIIC.Autos 的相关链接 ([#18033](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18033)) @BrKDDD @Rbqwow
* 手动更新数据的流程中，将更换主题步骤移至更新数据之后 ([#18173](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18173)) @youzibigg

### 其他 | Other

* 新增本地图片离线识别评估工具（DebugTask 评估原语与 maa_core_eval.py）([#18181](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18181)) @ABA2396 @status102

### MaaMacGui

#### 新增 | New

* 刷理智设置页新增今日关卡小提示 ([#122](https://github.com/MaaAssistantArknights/MaaMacGui/pull/122)) @zhangweijian97

</details>
