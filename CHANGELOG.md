## v6.4.0

### 新增 | New

* JP RebuildingMandate and Together theme (#15931) @Manicsteiner
* YostarEN RebuildingMandate + Together @Constrat
* 调整 mumu 国际版截图增强支持提示 @ABA2396
* 合并 处于战斗中 检测, 增加倍速记忆, 支持战斗中 不带跳过按钮的 带头像对话教程 (#15706) @status102
* 设置界面选项页允许折叠 @ABA2396
* PC 端运行时要求以管理员身份重启 MAA (#15748) @SherkeyXD
* 自动战斗-其他活动支持使用作业列表加载作业集并从中选择作业 (#15804) @status102

### 改进 | Improved

* optimize png @Constrat
* 优化自动检测连接与每次重新检测连接提示 @ABA2396
* EN, JP, KR MiniGame AT「相談室‧御影」小遊戲 (#15870) @Daydreamer114
* ASST_DEBUG 在 lazy_parse 后不再重新生成任务信息 (#15895) @status102
* scopeLog 记录函数进出时间 @status102
* TaskQueue任务属性变更记录路径优化 @status102
* 适配国服游戏内暂停期间部署 (#15888) @status102
* 适配PlayCover的BGR优化截图 (#15810) @hguandl

### 修复 | Fix

* 修复今日关卡小提示中奶盾芯片数量顺序错误 @ABA2396
* Fix YoStarJP Sidestory GO OCR error (#15906) @JasonHuang79
* Make Starting Operator searching dropdown case insensitive for en (#15879) @Zenith00
* 移除不必要的配置迁移触发条件 @status102
* 修改任务名过滤换行 @status102

### 文档 | Docs

* 使文档站中侧边栏的功能介绍板块默认展开 (#15927) @lucienshawls

### 其他 | Other

* macOS build scripts (#15936) @hguandl
* MaaDeps cache asset (#15933) @hguandl
* KR RebuildingMandate ocr edit @HX3N
* EN Rebuilding Mandate @Constrat
* KR RebuildingMandate and Together UiTheme (#15932) @HX3N
* 调整每次重新检测勾选框选中时的颜色 @ABA2396
* 繁中服「墟」導航文字 (#15925) @momomochi987
* `战斗列表` 选项更名为 `多作业模式`, 以区分于作业容器 (#15913) @status102 @Constrat @HX3N @Manicsteiner
* 更正最小化的描述 @MistEO
* 调整PC端输入选项及描述 @MistEO
* ClientType 常量替换 @status102
* 剿灭string 常量 @status102
* 刷理智筛选候选关卡后在日志中记录 @status102
* rft!: 自动战斗协议 干员技能默认值变更为 0 (不指定技能) (#15898) @status102
* cleanup warnings - unused variables and unused includes @Constrat
* 自动战斗跳过干员属性要求提示 增加 精英化例外 及 跳过后不点赞说明 (#15881) @status102 @Constrat @HX3N @momomochi987
