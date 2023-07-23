## v4.20.0

### 新增

- 适配萨米肉鸽 (#5393) @DavidWang19 @SherkeyXD @LingXii @qxdqhr @HauKuen @yyjdelete @Ziqing19 @mathlover @yc080713 @mikucy @hguandl @vic233333 @salenth
- 定时任务可切换配置 (#5323) @ABA2396 @MistEO

### 改进

- 自动关闭“检测到有未下载的语音资源”窗口 (#5396) @Hydrogina
- 肉鸽debug日志写入图片前限制文件夹内图片数量 (#5325) @BladeHiker
- 优化HttpClient构建过程 (#5357) @ChingCdesu @HisAtri
- 自动检测更新的随机延迟改为一小时 @ABA2396
- 更改更新弹窗格式 @ABA2396
- 优化水月部分地图部署逻辑 (#5576) @Yumi0606
- 全局配置不存在时会优先使用当前配置的字段，并赋值给全局配置 @ABA2396
- 调整窗口标题 @TiSpH

### 修复

- 添加定时器缺少的判断 @ABA2396
- 点击技能启动按钮时，验证按钮已消失，防止卡死在子弹时间中 (#5374) @chaserhkj
- 添加山的干员名字纠错规则 (#5372) @chaserhkj
- add CloseEvent task for Phantom Roguelike (#5358) @chaserhkj
- 强制启动定时任务未在运行时不关闭方舟 #5346 @ABA2396
- 尝试修复运行时下载完成更新包后卡死 (#5327) @ABA2396
- 强制定时启动无法保存; 配置名称过长显示问题 (#5532) @TiSpH
- 定时功能设置无法更新 @ABA2396
- 0 时的定时任务无法提前重启 (#5571) @TiSpH
- 修复定时器配置从未选择时的逻辑错误 @ABA2396
- 修复输入非法时间时，输入框会重置但却存储错误数据的问题 @ABA2396

### 其他

- 优化了互斥规划问题的 dlx 算法实现在一定情况下的速度 @lhhxxxxx
- 更新裁图小工具 @MistEO
- 在文档的导航栏里加上前往官网的链接 @AnnAngela
- 修改关卡重连描述 @ABA2396
- 代码清理 @ABA2396
- fix typo in AsstCaller.h (#5435) @eltociear
- 添加一键编译macOS Core的脚本 @hguandl
- 在用户有代理的情况下，提升国外镜像的优先级 (s3限速上行50Mbps) @ChingCdesu
- AsstConf.h 对 clang-tidy 不友好的问题 (#5510) @hxdnshx
- 规范化step name (#5517) @SherkeyXD
- 删除export后面多余的空格 (#5515) @SherkeyXD
- 修复对ranges支持检测的判断条件 (#5548) @hguandl
- 修复初次使用多出来一个Global配置的问题 @MistEO
- added new ignored Sami IS4 images @Constrat
- 打包 macos dylib @AnnAngela

### For overseas

#### Common

- FC event stages implementation for KR, JP, EN @Constrat

#### YostarEN

- raised threshold for StageAnnihilation.png (#5234) @Constrat
- Added correct StageAnnihilation.png after update @Constrat
- Visiting only 1 Friend Clue Exchange in YostarEN (#5424) @Constrat
- Missing .png for YostarEN and modified tools/ignore_list_of_templates.txt (lack of Chinese characters) (#5458) @Constrat
- OperBox not retrieving Oper Class bar in EN @Constrat
- added missing strings form missing_translate-YostarEN @Constrat
- Workaround for EN infrastracture oper regex (#5549) @Constrat

#### YostarKR

- Improved operator awareness in infrastructure (#5564) @zewoosJ
- add tutorial docs for kr @zewoosJ @178619

#### txwy

- 繁中服干员名识别错误 @Arcelibs

### For dev

- Python api 更新 同时请求多个url下载 (#5390) @HisAtri
- 使用字典推导式 (#5381) @HisAtri
- 修复Python更新器中重试次数的问题 (#5402) @Q-ModifiedJ
- 适配pythonAPI到最新版 (#5347) @DoyoDia
