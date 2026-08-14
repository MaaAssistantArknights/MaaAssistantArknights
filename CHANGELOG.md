## v6.17.0-beta.1

### Highlights

#### 外服 PC 端适配

PC 端游戏窗口标题按客户端类型解析，外服明日方舟 PC 端也能识别与连接。明日方舟 PC 端为社区维护、不保证可用性，如非必要建议优先使用 ADB 连接模拟器或手机。

#### 像素画支持粘贴

像素画自动填色新增粘贴功能：可直接从剪贴板粘贴图片，也支持粘贴 4 字以内的文本生成像素画。

#### 繁中服界园肉鸽 DLC 适配

繁中服启用界园肉鸽 DLC 分队（知学分队、商贾分队等），并补充相关参数与 OCR 适配。

<details>
<summary><b>English</b></summary>

#### PC Client Support for Global Servers

The PC client window title is now resolved by client type, so global Arknights PC clients can also be recognized and connected. Note that the Arknights PC client is community-maintained with no availability guarantee — connecting via ADB to an emulator or a phone is still recommended.

#### Pixel Art Paste

Pixel art auto-filling now supports pasting: paste an image directly from the clipboard, or paste text of up to 4 characters to generate pixel art.

#### txwy Jie Garden Roguelike DLC

Enabled the Jie Garden roguelike DLC squads (e.g. Knowledge Squad, Merchant Squad) for txwy, along with related parameters and OCR adaptations.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.17.0-beta.1 (2026-08-14)</b></summary>

### 新增 | New

* 像素画自动填色新增粘贴功能，支持从剪贴板粘贴图片与 4 字以内的文本 ([#17662](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17662) [#17689](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17689)) @H2O-MERO @ABA2396
* 繁中服启用界园肉鸽 DLC 分队并适配相关参数与 OCR 对照 ([#17705](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17705)) @momomochi987
* MaaCore 新增扩展 C 接口 `AsstCallerExtra` ([#17701](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17701)) @hguandl

### 改进 | Improved

* 多作业模式下导航名改由 Core 自动从地图数据读取，并新增 `nav_name_override` 参数支持手动覆盖导航识别名 ([#17687](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17687)) @status102 @hguandl
* PC 端游戏窗口标题按客户端类型解析，连接与结束模拟器支持不同语言的 PC 端，并优化 PC 端相关描述文案 ([#17679](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17679)) @HX3N @ABA2396
* 显卡兼容性提示在每次开始运行时输出，避免被任务日志刷掉后无法看到 @ABA2396
* 调整 core 崩溃后和未知异常的错误提示 @ABA2396
* 为软件更新包下载添加重试 ([#17675](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17675)) @bkzzzz
* 自动战斗自动编队切换职业时切换回全部职业分类，避免游戏未重置 UI 位置 @status102
* 繁中服调整部分干员与关卡名称 OCR ([#17703](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17703)) @momomochi987
* YostarJP OCR fixes ([#17704](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17704)) @Manicsteiner
* YostarKR update localization with official terminology @HX3N

### 修复 | Fix

* 修复 NotifyIcon 双击间隔为 0 时的启动崩溃 ([#17691](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17691)) @bkzzzz
* 修复连接配置下拉框打开和滚动时整个页面位移的问题 @ABA2396
* 修复 MuMu 触控增强等支持划火柴的触控模式下，划火柴模式开关参数未正确传递生效的问题 ([#17652](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17652)) @Rbqwow
* YostarKR handle startup notification during account switch @HX3N

### 文档 | Docs

* 更新新手入门文档，简化下载安装步骤并说明日志包生成方式 @ABA2396
* 修正多语言文档错别字，更新作业协议 `nav_name_override` 字段说明 ([#17708](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17708)) @apricity093 @hguandl

### MaaMacGui

#### 新增 | New

* 支持作业集 @hguandl
* 支持奇象巡展像素画 @hguandl

#### 改进 | Improved

* 调整像素画选项文案 @hguandl

</details>
