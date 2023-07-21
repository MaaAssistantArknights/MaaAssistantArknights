## v4.20.0-rc.1

### 新增

- 全局配置不存在时会优先使用当前配置的字段，并赋值给全局配置 @ABA2396
- added missing strings form  missing_translate-YostarEN @Constrat
- FC event stages implementation for KR @Constrat
- FC event stages implementation for JP @Constrat
- FC event stages implementation for EN @Constrat

### 改进

- 优化水月部分地图部署逻辑 (#5576) @Yumi0606

### 修复

- 0时的定时任务无法提前重启 (#5571) @TiSpH
   - 调整窗口标题 @TiSpH
   - 0时的定时任务无法提前重启 @TiSpH
- Workaround for EN infrastracture oper regex (#5549) @Constrat
- 修复定时器配置从未选择时的逻辑错误 @ABA2396
- 定时器保存问题 @ABA2396
- 修复对ranges支持检测的判断条件 (#5548) @hguandl
   - 修复对ranges支持检测的判断条件 @hguandl
- 修复输入非法时间时，输入框会重置但却存储错误数据的问题 @ABA2396

### 其他

- Update tasks.json (#5580) @Arcelibs
   - Update tasks.json @Arcelibs
- 修改第一层提丰放置顺序以及站位 (#5567) @salenth
   - 修改第一层提丰放置顺序以及站位 @salenth
- Auto Update Game Resources - 2023-07-20 @MistEO
- Merge branch 'dev' of https://github.com/MaaAssistantArknights/MaaAssistantArknights into dev @MistEO
   - 修复初次使用多出来一个Global配置的问题 @MistEO
- added new ignored Sami IS4 images @Constrat
- Merge branch 'dev' of https://github.com/MaaAssistantArknights/MaaAssistantArknights into dev @ABA2396
   - 未配置全局变量时也进行储存 @ABA2396
- Auto Update Game Resources - 2023-07-20 @MistEO
- Improved operator awareness in infrastructure (#5564) @zewoosJ
   - 개발 뉴비 영업용 @zewoosJ
   - Improved operator awareness in infrastructure @zewoosJ
