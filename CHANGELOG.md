## v4.15.0

我们的全新界面 [MaaX](https://github.com/MaaAssistantArknights/MaaX/releases/latest) 在历经长达两年半的 ~~咕咕~~ 开发后，终于公测啦 ✿✿ヽ(°▽°)ノ✿ 欢迎大家尝鲜体验！  
After a long period of development, our new GUI [MaaX](https://github.com/MaaAssistantArknights/MaaX/releases/latest) is finally in public beta. ✿✿ヽ(°▽°)ノ✿ Welcome to experience it!

### 新增

- 保全派驻 `约翰老妈新建地块`、`雷神工业测试平台` 内置作业 @HMZyueshen @AnnoyingFlowers
- 干员识别 功能 (#4368) @GD-GK @moomiji @MistEO @ABA2396
- 自定义基建根据配置自动修改订单/产物种类 (#4293) @Roland125
- 支持 mumu12 模拟器自动检测连接 @MistEO
- 新增 ~~房贷还款~~ 碳本导航 @ABA2396

### 改进

- Mac UI 全新设计 支持任务链、回调日志、自定义基建、公招仓库识别等功能 @hguandl
- WPF UI 代码结构优化、性能优化、识别工具界面布局调整、部分 bug 修复等 (#4352) @moomiji @ABA2396
- 肉鸽支持识别并选择临时招募干员，优化编队逻辑 (#4433) (#4410) @LingXii
- 优化并修复抄作业子弹时间的逻辑 @MistEO
- 自动基建 伊内斯效率更新（按8小时计算） (#4351) @HeroTch
- 肉鸽骑士结局无法自动作战（Issue #4320），更新了新地图 (#4329) @LingXii
- 调整基建默认选项，`未进驻` 默认开、`蹭信赖` 默认关 @MistEO
- 支持 ota 更新后直接加载资源 @ABA2396
- 调整部分滑动延迟，放到配置文件中 @MistEO

### 修复

- 修复 mac 平台掉落识别错误 @horror-proton
- 为 Config 文件写入添加锁，尝试解决配置丢失问题 (#4373) @LiamSho
- 修复 cache 文件读取路径错误 @ABA2396
- 修复抄作业水陈关不掉技能的问题 @MistEO
- 点两次开始行动导致取消吃理智药 fix #4326 @ABA2396
- 修复 UI Theme 跟随应用模式后的非预期同步 (#4316) @moomiji
- 修复保全关卡名识别错误 @cenfusheng @ABA2396 @MistEO
- 修复肉鸽新关卡 `瞻前顾后` 识别错误 @MistEO
- 修复 `U-Official` 识别错误 @ABA2396
- fix typo in CopilotTip (#4419) @Cryolitia
- increase post delay of RecruitNowConfirm @horror-proton
- 部分情况下活动关导航不进入章节 @ABA2396
- 修复更新出错的问题 @zzyyyl

### 其他

- Add retry and version check for PlayTools @hguandl
- 代码优化调整等 @horror-proton @MistEO @ABA2396
- 将多语言的列表状态统一储存 @ABA2396
- 打开 debug 模式下的任务 JustReturn 检查 (#4408) @zzyyyl
- 添加随机延迟，防止同时更新 @ABA2396
- 优化了点有的没的 @MistEO @zzyyyl @ABA2396

### For develops

- `AsstLoadResource` 接口在文件夹不存在时将返回 false @MistEO
- 为 MaaCore 添加抄作业神秘代码支持 (#4305) @aa889788
- CI 编译 macOS 版统一用 Release @hguandl

### For overseas

#### TXWY

- 繁中服 綠野幻夢活動 (#4317) @vonnoq

#### YostarKR

- Operator add for KR (#4384) @zewoosJ @MistEO
