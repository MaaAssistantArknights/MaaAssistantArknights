## v5.16.8

### 修复 | Fix

* 自动编队干员Ocr左侧内缩过度 @status102
* RegionOcr阈值过滤膨胀后超出原roi @status102
* 自动编队roi溢出闪退 @status102
* 修复roi超限 @status102

### 其他 | Other

* 别再把 maa 放在 Program Files 里了 @ABA2396

## v5.16.7

### 改进 | Improved

* 选择 Mirror酱 更新渠道更新软件版本但未填写 cdk 时 Fallback 至海外源 @ABA2396
* 优化 Placeholder 提示 @ABA2396
* 优化检查更新逻辑 @ABA2396
* 自动战斗编队优化干员识别, 修复roi未包含完整干员名 (#12740) @status102 @pre-commit-ci[bot]

### 修复 | Fix

* 使用理智药超限后减少后无法确认使用理智药 @status102
* Mac修复Mirror酱资源更新问题 @hguandl

### 其他 | Other

* wpf版本不一致仅允许DebugVersion @status102
* 地图截图改用 jpeg @ABA2396
* 使用理智药后状态未能更新 @status102
* 自动战斗循环 BattleStartPre retry 3次 @ABA2396
* task schema update @SherkeyXD
* 调整战斗列表说明 @status102
* 调整界面提示 @ABA2396
