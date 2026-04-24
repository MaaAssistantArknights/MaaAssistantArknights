## v6.8.0

### Highlights

#### 独立更新器与本地拖入更新

更新流程已重构为独立更新器方案，并支持直接拖入符合命名规则的压缩包更新。可拖入完整包，例如 MAA-v6.8.0-win-x64.zip，也可拖入 OTA 增量包，例如 MAAComponent-OTA-v旧版本_v新版本-win-x64.zip。压缩包名称、架构与版本链必须严格匹配；若文件名被系统追加 (1)、-副本，或版本号、架构与当前安装不一致，都会被直接拦截。该功能适用于无法直接连接 GitHub、但可通过群文件等渠道获取完整包或增量包的用户，且需在 v6.8.0-beta.2 及以上版本中使用。

#### 建议内测版用户执行一次完整包清理替换

在独立更新器的编写过程中，我们修复了一个自内测版发布之初就存在的问题：内测版在与正式版或公测版对比时，removelist 可能会错误地将所有目录加入删除列表。建议所有内测版用户下载一次完整包，并拖入软件内执行清理替换。通过完整包更新时，MAA 会保留 debug、cache、data、config 文件夹，其余文件会移入回收站；若目录内有自行添加或修改过的文件，建议提前备份。

#### 玩法与海外服支持扩展

本版本新增按指定过期天数使用理智药、界园肉鸽月度小队与深入调查等功能，并补充海外服线索快速放置、OS 关卡与小游戏资源、OCR 与本地化内容，进一步提升多地区服支持的完整度。

----

#### Standalone Updater and Update Flow Overhaul

The update flow has been rebuilt around a standalone updater, and MAA now supports updating by directly dragging in a correctly named archive. You can import either a full package, such as MAA-v6.8.0-win-x64.zip, or an OTA package, such as MAAComponent-OTA-vOLD_VERSION_vNEW_VERSION-win-x64.zip. The filename, architecture, and version chain must match exactly; files renamed with suffixes like (1) or copy, or files whose version or architecture does not match the current installation, will be rejected immediately. This is intended for users who cannot access GitHub directly but can obtain full packages or OTA packages from other channels such as community file shares. This feature requires v6.8.0-beta.2 or later.

#### Recommended Full-Package Cleanup for Beta Users

While developing the standalone updater, we found and fixed a long-standing issue that had existed since the earliest beta releases: when comparing a beta build against a stable or public build, removelist could incorrectly add every directory to the deletion list. We therefore recommend that all beta users download a full package once and drag it into MAA to perform a cleanup replacement. When updating from a full package, MAA preserves the debug, cache, data, and config folders, while other files are moved to the Recycle Bin. If you have added or modified files manually, back them up in advance.

#### Gameplay and Overseas Support Expansion

This release adds support for consuming sanity potions by specified expiration days, JieGarden monthly squads and deep investigations, and broader overseas support with faster clue placement, OS stage and mini-game resources, OCR improvements, and localization updates.

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
