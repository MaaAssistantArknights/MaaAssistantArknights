## v4.28.0-beta.2

### 新增 | New

- 日服支持加工站换班 @ABA2396
- 最快截图耗时超出阈值后，输出额外提示文本 (#7489) @status102
- YoStarJP 狂人号 navigation change (#7488) @Manicsteiner
- SideStory「银心湖列车」活动导航 @ABA2396

### 改进 | Improved

- 遇到问题时提前结束选取密文板 (#7501) @ABA2396 @status102
- 优化保全关卡识别逻辑，改为识别LT-n (#7527) @status102
- 肉鸽适配新干员用法 (#7500) @Lancarus

### 修复 | Fix

- corrected ReceiveAward txwy template fix #7389 @Constrat
- 日服识别 斥罪->贝娜 @ABA2396
- 日服识别错误 麒麟->林 @ABA2396
- wrong task `SN-Open` (#7498) @zzyyyl
- SN navigation template change @Constrat
- 外服刷理智任务没有源石库存时卡死 @ABA2396
- 多种模拟器多开时关闭错误模拟器 @ABA2396

### 其他 | Other

- reduced screencap timeout (#7476) @Constrat
- 文档目录结构大重构 (#7423) @SherkeyXD
- 规范化ci文件格式及step命名，优化pr-checker的触发逻辑及表现形式 (#7512) @SherkeyXD
- 重构 SettingsViewModel Init @ABA2396
- 修改NoStone图片模板 @ABA2396
- KillEmulator记录地址和端口 @ABA2396
- PR Checker增加对提交父项数量的检查以避免通过改名应对 @status102
- furni忽略拼写检测 @ABA2396
- 反斜杠处理错误 @ABA2396
- typo in task name (#7478) @Constrat
- sample.py 获取地址从相对路径改为绝对路径 (#7483) @SummerDiver
- 修复除version.json外无文件更新时不保存version @ABA2396
- fix coredump when minitouch failed to restart @horror-proton
