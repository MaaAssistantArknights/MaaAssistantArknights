## v5.11.0-beta.1

### 新增 | New

* Mac GUI重构&多语言 (#11295) @hguandl
* SideStory「出苍白海」导航 (#11294) @SherkeyXD
* YostarJP manually add BP new operators (#11286) @Manicsteiner
* add BP new operators @Constrat
* YostarEN BP navigation @Constrat
* SSS#5 ocr replace for conductive elements @Constrat
* SSS#5 for global @Constrat

### 改进 | Improved

* WpfGui重构 拆分`一键长草-刷理智任务` (#11223) @status102
* 账号切换退出账号增加OCR (#11258) @Daydreamer114
* 优化葛朗台，碎石在理智不会溢出时不再等待 @status102
* 优化 StageManager 代码结构 (#11233) @ABA2396 @status102
* WpfGui重构 拆分`一键长草-领取奖励任务` (#11228) @status102
* WpfGui重构 拆分`一键长草-基建任务`任务 (#11219) @status102
* WpfGui重构 拆分`一键长草-获取信用及购物任务` (#11218) @status102
* 简化绑定 @status102
* WpfGui重构 拆分`一键长草-自动肉鸽` (#11201) @status102
* 优化参数解析 @ABA2396
* WpfGui重构 拆分`设置-远程控制` (#11202) @status102
* WpfGui重构 拆分`设置-定时执行` (#11192) @status102

### 修复 | Fix

* 设置Xcode版本为15.3 (#11298) @hguandl
* 萨卡兹肉鸽点刺成锭分队开局思维阻塞 (#11276) @Daydreamer114
* missing battle_data BP event @Constrat
* 双击托盘图标可能导致窗口位置错误 @ABA2396
* Mac GUI日志&肉鸽难度设置 (#11273) @hguandl
* 为 ios端降低 Sarkaz@Roguelike@DecreaseBurdenAbandonThought 任务模版匹配的 threshold (#11266) @Alan-Charred
* 干员识别中出现卫戍中的 4★ 预备干员 及 特有 6★ @ABA2396
* Mac GUI 优化作业列表 (#11250) @hguandl
* 新下载的 MAA 会在加载关卡时报错 @ABA2396
* 繁中服無法訪問好友 (#11248) @momomochi987 @pre-commit-ci[bot]
* macOS 肉鸽参数&客户端包名 (#11245) @hguandl
* UI 更新后自动填充有概率拉取其他设施内干员 @ABA2396
* 避免非预期的任务参数设置检查 (#11232) @status102
* 肉鸽构想不足过高阈值导致 mac 版无法识别 @Daydreamer114
* 切换配置后不会自动启动 @ABA2396
* API 中被删除的已关闭活动关卡被误判为常驻关卡 @ABA2396
* 刷理智失败次数限制未生效 @ABA2396
* 修改邮件通知中特殊字符的处理方式 (#11169) @LogicDX342 @status102
* 访问好友在部分背景下无法进入 @ABA2396
* 命令行切换配置后弹出公告 @ABA2396
* 加载顺序 @ABA2396
* 使用命令行切换配置时有可能使关卡选择被覆盖 @ABA2396
* IS2 missing templates for recruiting and obtaining relics (#11196) @nnnn1111 @pre-commit-ci[bot]
* not entering recruitment when in mall with recruitment permit item on screen @Constrat
* 荒芜拉普兰德 OcrReplace (#11184) @Saratoga-Official

### 文档 | Docs

* Auto Update Changelogs of v5.11.0-beta.1 (#11297) @github-actions[bot]
* 修改常见问题中的下载/安装问题的描述 (#11229) @ClozyA

### 其他 | Other

* Release v5.11.0-beta.1 (#11291) @ABA2396
* tools: style for clone_data powershell script [skip changelog] @Constrat
* remove Yostar servers reclamation algorithm temp edits @Manicsteiner
* 管理员权限启动无法使用开机自启 (#11290) @ABA2396 @momomochi987
* tools: style linking [skip changelog] @Constrat
* 因为资源版本上报失败时添加提示 @ABA2396
* YostarKR add BP new operators (#11285) @HX3N
* 移除多余代码 @ABA2396
* YostarKR BP navigation (#11279) @HX3N
* YostarJP BP navigation @Manicsteiner
* Translations update from MAA Weblate (#11255) @AlisaAkiron
* WpfGui`游戏设置`修改为`运行设置` (#11209) @status102 @Constrat
* EN tweaks [skip ci] @Constrat
* 通过控制台退出模拟器时不再kill端口 @ABA2396
* YostarJP ocr replace 魔王 (#11222) @Manicsteiner
* 成功代理不再减少任务失败计数 @ABA2396
* Auto update by pre-commit hooks [skip changelog] @pre-commit-ci[bot]
* 移除过渡代码 @ABA2396
* YostarKR SSSBuffChoose ocr fix @HX3N
* automatically remove spaces for YoStarKR in CharsNameOcrReplace (#11205) @Constrat
* YostarKR SSS#5 ocr added (#11200) @HX3N
* 拆分 设置-界面设置 (#11188) @status102
* 繁中服保全派駐的新定向導能元件 @momomochi987
* 拆分 设置-游戏设置 (#11185) @status102
* rename `ExternalNotificationDataContext` to `ExternalNotificationSettings` @status102
* rename `VersionUpdateDataContext` to `VersionUpdateSettings` @status102
