## v4.26.0-beta.3

### 新增

- 肉鸽增加不期而遇事件名输出 (#7112) @status102
- 调整理智药使用为按瓶计数，移除了博朗台在使用理智药不溢出情况下的额外等待，调整Wpf理智药使用输出 (#7022) @status102
- 新增获取信用-借助战时使用指定编队 (#7061) @status102 @broken-paint @MistEO
- 萨米肉鸽使用密文板  (#7086) @Lancarus @zzyyyl
- Add Operator JP/12章 @wallsman

### 改进

- 增加雷电模拟器 nc 截图方式 (#7062) @zzyyyl
- add some mirror for resource updates @MistEO
- 优化部分情况下，单个刷理智任务在多次执行时等待时间过长 (#7108) @status102
- 优化Wpf关卡数据更新提示，改为Growl.Info (#7103) @status102
- 优化萨米肉鸽部分关卡策略 (#7099) (#7075) @Lancarus

### 修复

- 調整繁中服「照我以火」導航任務、替換水月肉鴿 "繼續探索" 圖片 (#7085) @momomochi987
- As in my Adumbration operator implementation @Constrat
- Morgan 摩根 ocrreplace [skip ci] @Constrat
- c# warnings @MistEO
- 修复Wpf无法复制版本号至剪贴板 (#7109) @status102
- 优化设置引导界面的视觉效果，并给 UI Theme 添加翻译 (#7047) @SherkeyXD
- Mizuki I.S. encouter regex [skip ci] @Constrat
- Terra Research Commission EN regex [skip ci] @Constrat
- 叙拉古人活动导航 @ABA2396
- Il Siracusano enter screen change @Constrat
- 修复日文文档中顿号导致的渲染问题 @SherkeyXD
- coredump after failing to restart minitouch @horror-proton
- 修复肉鸽编队会漏选干员的问题（比如>=3页） (#7038) @wengzhe
- fix：萨米肉鸽层名ocr错误 @Lancarus
- resource updater + updated data (#7079) @Constrat
- 修复在点击停止后下一次开始任务时可能操作延时很高的问题 (#7052) @zzyyyl
- duplicate update in stage.json (my mistake from 859802071176f9f145dabd6f3c3ecb442cc3663c) @Constrat
- 修复日服基建换班卡在宿舍 (#7068) @MejiroSilence
- 修复 #7016 可能影响的部分任务 (#7066) @zzyyyl
- SkipThePreBattlePlot lowered threshold @Constrat
- 修复公招多选 Tags 选项不保存的问题 (#7065) @broken-paint
- fix code block in user guide for CLI (#7060) @wangl-cc

### 其他

- 多划几次 @ABA2396
- format @zzyyyl
- manual after res-update fix @Constrat
- Wpf迁移MaaCore引用，统一合并至MaaService (#6945) @status102
