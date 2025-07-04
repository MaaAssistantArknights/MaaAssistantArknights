## v5.18.2

### 新增 | New

* wpfgui 添加第三备选关卡 (stage4) (#13106) @Daydreamer114
* 优化 CropRoi 使用体验 @SherkeyXD
* 新增资源更新进度显示 @ABA2396
* 肉鸽更新干员招募逻辑 (#13071) @Saratoga-Official
* 显示剩余理智 @hguandl
* warn about known problematic injected dlls @dantmnf
* YostarEN Dahuang City theme @Constrat

### 改进 | Improved

* 更新繁中服線索與部分截圖 (#13094) @momomochi987 @pre-commit-ci[bot]
* SSReopen 使用 XX-OpenOpt 代替 SideStoryReopen 导航 (#13078) @status102
* 减少运行中 Task 修改 @status102

### 修复 | Fix

* 当首选关卡为剿灭时，后续备选关卡和剩余理智出现问题时不会报错  @ABA2396
* 修复因重定向导致 ETag 缓存与原始地址不符的问题 @ABA2396
* 未勾选自动确认 3 星且有对应倾向 3 星 tag 时不会刷新 @ABA2396
* 繁中服無法識別荒蕪拉普蘭德 (#13125) @vonnoq
* 存在更新时问题反馈提示显示不全 @ABA2396
* 规避 WPF 剪贴板卡顿，修复剪切不清除文本的问题。支持 (Rich)TextBox 复制/剪切/粘贴 和 DataGrid 复制 @ABA2396
* 不知道为啥有时候肉鸽难度识别最后会多个0，ui打个补丁 @ABA2396
* 调整 YoStarEN 大荒城公招、抽卡的截图以修复识别失败的问题 (#13117) @gui-ying233
* Toast遮挡右侧界面 @status102
* 引航者试炼#5 TN-1 ~ TN-4地图view[1] & 干员部署修复 @status102
* 肉鸽结算Ocr 非数字替换 @status102
* 写的什么jb导航 @ABA2396
* 肉鸽结算 Ocr bin threshold @status102
* 难度识别出非数字时崩溃 @ABA2396
* Mac Mirror酱更新检测 @hguandl
* YostarEN & YostarJP HS rerun navigation @Constrat
* YostarKR HS rerun navigation (#13124) @HX3N
* Mumu5.0 MumuExtra @Daydreamer114
* no unsigned subtraction @SherkeyXD
* KR/JP SS@Store@Purchase OCR @Daydreamer114
* txwy failed dataload @Constrat

### 文档 | Docs

* 更新 Linux 开发环境配置教程 加入 Mac 开发建议 @zhangweijian97
* 用户文档基建换班添加队列轮换说明 (#13064) @Lemon-miaow @pre-commit-ci[bot]

### 其他 | Other

* 调整单击托盘图标动作，符合常规软件行为 @ABA2396
* 调整命名空间 @ABA2396
* 繁中服_周年月卡領取 &「揭幕者們」活動導航 (#13092) @momomochi987
* 资源日期使用本地时间显示 @ABA2396
* 调整关卡选择中的剿灭显示 @ABA2396
* 显示错误 @ABA2396
* 肉鸽callback中Int类型限定 @status102
* 繁中服「荒原安保派駐」定向導能元件 (#13057) @momomochi987
* remove TaskTransitionVisualizer @SherkeyXD
* use SecureZeroMemory for safety @SherkeyXD
* SSS#7 Dossoles global @Constrat
* meojson update to v4.4.1 & remove json5 (#13079) @status102
