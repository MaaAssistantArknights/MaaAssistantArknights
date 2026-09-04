## v6.17.1

----

以下是详细内容：

<details open>
<summary><b>v6.17.1 (2026-09-04)</b></summary>

### 修复 | Fix

* Win32 输入（如 Seize 的 SendInput）为异步注入且无内置节拍，不等待会使整段滑动在毫秒级完成、被游戏判定为点击；在插值步间补充等待，修复此问题 @ABA2396
* 部分早期 SS 关卡 ID 含数字后缀（如 act54side_d01），正则未覆盖导致代码匹配失败；补充 `(d\d+)` 分支以正确识别此类关卡 ID ([#18037](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18037)) @status102
* 补充黑流树海肉鸽楼层 OCR 识别正则，修正出发前往目标区域坐标 ([#18037](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18037)) @ZiyinLin
* 增加特别纪念 tab 后仓库识别报错：调整 DepotAllTab / DepotMaterialTab ROI 配置 ([#18030](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18030)) @ABA2396
* `{key=}` 嵌套替换原先仅支持 `\w+` 字符集，无法覆盖含 `.` 与 `@` 的引用（如 `UserAdditional.Add`、`MiniGame@ALL@xxx`）；扩展为匹配至右花括号 ([#18030](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18030)) @ABA2396
* 修复 MuMu / 雷电模拟器连接及系统通知提示文本中的反斜杠转义显示错误 @ABA2396
* 指定编队选择第二或第三队时，因坐标区域过宽（10px）有低概率误触改名按钮；收窄为 5px @ABA2396

### 新增 | New

* 补齐多项设置项与界面提示，修正歧义命名：含自动肉鸽「烧水使用分队」改为「刷开局使用分队」、ADB 退出行为描述优化、外部通知停滞提示补充、定时启动配置说明完善等 ([#18030](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18030)) @ABA2396

### 文档 | Docs

* 在五语言基建排班协议文档中新增 RIIC.Autos 自动生成基建排班表工具链接 ([#18022](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18022)) @BrK
* 对照代码全面修正五语言文档（集成协议、copilot/sss/schema、maa-cli 手册、开发教程等），更新 UI 名称对齐实际界面文案 ([#18030](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18030)) @ABA2396

</details>

<details>
<summary><b>v6.17.0 (2026-09-02)</b></summary>

### 新增 | New

* 新增适配黑流树海肉鸽，支持 ｢刷等级，快速飞三层｣ ｢刷源石锭，投资完成后自动退出｣ ｢刷襁褓动物｣ 三个策略 ([#17380](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17380)) @DavidWang19 @ZiyinLin @sakevel @status102 @walkerljy @ABA2396
* PC 端连接截图前自动规避光标与窗口遮挡：主界面识别前把光标移到窗口中心并等待视差动画，其他界面识别前移到不影响识别的位置；window-pos 鼠标输入方式下非主界面识别会把窗口移出屏幕并在断开时自动恢复（连接设置新增 ｢窗口恢复｣ 按钮），该输入方式下截图方式限定为 PrintWindow ([#17776](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17776)) @H2O-MERO @status102
* 模拟器分辨率变化时中断当前任务并提示，避免在变更后的分辨率下继续识别；重新开始任务即可应用新分辨率 @ABA2396
* 新增启动文件缺失检查，发现安装文件缺失时提示，并支持从更新源重新下载完整包自动修复 ([#17725](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17725)) @ABA2396
* 页签切换过渡动画与设置页导航平滑滚动：主界面页签、任务链、常规/高级设置等内容切换加入方向性过渡动画，设置页导航平滑滚动并跟随过渡动画档位 ([#17799](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17799)) @ABA2396
* 像素画自动填色新增粘贴功能，支持从剪贴板粘贴图片与 4 字以内的文本 ([#17662](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17662) [#17689](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17689)) @H2O-MERO @ABA2396
* 掉线提示接入外部通知，日志与通知等级提高到 Err，启用外部通知 ｢错误时发送｣ 后推送 @ABA2396
* 自动战斗支持指定职业以区分同名干员，并兼容职业大小写 ([#17544](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17544)) @status102
* 自动战斗使用技能支持超时参数 ([#17734](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17734)) @status102
* 库存保持任务支持因理智不足跳过后续任务 ([#17741](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17741)) @ABA2396
* 战斗开始等待时间支持通过 config.json 的 battleStartTimeoutSeconds 配置（10~300 秒，默认 60 秒）([#17329](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17329)) @wzacolemak @status102
* 关卡小提示中显示同名活动进行中的小游戏入口提示 @ABA2396
* CustomWebhook 预置模板新增企业微信（WeCom）与 ntfy 选项 ([#17695](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17695)) @momomochi987

### 改进 | Improved

* 重写基建效率算法，常规（默认）模式支持跨设施组合，新增基建跨设施组合设置 ([#17835](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17835)) @status102 @Lancarus @ABA2396
* 基建常规模式宿舍换班流程调整：新增菲亚梅塔恢复开关（默认关闭），第一轮宿舍仅执行菲亚梅塔配对，配对失败时不再清空宿舍；低心情干员改于第二轮宿舍补位时安置 ([#17980](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17980)) @ABA2396
* 移除掉线重连逻辑，恢复链状态复杂且维护成本高；通宵挂机请改用定时启动与强制定时启动 ([#17742](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17742)) @ABA2396
* 🎉🎉🎉 PC 端鼠标输入新增完全无法使用的 SendMsg/PostMsg 全后台模式 🎉🎉🎉 @ABA2396
* PC 端 SendMessageWithCursorPos/PostMessageWithCursorPos 鼠标输入方式截图前后自动挪开并还原鼠标位置（截图期间会短暂阻塞鼠标输入）；高频截图任务下会明显卡顿，建议使用 SendMessageWithWindowPos @ABA2396
* 调整 PC 端连接的鼠标输入与截图方式默认值，界面警告补充推荐使用方式与热键设置建议 ([#17839](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17839)) @H2O-MERO
* 基建制造/贸易/发电站入口改用纯色数色识别 ([#17951](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17951)) @ABA2396
* 统一自定义下拉控件的开合交互（点外部关闭、连击保持），｢背景设置｣ ｢神秘代码｣ 的文件选择下拉迁移到新 TreeComboBox 控件，并修复弹层滚动导致设置页跳顶 ([#17813](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17813)) @ABA2396
* 可搜索 ComboBox 启用 UI 虚拟化，减轻肉鸽开局干员等大列表搜索时的卡顿 @ABA2396
* 界面硬编码文本改为资源化，并统一 Core 与 GUI 日志语言为英文 ([#17984](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17984)) @ABA2396
* 多作业模式下导航名改由 Core 自动从地图数据读取，并新增 `nav_name_override` 参数支持手动覆盖导航识别名 ([#17687](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17687)) @status102 @hguandl
* 自动战斗自动编队按干员组最低练度跳过浏览低等级干员，缩短编队耗时 ([#17751](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17751)) @status102
* PC 端游戏窗口标题按客户端类型解析，连接与结束模拟器支持不同语言的 PC 端，并优化 PC 端相关描述文案 ([#17679](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17679)) @HX3N @ABA2396
* 重构连接设置的 Extras 连接配置结构，PC 端（Win32）连接配置独立拆分 @status102
* 重构黑流肉鸽节点路线（routes）读取，配置解析错误时正确报错 ([#17820](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17820)) @status102
* 干员数据增加子职业解析，找不到干员与多稀有度干员跳过重复检查，召唤物职业解析临时兼容 @status102
* 查找干员在职业未知时回退到按名称匹配，避免检索失败 ([#17735](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17735)) @yali-hzy
* 刷理智代理倍率识别改用 RGB 颜色匹配，提升识别稳定性 ([#17719](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17719)) @status102
* 保全作业浏览时不自动添加到作业列表 @status102
* 更新保全作业，新增日达诺夫园区与荒废灯塔保全作业 @Saratoga-Official
* 优化理智作战高级设置界面布局，调整自动战斗缺少干员时的提示与报错描述，明确 ｢特别关注｣ 影响识别时的处理方式 @ABA2396
* 任务因内存不足停止时给出专门提示，建议关闭部分程序或重启 MAA 后重试 @ABA2396
* 调整基建干员冲突提示，检测到干员已进驻其他设施时将自动确认调动，日志不再标红 @ABA2396
* 显卡兼容性提示在每次开始运行时输出，避免被任务日志刷掉后无法看到；调整 core 崩溃后和未知异常的错误提示 @ABA2396
* 为软件更新包下载添加重试 ([#17675](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17675)) @bkzzzz
* 截图耗时 100ms 以上且未启用截图增强时，补充截图优化建议，并优化截图增强报错与设置指引 @ABA2396
* 优化启动设置页提示 @ABA2396
* 打包时用原生启动器替换 MAA.exe 的 apphost，MAA.exe 被单独移动等文件不完整的情况能给出明确提示，不再误报未安装 .NET ([#17727](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17727)) @ABA2396
* 切换客户端后断开现有连接 @ABA2396
* 自动战斗自动编队切换职业时切换回全部职业分类，避免游戏未重置 UI 位置 @status102
* 繁中服调整文件与界面的在地化用词 ([#17856](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17856)) @momomochi987
* 繁中服调整部分干员与关卡名称 OCR 并增加容错 ([#17703](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17703) [#17828](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17828)) @momomochi987
* 繁中服补全并调整界园肉鸽通宝权重，更新界园肉鸽干员管理入口模板 ([#16306](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16306) [#16634](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16634)) @travellerse @abmcar
* YostarEN/JP/KR update MiniGame SPA S2 templates ([#17808](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17808)) @Constrat @Manicsteiner @HX3N
* YostarKR update localization with official terminology, normalize BlackFlow roguelike terminology, and polish task-log strings and clue terminology @HX3N

### 修复 | Fix

* 更新器等待主程序退出增加超时强制结束，主程序意外滞留时更新不再无限等待；主程序启动中止时改为立即退出进程；更新器获取父进程句柄权限不足时不再跳过等待 ([#17930](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17930)) @ABA2396
* 修复作业解析同名干员时可能取错条目的问题，改为取稀有度最高条目，阿米娅技能 3 判断不再受哈希顺序影响，并在技能 3 的支持条件中允许阿米娅 ([#17893](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17893)) @ABA2396 @yali-hzy
* 修复自动战斗与肉鸽作业 `role` 字段的职业解析，未指定或未知职业名不再被误判为无人机职业，并在技能 3 的支持条件中允许阿米娅 ([#17819](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17819)) @yali-hzy
* 修复从公招的选择招募时限界面开始自动公招功能，会触发循环操作的问题 @ABA2396
* 修复可搜索 ComboBox 的一批问题：语言切换等场景下选项绑定失效、指定材料被清空、初始化时将已绑定值覆盖为列表第一项（肉鸽开局干员重启后被重置）、下拉列表滚轮一次滚到底、换源统一由扩展接管并修复中间值写回产生的重复日志 ([#17759](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17759)) @status102 @ABA2396
* 修复切换界面语言后部分下拉列表与提示停留旧语言的问题（肉鸽刷通关时长目标、黑流培养目标、自动战斗单位支持用法、隐蔽战线事件、肉鸽开局干员提示）@ABA2396
* 修复从 PC 端切回模拟器需要重启后才能连接 @ABA2396
* 修复 NotifyIcon 双击间隔为 0 时的启动崩溃 ([#17691](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17691)) @bkzzzz
* 修复连接配置下拉框打开和滚动时整个页面位移的问题 @ABA2396
* 修复自定义 Webhook 无法通过 Headers 设置 Content-Type 导致通知发送失败的问题，消息体占位符补全 Json 转义 @ABA2396
* 修复 ｢开始任务：｣ 分隔栏在界面最小化时无法正确添加、Rectangle 无法显示的问题 @ABA2396
* 修复小游戏界面开始任务时连接模拟器失败无任何提示 ([#17887](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17887)) @ABA2396
* 修复无配置文件时首次启动更新源显示为空 @ABA2396
* 修复亮色模式下下拉框高亮文本颜色错误 @ABA2396
* 修复像素画英文界面按钮显示不全、切换语言时适配与抖动下拉框内容未实时切换 @ABA2396
* 修复成就 ｢不务正业｣ 达成条件文案未跟随术语显示 ([#17834](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17834)) @H2O-MERO
* 放宽 Miss.Christine 的 OCR 识别容错 @Lancarus
* 繁中服适配新版登录界面 ([#17853](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17853))，调整「辭歲行」OCR 文字 ([#17910](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17910)) @momomochi987
* YostarKR correct CharsNameOcrReplace regex, update SilverAsh ocrReplace, and fix ocrReplace regex for 烛煌 ([#17946](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17946)) @HX3N
* YostarKR loosen SkipThePreBattlePlot match threshold for lighter plot overlays @HX3N
* YostarKR fix Roguelike recruitment giving up instead of recruiting @HX3N
* YostarKR handle startup notification during account switch @HX3N
* YostarKR update Roguelike@TraderRandomShoppingConfirm template @HX3N
* YostarEN fix Eyja alter OCR regex @Constrat
* YostarEN rework Sami IS floor detection to match region names instead of temperature numbers @Constrat

### 文档 | Docs

* 新增 MaaFramework 控制单元自动下载脚本及开发文档说明 ([#17786](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17786)) @Justin-Emils
* 同步更新五语言文档（连接与设备说明、集成协议、copilot 等任务 schema、开发教程与 FAQ）([#17879](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17879)) @ABA2396
* 补充集成协议文档与 schema（nmsDistance 字段、copilot-schema、callback-schema），修正 MirrorChyan 拼写与 config.md 示例 @ABA2396
* 翻译 ja-jp 版 maa-cli 文档并标注机器翻译 @ABA2396
* 添加 Mac GUI 过渡公告 ([#17965](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17965)) @hguandl
* 更新仓库分支名 dev → dev-v2，修复文档站 ｢编辑此页｣ 链接 404 ([#17920](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17920)) @satgo1546
* 补充拖入更新文件的管理员权限说明 ([#17842](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17842)) @youzibigg
* CI 教程发布分支由 master 更新为 master-v2 ([#17823](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17823)) @youzibigg
* 更新手入门文档，简化下载安装步骤并说明日志包生成方式 @ABA2396
* 肉鸽文档补充黑流树海推荐开局 ([#17753](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17753)) @Rbqwow @ABA2396
* 修正多语言文档错别字，更新作业协议 `nav_name_override` 字段说明 ([#17708](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17708)) @apricity093 @hguandl
* 补充会客室取下线索的说明 @ABA2396

### 其他 | Other

* 修复 Unix 平台上构建 MaaWpfGui 报错 ([#17958](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17958)) @satgo1546

### MaaMacGui

#### 新增 | New

* 支持作业集 @hguandl
* 支持奇象巡展像素画 @hguandl

#### 改进 | Improved

* 调整像素画选项文案 @hguandl

</details>
