## v4.19.2

### 新增

- 新增 LE 活动关卡导航 @ABA2396

### 改进

- 雷神保全浊蒂版&约翰保全浊蒂版修改 修改职业名称为英文，同步作业站版本 (#5172) @junyihan233
- 重训了国服, 日服, 台服和韩服的OCR模型 (#5151) @Plumess
- 更新了简中和英文的ocr det模型, 以及英文的ocr rec模型 (#5156) @Plumess
- 优化退出模拟器，添加日志 @ABA2396
- 随机展示简中文档中模拟器支持列表 (#5161) @bakashigure @AnnAngela
- 优化界面显示 @moomiji @ABA2396
- 关卡提示支持显示多个活动 @ABA2396

### 修复

- 修复特别关注识别错误、部分干员名错误 @MistEO
- 修复抄作业编队超过 10 人时无法选人的问题 @MistEO
- 修复自动基建 制造站 bskill_man_exp4 技能错误 @MistEO
- 扩大roi范围，修复PlayCover无法开始唤醒 #5099 @ABA2396
- 点赞接口序列化两次导致后端报错 @ABA2396

### 其他

- fix filename @horror-proton
- Auto Update Game Resources
- 修正拼写错误 @ABA2396 @kongwei981126 @doquangminh28
- Update en-bug-report.yaml @ABA2396
- 移除不需要的changelog @AnnAngela
- 删除多余mac完整包 @AnnAngela
- Do not force push tag @MistEO
- 集成战略文档更新 (#5186) @LingXii
- 发版检测到docs发生变动时部署 @AnnAngela

### For overseas

#### YostarEN

- translation refactoring of en-us.xaml for YostarEN (#5111) @Constrat @ABA2396
- Difficulty selection for YostarEN (#5200) @Constrat
- Fight Task Adverse Start YostarEN (#5196) @Constrat
- OperBoxNameOCR tasks.json YostarEN (#5169) @Constrat

#### YostarKR

- recruitment failure with DP-Recovery tag for KR (after OCR update) @178619
- Update ocrReplaces for KR @178619
