## v6.8.0

### 牛牛初体验 DeepSleep，被吓到眩晕瘫坐，那一刻就像看到原子弹爆炸 | Highlights

本次版本更新我们依旧专注于提升用户体验。我们将始终秉持长期主义的原则理念，在尝试与思考中踏实前行。

#### 独立更新器与本地拖入更新

我们优化了 MAA 的更新流程，将更新部分拆分成独立的 exe 文件，以增强更新过程的可靠性。

***\[仅 Windows\]*** 另外，我们为无法直接下载 OTA 更新包，但可以从群文件等其他渠道获取完整包或增量包的用户，提供了直接拖入更新的功能。你只需要将对应的压缩包直接拖入 MAA 主窗口，牛牛就会自动识别并执行更新。

请确保压缩包名称符合完整包命名规则（`MAA-v{新版本}-win-{架构}.zip`）或 OTA 包命名规则（`MAAComponent-OTA-v{旧版本}_v{新版本}-win-{架构}.zip`），且架构（`x64` 或 `arm64`）与版本链（`{旧版本}`）和当前安装的 MAA 完全匹配，否则更新将被拒绝。

##### 建议内测版用户执行一次完整包清理替换

在独立更新器的编写过程中，我们修复了一个自内测版发布之初就存在的问题：

内测版在与正式版或公测版切换更新时，旧版本残留文件可能无法被正确清理，从而影响更新结果。

建议所有内测版用户下载一次完整包，并拖入 MAA 主窗口执行清理替换。

通过完整包更新时，MAA 会保留 debug、cache、data、config 文件夹，其余文件会移入回收站；若你有自行添加或修改过文件，建议提前备份。

#### 一键长草优化

我们优化了一键长草功能的理智作战部分，现在可以指定在活动结束前 48 小时吃当期过期的理智药，确保能将更多的理智药投入到当期活动中。

我们为自动肉鸽部分新增了界园肉鸽月度小队与深入调查的支持，你现在可以在自动肉鸽的策略选项选择对应的策略了。

#### 海外服专属优化

我们在这个版本新增了海外服的基建线索快速放置功能，以及 SideStory「雪山降临1101」小游戏“喀兰贸易技术研发部”资源、OCR 与本地化内容。

----

### Highlights

This update continues our focus on improving user experience. We will always adhere to the principle of long-termism, steadily moving forward through experimentation and reflection.

#### Standalone Updater and Local Drag-and-Drop Update

We have optimized the MAA update process, splitting the update portion into independent exe files to enhance the reliability of the update process.

***[Windows Only]*** Additionally, for users who cannot directly download OTA packages but can obtain full or incremental packages from other sources, we have provided a drag-and-drop update function. You only need to drag the corresponding archive directly into the MAA main window, and MAA will automatically recognize and execute the update.

Please ensure that the archive name matches either the full-package convention (`MAA-v{new version}-win-{architecture}.zip`) or the OTA-package convention (`MAAComponent-OTA-v{old version}_v{new version}-win-{architecture}.zip`), and that the architecture (`x64` or `arm64`) and version chain (`{old version}`) fully match the currently installed MAA; otherwise, the update will be rejected.

##### We recommend that beta users perform a full package cleanup and replacement.

During the development of the standalone updater, we fixed an issue that existed since the initial beta release:

When switching between beta builds and stable or public-preview builds, leftover files from the previous version may not be cleaned up correctly, which can affect the update result.

We recommend that all beta users download the full package once and drag it into the MAA main window to perform a cleanup and replacement.

When updating with the full package, MAA will retain the debug, cache, data, and config folders; other files will be moved to the recycle bin. If you have added or modified any files, we recommend backing them up beforehand.

#### *Farming* Optimization

We've optimized the *Combat* of the *Farming*. You can now specify that using sanity potions that expire this week within 48 hours before the activity ends, ensuring more sanity potions are used in the current event.

We've added support for the *Monthly Squad* and *Deep Investigation* for *Sui's Garden of Grotesqueries* in the *Auto I.S.*. You can now select the corresponding strategy in the *Auto I.S.* options.

