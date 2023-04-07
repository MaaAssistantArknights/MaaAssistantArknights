## v4.13.0

### 新增

- 初步支持作业视频识别 (#4030) @MistEO
- 初步支持控制在 Mac 上运行的 IOS 版明日方舟 (#4161) @hguandl
- 界面夜间模式 (#4020) @ABA2396 @Cryolitia
- macOS OTA 支持 @hguandl

### 改进

- 更新游戏资源，降低尤里卡会客室优先级 @lza11111 @MistEO
- 优化自动肉鸽招募与战斗逻辑框架 (#4136) @LingXii
- 重构 Controller 架构 (#3907) @aa889788 @MistEO
- WPF 大重构 (#4067) @LiamSho @DavidWang19 @MistEO
- 从 curl 迁移到 libcpr，支持 win7 等系统上传企鹅数据 (#4088) @aa889788 @MistEO
- fallback to NativeIO when AdbLite not inited (#4109) @aa889788
- proxy default protocol / start location (#4100) @LiamSho
- 增加 json 请求超时限制 (#4065) @DavidWang19
- 新增“定时检测更新”功能 @ABA2396
- 未选择任务时界面报错信息 #4034 @ABA2396
- 危机合约版本更新无法刷钱本等物质搜寻关 #4051 @ABA2396
- 关闭基建宿舍复查功能 @MistEO
- 将海蒂加入阿卡胡拉作业黑名单 (#4044) @Snow-dash
- 出现未知错误时停止任务 @ABA2396

### 修复

- 修复 Mac LS-6 tag 错误 @ABA2396
- CE-6 代理识别错误 @ABA2396
- 修复定时器显示问题 @ABA2396
- 自定义关卡与备选关卡同时开启时的逻辑问题 #4029 @ABA2396
- 修复备选关卡不会自动大写的问题 @ABA2396
- 尝试修复CE-5导航问题 @ABA2396
- 主线关卡导航问题 @ABA2396
- 关闭蓝叠中国模拟器不完全 #4079 @ABA2396
- 修复自动战斗未加载更新后json的问题 (#4064) @DavidWang19
- add missing item in AsstMsg @horror-proton
- add timeout for IOHandler read (#4095) @aa889788
- fix some warnings and crash @MistEO
- improve post processing in depot item quantity match @horror-proton
- fix deadlock in UI thread @dantmnf
- fix error in background update check @dantmnf
- 尝试修复干员撤退不掉的问题 @MistEO
- 修复采购中心识别问题 @ABA2396
- 修复 UI 线程死锁 (#4164) @dantmnf
- 尝试修复蓝叠HyperV关闭不完全 @ABA2396
- 修复连接错误和闪退 (#4151) @aa889788

### 其他

- fix build error with -Werror=restrict (#4052) @aa889788 @horror-proton
- 修复 Clang / GCC 编译warnings (#4043) @hguandl
- 优化代码 @ABA2396
- 加速 `tasks.json` 的读取 (#3977) @zzyyyl
- prevent unnecessary copy in TilePack @horror-proton
- 优化作业缓存路径 @MistEO
- 修改LinkStart任务执行顺序 @ABA2396
- 修改添加基建任务判断条件 @ABA2396
- 修改蓝叠模拟器关闭方式 @ABA2396
- 调整Level6资源关卡代理判断延迟，替换识别图片 @ABA2396
- 格式化代码 @ABA2396
- put all config keys in ConfigKeys class; feat: allow customizing window title prefix through config (#3993) @KevinT3Hu
- 更新文档 @MistEO @Rbqwow @ABA2396
- stop build if MaaDeps is missing (#4168) @dantmnf
- rename cpp sample @MistEO
- 牛牛继续喝！ @MistEO

### For overseas

#### Common

- 增加海外悠星服务器153基建的作业（截止至玛恩纳） (#4050) @BlackNorton
- sort overseas clients' json @liuyifan-eric

#### YostarJP

- JP OF-1 credit support @liuyifan-eric
- Fix 承曦グレイ （YoStarJP） (#4009) @cenfusheng
- Doc : Update 夕景に影ありて + また会えたね (JP) (#4142) @wallsman

#### YostarKR

- clientKR Roguelike@Dropsflag update (#4093) @Eundong

#### TXWY

- 修复 礫（txwy）的文字识别问题 (#4009) @cenfusheng
- 繁中服更新塵影餘音 (#4104) @vonnoq
- 修改活動導航入口 (#4112) @vonnoq