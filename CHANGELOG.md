## v5.18.0-beta.1

### 新增 | New

* 成就系统 (#12823) @ABA2396 @Constrat @status102 @HX3N

### 改进 | Improved

* 刷理智增加未耗尽战斗次数提示输出 @status102
* 优化导航逻辑 @ABA2396
* 合并Tooltip @status102
* 肉鸽战斗使用费用、击杀数缓存, 优化性能占用 @status102
* wpf属性监听简化 (#12725) @status102
* remove unneccesary InfrastTrainingTime from EN @Constrat
* use text ocr instead of template for battlequick @Constrat

### 修复 | Fix

* 繁中服 傀影肉鴿 卡在通關劇情 (#12952) @momomochi987
* 未进入战斗期间检测技能使用 @status102
* 还原外服的StageDrops-TimesRec @status102
* 代理次数 @ABA2396
* 编译兼容Swift 6.0 @hguandl
* MacOs 资源更新改进 @hguandl
* 高分辨率下无法进入特定关卡（虽然修了但还是不建议用 2k 4k @ABA2396
* 使用传入的httpService代替全局引用 @status102
* 刷理智掉落识别-代理倍率roi @status102
* 基建训练室技能专精剩余时间Ocr错误 @status102
* 基建专精干员名及技能Ocr使用原图以提高ocr成功率 @status102
* 在招募干员的过程中闪退导致死循环 @ABA2396
* 刷理智战斗后掉落识别代理倍率次数中文关键词使用原图, 以避免阈值过滤造成的边缘细节丢失 @status102
* rect width & height = 0 @status102
* StageDrops-Item warning @status102
* YostarKR changed EnterInfrastSami due to low recognition score @HX3N
* EPChapterToEP for EN @Constrat
* microseconds -> milliseconds @ABA2396
* YostarKR add ReceptionMessageBoard and update templates @HX3N

### 文档 | Docs

* 高分辨率滑动修复文档 @ABA2396
* 调整描述 @ABA2396
* 调整镜像下载失败描述 @ABA2396

### 其他 | Other

* copyright @soundofautumn
* wpf TooltipBlock文本为空时, 不显示 Tooltip 文本框 @status102
* wpf 各种提示 (#12930) @status102 @HX3N @Constrat @ABA2396
* 基建技能专精状态分析 @status102
* 调整依赖库安装脚本输出 @ABA2396
* 怎么真有人看到 Y 确认 是用鼠标去点而不是输入 Y 回车 @ABA2396
* update infrast training room @momomochi987 @HX3N @Constrat
