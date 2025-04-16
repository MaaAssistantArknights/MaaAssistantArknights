## v5.15.2

### Highlight

本次更新我们带来了保全派驻新作业与自定义背景图片填充模式，修复若干已知问题，让牛牛跑得更快更稳~

#### 保全派驻作业更新

新增以下场景的保全派驻作业支持：
- 阿卡胡拉丛林（萨拉托加）
- 尚蜀夜市/多索雷斯在建地块/荒地群兽音乐厅（DL_君逸寒）

#### 背景设置自定义升级

「背景设置」新增自定义填充模式，可以放心选择喜欢的图片啦  
（设置路径依然在「设置」→「背景设置」，搭配透明度食用更佳哦~）

#### 问题修复专项

- 修复导能元件选择异常
- 解决线索窗口无法关闭的 BUG
- 优化 OF-1 导航逻辑，适配新 UI
- 修复理智勾选「指定材料」刷取「不选择」时 UI 崩溃 → [详情](https://www.bilibili.com/opus/1055580503270227987)

----

以下是详细内容：

### 新增 | New

* 新增 阿卡胡拉丛林 保全派驻作业 (#12375) @Saratoga-Official
* 新增 尚蜀夜市、多索雷斯在建地块、荒地群兽音乐厅 保全派驻作业 (#12370, #12374) @junyihan233
* Background stretch modes (#12365) @BxFS @ABA2396
* 在肉鸽招募中增加m3作为群奶 (#12353) @Cerulime

### 改进 | Improved

* 基建干员选择滑动优化 @status102
* 加快无基建技能干员选择速度，加快选择列表回正速度 (#12363) @ABA2396
* 优化 OF-1 截图 @ABA2396

### 修复 | Fix

* 自动战斗卡子弹时间，锁干员导致cache混乱 (#12369) @Alan-Charred
* EN SSS buffs regex @Constrat
* 导能元件界面更改 @ABA2396
* 刷理智掉落物指定参数异常检查 @status102
* 无法关闭线索窗口 @ABA2396

### 文档 | Docs

* 优化文档 (#12361) @SherkeyXD @Rbqwow
* add glossary missing operator name (#12360) @rosmontisu

### 其他 | Other

* 更新Mirror酱下载为按钮，保持动画一致 @ChingCdesu
* 移除battleHelper中的BestMatcher复用 (#12136) @status102
* 优化界面显示 @ABA2396
* tile resource (#12372) @MistEO @github-actions[bot]
* KR translation tweaks (#12366) @HX3N
* 自动战斗加点输出 @status102
