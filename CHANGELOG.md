## v4.25.0-beta.1

### 新增

- 支持夜间界面主体（支持多模板匹配，任一达到阈值即视为命中） (#6608) @MistEO @Constrat @zzyyyl
- 肉鸽设置-中止行为 可设置等待至本次战斗结束后 (#6591) @status102
- 肉鸽刷源石锭增加不去第二层的选项 (#6578) (#6593) @Lancarus
- 支持运行期修改部分任务设置 (#6583) @zzyyyl
- 抄作业新增av号、bv号对分p的支持 (#6670) @AnnoyingFlowers
- 公告弹窗 (#6557) @Constrat @ABA2396

### 改进

- 在UI中隐藏不可用的选项 (#6536) @SherkeyXD @ABA2396
- 优化部分肉鸽关卡策略 (#6508) (#6679) (#6656) (#6590) @Lancarus @zzyyyl
- WpfGui自动战斗解析作业后，提示文本移动至最顶端 (#6625) @status102
- BattleQuickFormationRole cleared templates [skip ci] @Constrat
- 优化肉鸽失败后，开始下一次探索的速度 (#6602) @Lancarus
- 优化肉鸽招募策略 (#6579) @Lancarus

### 修复

- 修复无法对后续加载的图片资源进行存在检测的错误，跳过宿舍技能检查 (#6686) @MistEO @status102
- OCR manipulation params for QuickFormation (#6697) @Constrat
- fix host executable suffix error @aa889788
- 泰拉大陆调查团 EN regex [skip ci] @Constrat
- 修复肉鸽获取 Sami@Roguelike@StrategyChange 等任务时的闪退问题 @zzyyyl
- Sync with OS not working  (#6672) @Cryolitia
- Combat with Support templ error fix lazy load template error Fight with Support #6584 [skip ci] @Constrat
- 修复错误条件下UI显示"投资后进入第二层"按钮的问题 (#6676) @SherkeyXD
- 修复抄作业网络连接失败的问题 @zzyyyl
- 修复基建换班后刷理智失败的问题 @zzyyyl
- 修复模拟器连接错误的问题 (#6661) @SherkeyXD
- fix crash caused by `Matcher::set_task_info` @horror-proton
- 尝试修复掉落识别闪退的问题 @zzyyyl
- 修复肉鸽自动撤退字段引发闪退的问题 (#6650) @AnnoyingFlowers
- templThreshold @zzyyyl
- smoking test @zzyyyl
- 修复模板匹配阈值解析错误的问题 @zzyyyl
- 尝试修复基建的模板匹配错误 @MistEO
- disabled cache for multi template recognition ref: https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/6608#issuecomment-1742462512 @Constrat
- 修复 `templ size is too large` 时闪退的问题 @zzyyyl
- typo in StartToWakeUpOCR EN @Constrat
- 修复Costura未正常启用的问题 (#6632) @SherkeyXD
- LoopTimes no longer negative fix #6634 @Constrat
- 修复肉鸽助战招募卡住放弃的问题 (#6630) @zzyyyl
- improve ocr roi in DepotImageAnalyzer @horror-proton
- 修复连接错误 (#6607) @zzyyyl
- 修复任务链结束后操作会在错误情况下被触发的错误 (#6574) @status102
- 修复需要凹直升开局的值判断错误 @Lancarus
- 修复非刷源石锭时遇到商店直接退出的问题 @zzyyyl
- 修复日服傀影肉鸽大盗当头识别错误 (#6592) @MejiroSilence
- 修复自动战斗-战斗列表拖拽导致崩溃 (#6555) @status102
- 修改肉鸽模式一些ocr参数 (#6580) @AnnoyingFlowers
- 尝试修复#6552导致的死循环 (#6577) @Lancarus
- 修复超出生成任务上限时访问越界的问题，增加生成任务上限至 10000 @zzyyyl
- i18n: 修复日服肉鸽导航问题 (#6559) @MejiroSilence
- 修复资源更新时的崩溃问题 @MistEO
- 修复肉鸽编队时识别不到最下面一个干员名以及不能正确选技能的问题 (#6552) @AnnoyingFlowers
- 修复跳过领取日常奖励的问题 @zzyyyl

### 其他

- 根据 qodana 优化代码 (#6542) @ABA2396
- 重构：WPF 配置部分 (#6549) @Cryolitia
- tasks.json 大修改 (#6563) @zzyyyl
- 肉鸽相关 tasks.json 优化 (#6596) @zzyyyl
- 肉鸽主题从Status分离 (#6646) @status102
- 优化文档视觉效果，添加文档编写指南 (#6611) @SherkeyXD
- 多文件任务从 json 层面合并后再重新解析 (#6478) @zzyyyl @Constrat
- 移除之前弃用的肉鸽模式 "2" @zzyyyl
- 增加接口 get_json(), 运行时可以使用 Task.get_json() 获取任务配置的原生 json 内容 @zzyyyl
- add ability to download specific maadeps (#6700) @aa889788
- debug: 删除一些日志 @zzyyyl
- 移除 qodana 的插件来恢复工作流 (#6675) @hxdnshx
- debug: 增加一项检查和一些日志 @zzyyyl
- hidden "Manual switch" button (#6665) @Constrat
- format @zzyyyl
- `源文件` -> `Source` @zzyyyl
- updated event schedule @Constrat
- added new ignored templates [skip ci] @Constrat
- Auto Update Game Resources - 2023-10-01 @MistEO
- Update 3.5-肉鸽辅助协议.md @Lancarus
- VS22 Solution Explorer name change (#6599) @Constrat
- Update 1.2-常见问题.md @Lancarus
- missing texts EN @Constrat
- missing GA navigation in local @Constrat
- clang-formatter.py 支持忽略指定路径 @zzyyyl
- format (#6601) @zzyyyl
- 修改肉鸽开始行动和商店投资达到上限时退出的实现方式 (#6597) @zzyyyl
- i18n: small correction in en-us @Constrat
- 调整 copilot 小提示内容 @ABA2396
- 补充 Hyper-V 说明 (#6572) @Rbqwow
- 格式化 xaml @ABA2396
- 修改公告页面格式，允许复制，修改弹出逻辑 @ABA2396
- 提取 FetchResponseWithEtag 函数 @ABA2396
- Linux编译教程中增加NUR (#6566) @Cryolitia
- sytle: 简化 using @ABA2396
- 统一 URL 存放位置 @ABA2396
- en SanityReport formatting [skip ci] @Constrat

### For develops

- Add ThriftController (#6573) @aa889788
