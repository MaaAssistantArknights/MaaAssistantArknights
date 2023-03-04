## v4.12.0-beta.1

### 新增

<!-- 新功能 / New features -->

N/A

### 改进

<!-- 性能或行为改进 / Performance or behavior improvements -->

- 更新内置基建作业文件 @yd278
- 加速肉鸽招募时滑动 @zzyyyl
- 自动公招时增加掉线识别 @DaysGone233
- 添加 4 点检查 web 是否更新关卡 @ABA2396
- 优化退出模拟器操作，添加超时时间 @ABA2396
- 更新抄作业界面小提示 @MistEO @ABA2396

### 修复

<!-- 问题修复 / Bug fixed -->

- 修复部分关卡结算界面的问题 @zzyyyl
- 修复肉鸽招募有时漏干员的问题 @zzyyyl
- 宿舍填充蹭信赖干员时，可能挡住名字无法识别的问题 @fearlessxjdx
- 修复窗口最小化时不能通过托盘修改语言的问题 @ABA2396
- 未点开设置时无法保存窗口位置 @ABA2396
- 修复肉鸽助战刷新问题 @WLLEGit
- 修复重岳的识别错误 @cenfusheng

### 其他

<!-- 用户无感知修改 / User-unaware changes -->

- 涉及干员名称的 ocrReplace 更改为替换整个 str @liuyifan-eric
- 增加任务的表达式计算 @zzyyyl
- 实验性增加 OCR 字符等价类支持 @horror-proton @liuyifan-eric
- 修改资源文件中换行的写法 @ABA2396
- 添加坐标工具 @liuyifan-eric
- 修改调试版本解析问题 @ABA2396
- 简化关卡导航写法 @ABA2396
- 修复 README 瑕疵 @zzyyyl
- 直接调用 get_instance 而不是使用静态的 OcrData @liuyifan-eric
- 格式化代码 @zzyyyl @yd278 @ABA2396
- 更新常见问题链接 @ABA2396

### For overseas

#### Common

- Supported the use of medicine that is about to expire. @liuyifan-eric
- Localized for the release note view. @liuyifan-eric
- Added help links for overseas clients. @liuyifan-eric @ABA2396

#### YostarJP / Japanese

- Fixed DH-9 navigation. @liuyifan-eric
- Added selection support unit for IS function. @liuyifan-eric
- Fixed IS function stuck of some levels. @liuyifan-eric
- Fixed operator name OCR error. @liuyifan-eric
- Fixed client language file error. @ABA2396
- Blue-ray items. @wallsman

#### YostarKR

- Fixed typos of resource file. @ABA2396

### For develop

- Python 添加 Hyperv 蓝叠的端口获取与模拟器启动函数 @WLLEGit
