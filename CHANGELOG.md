## v4.18.0

### 新增

- 支持 `尖灭测试作战` 抄作业 @MistEO @yuanyan3060 @lza11111

### 改进

- 自动战斗编队列表识别不到时，自动划回来重试 @MistEO
- 肉鸽中不再进阶未精二的干员 (#4825) @whatss7
- 刷理智若检测到OF-1自动停止 @MistEO
- 软件更新弃用 github api，并全面启用 maa version api @MistEO
- 博朗台模式同时支持等理智药 @MistEO @ABA2396
- 为窗口标题添加活动名 @MistEO

### 修复

- 延长战斗中暂停的后延迟 @MistEO
- 全局减小 adb 滑动距离 @MistEO
- 修复干员识别偶尔滑动失效提前结束的问题；并少存一点日志图 @MistEO
- 调整鸿雪肉鸽默认技能为1技能 @MistEO
- 修复连接失败时任务仍被添加的问题 @MistEO
- GUI display issues @moomiji
- 修复干员识别replace不生效的问题 @MistEO
- 控制台退出模拟器统一添加等待时间 #4808 @ABA2396
- 每次修改客户端类型时主动重新读取资源 @MistEO

### 其他

- 更新海外客户端数据源，移除对 ArknightsGameData 的引用 @MistEO
- 对蓝叠模拟器Hyper-V的补充说明(#4806) (#4866) @SherkeyXD
- 修改抽卡重复提示条件，每次启动都会显示抽卡提示 @ABA2396
- fix s3 synchronization path @GalvinGao
- ui 相关日志移至 gui.json @ABA2396
- 更新文档 @MistEO
- fix typo: Github -> GitHub (#4883) @eltociear

### For overseas

#### Common

- Fix title version error @MistEO

#### YostarJP

- update JP templates for support of recent features @liuyifan-eric

#### YostarEN

- update InfrastFilterMenuNotStationedButton for EN (#4827) @Enochen

#### YostarKR

- Makes Mizuki theme roguelike available for KR (initially) and updates localization (#4894) @178619
- Update resources for KR (#4868) @178619

