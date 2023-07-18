## v4.20.0-beta.1

### 新增

- 适配萨米肉鸽 (#5393) @DavidWang19 @SherkeyXD @LingXii @qihongrui @HauKuen @yyjdelete @Ziqing19 @mathlover @yc080713
- 定时任务可切换配置 (#5323) @ABA2396 @MistEO

### 改进

- 自动关闭“检测到有未下载的语音资源”窗口 (#5396) @Hydrogina
- 肉鸽debug日志写入图片前限制文件夹内图片数量 (#5325) @BladeHiker
- 优化HttpClient构建过程 (#5357) @ChingCdesu @HisAtri
- 更改更新弹窗格式 @ABA2396

### 修复

- 添加定时器缺少的判断 @ABA2396
- 点击技能启动按钮时，验证按钮已消失，防止卡死在子弹时间中 (#5374) @chaserhkj
- 添加山的干员名字纠错规则 (#5372) @chaserhkj
- add CloseEvent task for Phantom Roguelike (#5358) @chaserhkj
- 强制启动定时任务未在运行时不关闭方舟 #5346 @ABA2396
- 尝试修复运行时下载完成更新包后卡死 (#5327) @ABA2396

### 其他

- 更新裁图小工具 @MistEO
- 在文档的导航栏里加上前往官网的链接 @AnnAngela
- 修改关卡重连描述 @ABA2396
- 代码清理 @ABA2396
- fix typo in AsstCaller.h (#5435) @eltociear

### For overseas

#### YostarEN

- raised threshold for StageAnnihilation.png (#5234) @Constrat
- Added correct StageAnnihilation.png after update @Constrat
- Visiting only 1 Friend Clue Exchange in YostarEN (#5424) @Constrat
- Missing .png for YostarEN and modified tools/ignore_list_of_templates.txt (lack of Chinese characters) (#5458) @Constrat

### For dev

- Python api 更新 同时请求多个url下载 (#5390) @HisAtri
- 使用字典推导式 (#5381) @HisAtri
- 修复Python更新器中重试次数的问题 (#5402) @Q-ModifiedJ
- 适配pythonAPI到最新版 (#5347) @DoyoDia
