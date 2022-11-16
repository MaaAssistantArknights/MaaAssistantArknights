**若更新后滑动操作没反应、自动战斗没反应、肉鸽不上人等，请进入 `设置` - `连接设置` - `强制替换 ADB`**

- 重构 adb 控制器，启用 minitouch 进行滑动操作，并伪装了进程名，在保证安全的前提下，大幅优化所有滑动操作速度 @MistEO @zzyyyl @horror-proton
- 新增 自动战斗 `ButtleTime`, `costs` 字段 @MistEO
- Support Base function for JP client @[ajk](https://space.bilibili.com/341526)
- 更新 危机合约、IS-S 关卡 地图数据 @MistEO
- 优化 水月肉鸽 安全屋选项，支持安全屋再利用 @txtxj @zzyyyl
- 优化 文字识别器 参数结构 @zzyyyl
- 优化 macOS 截图操作，新增支持 nc 转发，并禁用可能造成内存泄漏的 gzip 方式 @hguandl
- 修复 活动关卡导航 @zzyyyl
- 修复 macOS 繁中肉鸽 文字识别错误 @hguandl
- 修复 macOS 信用商店 设置错误 @hguandl
- 修复 下载 镜像站地址，修复 内测版 打包策略 及 CHANGELOG 生成 @horror-proton @zzyyyl @MistEO
- 修复 界面 不支持通知时崩溃的问题，使用兼容版通知 @zzyyyl
- 清理 代码 等 @MistEO @zzyyyl
- 新增 调试后门，在启动前新建 `DEBUG.txt`，将会在每次运行时都重新读取资源文件 @MistEO
- 新增 Issue Bot mac 标签 @zzyyyl
