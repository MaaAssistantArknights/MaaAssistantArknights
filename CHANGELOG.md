## v4.28.0

### 新增 | New

- wpf设置-连接设置新增近期截图耗时 (#7473) @status102
- 繁中服生息演算 & 落葉逐火活動導航 (#7559) @momomochi987
- 增加公招多选策略 (#7463) @lpowo @Constrat @horror-proton
- 可以通过右键选择仅运行一次任务 @ABA2396
- YostarEN Stultifera Navis navigation @Constrat
- 新增检测最快截图方式后输出截图方法和截图测试耗时 (#7441) @status102 @ABA2396
- 完成后选项 模拟器返回桌面 (#7185) @Manicsteiner
- YoStarJP 狂人号 navigation (#7417) @Manicsteiner
- 保全支持开局替换部分满足条件的非核心干员 (#7347) @zzyyyl @status102
- 日服支持加工站换班 @ABA2396
- 最快截图耗时超出阈值后，输出额外提示文本 (#7489) @status102
- YoStarJP 狂人号 navigation change (#7488) @Manicsteiner
- SideStory「银心湖列车」活动导航 @ABA2396

### 改进 | Improved

- 优化战斗列表部分逻辑 (#7565) @status102
- 自动战斗部分操作增加时间输出并输出至gui.log (#7453) @status102
- 遇到问题时提前结束选取密文板 (#7501) @ABA2396 @status102
- 优化保全关卡识别逻辑，改为识别LT-n (#7527) @status102
- 肉鸽适配新干员用法 (#7500) @Lancarus

### 修复 | Fix

- 修复在连续战斗次数列表展开时，部分情形下无法正确关闭 (#7563) @status102
- YoStarJP furniture drop (#7467) @Manicsteiner
- fix crashing after minitouch fails to restart @horror-proton
- 修复重岳等干员带三技能时点错的问题 (#7450) @zzyyyl
- fix unable to stop in ReportDataTask @horror-proton
- pointer assignment in StageDropsTaskPlugin @horror-proton
- 修复肉鸽编队时选择干员重复点击导致未选中的问题 (#7393) @zzyyyl
- 对理智药识别结果排序 @status102
- 移除不必要的引用 @status102
- 修复 #7410 导致的 Release 下模板图片加载失败的问题 @zzyyyl
- 修复剿灭退出时卡在情报汇总的问题 (#7409) @zzyyyl
- YostarJP regex for alter and common error operators fix #7402 @Constrat
- 修复傀影肉鸽结算识别可能导致崩溃的错误 @status102
- 修复使用临期理智药设置无法在任务中修改的错误 (#7383) @status102
- 信用商店购物时如有干员合同无法购物 @ABA2396
- 修正命名大小写 (#7384) @status102
- 添加-i -o @AnnAngela
- 繁中服“退出明日方舟”无效的问题 @AnnAngela
- 修复地图资源特殊文件名下载失败 @ABA2396
- 错误提交的测试内容 @ABA2396
- 通过通知栏图标重复退出时报错 @ABA2396
- 修复smoke-testing重复加载外服和国服资源导致潜在的非预期TaskData字段问题 (#7511) @status102 @Constrat
- 修复最快截图用时输出数字未能添加千分位分隔符 @status102
- 修复在截图最快方式检测不成功时，不输出数值 @status102
- corrected ReceiveAward txwy template fix #7389 @Constrat
- 日服识别 斥罪->贝娜 @ABA2396
- 日服识别错误 麒麟->林 @ABA2396
- fix coredump when minitouch failed to restart @horror-proton
- 修复除version.json外无文件更新时不保存version @ABA2396
- sample.py 获取地址从相对路径改为绝对路径 (#7483) @SummerDive
- wrong task `SN-Open` (#7498) @zzyyyl
- typo in task name (#7478) @Constrat
- SN navigation template change @Constrat
- 外服刷理智任务没有源石库存时卡死 @ABA2396
- 多种模拟器多开时关闭错误模拟器 @ABA2396
- 反斜杠处理错误 @ABA2396

### 其他 | Other

- 更新 Qodana 版本到 2023.3 EAP (#7424) @SherkeyXD
- 重构BattleHelper的m_cur_deployment_opers为vector (#7412) @status102
- Revert "feat: 重构 TaskData (#7410)" @zzyyyl
- 资源更新时发起资源同步 @AnnAngela
- resource: 更新新版危机合约地图 (#7399) @status102
- 优化更新日志生成逻辑 (#7374) @AnnAngela
- 移除CheckBox右键额外调用 (#7537) @status102
- 规范化ci文件格式及step命名，优化pr-checker的触发逻辑及表现形式 (#7512) @SherkeyXD
- 重构 SettingsViewModel Init @ABA2396
- 文档目录结构大重构 (#7423) @SherkeyXD @AnnAngela
- reduced screencap timeout (#7476) @Constrat
- 重构 TaskData (#7426) @zzyyyl
- 提取勾选框左右键点击单次执行逻辑 @ABA2396
- 更新日志跳过资源更新 (#7536) @zzyyyl
- 优化pr-checker的评论及日志格式，并自动删除先前的评论 (#7535) @SherkeyXD
- 更新 issue-checker，调整 labels (#7460) @zzyyyl
- 重构 TaskData (#7410) @zzyyyl
- added reclamation algorithm task to view @Constrat
- locked txwy reclamation algorithm. waiting for adaptation (to reduce issues number) @Constrat
- Reclamation Algorithm task for txwy (probably doesn't work as it will need adaptation) fix: #7555 @Constrat
- PR Checker 忽略从 dev 发起合并的拉取请求 (#7462) @zzyyyl
- 添加log @AnnAngela
- PR Checker增加i18n匹配 @status102
- PR Checker在Merge之后不再运行 (#7451) @SherkeyXD
- PR Checker增加edited触发 @status102
- Auto Update Game Resources - 2023-11-30 @Constrat
- 对PR提交的commit名进行检查 (#7414) @status102
- Auto Update Game Resources - 2023-11-29 @Constrat
- line overwrite bypass in res-update (#7418) + SN GUI changes @Constrat
- Manual YostarEN Auto Update Game Resources @Constrat
- Auto Update Game Resources - 2023-11-28 @Constrat
- 补全战斗流程协议 actions 的 skill_times 参数 (#7413) @hmydgz
- 修正 gh_token @AnnAngela
- Auto Update Game Resources - 2023-11-26 @status102
- 迁移剩下的肉鸽数据，移除Status相关逻辑 (#7331) @status102
- Auto Update Game Resources - 2023-11-26 @status102
- c -> C, total downloas @Constrat
- c -> C + total downloads @Constrat
- 移除重复 @AnnAngela
- 发版 pr 自动添加 reviewer @AnnAngela
- pr-checker 在发版时不运行 @zzyyyl
- pr-checker 在发版时不运行 @zzyyyl
- pr-checker 在发版时不运行 @zzyyyl
- revert db4171b684a182021e57426bef819d2b746ee198 SN changes @Constrat
- Auto Update Game Resources - 2023-12-08 @Constrat
- Update ci.yml @ABA2396
- smoke-testing.yml @ABA2396
- [skip ci] i18n: Translations update from MAA Weblate (#7480) @LiamSho @ABA2396
- 修改NoStone图片模板 @ABA2396
- KillEmulator记录地址和端口 @ABA2396
- PR Checker增加对提交父项数量的检查以避免通过改名应对 @status102
- furni忽略拼写检测 @ABA2396

