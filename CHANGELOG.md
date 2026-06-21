## v6.13.0-beta.1

### 新增 | New

* 新增可通过手动修改配置文件开启 6 星自动公招，将是否招募和招募时间选项移入常规设置 (#17154) @ABA2396
* Android OCR 推理使用 NCNN (#17133) @Aliothmoon
* 添加落叶逐火复刻入口任务 @SherkeyXD
* Custom Webhook 新增预置模板功能 (#17081) @pboymt

### 改进 | Improved

* 无法显示系统通知时回退到软件内通知，启动时日志中的通知不可用提示改为 growl 提示 (#17165) @ABA2396
* ci：将 /.github/workflows 中的 github-actions 组更新 2 次 (#17163) @dependabot[bot] @dependabot[bot]
* 统一停止逻辑 (#17087) @ABA2396

### 修复 | Fix

* 修复MAAUnified CI 选错ref的问题 (#17143) @Halo5082
* YostarKR correct ClickChapterNewDefaultProgress roi @HX3N
* 漏了 @status102
* 自动战斗导航retry异常, 跳过作业 @status102
* HandleUpdateFromMaaApi 未能遵循正确的代理设置 @status102
* 修正落叶落叶逐火入口OCR文本 @SherkeyXD
* 肉鸽战斗结束后招募误入StartExplore @Saratoga-Official
* 水月大海的遗产和狗眼婆娑事件名识别错误 @Saratoga-Official
* GetDrop增加LoadingText等待 @Saratoga-Official
* 修复CloseEvent和CloseCollection同时出现导致的问题 @Saratoga-Official

### 其他 | Other

* 调整日志超时提示 @ABA2396
* YostarKR crop JieGarden@Roguelike@StartAction.png to improve match score @HX3N
* Yostar SSS#10 极寒 (#17137) @Manicsteiner
* 怪猎一期复刻 CF-EX-8, CF-S-1 view1参数 @status102
* 移除依赖库安装的提权操作 @ABA2396
* 調整繁中服 "CharsNameOcrReplace" 部分內容 (#17113) @momomochi987
