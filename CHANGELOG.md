## v5.1.0

### 新增 | New

- 生息演算2 (#8190) @SherkeyXD @hguandl @ABA2396
- 开放每日免费单抽功能 (#8185) @SherkeyXD
- 新增只购买折扣信用商品（白名单商品除外）的选项和少于300信用点停止购物的选项 (#7659) @lpowo @status102
- 基建领取专精功能 (#7754) @broken-paint @Constrat
- stop and log when missing operators during formarion (#8051) @KevinT3Hu
- 强制定时启动添加提示对话框并可选是否显示窗口 (#8062) @moomiji
- 运行 `ReclamationAlgorithm` 时默认运行 `Reclamation2` 而不是旧演算 (#8229) @horror-proton @ABA2396
- 合成玉签到task (#8223) @SherkeyXD @status102
- 远程控制对非 https 连接仅提出警告 (#8061) @SherkeyXD
- 抽卡提示不许不看 @ABA2396

### 改进 | Improved

- 更新连战文档 (#8308) @Rbqwow
- ScreenCost截图失败次数为0时，不返回fault_times字段 @status102
- 缓存待部署区识别，减少CPU占用 (#8275) @status102
- 更新文档到.net8 @ABA2396
- 移除CopilotTask不必要的传参 @status102
- 更新部分step的版本 @SherkeyXD
- 增加账号切换时登录键点击重试 @status102
- 更新文档 (#8283) @Rbqwow
- ScreencapCost输出失败次数 @status102
- 肉鸽适配新干员用法 (#8273) @Lancarus
- 肉鸽投资后结束本次战斗 (#8268) @status102
- 生息演算开局教程点击加速 @status102
- 优化肉鸽投资速度 (#8177) @status102
- 生息演算高级设置文本框增加换行 @status102
- 调整生息演算任务 @status102
- 增加生稀盐酸跳过等待 @status102
- 更新界面文字 (#8206) @AnnAngela

### 修复 | Fix

- gui.log 无法显示完整版本号 @ABA2396
- 修复自动检测失败时可能导致WpfGui崩溃 (#8288) @status102
- 未开启吐司通知时仍弹出通知 @ABA2396
- fix typo @ABA2396
- 生息演算开局教程对话有概率卡住 @status102
- fix typo and undefined behavior of minmax @horror-proton
- 修复无限提示远控地址为空的问题 @SherkeyXD
- 启动时连续截图出错10次导致MAA崩溃 (#8280) @status102
- 修复Mac生息演算任务名 @hguandl
- Mac 全部启用任务时排除生息演算 @hguandl
- fix appimage cross build issue @horror-proton
- 补充肉鸽投资系统出错时余额识别替换 @status102
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
- 优化可变位置文字识别 @ABA2396

### 其他 | Other

- 升级依赖，移除package-lock.json @SherkeyXD
- added regex for module-reclamation (#8302) @Constrat
- YoStarKR add training template image and text (#8295) @HX3N
- 添加等宽字体的备用字体 @SherkeyXD
- Release v5.1.0-beta.2 (#8227) @ABA2396
- Release v5.1.0-beta.1 (#8198) @ABA2396
- Auto Update Changelogs of v5.1.0-beta.1 (#8199) @github-actions[bot] @ABA2396
- YoStarJP ocr fix shamare, ansel, nien (#8194) @Manicsteiner
- YoStarJP ocr fix #8180 (#8184) @Manicsteiner
- Update CHANGELOG.md @ABA2396
- 格式化代码 @ABA2396
- Auto Update Changelogs of v5.1.0-beta.2 (#8267) @github-actions[bot] @status102 @ABA2396
- 移除mumu6 (#8284) @Rbqwow
- 升级依赖，调整视觉效果 (#8202) @SherkeyXD
- YoStarJP ocr fix (#8260) @Manicsteiner
- YoStarKR 理想都市 navigation (#8265) @HX3N
- YoStarJP 理想都市 navigation (#8263) @Manicsteiner
- EasterEggs 前两次点击不生效 @ABA2396
- Update release-nightly-ota.yml @ABA2396
- 内测版 wpf tag 错误 @ABA2396
- 内测版修改ui版本号 @ABA2396
- 部分需重启生效的选项添加重启确认弹窗 @ABA2396
- YoStarKR ocr fix (假日威龙陈/苍苔/青枳/淬羽赫默 etc) (#8246) @HX3N
- 修改变量名 @ABA2396
- 抽卡风险提示 @ABA2396
- 添加注释 @ABA2396
- 抽卡提示不许勾下次不再提示 @ABA2396
- 删除多余内容 @ABA2396
- modified ko-kr.xaml (#8235) @HX3N
- YoStarKR update StageAnnihilation.png (#8239) @HX3N
- YoStarKR ocr fix (#8236) @HX3N
- 生息演算提示文本调整 @status102
- 全反选排除生息演算 @ABA2396
- 移除肉鸽结算输出多余空格 @status102
- Update zh-cn.xaml (#8218) @AnnAngela
- Update zh-cn.xaml (#8214) @AnnAngela
