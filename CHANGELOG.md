## v4.8.5

- 修复 macOS 依赖库闪退 @hguandl
- 修复 未勾选自动下载更新包时无法自动更新 @ABA2396
- 修复 关卡导航为 当前/上次 时，开启刷信用死循环的问题 @ABA2396
- 修复 进程已存在时无法最小化启动模拟器 @ABA2396
- 修复 掉落/仓库数量识别错误导致崩溃的问题 @MistEO
- 优化 界面 刷信用功能提示 @ABA2396

### For overseas

- Fix the stage name error in the EN client @RiichiTsumo

## v4.8.4

- 修复 拉起模拟器时的报错 @ABA2396
- 修复 日期解析报错 @ABA2396
- 更新 文档 @WWPXX233

### For overseas

- Fix the errors of Base and Daily tasks in the EN client @Pitiedwzr
- Fix the errors of Recruitment in the JP client @horror-proton
- Support more Base functions of the ZH_CHT client @vonnoq

## v4.8.3

- 修复 `照我以火` 活动关卡导航错误 @MistEO

## v4.8.2

- 更新 `照我以火` EX 关卡地图数据 @MistEO
- 更新 macOS, Linux OCR 部署库，提高识别准确率 @hguandl @horror-proton
- 新增 启动模拟器时自动最小化 选项 @ABA2396
- 修复 界面 时间转换错误 @ABA2396
- 修复 Linux CI 任务名 @horror-proton

### For overseas

- 新增 繁中服 访问好友、采购中心支持 @vonnoq
- Retrained the OCR model of the JP client @MistEO

## v4.8.2-beta.1

- 重新支持 macOS, Linux CI 打包 @hguandl @horror-proton
- 优化 助战赚信用 功能，每天仅执行一次 @ABA2396
- 更新 自定义基建 所有内置作业为 12 月新版 @HMZyueshen
- 更新 OCR 部署库，重新训练 OCR 模型，再次提高识别准确率 @MistEO
- 修复 自定义基建，宿舍内 sort 和 autofill 同为 true 时，存在的选人 bug @fearlessxjdx
- 修复 繁中服 基建不收赤金的问题 @vonnoq
- 修复 界面 关闭关卡导航后的问题 @ABA2396
- 更新 文档 @ABA2396 @MistEO @WWPXX233

### For overseas

- Support YoStarEN Auto Base function. @Pitiedwzr
- Update EN docs @CallMeChin
- Add Overseas Clients Adaptation docs. @ABA2396  
    It's really simple, feel free to join us!

### For develops

- Update Python APIs, support to set pramas for Maa instance. @popjdh
- Support the starting with Support Unit in Auto IS, but without gui. @WLLEGit
- Add Overseas Clients Adaptation docs. @ABA2396

## v4.8.1

- 修复活动关卡导航错误 @ABA2396

## v4.8.0

- 启用 ONNX Runtime 进行 OCR，并重新训练了各个语言的识别模型 @MistEO @horror-proton @hguandl @aa889788 @zhangchi0104 @mole828 @WWPXX233  
  _大幅降低内存占用、提高推理速度、减小文件体积，且不再区分 NoAVX 版本_
- 更新 `照我以火` 活动数据，新增关卡导航 @MistEO @ABA2396
- 新增 支持 暂停部署干员，请进入设置开启~ @aa889788 @MistEO  
  _测试功能，仅 MaaTouch 支持较好，其他触控方式不推荐使用_
- 新增 借助战打一局赚 30 信用功能，请进入设置开启~ @Hydrogina @MistEO @ABA2396
- 新增 自定义导航 难度选择，请在关卡名后加入 `Hard` / `Normal` 使用 @ABA2396 @MistEO
- 新增 启动时 删除 `.old` 文件 @MistEO
- 新增 Linux Release @horror-proton
- 合并 `访问好友` 和 `信用购物` 功能 @MistEO
- 重构 项目工程、目录结构，重命名 部分文件 @MistEO @horror-proton @hguandl
- 重构 内部回调体系、重构实例继承体系、重构文件结构树 @MistEO
- 优化 自动战斗/肉鸽 技能图标识别 @horror-proton
- 优化 界面 语言切换提示 @ABA2396
- 优化 界面 完成后 关机、休眠 选项，将会保存该两项设置 @MistEO
- 优化 界面 设置布局 @ABA2396
- 优化 肉鸽 艾妮莉 技能 @WWPXX233
- 修复 肉鸽 卡在放弃招募界面的问题 @MistEO
- 修复 肉鸽 不期而遇 放弃的问题 @MistEO
- 修复 肉鸽 打到一半突然摆烂的问题 @MistEO
- 修复 自动战斗 编队偶尔失败的问题 @MistEO
- 修复 自动战斗 偶尔不跳过剧情的问题 @MistEO
- 修复 干员 识别错误 @nightofknife @MistEO
- 修复 OF 关卡导航识别问题 @ABA2396
- 修复 自动公招 时间识别错误导致反复点击某个槽位的问题 @horror-proton
- 修复 企鹅用户 ID 解析错误 @MistEO
- 修复 adb 断开重连后，不使用 minitouch 的问题 @MistEO
- 修复 更新包 下载完成后崩溃的问题 @MistEO
- 更新 文档 @MistEO @ABA2396 @Mr-Milk @ChildeRolando
- 更新 ISSUE 模板 @MistEO

### For overseas

- Support Login, Base, Recruiting, Combat, Friends visiting, Store shopping, Daliy, Auto I.S. for the KR client @178619
- Fix some resources of KR client and update the documentation @178619
- Add the translations and documentations for the KR client @178619
- Fix drops recognition errors in JP client @MistEO
- Fix the bug of IS stuck in JP client @MistEO
- Fix wake-up error in JP client @liuyifan-eric
- Update the translations and documentations for the JP client @wallsman
- Organize overseas client profiles @liuyifan-eric

### For developers

- Add `async_connect`, `async_click`, `async_screencap` API, plz refer to `AsstCaller.h` to use @MistEO
- Deprecate `Visit` task and merge it into `Mall` task @MistEO
- Add Linux CI @horror-proton @aa889788 @hguandl
- Explicitly C APIs return value and parameter type, should be compatible with the original interface, but may also need to modify some integration code @MistEO
- Update Rust APIs for the above changes @horror-proton
- Add checksums for download libs @aa889788
