## v5.18.0

### 新增 | New

* 傀影肉鸽添加难度选择 (#12897) @SherkeyXD
* mumu 5.0 路径自动检测 @ABA2396
* 成就系统 (#12823) @ABA2396 @Constrat @status102 @HX3N @Manicsteiner
* YostarEN gaming theme + JP optimization @Constrat

### 改进 | Improved

* OF-1德克萨斯使用2技能 @status102
* 自动编队跳过空干员职业组 @status102
* 刷理智增加未耗尽战斗次数提示输出 @status102
* 优化导航逻辑 @ABA2396
* remove unneccesary InfrastTrainingTime from EN @Constrat
* 肉鸽战斗使用费用、击杀数缓存, 优化性能占用 @status102
* use text ocr instead of template for battlequick @Constrat

### 修复 | Fix

* IS1 投资策略 @Daydreamer114
* Miss.Christine 识别错误 @ABA2396
* 仅主刷理智任务激活指定次数未消耗殆尽警告 @status102
* KR EP navigation @Daydreamer114
* 繁中服 肉鴿助戰可能招募失敗 (#12966) @momomochi987
* 未进入战斗期间检测技能使用 @status102
* 代理次数 @ABA2396
* 高分辨率下无法进入特定关卡（虽然修了但还是不建议用 2k 4k @ABA2396
* Roguelike@RecruitWithoutButton @ABA2396
* 在招募干员的过程中闪退导致死循环 @ABA2396
* 成就 id 错误 @ABA2396
* 午夜 0 点没算 0 点 @ABA2396
* 使用 Mirror 源的时候也显示了强制使用 Github 的TooltipBlock @ABA2396
* YostarKR add ReceptionMessageBoard and update templates @HX3N
* YostarKR changed EnterInfrastSami due to low recognition score @HX3N
* 还原外服的StageDrops-TimesRec @status102
* 繁中服 傀影肉鴿 卡在通關劇情 (#12952) @momomochi987

### 文档 | Docs

* 高分辨率滑动修复文档 @ABA2396
* 调整描述 @ABA2396
* 调整镜像下载失败描述 @ABA2396

### 其他 | Other

* microseconds -> milliseconds @ABA2396
* UseReaminingSanityStageTip (#12959) @HX3N
* 基建高级设置中部分元素未正确隐藏 (#12988) @lucienshawls
* 自动战斗干员编队时, 若连续错误Ocr贴边干员名为_会导致跳过该职业 @status102
* 刷理智掉落识别-代理倍率roi @status102
* 基建训练室技能专精剩余时间Ocr错误 @status102
* 基建专精干员名及技能Ocr使用原图以提高ocr成功率 @status102
* 使用传入的httpService代替全局引用 @status102
* EPChapterToEP for EN @Constrat
* rect width & height = 0 @status102
* StageDrops-Item warning @status102
* 刷理智战斗后掉落识别代理倍率次数中文关键词使用原图, 以避免阈值过滤造成的边缘细节丢失 @status102
* 编译兼容Swift 6.0 @hguandl
* 资源更新改进 @hguandl
* 不计算理智回复成就 @ABA2396
* 恢复进度失败 @ABA2396
* 合并Tooltip @status102
* wpf属性监听简化 (#12725) @status102
* wpf TooltipBlock文本为空时, 不显示Tooltip文本框 @status102
* LocalizationHelper 支持 GetFormattedString @ABA2396
* 翻译引用 @ABA2396
* 更新繁中翻译 @ABA2396
* remove duplicate templates from JP @Constrat
* 减少预期外的error日志 @status102
* 统一更新copyright @soundofautumn
* 移除解决方案名 @ABA2396
* 移除解决方案名 @ABA2396
* 窗口允许最大化 @ABA2396
* 补全本地化文本中一处设置的完整引导路径 (#12997) @lucienshawls
* asst任务状态 (#12985) @status102
* 更正 指定材料 的提示 (#12977) @status102 @HX3N
* Load 时替换翻译引用 @ABA2396
* 迁移理智, 战斗次数, 代理倍率, 关卡理智缓存 (#12986) @status102
* 更正英文本地化文本中的两处错误 (#12987) @lucienshawls
* 读取mumu新版注册表路径 @ABA2396
* update ignore templates @Constrat
* 刷理智任务手动填写关卡名增加空关卡名对应关卡提示 @status102
* YostarJP Gaming theme (#12970) @Manicsteiner
* YostarKR Poly Vision Museum Theme @HX3N
* YostarKR ocr fix and update Infrast templates @HX3N
* Release v5.18.0-beta.1 (#12956) @ABA2396
* Release v5.18.0-beta.1 (#12954) @ABA2396
* 多余文件描述 @ABA2396
* 统一copyright @soundofautumn
* 修正注释warning @soundofautumn
* wpf各种提示 (#12930) @status102 @HX3N @Constrat @ABA2396
* update txwy for update infrast training room (#12953) @momomochi987
* update KR for update infrast training room @HX3N
* update EN for update infrast training room @Constrat
* Release v5.17.2 (#12941) @status102
* 基建技能专精状态分析 @status102
* 调整依赖库安装脚本输出 @ABA2396
* 怎么真有人看到 [Y] 确认 是用鼠标去点而不是输入 Y 回车 @ABA2396
