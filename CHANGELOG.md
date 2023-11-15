## v4.27.0-beta.1

### 新增

- 肉鸽刷等级模式新增结算输出并保存截图至achievement文件夹 (#7276) @status102
- 关卡掉落汇报至一图流 (#7235) @ChingCdesu @MistEO @ABA2396
- 自动战斗-战斗列表新增无限吃理智药选项 (#7240) @status102
- txwy resource updater (#7259) @Constrat @AnnAngela
- 支持识别长于一页的掉落列表 (#7202) @horror-proton
- updated txwy resources with #7259 @Constrat
- 游戏资源热更进度显示 #6666 @ABA2396
- 添加游戏资源更新失败提示 @ABA2396
- 外部通知设置显示移至UI判断 (#7244) @SherkeyXD
- 支持运行任务时阻止休眠 (#7218) @SherkeyXD
- YoStarJP 孤星 sidestory navigation (#7242) @Manicsteiner
- adb路径填写错误时弹窗提醒 @ABA2396
- adapted MiningActivityDigging YostarEN @Constrat
- YostarEN Sidestory Lone Trail navigation @Constrat
- 战斗列表输入关卡名的文本框新增提示文本 (#7239) @status102
- Furniture Drop localization (#7203) @Constrat @ABA2396
- GetImageFromRoi小工具支持直接上mask (#7223) @SherkeyXD

### 改进

- 移除未使用的变量 (#7281) @status102
- 优化理智识别模块，取消预处理，取消内部重试 (#7280) @status102
- 修改游戏资源更新后的显示内容 @ABA2396
- 优化萨米肉鸽部分关卡逻辑 (#7255) @Lancarus
- 重构肉鸽数据存储，迁移肉鸽主题 (#7138) @status102
- 详细介绍：抄作业:优化 + 刷理智:自动重复代理相关 (#7180) @Yanstory
- 统一 views::reverse @zzyyyl
- 肉鸽适配新干员默认技能 (#7181) @Lancarus
- 使用 `std::erase` 取代 erase-remove 惯用法 (#7179) @zzyyyl
- 优化账号切换日志输出 @zzyyyl
- 自动在刷理智第一次执行时调整连战次数为1 (#7210) @status102

### 修复

- 修复肉鸽主题迁移导致的错误 (#7308) @status102
- YoStarJP 孤星 sidestory navigation update (#7305) @Manicsteiner
- 修复 刷理智-指定材料 选项取消无效的问题 (#7303) @zzyyyl
- 修复肉鸽修改参数后不生效的问题 (#7289) @zzyyyl
- vcxproj & filters @zzyyyl
- 修复外服未实装连续战斗导致多次尝试 (#7294) @status102
- 修复在部分情况下，自动战斗-自动编队时干员页面最边缘干员名字未露出，导致编队时表现错误 (#7253) @status102
- update threshold for StageDropsImageAnalyzer @horror-proton
- 修复部署时补偿距离过大，导致部署朝向滑动起始点超出范围 (#7284) @status102
- 修复错误的下载提示，在某个文件下载错误后直接返回 @ABA2396
- 修复因为触底反弹选中的错误的干员 @ABA2396
- 修复滑动次数过少导致加工站放入错误的干员 @ABA2396
- TXWY resource updater FINAL  fix @Constrat
- 移动cpp文件的位置 (#7269) @status102
- resource updater clone token @Constrat
- 尝试修复部署时滑动超出范围 (#7215) @status102
- Exit Arknights of YoStar servers (#7243) @Manicsteiner
- WordOcr for navigation @Constrat
- 修复吃理智药在网络波动时可能产生重复使用的错误 (#7190) @status102 @zzyyyl
- 153_3换加工站异常 @ABA2396
- Missing Text OCR when Limited + Expire monthly (#7229) @Constrat
- 统一 views::range @status102
- 修复移除超额理智药后，如果没有使用任何药品会导致反复识别的错误 (#7195) @status102
- 设置为管理员权限启动的 MAA 无法开机自启 @ABA2396
- 移除游戏资源更新中提示信息的`请勿关闭MAA`说明 (#7191) @status102
- 修复干员识别没有收起职业栏导致最后一列干员可能识别错误的问题 @zzyyyl
- update recruit m_slot_fail @horror-proton
- minimum 0 for Recruit Time, Invest and Roguelike Start times @Constrat
- 修复地图资源特殊文件名下载失败 @ABA2396

### 其他

- Update SECURITY.md @AnnAngela
- Create SECURITY.md @AnnAngela
- 迁移肉鸽主题，迁移肉鸽主题和模式检查 (#7304) @status102
- 修改adb pair部分 (#7286) @Rbqwow
- revert: 回调部署干员朝向滑动距离为400，回调滑动duration为100，增加补偿距离上限 (#7277) @status102
- 基建少滑几次 @ABA2396
- Resource Update - TXWY data fix @Constrat
- Update main.cpp @Constrat
- Auto Update Game Resources - 2023-11-10 @Constrat
- 迁移肉鸽部分数据 (#7184) @status102
- format devcontainer.json @horror-proton
- add GitHub codespace to dev instructions @horror-proton
- add devcontainer.json for GitHub codespace @horror-proton
- 补充使用BattleUseOper->preDelay的注释 @status102
- Auto Update Game Resources - 2023-11-08 @MistEO
- Auto Update Game Resources - 2023-11-07 @MistEO
- Update JP/LONETRAIL 孤星 (#7236) @wallsman
- removed unused templates @Constrat
- name @Constrat
- 调整GetImageFromROI对未声明`template`任务截图时，直接使用任务名命名输出图片 (#7207) @status102
- Update sync-resource.yml @Cryolitia
- Add Sync-Resource-CI (#7186) @Cryolitia
- 修改 153 基建排班表 @ABA2396
