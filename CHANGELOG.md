## v4.24.0-rc.2

### 修复

- 修复肉鸽招募卡住的问题 (#6528) @Lancarus
   - 凹直升模式 @Lancarus
   - 回滚招募点击任务 @Lancarus
   - 回滚l RecruitWithOutButton 任务 @Lancarus
   - 脑子坏了，多擦掉一些边缘就行了应该 @Lancarus
   - 再适配一下截图 @Lancarus
   - 进一步降低匹配度阈值，增加前任务的延时 @Lancarus
   - 尝试修复肉鸽招募卡主的问题 @Lancarus
- 自动战斗-战斗列表启用时，对所有作业追加摆完挂机选项操作 (#6533) @status102
   - 修改文档中关于账号切换容易产生误解的地方 #6500 @status102
   - 修改输入框Enabled以优化输入作业号时的表现 @status102
   - 自动战斗-战斗列表启用时，自动追加摆完挂机选项 close #6507 @status102
- 修复ProcessTask停不掉的问题 @MistEO
- 修改理智数据格式 (#6525) @zzyyyl @status102
   - 将理智数据用 object 而不是 array 存储 @zzyyyl
   - use `from_chars` instead of `stoi` to avoid exception @zzyyyl
   - 修复理智数据的一系列问题 @zzyyyl
   - 在Core检查理智获取数据合法性 fix #6519 @status102
- 修复理智预测时间格式 (#6514) @status102
   - 移除小时、分钟数内部的空格以保持紧凑 @status102
   - 修改理智预测时间格式 @status102
- 继续尝试修复肉鸽卡在招募界面 (#6521) @Lancarus
   - 继续尝试修复肉鸽卡在招募界面 @Lancarus
- 修复 理智获取数据格式错误 导致的崩溃 (#6519) @status102
   - 修复 理智获取数据格式错误 导致的崩溃 @status102

### 其他

- 添加对账号切换的说明 (#6500) @A-JiuA
   - 添加对账号切换的说明 @A-JiuA
