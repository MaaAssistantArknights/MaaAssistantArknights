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
