## v4.19.3-beta.1

### 新增

- 定时任务强制启动 (#5280) @ABA2396 @MistEO
- 添加 `吃理智药` `指定次数` 右键选中功能，通过右键选中时重启后不保存 @ABA2396

### 改进

- 关闭模拟器后重置连接状态，减少无意义重连尝试 @ABA2396
- 添加连接超时 @ABA2396

### 修复

- 将 OCR replace 改为有序 @MistEO
- 未设置时剩余理智勾选状态错误显示 @ABA2396
- 通过窗口关闭模拟器的日志错误 @ABA2396
- 集合已修改；可能无法执行枚举操作 @ABA2396
- 修复配置名为null时添加配置报错，留空时以当前时间作为配置名 @ABA2396
- 活动结束后活动关卡被误判成常驻关卡 @ABA2396

### 其他

- workaround for destruction order issue @horror-proton
- 更新 Linux 编译 workflow 链接 (#5252) @AnnAngela
- `""` -> `「」` @ABA2396
- 文档实时显示贡献者 @AnnAngela
- 在连接前提示替换adb @ABA2396


### For overseas

#### YostarEN

- Modified operators regex for Auto Squad and role button (#5249) @Constrat
- Update en-us.xaml @ABA2396
- typo: useless \n in EN GUI (#5236) @Constrat

#### txwy

- 台服 基建識別 (#5256) @vonnoq
- 台服 理想城活動導航 (#5212) @vonnoq
- 台服 引航者试炼 (#5283) @vonnoq
