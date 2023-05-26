## v4.19.0-beta.1

### 新增

- Supported Auto I.S. (roguelike) for YostarKR, YostarJP, YostarEN @178619 @su6aru @liuyifan-eric @Pitiedwzr @RiichiTsumo
- 切换配置功能 @MistEO @ABA2396
- 自定义关卡快捷输入 @ABA2396

### 改进

- 扩大任务设置可选范围 @moomiji
- 添加刷理智任务设置中显示基建计划功能 @moomiji
- improve ConfirmExit() @moomiji
- 添加隐藏关闭按钮功能 @moomiji
- 添加托盘图标强制显示MAA功能 @moomiji
- 增加一个文件被占用的解决方法 @MistEO
- 加回 gui.json 备份机制 @MistEO

### 修复

- 修复干员 Box 识别错误 @MistEO
- 拖动任务顺序后无法保存修改后的顺序 @ABA2396
- 关卡名OCR识别大小写出错导致掉落上传失败 #4867 @ABA2396
- 尝试修复关卡选择变为null的情况 #4871 @ABA2396
- 关卡选择为null时仍会执行借助战任务 @ABA2396
- 修复 `火哨` 的基建技能 @MistEO

### 其他

- 创建本地化文档自动翻译工具 & 更新本地化文档 (#4912) @UniMars @wallsman
- 修复资源更新器不更新水月肉鸽的问题 @MistEO
- update meojson (#4789) @MistEO
- 部分 CI 优化 @MistEO
- Update operators.md (#4929) @wallsman
- 修改对于蓝叠的支持说明 @ABA2396 @MistEO

### For overseas

#### YostarJP

- Supported Auto I.S. (roguelike) (#4936) (#4921) @su6aru @liuyifan-eric
- JP navigation to IS event stages @liuyifan-eric
- update JP json for gacha pull @liuyifan-eric
- IS-10 (Yostar ongoing) slowly swipe to stage @liuyifan-eric
- JP oper ocr replace update @liuyifan-eric
- ad hoc fix for JP ocr of 阿 ア @liuyifan-eric

#### YostarEN

- Supported Auto I.S. (roguelike) (#4936) @Pitiedwzr @RiichiTsumo

#### YostarKR

- Supported Auto I.S. (roguelike) (#4932) @178619
- Localization fix for ko-KR (#4930) @GyeRyak
