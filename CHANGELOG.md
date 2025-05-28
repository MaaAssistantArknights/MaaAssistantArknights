# ⚠️ 重要提醒
本次更新调整了模板图的目录结构，**请勿直接覆盖旧版本文件夹**，否则可能导致部分旧文件未被正确删除，从而引发识别问题。

推荐的更新方式有两种：
1. 使用自动更新功能（推荐）：自动更新将会正确清理旧文件。
2. 手动更新时：请将新版本解压到一个全新的文件夹，再将原有的 config 文件夹复制过去，以保留您的配置。

## v5.17.0-beta.1

### 新增 | New

* 繁中服 薩卡茲肉鴿 (#12800) @momomochi987
* 自动战斗费用识别添加缓存, 减少性能消耗 (#12765) @status102

### 修复 | Fix

* KR 驮兽旅行家 OCR @Daydreamer114
* global templates are not loaded (#12787) @HX3N

### 其他 | Other

* 我面前站不下这许多人 @AnnAngela
* 更新识别工具逻辑与显示效果 (#12791) @ABA2396
* 模板图允许从子文件夹加载 (#12717) @SherkeyXD
* 截图测试可以在自动检测的情况下使用 @ABA2396
* 重启函数添加 CallerMemberName，UntilIdleAsync 添加消抖 @ABA2396
* YostarJP roguelike ocr edit (#12789) @Manicsteiner
* bump maa-cli to 0.5.5 (#12664) @wangl-cc
* 防止缓存的任务文件被意外覆盖 @status102
* YostarJP FormationOCR params (#12783) @Manicsteiner
* 调整技能识别保存截图的阈值 @ABA2396
* TooltipBlock封装 (#12773) @status102 @ABA2396
