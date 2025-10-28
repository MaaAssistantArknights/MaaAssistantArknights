## v5.27.0-beta.2

### 新增 | New

* 增加对界园肉鸽 DLC 1 三个新分队的支持 @Alan-Charred
* 界园肉鸽不期而遇实现选项识别 (#14540) @Alan-Charred
* 增加纯数色匹配 (#14536) @ABA2396
* 添加主线 16 章导航 @ABA2396
* 支持生息演算与肉鸽遇到不认识的主题时自动切换 @ABA2396
* 添加 生活队凹开局密文板 与 坍缩范式列表 TooltipBlock @ABA2396
* core 异常退出 ui 添加日志记录 @ABA2396

### 改进 | Improved

* 主线导航目标关卡默认在屏幕内时不再划到最右边后往前寻找 @ABA2396
* 调整雷电截图增强提示，强制要求模拟器分辨率为横屏模式 @ABA2396
* 调整连接失败尝试启动模拟器的描述 @ABA2396

### 修复 | Fix

* 无法加载 cache 资源 @ABA2396
* 改用 CustomeSkip 来跳过新时装动画 (#14566) @Alan-Charred
* 调换使用错误的排序函数 (#14555) @Alan-Charred
* 给生息演算点击 "开始行动" 的任务加点延迟，防止错过确认弹窗 @Alan-Charred
* 猪玛写 switch 不写 break @ABA2396
* 齐聚主题无法导航肉鸽和生息演算 @Saratoga-Official
* ja-jp 泰拉大陆调查团 识别错误 @ABA2396
* ocr for Executor the Ex Foedere for EN @Constrat

### 文档 | Docs

* 更新 Windows 模拟器文档，调整支持列表并添加详细说明 (#14534) @AnnAngela
* 更正文档编写指南的一处typo @lucienshawls
* 对 codespace 相关链接进行小修小补 (#14564) @lucienshawls
* 接入 DeepWiki (#14563) @lucienshawls
* 全语言开发文档章节整理 (#14562) @MistEO
* 把 codespace 移到犄角旮旯 (#14560) @MistEO

### 其他 | Other

* 移除 devcontainer 轻量环境的部分非必要依赖 (#14499) @lucienshawls
* 重复访问好友添加 Ocr 兜底 @Saratoga-Official
* 添加 png 后缀 @SherkeyXD
* 增加 wpf 项目cmake build脚本 @status102
* ui 日志增加 adb devices 输出 @ABA2396
* 基建降低会清空其他干员效率的技能优先级 @ABA2396
* polish PixelAnalyzer a bit (#14538) @Alan-Charred
