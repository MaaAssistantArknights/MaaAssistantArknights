## v4.8.0-beta.3

- 重构 项目工程、目录结构，重命名 部分文件 @MistEO @horror-proton @hguandl
- 重新支持 macOS 打包，并集成新接口 @hguandl @horror-proton
- 新增 支持 暂停部署干员 @MistEO  
  _目前仅 MaaTouch 效果较好，minitouch 在性能较差的电脑上会一直暂停着不动，adb input 不支持该功能_
- 新增 启动时 删除 `.old` 文件 @MistEO
- 优化 界面 设置布局 @MistEO @ABA2396
- 修复 干员 识别错误 @nightofknife @MistEO
- 更新 文档 @MistEO @Mr-Milk @ChildeRolando

### For overseas

- Fix some resources of KR client and update the documentation @178619
- Fix the bug of IS stuck in JP client @MistEO

### For developers

- Explicitly C APIs return value and parameter type, should be compatible with the original interface, but may also need to modify some integration code @MistEO
- Update Rust APIs for the above changes @horror-proton

## v4.8.0-beta.2

- 修复 `风雪过境` 关卡名识别错误 @MistEO
- 优化 `风雪过境` 关卡导航 @ABA2396
- 修复 肉鸽 不期而遇 放弃的问题 @MistEO
- 修复 adb 断开重连后，不使用 minitouch 的问题 @MistEO
- 修复 更新包 下载完成后崩溃的问题 @MistEO

## v4.8.0-beta.1

- 更新 `风雪过境` 关卡数据 @MistEO
- 修复 `风雪过境` 关卡导航 @ABA2396
- 修复 基建、自动编队 干员名识别错误 @MistEO
- 修复 信用战斗 部分卡住的问题 @Hydrogina @MistEO
- 优化 肉鸽 艾妮莉 技能 @WWPXX233
- 修复 打包错误 @MistEO

### For Overseas

- Fixed visiting and added more ocrReplaces to KR @178619

## v4.8.0-alpha.1

- 启用 ONNX Runtime 进行 OCR，大幅降低内存占用、提高推理速度、减小文件体积，并不再区分 NoAVX 版本 @MistEO @horror-proton @hguandl @aa889788  
    ~~终于踢了 PPOCR，但目前识别准确率有轻微下降，待后续版本优化。且目前暂未支持 Linux, macOS 版本~~
- 新增 借助战打一局赚 30 信用功能，请进入设置开启~ @Hydrogina
- 新增 `MaaTouch` 连接配置 项。推荐在 minitouch 无法使用、但又不想用 `兼容触控模式` 时开启 @aa889788 @MistEO
- 新增 自定义导航 难度选择，请在关卡名后加入 `Hard` / `Normal` 使用 @ABA2396 @MistEO
- 合并 `访问好友` 和 `信用购物` 功能 @MistEO
- 重构 内部回调体系、重构实例继承体系、重构文件结构树 @MistEO
- 修复 水月肉鸽 不期而遇关卡直接放弃的问题 @MistEO
- 修复 minitouch 路径错误无法使用的问题 @MistEO
- 修复 自动公招 时间识别错误导致反复点击某个槽位的问题 @horror-proton
- 修复 OF 关卡导航识别问题 @ABA2396
- 修复 自动战斗 编队偶尔失败的问题 @MistEO
- 修复 自动战斗 偶尔不跳过剧情的问题 @MistEO
- 优化 界面布局及描述 @ABA2396
- 更新 文档 @ABA2396 @MistEO

### For Overseas

- Support Login, Base, Recruiting, Combat, Friends visiting, Store shopping, Daliy, Auto I.S. for the KR client @178619
- Fix drops recognition errors for the JP client @MistEO
- Update the translations and documentations for the JP client @wallsman
- Add the translations and documentations for the KR client @178619

### For Developers

**The `master` branch cannot be built on Linux and macOS because the ONNX Runtime adaptation is not finished yet, so please use `stable` branch for now. Feel free to join us in this work!**

- Deprecate `Visit` task and merge it into `Mall` task @MistEO
- Add `async_connect`, `async_click`, `async_screencap` API, plz refer to `AsstCaller.h` to use @MistEO
