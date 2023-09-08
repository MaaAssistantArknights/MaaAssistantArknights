## v4.24.0-beta.1

### 新增

- 新增复刻活动时，连续刷普通1-9关 (异形UI暂不支持，例如愚人号)，支持上报至企鹅物流 (#6230) @status102
- 增加刷理智每次打关卡之前时的理智剩余量输出 (#6146) @status102
- 支持 SideStory「不义之财」活动导航 & 添加 "SideStoryStage" 模板 roi @ABA2396
- shutdown/hibernate after all other MAAs have exited, or just exit itself (#6147) @Neptsun
- 添加CLI支持 (#6144) @wangl-cc
- 新增肉鸽干员自动撤退字段 (#6241) @AnnoyingFlowers
- 注册表查询bluestacks hyper-v的conf位置 (#6083) @Gliese129
- 支持woolang绑定 (#6142) @mrcino

### 改进

- 优化萨米肉鸽策略 (#6266) @Lancarus
- 优化刷源石锭模式的逻辑，跳过战斗后招募，增加进入纵向节点的逻辑 (#6140) @Lancarus

### 修复

- 修复萨米肉鸽关卡json“生人勿进”->"生人勿近" (#6262) @Lancarus
- 尝试修复网络波动造成的碎石不用问题 (#6220) @zzyyyl
- 修复萨米肉鸽作战关卡逻辑错误 (#6235) @mole828
- 萨米商店不识别第二排商品 (#6232) @Lancarus
- 修复肉鸽道中抓取干员 山 或 林 后重复开启技能的问题 (#6221) @AnnoyingFlowers
- regex for trader shopping in I.S. @Constrat
- 萨米boss关正则替换错误 (#6211) @Lancarus
- 麒麟X夜刀->麒麟R夜刀 (#6195) (#6199) @Constrat @SherkeyXD
- fix failing to choose rewards from the last run @178619
- 修复仓库识别无法在表格框内滚动 @ABA2396

### 其他

- A Flurry to the Flame event navigation @Constrat
- corrected a44dc12 time and event naming @Constrat
- 修正格式 @ABA2396
- 对CLI文档的侧边栏支持 (#6199) @SherkeyXD
- 尽可能稳定的打更多层数 -> 尽可能稳定地打更多层数 (#6200) @govizlora
- Untrastrated fix @wallsman
- 跟进蓝叠Hyper-V的连接指导 (#6083) (#6137) @SherkeyXD
- 取消连接时加载资源 @ABA2396
- 加点加载资源和连接模拟器的日志 @ABA2396
- i18n: Translations update from MAA Weblate (#6151) @weblate
- add log @AnnAngela

### For overseas

#### YostarKR

- update resources for YoStarKR @178619
- Update localization for YoStarKR @178619
- Update ocrReplaces for YoStarKR @178619

#### YostarEN

- updated templates for EN (#6094) @Constrat

#### txwy

- 繁中文件大更新 (#6226) @momomochi987
- 對照陸服，整理與補充繁中服的 template 跟 tasks  (#6196) @momomochi987

#### YostarJP

- (JP) Fix autoupdate change typo @wallsman
- （JP）紅炎遣らう落葉 Update @wallsman
- [JP] readme Update test / Japanese font readability (#6228) @wallsman
