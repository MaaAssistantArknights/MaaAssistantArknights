## v4.7.3

- 新增 `MaaTouch` 连接配置 项，推荐在 minitouch 无法使用，但又不想用 `兼容触控模式` 时开启 @aa889788 @MistEO
- 修复 minitouch 路径错误无法使用的问题 @MistEO

## v4.7.2

- 修复 刷理智 掉落识别 在极端情况下疯狂上传的问题 @MistEO
- 修复 刷理智 时间缓存不生效的问题 @MistEO
- 修复 夜神模拟器 偶现截图黑屏的问题 @MistEO
- 优化 水月肉鸽 部分地图部署策略 @WWPXX233
- 更新 文档 @ntgmc

### For Overseas clients

- Fix OCR error for ZH_TW client @horror-proton

## v4.7.1

- 修复 强制替换 ADB 进程被占用时失败的问题 @MistEO
- 修复 macOS 启动卡住的问题 @hguandl
- 修复 干员名识别错误 @cenfusheng
- 新增 界面 触控相关提示，~~再问紫砂！~~ @MistEO
- 新增 检查更新 镜像 API @tangge233

### For Overseas clients

- Fix the stuck in the encounter in IS @MistEO
- Update documentation @wallsman

## v4.7.0

- 新增并全面启用 minitouch，大幅优化所有操作速度 @MistEO @zzyyyl @horror-proton @hguandl
- 新增 兼容触控模式，请进入设置 - 连接设置中启用。不推荐开启，仅建议在 minitouch 无法工作时使用 @MistEO
- 新增 自动战斗 `ButtleTime`, `costs` 字段 @MistEO
- 新增 自定义基建 `description_post` 字段，整理内置作业格式 @MistEO
- 更新 危机合约、IS-S 关卡 地图数据 @MistEO
- 优化 水月肉鸽 安全屋选项，支持安全屋再利用 @txtxj @zzyyyl
- 优化 自定义基建 内置 243 3 换作业 @Black1312
- 优化 macOS 截图操作，新增支持 nc 转发，并禁用可能造成内存泄漏的 gzip 方式 @hguandl
- 优化 文字识别器 参数结构 @zzyyyl @MistEO
- 修复 活动关卡导航 @ABA2396
- 修复 肉鸽 关卡识别错误时狂吃内存的问题 @zzyyyl
- 修复 基建 制造/贸易站 偶尔点漏一个房间的问题 @MistEO
- 修复 基建 干员 `霜叶` 识别错误 @Black1312
- 修复 macOS 繁中肉鸽 文字识别错误 @hguandl
- 修复 macOS 信用商店 设置错误 @hguandl
- 修复 POSIX 下 socket 超时时间过长的问题 @aa889788
- 修复 下载 镜像站地址，修复 内测版 打包策略 及 CHANGELOG 生成 @horror-proton @zzyyyl @MistEO
- 修复 界面 基建设置心情阈值显示错误的问题 @ABA2396
- 修复 界面 不支持通知时崩溃的问题，使用兼容版通知 @zzyyyl
- 修复 Java 接口路径错误 @CaoMeiYouRen
- 清理 代码 等 @MistEO @zzyyyl
- 新增 调试后门，在启动前新建 `DEBUG.txt`，将会在每次运行时都重新读取资源文件 @MistEO
- 新增 Issue Bot mac 标签 @zzyyyl
- 更新 文档 @MistEO @DavidWang19

### For Overseas clients

- Support Base function for the JP client @ajk946009123
- Support Annihilation Combat for the JP and EN clients @MistEO @wallsman
- Fix StartUp and Daily error for the JP client @liuyifan-eric
- Fix a dead loop when recruiting opers in the auto-IS function of the EN client @MistEO
