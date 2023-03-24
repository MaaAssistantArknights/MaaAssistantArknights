## v4.13.0-beta.1

### 新增

- 界面夜间模式 (#4020) @ABA2396
- 更新危机合约地图及其他游戏数据 @lza11111 @MistEO
- macOS OTA 支持 @hguandl

### 改进

- 未选择任务时界面报错信息 #4034 @ABA2396
- 危机合约版本更新无法刷钱本等物质搜寻关 #4051 @ABA2396
- 关闭基建宿舍复查功能 @MistEO
- 将海蒂加入阿卡胡拉作业黑名单 (#4044) @Snow-dash

### 修复

- 修复 Mac LS-6 tag 错误 @ABA2396
- CE-6 代理识别错误 @ABA2396
- 修复定时器显示问题 @ABA2396
- 自定义关卡与备选关卡同时开启时的逻辑问题 #4029 @ABA2396
- 修复备选关卡不会自动大写的问题 @ABA2396
- 尝试修复CE-5导航问题 @ABA2396
- 主线关卡导航问题 @ABA2396

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

### For overseas

#### Common

- 增加海外悠星服务器153基建的作业（截止至玛恩纳） (#4050) @BlackNorton
- sort overseas clients' json @liuyifan-eric

#### YostarJP

- JP OF-1 credit support @liuyifan-eric
- Fix 承曦グレイ （YoStarJP） (#4009) @cenfusheng

#### TXWY

- 修复 礫（txwy）的文字识别问题 (#4009) @cenfusheng
