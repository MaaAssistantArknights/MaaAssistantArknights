## v5.16.1

### 小修小补 | Hotfix

本次版本更新主要修复了 v5.16.0 中存在的若干问题，并对部分功能进行了优化。

#### 基建与编队修复

我们修复了一键轮换模式未勾选设施导致的异常问题，修复了新版本 UI 下自定义基建模式在未展开职业分组时的编队错误，并修正了基建模式切换的稳定性问题。

同时修复了干员选择 CONFESS-47 无法正确识别的问题。

#### 功能优化

优化了赠送单抽的逻辑处理，改进了 VC++ 安装脚本的错误提示，并在所有镜像不可用时提供更清晰的更新失败提示。

如果你在使用过程中遇到任何问题，请通过 `设置`-`问题反馈` 及时反馈~

----

### Minor Fixes | Hotfix

This version primarily addresses several issues from v5.16.0 and includes optimizations for certain features.

#### Base and Squad Fixes

We've fixed issues with the one-click rotation mode when facilities weren't selected, resolved squad formation errors when class groups weren't expanded, and improved the stability of base mode switching.

Also fixed the operator selection issue for CONFESS-47.

#### Improvements

Optimized the logic for single free pulls, improved error reporting for VC++ installation scripts, and added clearer update failure notifications when all mirrors are unavailable.

If you encounter any issues while using, please report them promptly via `Settings` - `Issue Report`~

----

以下是详细内容：
Below is the full changelog:

### 改进 | Improved

* VC++执行安装脚本暴露错误 @status102
* 优化赠送单抽逻辑 @ABA2396
* All mirrors are not available 时日志栏显示更新失败 @ABA2396

### 修复 | Fix

* 修复GetImageFromROI小工具以适配多文件tasks结构 @Lemon-miaow
* 修复开发者帮助文档内的 Typo (#12549) @APeng215
* 无法切换连战次数 @ABA2396
* 不能正确选择干员CONFESS-47 @ABA2396
* 修复一键轮换模式未勾选设施导致功能异常问题 @ABA2396
* 未展开职业分组时编队错误 @ABA2396
* 更新未切换基建模式会导致变回常规模式 @ABA2396

### 其他 | Other

* double int and string map lookup for infrast_data @Constrat
* ignore list templates for global @Constrat
* 调整简中繁中一键轮换说明 @status102
* 添加渐变附加属性 @ABA2396
* 移除干员识别测试说明 @ABA2396
* 移除 depot 测试提示 @ABA2396
* depot 图片只在 debug 模式下保存 @ABA2396
