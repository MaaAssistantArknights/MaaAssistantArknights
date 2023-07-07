## v4.19.3-rc.1

### 新增

- 指定材料也支持右键单次执行 @ABA2396
- 添加内置作业下拉框 @moomiji
- 从剪贴板中复制作业路径 @moomiji

### 改进

- 更新活动游戏数据、地图资源等 @MistEO @yuanyan3060
- 对镜像进行延迟测试，并选择最优镜像 (#5311) @LiamSho @ChingCdesu
- 吃理智药选中时勾选吃源石不改变理智药选中状态 @ABA2396

### 修复

- 修复指定材料刷取数量显示状况不及时响应 @ABA2396
- 关卡选择检测未将对象引用设置到对象的实例 @ABA2396
- 回滚访问好友相关阈值 @MistEO @ABA2396
- 修复start后 running 状态不变的问题 @MistEO
- 尝试修复手动输入关卡名时偶现主选关卡变为空 @ABA2396
- NewConfigurationName为空字符串也自动改为当前时间 @ABA2396
- 在RunningState类中用状态机替换idle处理 (#5293) @ABA2396

### 其他

- 修复docs变动检测逻辑 @AnnAngela
