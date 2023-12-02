## v4.28.0-beta.1

### 新增 | New

- 新增检测最快截图方式后输出截图方法和截图测试耗时 (#7441) @status102 @ABA2396
- 完成后选项 模拟器返回桌面 (#7185) @Manicsteiner
- YoStarJP 狂人号 navigation (#7417) @Manicsteiner
- 保全支持开局替换部分满足条件的非核心干员 (#7347) @zzyyyl @status102

### 改进 | Improved

- 自动战斗部分操作增加时间输出并输出至gui.log (#7453) @status102

### 修复 | Fix

- fix crashing after minitouch fails to restart @horror-proton
- 修复重岳等干员带三技能时点错的问题 (#7450) @zzyyyl
- fix unable to stop in ReportDataTask @horror-proton
- pointer assignment in StageDropsTaskPlugin @horror-proton
- 修复肉鸽编队时选择干员重复点击导致未选中的问题 (#7393) @zzyyyl
- 修复 #7410 导致的 Release 下模板图片加载失败的问题 @zzyyyl
- 修复剿灭退出时卡在情报汇总的问题 (#7409) @zzyyyl
- YostarJP regex for alter and common error operators fix #7402 @Constrat
- 修复傀影肉鸽结算识别可能导致崩溃的错误 @status102
- 修复使用临期理智药设置无法在任务中修改的错误 (#7383) @status102
- 信用商店购物时如有干员合同无法购物 @ABA2396
- 繁中服“退出明日方舟”无效的问题 @AnnAngela

### 其他 | Other

- 更新 Qodana 版本到 2023.3 EAP (#7424) @SherkeyXD
- 优化更新日志生成逻辑 (#7374) @AnnAngela
- 资源更新时发起资源同步 @AnnAngela
- 重构BattleHelper的m_cur_deployment_opers为vector (#7412) @status102
- 添加-i -o @AnnAngela
- 对理智药识别结果排序 @status102
- 修正命名大小写 (#7384) @status102
- 重构 TaskData (#7426) @zzyyyl
- 对PR提交的commit名进行检查 (#7414) @status102
- line overwrite bypass in res-update (#7418) + SN GUI changes @Constrat
- 补全战斗流程协议 actions 的 skill_times 参数 (#7413) @hmydgz
- 修正 gh_token @AnnAngela
- 迁移剩下的肉鸽数据，移除Status相关逻辑 (#7331) @status102
- YostarEN Stultifera Navis navigation @Constrat
- Update CHANGELOG.md @ABA2396
