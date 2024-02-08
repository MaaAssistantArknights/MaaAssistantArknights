## v5.1.0-beta.2

### 新增 | New

- 合成玉签到task (#8223) @Sherkey @status102
- 抽卡提示不许勾下次不再提示 @ABA2396
- 运行 ReclamationAlgorithm 时默认运行 Reclamation2 而不是旧演算 (#8229) @horror-proton @ABA2396

### 改进 | Improved

- 远程控制对非 https 连接仅提出警告 (#8061) @Sherkey
- 肉鸽适配新干员用法 (#8273) @Lancarus
- 肉鸽投资后结束本次战斗 (#8268) @status102
- 优化肉鸽投资速度及输出 (#8177) @status102
- 调整生息演算任务 @status102
- 增加生稀盐酸跳过等待 @status102
- 全反选排除生息演算 @ABA2396
- Mac 全部启用任务时排除生息演算 @Hao Guan
- 优化可变位置文字识别 @ABA2396
- 部分需重启生效的选项添加重启确认弹窗 @ABA2396

### 修复 | Fix

- 生息演算开局教程对话有概率卡住 @status102
- 修复无限提示远控地址为空的问题 @Sherkey
- 连续截图出错30次 / 启动时连续截图出错10次 导致MAA崩溃 (#8280) @status102
- 写剪贴板时应该用Unicode而不是ANSI (#8204) @Immueggpain
- 修复肉鸽存款余额ocr错误 @status102
- 修复干员小满概率性识别为`小/满` @status102
- 延长进图等待时间 @ABA2396
- 生息演算卡在主界面 @ABA2396
- 修复没有赠送次数时单抽任务报错的问题 (#8233) @SherkeyXD
- 修复生息演算部分情况下无法跳过开局对话 @status102
- 尝试修复剩余理智连战次数无法选择 @status102
- 干员识别修正: 红集 -> 红隼 @aur3l14no
- 修复更新后弹窗错误 @ABA2396
- 修复生息演算在卡顿情况下无法正常退出关卡 @ABA2396
- 限制连续战斗次数调整失败重试次数 @status102
- 生息演算卡在进入第四日 @ABA2396
- screencap failure with amdgpu on some emulators (even with GeneralWithoutScreencapErr) (#8225) @aur3l14no
- 修复部分分辨率下无法识别删除存档 @ABA2396
- 内测版 wpf tag 错误 @ABA2396
- 内测版修改ui版本号 @ABA2396

### 其他 | Other

- 修复Mac生息演算任务名 @Hao Guan
- ScreencapCost输出失败次数 @status102
- fix typo and undefined behavior of minmax @Horror Proton
- 生息演算高级设置文本框增加换行 @status102
- EasterEggs 前两次点击不生效 @ABA2396
- 抽卡风险提示 @ABA2396
- 生息演算提示文本调整 @status102
- 移除肉鸽结算输出多余空格 @status102
- Update release-nightly-ota.yml @ABA2396
- 更新界面文字 (#8206、#8214、#8218) @AnnAngela
- fix appimage cross build issue @Horror Proton
- 更新文档 (#8283) @Rbqwow

### For Overseas

##### YoStarJP

- YoStarJP 理想都市 navigation (#8263) @Manicsteiner
- YoStarJP ocr fix (#8260) @Manicsteiner

##### YoStarKR

- YoStarKR 理想都市 navigation (#8265) @HX3N
- YoStarKR ocr fix (假日威龙陈/苍苔/青枳/淬羽赫默 etc) (#8246) @HX3N
- YoStarKR update StageAnnihilation.png (#8239) @HX3N
- YoStarKR ocr fix (#8236) @HX3N
- modified ko-kr.xaml (#8235) @HX3N
