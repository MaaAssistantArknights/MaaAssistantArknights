## v4.12.0

### 新增

- 新增 Windows ARM64 支持 (#3346) @dantmnf @horror-proton @hguandl @zzyyyl
- 新增给作业点赞的功能 (#3719) @MistEO @ABA2396
- 增加 “更新日志” 按钮 @zzyyyl
- 添加连接历史记录的删除按钮 (#3899) @ABA2396
- 新增游戏掉线时不重连选项 (#3839) @KevinT3Hu

### 改进

- 优化战斗中技能识别，改用深度学习模型 (#3918) @MistEO
- 更新游戏资源 2023/3/7 @MistEO @yuanyan3060
- 隐藏生息演算入口 @WLLEGit @hguandl
- 优化 changelog 弹窗 (#3882) @zzyyyl
- 新增 dll not found 错误处理 @ABA2396
- 错误处理界面添加常见问题链接 @ABA2396
- 更新内置基建作业文件 @yd278
- 加速肉鸽招募时滑动 @zzyyyl
- 自动公招时增加掉线识别 @DaysGone233
- 添加 4 点检查 web 是否更新关卡 @ABA2396
- 优化退出模拟器操作，添加超时时间 @ABA2396
- 更新抄作业界面小提示 @MistEO @ABA2396

### 修复

- 修复 MacGui 基建设施选择 @hguandl
- 修复 肉鸽助战 招募后卡在干员立绘界面的问题 @WLLEGit
- 修复 levels.json 读取错误 @horror-proton
- 修复部分关卡结算界面的问题 @zzyyyl
- 修复肉鸽招募有时漏干员的问题 @zzyyyl
- 宿舍填充蹭信赖干员时，可能挡住名字无法识别的问题 @fearlessxjdx
- 修复窗口最小化时不能通过托盘修改语言的问题 @ABA2396
- 未点开设置时无法保存窗口位置 @ABA2396
- 修复肉鸽助战刷新问题 @WLLEGit
- 修复重岳的识别错误 @cenfusheng
- 修复自动作战，部分地图中干员技能无法开启的问题 @MistEO
- 修复“借助战一日只执行一次”没有生效的问题。 (#3940) @Hydrogina @ABA2396

### 其他

- 为所有资源添加 OTA 支持 (#3872) @liuyifan-eric
- Changelog 生成器 (#3903) @zzyyyl
- 更新内测版构建脚本 @horror-proton
- OcrReplace 部分代码重构 @liuyifan-eric
- 增加任务的表达式计算 @zzyyyl
- 修改资源文件中换行的写法 @ABA2396
- 修改调试版本解析问题 @ABA2396
- 简化关卡导航写法 @ABA2396
- 格式化代码 @zzyyyl @yd278 @ABA2396 @MistEO
- 更新文档 @MistEO @ABA2396 @zzyyyl
- 解决 PUBLIC_HEADER 的 Warning @hguandl

### For overseas

#### Common

- Supported the use of medicine that is about to expire. @liuyifan-eric
- Localized for the release note view. @liuyifan-eric
- Added help links for overseas clients. @liuyifan-eric @ABA2396

#### YostarJP / Japanese

- Added selection support unit for IS function. @liuyifan-eric
- Fixed DH-9 navigation. @liuyifan-eric
- Fixed IS function stuck of some levels. @liuyifan-eric
- Fixed operator name OCR error. @liuyifan-eric
- Fixed client language file error. @ABA2396
- Blue-ray items. @wallsman
- JP 48h medicine recognition issue @liuyifan-eric
- Update JP 翠玉の夢 and add Operators (#3949) @wallsman

#### YostarKR

- Fixed typos of resource file. @ABA2396

### For develop

- 第三方依赖重构 (#3346)，请使用 `maadeps-download.py` 脚本下载项目依赖 @dantmnf @horror-proton @hguandl
- 增加 CustomTask，可以执行 tasks.json 内任意一个 task (#3930) @yuanyan3060
- Python 添加 Hyperv 蓝叠的端口获取与模拟器启动函数 @WLLEGit
- 默认关闭 adb-lite @MistEO
- use maatouch in python script by default @horror-proton
- 增加了一个分析 task 转移关系画图的脚本 (#3901) @LCAR979
- 添加坐标工具 @liuyifan-eric
- add fallback for python library search, etc. @horror-proton
- add new functions to the coordinate tool @liuyifan-eric
- 兜住某些情况下 tasks.json 解析炸了的异常 @zzyyyl
