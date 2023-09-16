## v4.24.0-beta.4

### 新增

- 自动战斗新增战斗列表选项（异形UI不支持） (#6350) @status102
   - 添加战斗列表相关说明 @status102
   - 自动战斗-战斗队列 新增 未三星结束时中止流程 @status102
   - 调整自动战斗UI @status102
   - 自动战斗-战斗列表清空按钮增加右键清除 未激活 / 已完成 项目 @status102
   - 修复在重新启动MAA后使用本地保存的战斗列表无法直接使用自动战斗 @status102
   - 自动战斗新增战斗列表选项 @status102
- WPF自动战斗对于作业描述的视频自动跳转增加av号支持 (#6363) @status102
   - WPF自动战斗对于作业描述的视频自动跳转增加av号支持 @status102
- 添加日服生息演算翻译 | Reclamation Algorithm JP (#6340) @THSLP13

### 改进

- 适配萨米肉鸽生活至上分队，优化部分逻辑 (#6385) @Constrat @Lancarus
   - ignore template @Constrat
   - 适配萨米肉鸽生活至上分队 @Lancarus
   - 优化萨米肉鸽策略 @Lancarus
- 基建换班-干员编组增加可用干员缓存，同时在单次换班任务中避免重复匹配已匹配的干员 (#6376) @status102
   - Apply suggestions from code review @status102
   - 加点注释 @status102
   - 基建换班-干员编组增加可用干员缓存，同时在单次换班任务中避免重复匹配已匹配的干员 @status102
- 再改改优化资源下载的写法 @MistEO
- 优化资源下载的写法 @MistEO
- 下载更新资源先使用临时文件缓存 @MistEO
- 优化定时任务 @ABA2396
- 更新游戏资源时弹窗提示 @MistEO

### 修复

- 将基建换班编组匹配的可用干员缓存移动至Status，以免多实例时发生冲突 (#6391) @status102
   - 将基建换班编组匹配的可用干员缓存移动至Status，以免多实例时发生冲突 @status102
- 修复task.png阈值 @MistEO
- 修复SettingsView的Idle不生效，将开始唤醒-账号切换在非Idle情况下锁定 (#6364) @status102
   - 开始唤醒-账号切换在WpfGui增加Trim @status102
   - 修复SettingsView的Idle不生效，将自动唤醒-账号切换在非Idle情况下锁定 @status102
- Gadget long name reduction for EN @Constrat
- etag 解析报错 @ABA2396
- 将账号切换输出文本中的目标账号由输入文本改为OCR结果，尝试修复账号切换出错 (#6353) @status102
   - 将账号切换输出文本中的目标账号由输入文本改为OCR结果，尝试修复账号切换出错 @status102
- 给关卡选择为 null 时打个补丁 @moomiji

### 其他

- [skip ci] i18n: Translations update from MAA Weblate (#6348) @weblate
   - Translated using Weblate (Korean) @weblate
- revert: EN version.json [skip ci] @Constrat
- Revert "chore: 兼容旧配置" @moomiji
- Auto Update Game Resources - 2023-09-13 @MistEO
- YostarEN recommended resolution (#6303) @Constrat
- [skip ci] i18n: Translations update from MAA Weblate (#6334) @weblate
   - Translated using Weblate (Chinese (Traditional)) @weblate
- EN infrast workshop and i18n @Constrat
- tools: ignore templates @Constrat