#### Overseas Server Exclusive Optimizations

In this version, we've added a quick Clue placement function for *Base* for overseas servers, as well as resources, OCR, and localized content for the minigame *Karlan Trade R&D* of the SideStory *Retracing Our Steps*.

----

以下是详细内容：

## v6.8.0

### 新增 | New

* 新增跨平台前端界面 MAAUnified V0.2，为后续多平台统一体验提供基础 (#16048) @Halo5082 @MistEO
* 新增可按指定过期天数使用理智药 (#13849) @soundofautumn @status102
* 新增界园肉鸽月度小队与深入调查支持 (#16271) @SherkeyXD
* 新增海外服线索快速放置，并在线索交流前自动清空已放置线索 (#14966) (#16054) @Constrat @travellerse

### 改进 | Improved

* 重构更新流程：支持拖入指定名称的压缩包执行更新，涉及 DLL 的更新改由独立更新器处理，并在启动时校验版本一致性 (#16308) (#16326) @ABA2396
* 更新器日志新增大小上限及文件移除、移动明细，提升更新问题排查体验 @ABA2396
* 干员数据重构，支持跨职业重名干员 (#16084) @status102
* 配置存储支持按条件写入，减少无效配置项并提升配置兼容性 (#15850) @status102
* 自动编队在预编队后会额外校验选中状态，降低误编队概率 @status102
* 作业版本号要求允许省略 patch 版本号 @status102
* 优化提示元素展示效果，并减少通知实现中的冗余 UI 线程切换 (#16196) @ABA2396 @EzraRT
* 优化算法性能与鲁棒性 (#16235) @lhhxxxxx
* 分辨率不支持时会输出当前分辨率，便于排查兼容问题 @ABA2396
* PC 端窗口绑定模式下禁用“完成后退出模拟器”，避免无效设置 @ABA2396

### 修复 | Fix

* 修复更新器的 UTF-8 解析与 removelist 误判问题，并在更新时额外保留 cache 文件夹，减少误删风险 @ABA2396
* 修复截图延迟极低时，更换产物或订单可能随机失败的问题 (#16330) @Roland125
* 修复定时任务触发时 UpdateStageList 的等待逻辑，以及 LinkStart 期间进入 SetFightParams 的死锁问题 @status102
* 非法 Enum 值现在会回退到属性默认值，避免异常配置继续传播 (#16138) @status102
* 修复旧主题变更导致的招募识别问题 @ABA2396
* 修复自动战斗鼠标长按分页时可能反复触发切换的问题 @status102
* 修复部分越界场景下的返回值与索引异常 @status102
* 修复基建开启设施无法保存的问题 @ABA2396
* 修复干员库存识别返回错误 ID 的问题 @status102
* 修复 macOS PlayTools/SCK 的若干兼容性问题 (#16276) @FireflySentinel
* 修复韩服 OSChapterToOS OCR 与 EN 服 IS 6 DLC 1 文本匹配问题 @Daydreamer114 @Constrat

### 文档 | Docs

* 完善安装文档的多语言翻译 (#16214) @JasonHuang79 @HX3N @Manicsteiner @momomochi987 @Constrat
* 补充 Windows Defender 误报 FAQ 指引 (#16145) @Leo91314
* 补充 AVD 截图增强与手动更新相关文档说明 (#16031) @satgo1546 @Rbqwow @MistEO @ABA2396

### 其他 | Other

* 添加单元测试框架，并为角色分配算法补充测试用例 (#16245) @lhhxxxxx
* 更新保全派驻作业 @Saratoga-Official
* 补充怒潮凛冬相关肉鸽招募逻辑与基建技能数值 (#16217) (#16260) @Reverse0xCC @drway
* 补充 EN/JP/KR 服 OS 关卡、小游戏与 OCR 资源，并更新韩服过期理智药相关本地化 (#16267) (#16268) (#16283) @Manicsteiner @HX3N @Constrat
* 更新繁中服「次生方案」活动、宿舍截图及部分 OCR 资源 (#16216) (#16298) @momomochi987
