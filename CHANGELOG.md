## v5.15.0-beta.2

### 新增 | New

* 将背景设置独立出来 (#12254) @ABA2396
* 添加自定义任务，仅在 debug 模式下可见入口 @ABA2396
* YostarEN GO navigation preload @Constrat
* YostarJP GO navigation (#12244) @Manicsteiner
* YostarKR GO navigation (#12242) @HX3N

### 改进 | Improved

* 反转右键效果时更新 tooltip 悬浮提示 @ABA2396
* 优化肉鸽难度选择显示 @ABA2396

### 修复 | Fix

* 线索翻页限制 10 次，超出次数或未识别到下一页强制给当页第一个好友送线索 @ABA2396
* 外部通知 SMTP 与 Qmsg 输入框样式错误 @ABA2396
* 落了个 ResetRecruitVariables @ABA2396
* mac下选择难度 999 导致卡死 (#12235) @Alan-Charred
* 修复宿舍填充干员时不经过检测盲点首位干员的问题 (#12234) @Alan-Charred
* 繁中服生息演算無法讀檔 (#12230) @momomochi987

### 文档 | Docs

* 添加ci文档 (#11924) @SherkeyXD @Rbqwow
* 调整新手上路步骤顺序 (#12228) @Rbqwow

### 其他 | Other

* 添加一个临时定时 (#12243) @ABA2396
* 将密码框样式独立出来 @ABA2396
* bump maa-cli to 0.5.4 (#12091) @wangl-cc
* 提前适应新 UI 的资源关卡和肉鸽图标 @ABA2396
* 给反转 CheckBox 加个样式 @ABA2396
* 添加背景模糊半径滑块 @ABA2396
* 添加反转主任务右键单击效果勾选项 @ABA2396
* 调整背景模糊默认半径 @ABA2396
* 可修改 gui.json 自定义 Background 模糊半径 @ABA2396
