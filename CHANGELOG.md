## v4.25.0-beta.2

### 新增

- 输出“使用 48 小时内过期的理智药”消息 (#6800) @broken-paint
   - 更新回调消息协议 @broken-paint
   - 修复KR回调消息协议格式 @broken-paint
   - 增加使用将要过期理智药消息 @broken-paint
- operbox operator names localization (#6726) @Constrat
- 使用MdXaml来提升Markdown渲染表现 (#6723) @SherkeyXD
   - 删除无用的包 @SherkeyXD
   - 优化公告和CHANGELOG的视觉效果 @SherkeyXD
   - CHANGELOG也使用MdXaml渲染 @SherkeyXD
   - 移除无用的using @SherkeyXD
   - 优化公告的渲染表现 @SherkeyXD
- 为MaaWpfGui加入一个远程控制功能。 (#6717) @hsyhhssyy @SherkeyXD
   - 修复了一处汇报不了任务的bug @hsyhhssyy
   - 修复了检测网络的相关问题 @hsyhhssyy
   - Merge branch 'dev' of https://github.com/hsyhhssyy/MaaAssistantArknights into dev @SherkeyXD
      - 添加远程控制部分的文档 @SherkeyXD
   - 将所有字符串l18n内容移动到一起，补全所有l18n并据其调整了一点界面格式. @hsyhhssyy
   - 删除意外添加的没用的内容 @hsyhhssyy
   - 删掉意外添加的没用的内容 @hsyhhssyy
   - 修复一处提示文本格式化错误。 @hsyhhssyy
   - 增加了对Https的校验 @hsyhhssyy
   - Merge branch 'MaaAssistantArknights-dev' into dev @hsyhhssyy
      - ResolveConflict @hsyhhssyy
   - 远程控制功能 @hsyhhssyy
- 抄作业自动跳过战斗中剧情 (#6721) @MistEO @Lancarus @status102
   - Apply suggestions from code review @MistEO
   - 按照review修改 @Lancarus
   - 优化剧情跳过检测频率，修改暂停时返回值为true @status102
   - 按照review修改 @Lancarus
   - 根据review将`check_skip_plot_button`移动到BattleProcess里面 @Lancarus
   - 战斗列表ocr替换 @Lancarus
   - 使用导航名判断是否为教学关 @Lancarus
   - 在战斗列表中将教学关取消自动编队 @Lancarus
   - 格式化代码 @Lancarus
   - 抄作业自动跳过战斗中剧情 @Lancarus
- 新增关卡导航在未找到设定关卡时，移动至最右侧、自动通关未通过的剧情关，再尝试查找关卡 (#6729) @status102
   - 调整剧情关按钮截图 @status102
   - 新增 自动战斗-战斗列表 在未找到设定关卡时，移动至最右侧并自动通关未通过的剧情关 @status102
- 公招无招聘许可时继续尝试刷新 (#6569) @broken-paint
   - 更新回调消息协议 @broken-paint
   - pref: 根据 qodana 优化代码 @broken-paint
   - 增加回调消息，增加多语言界面适配 @broken-paint
   - 优化任务逻辑 @broken-paint
   - 在GUI中加入选项 @broken-paint
   - 公招用完招募许可后继续刷新 @broken-paint

### 改进

- 优化代码，消除警告 @ABA2396
- 优化资源动态更新 @MistEO
- WpfGui关卡掉落输出增加千分位分隔符 (#6793) @status102
   - WpfGui关卡掉落输出增加千分位分隔符 @status102
- OTA时先更新动态文件，再更新其他的 @MistEO
- 优化 TaskData 的 json 解析 (#6739) @zzyyyl
   - 来点 `std::invoke_result_t` @zzyyyl
   - macOS compile error @zzyyyl
   - 优化 TaskData 的 json 解析 @zzyyyl
- 优化StartUp速度，首页展开理智窗口时StartUp会关上 (#6730) @status102
   - 减小延迟 @status102
   - 优化task @status102
   - 增加了一个二次检测进行保底 @status102
   - 优化StartUp速度，首页展开理智页时StartUp会关上 @status102
- 使用多模板匹配重构 ReturnButtons (#6741) @zzyyyl
   - 使用多模板匹配重构 ReturnButtons @zzyyyl
- 对DEBUG VERSION禁用版本更新及资源更新检查 (#6751) @status102
   - 对DEBUG VERSION禁用版本更新及资源更新检查 @status102

### 修复

- BlackReturn -> ReturnButton [skip ci] (#6806) @zzyyyl
- 修复在无法导航至指定关卡并通过初见剧情关后，无法进入指定关卡的错误 (#6792) @status102
   - 修复在无法导航至指定关卡并通过初见剧情关后，无法进入指定关卡的错误 @status102
- 修复当"远程控制"没有填入任何内容时的界面问题 (#6787) @hsyhhssyy
- trying to fix fight with support ref: 1f9f48128fc020ac191e427ff7d53dd8036a6f73 [skip ci] @Constrat
- 修复作战列表跳过剧情关后不重新寻找关卡 (#6772) @Lancarus
   - 修复作战列表跳过剧情关后不重新寻找关卡 @Lancarus
- 修复模板尺寸不同时的崩溃问题 @MistEO
- update UnlockClues for EN (#6749) @Enochen
   - lower UnlockClues ocr threshold for EN @Enochen

### 其他

- strategies -> strategy @Constrat
- tools: more adaptation ignored templates @Constrat
- Revert EN's Version.json @Constrat
- schema for templThreshold array or number @Constrat
- Fix #6752 & Fix #6776 (#6783) @Cryolitia
   - Fix #6752 & Fix #6776 caused by https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/42e9542cd1914193f49afdd6435c2b1a8b6eef22#diff-8f68e6988b089efca72b7954862a47442ec7eb08b37bde5a0c0f072cf4488dd3L9 @Cryolitia
- Revert "feat: 使用MdXaml来提升Markdown渲染表现" (#6770) @MistEO
   - Revert "feat: 使用MdXaml来提升Markdown渲染表现" @MistEO
- Auto Update Game Resources - 2023-10-08 @MistEO
- added KR operators up to Kirin R Yato @zewoosJ
