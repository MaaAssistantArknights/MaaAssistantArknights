## v4.19.3-beta.1

### 新增

- 定时任务强制启动 (#5280) @ABA2396 @MistEO
   - 强制启动时重置刷理智不保存的参数 @ABA2396
   - 修复多次调用AsstStart的问题 @MistEO
   - 修复running不准确的问题 @MistEO
   - Revert "fix: 修复 stop 停止时机" @MistEO
   - 修复 stop 停止时机 @MistEO
   - 修复强制执行 @MistEO
   - 添加强制启动定时任务 @ABA2396
- 添加 `吃理智药` `指定次数` 右键选中功能，通过右键选中时重启后不保存 @ABA2396
- 关闭模拟器后重置连接状态，减少无意义重连尝试 @ABA2396
- 添加连接超时 @ABA2396

### 改进

- 更新 Linux 编译 workflow 链接 (#5252) @AnnAngela

### 修复

- 将 OCR replace 改为有序 @MistEO
- 未设置时剩余理智勾选状态错误显示 @ABA2396
- 通过窗口关闭模拟器的日志错误 @ABA2396
- workaround for destruction order issue @horror-proton
- 集合已修改；可能无法执行枚举操作 @ABA2396
- Modified operators regex for Auto Squad and role button (#5249) @Constrat
   - ocrReplace not replacing correctly ch'en alter @Constrat
   - Modified operators regex for Auto Squad and role button @Constrat
- 修复配置名为null时添加配置报错，留空时以当前时间作为配置名 @ABA2396
- 活动结束后活动关卡被误判成常驻关卡 @ABA2396

### 其他

- `""` -> `「」` @ABA2396
- Merge branch 'master' into dev @AnnAngela
   - 更新 Linux 编译 workflow 链接 (#5252) @AnnAngela
- 右键选中改为生效一次，改变吃源石勾选状态显示 @ABA2396
- Auto Update Game Resources - 2023-06-26 @MistEO
- 实时显示贡献者 @AnnAngela
- Update en-us.xaml @ABA2396
- i18n: Translations update from MAA Weblate (#5248) @weblate
   - Translated using Weblate (Chinese (Traditional)) @weblate
- i18n: Translations update from MAA Weblate (#5240) @weblate
   - Translated using Weblate (Japanese) @weblate
- typo: useless \n in EN GUI (#5236) @Constrat
- 在连接前提示替换adb @ABA2396
- Auto Update Game Resources - 2023-06-21 @MistEO
- 台服 基建識別 (#5256) @vonnoq
   - 台服 基建識別 @vonnoq
- 台服 理想城活動導航 (#5212) @vonnoq
   - 台服 基建識別 @vonnoq
   - 台服 理想城導航 @vonnoq
   - 台服 理想城活動導航 @vonnoq
- 台服 引航者试炼 (#5283) @vonnoq
