## v5.16.2

### 一份时间，六份收益 | Highlight

在这个版本，我们将会带来自适应调整连战次数功能，这个功能可以让牛牛充分利用六周年版本的代理指挥功能更新。

简单来说，就是当你将【刷理智】功能的【连战次数】设置为“AUTO”后，牛牛将会自动调整游戏内连战次数。

并且这个调整将会结合你设置的【吃理智药】次数和【吃源石】次数，实现尽可能多地使用连战。

这样，就能在刷 1-7 或者无限吃 48 小时内过期的理智药等情况下，节约大量时间。

由于这个改动影响较大，可能产生 bug，欢迎你在遇到问题时使用【设置】的【问题反馈】功能生成日志压缩包，并将其反馈给我们~

----

以下是详细内容：

### 新增 | New

* 新增 AUTO 选项 @ABA2396
* 实现自适应调整连战次数 (#12555) @Yoak3n @ABA2396 @pre-commit-ci[bot]

### 改进 | Improved

* 优化干员识别 (#12562) @ABA2396

### 修复 | Fix

* correct variable name typo in cur_score (#12552) @ninekirin
* 无法进入中枢 @ABA2396
* 基建选人前收起职业栏 (#12556) @ABA2396 @pre-commit-ci[bot]

### 文档 | Docs

* Update CHANGELOG.md @ABA2396

### 其他 | Other

* english clarification for operbox) @Constrat
* 修改错误提示 @ABA2396
* YostarKR Infrast and texts (#12561) @HX3N
* add templates for #12556 and i18n for minigame @Constrat
* YostarJP infrast and texts (#12559) @Manicsteiner
* update ignore templates @Constrat
