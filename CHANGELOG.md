- 启用 ONNX Runtime 进行 OCR，大幅降低内存占用、提高推理速度、并减小文件体积，不再区分 NoAVX 版本。@MistEO @horror-proton @hguandl @aa889788  
    ~~终于踢了 PPOCR，但目前识别准确率有轻微下降，待后续版本优化。且目前暂未支持 Linux, macOS 版本~~
- 新增 借助战打一局赚 30 信用功能，请进入设置开启~ @Hydrogina
- 合并 `访问好友` 和 `信用购物` 选项 @MistEO
- 新增 `MaaTouch` 连接配置 项。推荐在 minitouch 无法使用、但又不想用 `兼容触控模式` 时开启 @aa889788 @MistEO
- 新增 自定义导航 难度选择，请在关卡名后加入 `Hard` / `Normal` 选择 @ABA2396 @MistEO
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

- Add `async_connect`, `async_click`, `async_screencap` API, plz refer to `AsstCaller.h` to use @MistEO
