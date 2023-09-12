## v4.24.0-beta.3

### 新增

- 基建 制造站、贸易站、控制中枢 新增支持干员编组配置 (#6249) @status102
   - 修改注释 @status102
   - 增加 控制中枢 支持干员编组配置，以及编组选择输出 @status102
   - 基建制造站和贸易站新增支持干员编组配置 @status102
- 新增开始唤醒时账号切换功能 (官服、B服 only) (#6268) @status102
   - 新增 官服、B服 账号切换功能 @status102
- 自动战斗-自动编队，新增自动补充低信赖干员、追加自定干员选项 (#6282) @status102
   - 调整自动战斗-追加自定干员文本框长度 @status102
   - 自动战斗-自动编队 增加自动追加自定干员设置 @status102
   - 调整自动编队与补充信赖选项的间距 @status102
   - 在未激活自动编队时，将补充低信赖干员设置为不可见 @status102
   - 新增自动编队时，自动补充低信赖干员选项 @status102
- 基建支持加工站 @MistEO
- 增加未通过企鹅检查的掉落截图，增加 UnknownStage 时的难度回调 (#6308) @zzyyyl
   - 增加未通过企鹅检查的掉落截图，增加 UnknownStage 时的难度回调 @zzyyyl
- YostarEN Reclamation Algorithm implementation (#6267) @ABA2396

### 改进

- 优化自动战斗-自动编队-追加自定干员选项UI，当选项禁用时，居中CheckBox (#6322) @status102
   - 优化自动战斗-自动编队-追加自定干员选项UI，当选项禁用时，居中CheckBox @status102
- 优化萨米肉鸽策略 (#6321) @Lancarus
   - 修改先行一步策略，不拿路网 @Lancarus
   - 优化萨米肉鸽策略 @Lancarus

### 修复

- 修复404的链接 (#6317) @HisAtri
- 修复因部分皮肤造成的关卡难度识别问题 (#6309) @zzyyyl
   - 修复因部分皮肤造成的关卡难度识别问题 @zzyyyl
- new EN Roguelike@ChooseOperConfirm.png @Constrat

### 其他

- i18n: Translations update from MAA Weblate (#6323) @weblate
   - Translated using Weblate (Chinese (Traditional)) @weblate
- fix error (#6300) @Rbqwow
   - resolve conflicts @Rbqwow
   - fix error @Rbqwow
- Update 1.1-详细介绍.md @Black1312
- Update 3.6-基建排班协议.md @Black1312
- Update MaaUrls.cs @Black1312
- 把多镜像下载器应用于Python接口 (#6247) @HisAtri
