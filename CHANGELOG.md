## v5.20.0-beta.2

### 主要更新 | Highlight

#### ⚠️ 重要提醒

本次更新调整了模板图的目录结构，若您正在从 v5.19.x 及以下版本**手动**更新至该版本，那么**请勿直接覆盖旧版本文件夹**，否则可能导致部分旧文件未被正确删除，从而引发识别问题。

推荐的更新方式有两种：
1. 使用自动更新功能（推荐）：自动更新将会正确清理旧文件。
2. 手动更新时：请将新版本解压到一个全新的文件夹，再将原有的 config 与 data 文件夹复制过去，以保留您的配置。

#### 界园肉鸽来咯～(∠・ω< )⌒★ | Highlight

牛牛火速为大家适配了界园肉鸽的刷钱模式！v5.20.0-beta.1 中存在的部分刷钱卡死问题已经被修复，现在大家可以愉快的给坎诺特上供啦~

至于刷等级模式，各位博士还请耐心等待 ~~，我们已经在压榨帕鲁干活了（划掉~~

---

以下是详细内容：

## v5.20.0-beta.2

### 新增 | New

* 界园肉鸽烧水支持票券 @SherkeyXD

### 修复 | Fix

* 修复界园主题选择问题 @DavidWang19
* 卡得偿所愿 @ABA2396
* 更新退出事件模板图 @DavidWang19

### 其他 | Other

* 刷开局精二仅为水月和萨米显示 @SherkeyXD
* 移除未被使用的wpf文本 @SherkeyXD
* 删除界园多余的模板图和task @DavidWang19
* EN IS5 squads @Constrat

## v5.20.0-beta.1

### 新增 | New

* 初步适配界园肉鸽 @DavidWang19 @SherkeyXD @status102 @HYY1116 @Saratoga-Official @HYY1116 @ABA2396 @weinibuliu @Daydreamer114 @SweetSmellFox
* 干员因模组不可选中时, 增加提示输出 (#13230) @status102

### 改进 | Improved

* 改进水月肉鸽干员部署逻辑 (#12671) @linlucath @Saratoga-Official

### 修复 | Fix

* 移除ClientType未选择, 统一变更为官服 (#13220) @status102
* ComboBox 剪贴板异常 @ABA2396
* 公告及更新日志背景颜色异常 @ABA2396

### 其他 | Other

* 刷理智界面设计器error @status102
* 基建任务枚举引用路径统一, 修复设计器error @status102
* 启动时成就打包 (#13221) @status102
* 移动所有主题的模板图 @SherkeyXD
* bump dependencies (#13121) @SherkeyXD
* KR JieGarden translation @HX3N

