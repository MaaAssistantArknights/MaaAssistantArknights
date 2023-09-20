## v4.24.0-beta.4

### 新增

- 自动战斗新增战斗列表选项（异形UI不支持） (#6350) @status102
- WPF自动战斗对于作业描述的视频自动跳转增加av号支持 (#6363) @status102

### 改进

- 适配萨米肉鸽生活至上分队，优化部分逻辑 (#6385) @Lancarus
- 基建换班-干员编组增加可用干员缓存，同时在单次换班任务中避免重复匹配已匹配的干员 (#6376) @status102
- 下载更新资源先使用临时文件缓存 @MistEO
- 优化定时任务 @ABA2396
- 更新游戏资源时弹窗提示 @MistEO

### 修复

- 修复task.png阈值 @MistEO
- 修复SettingsView的Idle不生效，将开始唤醒-账号切换在非Idle情况下锁定 (#6364) @status102
- etag 解析报错 @ABA2396
- 将账号切换输出文本中的目标账号由输入文本改为OCR结果，尝试修复账号切换出错 (#6353) @status102
- 给关卡选择为 null 时打个补丁 @moomiji

### 其他

- i18n: Translations update from MAA Weblate @LiamSho
- Revert "chore: 兼容旧配置" @moomiji
- EN infrast workshop and i18n @Constrat
- tools: ignore templates @Constrat

#### For overseas

#### YostarJP

- 添加日服生息演算翻译 | Reclamation Algorithm JP (#6340) @THSLP13

#### YostarEN

- Gadget long name reduction for EN @Constrat
- docs: YostarEN recommended resolution (#6303) @Constrat
