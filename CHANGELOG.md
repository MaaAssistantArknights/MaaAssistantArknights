## v4.19.3

### 新增

- 更新活动游戏数据、地图资源等 @MistEO @yuanyan3060
- 添加 `吃理智药` `指定次数` 右键选中功能，通过右键选中时重启后不保存 @ABA2396
- 定时任务强制启动 (#5280) @ABA2396 @MistEO
- 添加自动安装更新包选项 @ABA2396
- 指定材料也支持右键单次执行 @ABA2396
- 添加内置作业下拉框 @moomiji
- 从剪贴板中复制作业路径 @moomiji

### 改进

- 关闭模拟器后重置连接状态，减少无意义重连尝试 @ABA2396
- 添加连接超时 @ABA2396
- 错误信息解决方案高亮显示 @ABA2396
- 对镜像进行延迟测试，并选择最优镜像 (#5311) @LiamSho @ChingCdesu
- 吃理智药选中时勾选吃源石不改变理智药选中状态 @ABA2396

### 修复

- 将 OCR replace 改为有序 @MistEO
- 未设置时剩余理智勾选状态错误显示 @ABA2396
- 通过窗口关闭模拟器的日志错误 @ABA2396
- 集合已修改；可能无法执行枚举操作 @ABA2396
- 修复配置名为null时添加配置报错，留空时以当前时间作为配置名 @ABA2396
- 活动结束后活动关卡被误判成常驻关卡 @ABA2396
- 修复连接报错问题 @MistEO
- ArkItems 在反序列化失败时可能引发 `未将对象引用设置到对象的实例` @ABA2396
- 修复指定材料刷取数量显示状况不及时响应 @ABA2396
- 关卡选择检测未将对象引用设置到对象的实例 @ABA2396
- 回滚访问好友相关阈值 @MistEO @ABA2396
- 修复start后 running 状态不变的问题 @MistEO
- 尝试修复手动输入关卡名时偶现主选关卡变为空 @ABA2396
- NewConfigurationName为空字符串也自动改为当前时间 @ABA2396
- 在RunningState类中用状态机替换idle处理 (#5293) @ABA2396

### 其他

- workaround for destruction order issue @horror-proton
- 更新 Linux 编译 workflow 链接 (#5252) @AnnAngela
- `""` -> `「」` @ABA2396
- 文档实时显示贡献者 @AnnAngela
- 在连接前提示替换adb @ABA2396
- 修复docs变动检测逻辑 @AnnAngela

### For overseas

#### YostarEN

- Modified operators regex for Auto Squad and role button (#5249) @Constrat
- Update en-us.xaml @ABA2396
- typo: useless \n in EN GUI (#5236) @Constrat

#### txwy

- 台服 基建識別 (#5256) @vonnoq
- 台服 理想城活動導航 (#5212) @vonnoq
- 台服 引航者试炼 (#5283) @vonnoq

#### YostarKR

- add CrisisPopup.png for YoStarKR @178619
