## v4.22.0-beta.1

### 新增

- 萨米不再进入部分不期而遇战斗 (#5890) @cenfusheng
   - 萨米不再进入三个不期而遇战斗 @cenfusheng
- 肉鸽开局助战or自选干员后不再要求选用start属性干员 (#5824) @LingXii
   - 肉鸽开局助战or自选干员后不再要求选用start属性干员 @LingXii
- 为文档添加docsearch和sitemap支持 (#5785) @SherkeyXD
   - 恢复本地构建时删除的tsconfig内容 @SherkeyXD
   - 添加sitemap支持 @SherkeyXD
   - 优化文档的gitignore @SherkeyXD
   - 使用docsearch作为文档搜索插件 需要跟进日语/繁体/韩语翻译 @SherkeyXD
- 增加强制替换adb的信息提示，增加异常处理 (#5838) @ABA2396
   - 增加强制替换adb的信息提示，增加异常处理 @ABA2396
- 傀影肉鸽适配更多干员 (#5817) @cenfusheng
   - 增加傀影肉鸽干员适配 @cenfusheng
   - 增加傀影肉鸽对不同干员的适配 @cenfusheng
- feat：适配萨米肉鸽关卡“豪华车队” (#5798) @cenfusheng
   - feat：适配萨米肉鸽关卡“豪华车队” @cenfusheng
- 自动检测特定目录下的bluestacks.conf (#5751) @Gliese129
   - 自动检测特定目录下的bluestacks.conf @Gliese129

### 改进

- 更新繁中服肉鴿不期而遇 ocrReplace (#5882) @momomochi987
   - 調整縮排為 4 空白 @momomochi987
   - 調整縮排 @momomochi987
   - 更新不期而遇的 ocrReplace @momomochi987

### 修复

- fix#根据反馈调整Qodana 忽略特定的警告 (#5867) @hxdnshx
   - add single line ignore @hxdnshx
   - fix#根据反馈调整Qodana 忽略特定的警告 @hxdnshx
- d0dc294 auto update version issue @Constrat
- more long names regex for OperBox @Constrat
- ROI for BattleStartOCR, new IMG+ROI for Oper @Constrat
- OperBox regex, BattleMatcher regex, longname and IS @Constrat
- 强制替换adb报错 @ABA2396
- 修复纯烬艾雅法拉识别 (#5791) @Black1312
- 修正繁中服水月肉鴿會卡在 "緊急運輸" 的問題 (#5790) @momomochi987
   - 新增水月肉鴿不期而遇關卡名稱的 ocrReplace @momomochi987

### 其他

- Auto Update Game Resources - 2023-08-08 @MistEO
- Merge branch 'dev' of https://github.com/MaaAssistantArknights/MaaAssistantArknights into dev @Lancarus @javilak @MistEO @Large-Japchae
   - 按照竹雨佬的意见修改了部分肉鸽文档 (#5873) @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - doc：update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - Update 3.5-肉鸽辅助协议.md @Lancarus
      - 3.5-肉鸽辅助协议.md @Lancarus
      - 根据文档设置最后两个分组 @Lancarus
      - 修复傀影肉鸽中百家不选2技能 @Lancarus
   - 修复了傀影肉鸽的一些错误,更新了肉鸽协议 (#5859) @Lancarus
   - 修正 zh-cn.xaml 文本错误 (#5861) @javilak
      - Update zh-cn.xaml @javilak
   - 拆分所有肉鸽资源文件 (#5856) @MistEO
   - Goldenglow(澄闪) & Surtr(史尔特尔) ocrReplace (#5852) @Large-Japchae
- modified text for EX stages (WB event) @Constrat
- CI-在静态分析中增加基线 (#5757) @hxdnshx
   - move qodana baseline to .github directory @hxdnshx
   - add baseline analyze @hxdnshx
- typo: 2.2-DEVELOPMENT.md @Constrat
- Revert "feat:自动检测特定目录下的bluestacks.conf" (#5794) @MistEO
   - Revert "feat:自动检测特定目录下的bluestacks.conf" @MistEO
- typo of 1.3-模拟器支持.md (#5802) @BlueandwhiteXD
   - Update 1.3-模拟器支持.md @BlueandwhiteXD
- same as 0b03b68 @Constrat
- SSS Identification issue with NON-1080 emulators @Constrat
