## v4.15.0-rc.1

### 改进

- 执行识别任务时屏蔽识别按钮，避免二次开始任务 @ABA2396
- 今天小提示加入滑动条 @MistEO

### 修复

- WPF UI 资源加载错误 @MistEO
- Mac Gui 修复外服资源路径错误 @hguandl
- 修复水陈在特别关注的情况下的识别问题 @ABA2396
- 修复约翰老妈保全关卡 `混乱“派对”` 的识别错误 @ABA2396
- 部分情况下活动关导航不进入章节 @ABA2396
- 修复更新出错的问题 @zzyyyl
- 修复 视频识别 标签页切换异常 @ABA2396

### 其他

- 优化了点有的没的 @MistEO @zzyyyl
- CI编译macOS版统一用 Release @hguandl
- LoadResource 添加 onlyReloadOTA @ABA2396
- 调整设计时高度 @ABA2396

### For develops

- `AsstLoadResource` 接口在文件夹不存在时将返回 false @MistEO
