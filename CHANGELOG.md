## v4.15.0-beta.1

我们的全新界面 [MaaElectronUI](https://github.com/MaaAssistantArknights/MaaAsstElectronUI/releases/latest) 在历经长达两年半的 ~~咕咕~~ 开发后，终于公测啦 ✿✿ヽ(°▽°)ノ✿ 欢迎大家尝鲜体验！  
After a long period of development, our new GUI is finally in public beta. Welcome to experience it!  ✿✿ヽ(°▽°)ノ✿ 

### 新增

- 新增 保全派驻 `约翰老妈新建地块`、`雷神工业测试平台` 内置作业 @HMZyueshen @AnnoyingFlowers
- 自定义基建根据配置自动修改订单/产物种类 (#4293) @Roland125
- 新增 ~~房贷还款~~ 碳本导航 @ABA2396

### 改进

- Mac Gui 全新设计 支持任务链、回调日志、自定义基建、公招仓库识别等功能 @hguandl
- Win UI 代码结构优化、性能优化、识别工具界面布局调整、部分 bug 修复等 (#4352) @moomiji @ABA2396
- 优化抄作业子弹时间的逻辑 @MistEO
- 自动基建 伊内斯效率更新（按8小时计算） (#4351) @HeroTch
- 肉鸽骑士结局无法自动作战（Issue #4320），更新了新地图 (#4329) @LingXii
- 调整基建默认选项，`未进驻` 默认开、`蹭信赖` 默认关 @MistEO

### 修复

- 修复 mac 平台掉落识别错误 @horror-proton
- 为 Config 文件写入添加锁，尝试解决配置丢失问题 (#4373) @LiamSho
- 修复 cache 文件读取路径错误 @ABA2396
- 修复约翰老妈保全关卡 `混乱“派对”` 的识别错误 (#4401) @cenfusheng
- 修复肉鸽新关卡 `瞻前顾后` 识别错误 @MistEO
- 修复抄作业水陈关不掉技能的问题 @MistEO
- 修复 `U-Official` 识别错误 @ABA2396
- 点两次开始行动导致取消吃理智药 fix #4326 @ABA2396
- 修复 UI Theme 跟随应用模式后的非预期同步 (#4316) @moomiji

### 其他

- Add retry and version check for PlayTools @hguandl
- 代码优化调整等 @horror-proton @MistEO @ABA2396

### For develops

- 为 MaaCore 添加抄作业神秘代码支持 (#4305) @aa889788

### For overseas

#### TXWY

- 繁中服 綠野幻夢活動 (#4317) @vonnoq

#### YostarKR

- Operator add for KR (#4384) @zewoosJ @MistEO
