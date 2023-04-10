## v4.14.0-beta.1

### 新增

- 添加主线第十二章导航 @zzyyyl
- 启动MAA后直接最小化选项 (#4193) @moomiji
### 改进

- 绝大部分资源改为惰性加载，大幅提高启动速度和并优化内存占用 @MistEO
- 新增肉鸽不期而遇插件，优化招募与战斗策略 (#4241) @LingXii
- 更新技能识别模型，优化最上面一排干员技能的识别 @MistEO
- 重构界面部分样式，改用 HandyControls (#4200) @moomiji
- 简化肉鸽编队界面的选中识别 (#4246) @horror-proton
- 企鹅 id 无条件同步到界面 @MistEO
- 规避第十二章三倍掉落被上传的可能性 @MistEO

### 修复

- 修复傀影肉鸽招募错误 (#4241) @LingXii
- 修复战斗中狂点cooling干员的问题 @MistEO
- 修复部分视频识别错误 @MistEO
- 修复iOS识别不到战斗干员 @hguandl @MistEO
- 修复会客室偶现送完线索卡住的问题 @MistEO
- 修复一个罕见的肉鸽崩溃情况 @horror-proton
- 修改主线导航替换文本 #4226 @ABA2396
- 修复MAA主窗口最小化后弹窗位置错误  @MistEO
- 过滤中坚信物，提高仓库识别速度 (#4210) @orzFly @MistEO
- 修复更新镜像爆炸时不尝试其他镜像的问题，删掉几个爆炸的镜像 @MistEO

### 其他

- 删除已经用不上了的component包 @MistEO
- 扩大识别区域 @ABA2396
- 替换三星判断条件 @ABA2396
- 修改 EpisodeOcrReplace 替换内容 @ABA2396
- 添加标注 @ABA2396
- revert tiles data @MistEO

### For Overseas

#### YostarJP

- Fix dead loop in options @MistEO

#### YostarEN

- Fix level navigation #4226 @ABA2396
