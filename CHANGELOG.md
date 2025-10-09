## v5.26.1

### 新增 | New

* 新增 cdk 被封禁的提示信息 @ABA2396
* RM-1 (#14271) @Daydreamer114

### 改进 | Improved

* RegionOCRer 中 useRaw=false 时, 使用原图二值蒙版代替直接 OCR 二值图像 (#14276) @status102

### 修复 | Fix

* 游戏更新公招界面后无法确认招募 (#14335) @ABA2396
* 第一次访问 mirror酱 失败时错误提示 cdk 已过期 @ABA2396
* resource version @MistEO
* Update last_updated date in version.json @MistEO
* Update last_updated timestamp in version.json @MistEO
* 手动关闭模拟器后未重启 MAA 时 minitouch 可能失效 @ABA2396
* 尝试修复生息演算任务识别并删除编队时卡住的问题 (#14290) @Alan-Charred
* "MiniGame@RM-1@Wait" @Daydreamer114
* 增强 playtools 关闭连接时的异常处理，确保套接字安全关闭 (#14280) @RainYangty
* EN IS3 encounter ocr fix MAA,EN服水月肉鸽 事件名识别错误bug Fixes #1427214272 @Constrat
* 理智药使用数量ocr不准确时中断使用 @status102
* 使用理智药 执行减少次数循环在asst_stop时缺少中断判断 @status102
* 修复因失败导致次生预算出错 (#14267) @Saratoga-Official

### 文档 | Docs

* 补充 CopilotTask 的文档 (#14319) @Alan-Charred
* 添加目录自动跳转组件并使locale自动生成 (#14299) @lucienshawls
* 文档站新增字符画组件 (#14270) @lucienshawls
* 将文档中指向部分文档目录的链接改为指向对应目录下的第一篇文档 (#14292) @JasonHuang79

### 其他 | Other

* 使用 `BeginAnimation` 替代 `新建 Storyboard 并添加动画` @ABA2396
* run smoke test in lldb @horror-proton
* 将mac开发环境下的cmake_osx版本改为13.4 (#14283) @Pylinx171
* YostarJP ocr fix @Saratoga-Official
* 完善容器配置及依赖安装 (#14208) @lucienshawls
