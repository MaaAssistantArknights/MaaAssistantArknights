## v4.24.0-rc.1

### 新增

- 在非烧水模式，取消刷直升按钮 (#6484) @mole828
   - #fix 隐藏与否弄反了 @mole828
   - 在非烧水模式，取消刷直升按钮 @mole828
- 自动战斗跳过“以下干员出战后将被禁用，是否继续？”对话框 #4917 #4694 #5351 (#6492) @Hydrogina
   - 自动战斗跳过“以下干员出战后将被禁用，是否继续？”对话框 #4917 #4694 #5351 @Hydrogina
- 新增领取邮件功能 (#6438) @broken-paint

### 改进

- optimize time diff format in sanity report (#6512) @KevinT3Hu
   - optimize time diff format in sanity report @KevinT3Hu
- 优化部分 WPF 代码 (#6510) @ABA2396 @zzyyyl
   - 优化部分 WPF 代码 @ABA2396
   - 开机自启失败/多实例自启 @ABA2396
   - 优化部分 WPF 代码 @zzyyyl
- 优化 wpf 代码 (#6504) @zzyyyl
   - 优化部分 WPF 代码 @zzyyyl
   - 优化 WPF 显示理智回满时间的代码 @zzyyyl

### 修复

- 自动战斗-自动编队-补充低信赖干员: 修正补充顺序，避免在极端情况下补充错误 (#6499) @status102
   - 自动战斗-自动编队-补充低信赖干员: 修正补充顺序，避免在极端情况下补充错误 @status102
- 尝试修复肉鸽卡在招募界面 (#6516) @Lancarus
   - lager the roi of task "Roguelike@RecruitWithoutButton" @Lancarus
   - 尝试修复肉鸽卡在招募界面 @Lancarus
- 自动战斗-战斗列表，补充突袭结算图，修复从缓存文件夹添加作业时报错 (#6497) @status102
   - 自动战斗-战斗列表 从缓存文件夹里添加作业时，无法拷贝导致报错 @status102
   - 补充突袭结算图 @status102
- 修复凹直升功能造成的卡在招募立绘等bug (#6502) @Lancarus
   - 优化凹直升按钮的ui逻辑 @Lancarus
   - 修复凹直升模式造成的bug @Lancarus
   - 修复凹直升模式造成的bug feat：优化凹直升的ui显示 @Lancarus
- 删掉多余的模板图片 @zzyyyl
- 🐞 fix(Java-HTTP): 修复Java-HTTP api的编译问题 (#6490) @gylove1994
   - 🐞 fix(Java-HTTP): 修复Java-HTTP api的编译问题 @gylove1994

### 其他

- i18n: at on in date time @Constrat
- debug: debug 模式下增加一项可省略模板任务名的检查 @zzyyyl
- 文档杂项修改 (#6485) @Rbqwow
- Update 3.5-肉鸽辅助协议.md @Lancarus
