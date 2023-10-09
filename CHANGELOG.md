## v4.25.0-beta.3

### 新增

- 添加一个资源更新镜像服务器 @AnnAngela @MistEO
- 添加了淬羽赫墨与多萝西的基建技能联动 (#6820) @xyxyx718

### 修复

- 修复战斗列表中跳过剧情确认识别问题 (#6827) @Lancarus

### 其他

- 调整部分log输出为仅供ASST_DEBUG (#6824) @status102


## v4.25.0-beta.2

### 新增

- 输出“使用 48 小时内过期的理智药”消息 (#6800) @broken-paint
- 为 WpfGui 加入一个远程控制功能。 (#6717) @hsyhhssyy @SherkeyXD

### 改进

- operbox operator names localization (#6726) @Constrat
- 抄作业自动跳过战斗中剧情 (#6721) @MistEO @Lancarus @status102
- 关卡导航在未找到设定关卡时，移动至最右侧、自动通关未通过的剧情关，再尝试查找关卡 (#6729) @status102
- 公招无招聘许可时继续尝试刷新 (#6569) @broken-paint
- 优化资源动态更新 @MistEO
- WpfGui关卡掉落输出增加千分位分隔符 (#6793) @status102
- OTA时先更新动态文件，再更新其他的 @MistEO
- 优化 TaskData 的 json 解析 (#6739) @zzyyyl
- 优化StartUp速度，首页展开理智窗口时StartUp会关上 (#6730) @status102
- 使用多模板匹配重构 ReturnButtons (#6741) @zzyyyl
- 对DEBUG VERSION禁用版本更新及资源更新检查 (#6751) @status102

### 修复

- BlackReturn -> ReturnButton [skip ci] (#6806) @zzyyyl
- 修复在无法导航至指定关卡并通过初见剧情关后，无法进入指定关卡的错误 (#6792) @status102
- 修复当"远程控制"没有填入任何内容时的界面问题 (#6787) @hsyhhssyy
- trying to fix fight with support ref: 1f9f48128fc020ac191e427ff7d53dd8036a6f73 [skip ci] @Constrat
- 修复作战列表跳过剧情关后不重新寻找关卡 (#6772) @Lancarus
- 修复模板尺寸不同时的崩溃问题 @MistEO
- update UnlockClues for EN (#6749) @Enochen

### 其他

- strategies -> strategy @Constrat
- tools: more adaptation ignored templates @Constrat
- Revert EN's Version.json @Constrat
- schema for templThreshold array or number @Constrat
- Fix #6752 & Fix #6776 (#6783) @Cryolitia
- added KR operators up to Kirin R Yato @zewoosJ
