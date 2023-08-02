## v4.21.0

### 新增

- 新增 `火山旅梦` 活动导航 @ABA2396
- 更新 `火山旅梦` 部分游戏资源（地图还没更新）@MistEO
- 给牛牛抽卡提示增加阴间特效 (#5694) @SherkeyXD

### 改进

- 新增萨米肉鸽4~5层部署策略 (#5725) @LingXii
- 优化傀影肉鸽招募逻辑和棘刺队作战逻辑 (#5707) @Lancarus
- 进一步优化水月肉鸽部分关卡作战逻辑 (#5663) @Yumi0606
- 优化肉鸽招募逻辑 (#5621) @SherkeyXD
- 选择刷源石锭策略时强制启用投资选项 (#5625) @SherkeyXD
- 一些文字识别问题与死囚之夜部分高台站位 (#5710) @cenfusheng
- 使用 version summary api 检查更新 @MistEO
- Implement ADB hard restart. (#5666) @AsterNighT
- 修复干员青枳识别 (#5775) @Black1312

### 修复

- 更新的逻辑小错误 (#5752) @tangge233
- 关闭模拟器权限不够时失败报错 @ABA2396
- 连接失败后自动重启模拟器功能失效 @ABA2396
- 主线导航识别问题 @ABA2396
- mumu模拟器在游戏未启动时关闭游戏报错，增加关闭游戏日志和判断 @ABA2396
- 修复一处因 MarkdownIt 引入导致的404链接 (#5680) @abc1763613206
- 本地化文件错误/localization file error @ABA2396
- gui.log 在自重启后有几率无法正常写入 (#5624) @TiSpH

### 其他

- 新增 C# 代码静态分析 @hxdnshx
- 改进 macos-runtime-universal.zip 生成 @AnnAngela
- 主目录存在 DEBUG.txt 时每次开始任务都重新解析tasks.json (#5736) @ABA2396
- i18n: 5ec976c9194d68b9b5c78234152ab2eb1953c5d4 moved changes to new file to allow future edits @Constrat
- SSS available in global, removed from regex @Constrat
- i18n: Added EN translation to SSS files @Constrat
- typo: fix typo @ABA2396
- 界面选项改为 mumu 6 @MistEO
- 添加关联项目 MaaFramework @MistEO
- macos-runtime-universal.zip 生成过程自适应版本号 @AnnAngela
- 添加macos-runtime的resource文件夹和dylib的symlink @ChingCdesu
- 添加对于mumu12后台保活功能的设置的说明 (#5781) @SherkeyXD

### For overseas

#### YostarEN

- SSS Implementation for YostarEN @Constrat
- WB event navigation for EN @Constrat
- missing name-component for SSS + tweaked regex @Constrat
- Dossoles ROI and Stage name fixes + temp OCR rep for long names @Constrat
- ulterior changes / fixes from 9f2c7488b3310ac6a0e2304b7796181d88703f91 @Constrat
- Auto Update Wrong order, rerun before limited @Constrat
- Manual stage navigation for EN (fix #4874) @Constrat
- duplicate: repeats in Ocr replace EN tasks.json @Constrat
- better StartButton1.png @Constrat
- Wrong event in display @Constrat
- from ba75736 Manual Stage navigation EN @Constrat

#### txwy

- 繁中服水月肉鴿  @vonnoq @Lancarus @momomochi987 @Arcelibs @avw621314
- 繁中服支持牛牛抽卡 @ABA2396
- 重製繁中服部分截圖 (#5748) @momomochi987
- 繁中服水月肉鴿「委託完成」卡住 (#5766) @vonnoq

#### YostarJP

- 日服水月肉鸽事件选项确认页面 (#5749) @Manicsteiner
- JP : add Operators 登臨意 (#5636) @wallsman
