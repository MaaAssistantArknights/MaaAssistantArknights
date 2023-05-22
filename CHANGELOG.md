## v4.18.0-beta.1

### 新增

- Update resources for KR (#4868) @178619
   - Update resources for KR @178619
- 自动战斗编队列表识别不到时，自动划回来重试 @MistEO
- update JP templates for support of recent features @liuyifan-eric
- 肉鸽中不再进阶未精二的干员 (#4825) @whatss7
   - 肉鸽中不再进阶未精二的干员 @whatss7
- 刷理智若检测到OF-1自动停止 @MistEO

### 改进

- 软件更新mirrors做个负载均衡 @MistEO
- 软件更新弃用github api，并全面启用 maa version api @MistEO
- 更新海外客户端数据源，移除对 ArknightsGameData 的引用 @MistEO

### 修复

- 延长战斗中暂停的后延迟 @MistEO
- build error @MistEO
- 博朗台模式同时支持等理智药 @MistEO
- 全局减小adb滑动距离 @MistEO
- 修复干员识别偶尔滑动失效提前结束的问题；并少存一点日志图 @MistEO
- 调整鸿雪肉鸽默认技能为1技能 @MistEO
- 修复连接失败时任务仍被添加的问题 @MistEO
- GUI display issues @moomiji
- 修复资源更新CI被强制取消的问题 @MistEO
- 适配繁中新数据 @MistEO
- 修复资源更新出错不报错的问题 @MistEO
- 修复外服version错误 @MistEO
- 修复干员识别replace不生效的问题 @MistEO
- update InfrastFilterMenuNotStationedButton for EN (#4827) @Enochen
   - update tasks.json for EN @Enochen
- 控制台退出模拟器统一添加等待时间 #4808 @ABA2396
- 每次修改客户端类型时主动重新读取资源 @MistEO

### 其他

- Auto Update Game Resources - 2023-05-22 @MistEO
- Auto Update Game Resources - 2023-05-21 @MistEO
- 对蓝叠模拟器Hyper-V的补充说明(#4806) (#4866) @SherkeyXD
   - 对蓝叠模拟器Hyper-V的补充说明(#4806) @SherkeyXD
- 修改抽卡重复提示条件 @ABA2396
- 每次启动都会显示抽卡提示 @ABA2396
- fix s3 synchronization path @GalvinGao
- 博朗台模式界面 @ABA2396
- 博朗台模式界面 @ABA2396
- 删除无用的判断 @ABA2396
- 添加一些adbSwipeXDistanceMultiplier的注释 @MistEO
- Auto Update Game Resources - 2023-05-20 @MistEO
- 蓝叠hyperv跳转链接 @MistEO
- ui相关日志移至gui.json @ABA2396
