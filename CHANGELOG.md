## v4.17.0-rc.1

### 改进

- 调整 Hyper-V 蓝叠 地址检测逻辑，需勾选 `自动检测` + `每次检测` @MistEO
- ADB 替换失败则临时用本地的 @MistEO

### 修复

- 更新自动战斗点赞反馈提示语 (#4776) @UniMars
- 修复关卡 `消息传递`、窒息` 识别错误 (#4781) @cenfusheng
- 为 Resource loader 添加重入锁 @MistEO
- 修复正式版不显示 release note的问题 @MistEO
- 修复 CI release 不更新 version api 的问题 @MistEO

### 其他

- 修复繁中资源，冒烟测试添加所有外服资源检查 @MistEO
- 为冒烟测试的 nuget 添加重试 @MistEO
- ~~小水管爆了~~ 不再从 OTA 站下载更新包 @MistEO
- 更新文档、界面文字等 @MistEO
- 添加关于 MuMu 模拟器 12 的说明 (#4793) @AnnAngela
- Doc : JP シラクザーノ (#4782) @wallsman
- 调整冒烟测试报错时机 @MistEO
