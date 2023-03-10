## v4.12.0-beta.3


### 改进

- 优化战斗中技能识别，改用深度学习模型 (#3918) @MistEO
- 隐藏生息演算入口 @WLLEGit

### 修复

- 修复 CF-9 地图格子错误 @yuanyan3060
- 修复手动检查更新重启后不会打开 Changelog 弹窗的问题 @zzyyyl
- 修复 WpfGui 自动更新时 x64 也下载 arm64 的 ota 的问题 (#3921)  @zzyyyl

### 其他

- simplify release-nightly-ota workflow @horror-proton
- push tag once in release-nightly-ota workflow @horror-proton
- 优化 Changelog 生成器 @zzyyyl
- 优化 C# 部分代码 @ABA2396
- skip ota build for empty pack @horror-proton
- fix CI MaaDeps download (#3884) @MistEO @horror-proton @dantmnf
- add support for find ONNXRuntime @aa889788

### For develops


- 兜住某些情况下 tasks.json 解析炸了的异常 @zzyyyl
- 修复某些情况下 maadeps-download.py 抛异常的问题 @zzyyyl
