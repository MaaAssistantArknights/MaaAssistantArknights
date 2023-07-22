## v4.20.0-rc.1

### 改进

- 修改萨米第一层提丰放置顺序以及站位 (#5567) @salenth
- 优化水月部分地图部署逻辑 (#5576) @Yumi0606
- 全局配置不存在时会优先使用当前配置的字段，并赋值给全局配置 @ABA2396
- 调整窗口标题 @TiSpH

### 修复

- 0 时的定时任务无法提前重启 (#5571) @TiSpH
- 修复定时器配置从未选择时的逻辑错误 @ABA2396
- 定时器保存问题 @ABA2396
- 修复输入非法时间时，输入框会重置但却存储错误数据的问题 @ABA2396

### 其他

- 修复对ranges支持检测的判断条件 (#5548) @hguandl
- 修复初次使用多出来一个Global配置的问题 @MistEO
- added new ignored Sami IS4 images @Constrat
- 打包 macos dylib @AnnAngela

### For overseas

#### Common

- FC event stages implementation for KR, JP, EN @Constrat

#### YostarEN

- added missing strings form  missing_translate-YostarEN @Constrat
- Workaround for EN infrastracture oper regex (#5549) @Constrat

#### YostarKR

- Improved operator awareness in infrastructure (#5564) @zewoosJ
- add tutorial docs for kr @zewoosJ @178619

#### txwy

- 修复干员名识别 (#5580) @Arcelibs
