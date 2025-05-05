## v5.16.2

### Highlight

#### 一份时间，六份收益

在这个版本，我们将会带来自适应调整连战次数功能，这个功能可以让牛牛充分利用六周年版本的代理指挥功能更新。

简单来说，就是当你将【刷理智】功能的【连战次数】设置为“AUTO”后，牛牛将会在触发【吃理智药】【吃源石】时自动调整游戏内连战次数到可能的最大值，实现尽可能多地使用连战。

> 例如，当前理智为 14 ，触发【吃理智药】吃下 80 理智药后，当前理智变为 94，打消耗 21 理智的关卡时，牛牛会自动调整连战次数为 4 次。

这样，就能在刷 1-7 或者无限吃 48 小时内过期的理智药等情况下，节约大量时间。

**不过，目前有个已知问题，当已有较多理智的同时设置了较大的【吃理智药】【吃源石】次数，在打消耗理智较多的关卡时，容易产生理智溢出现象，即使开启了葛朗台模式。**

> 例如，当前理智为 100，设置了吃理智药，且仅剩 120 理智药，在打消耗 21 理智的关卡时，容易出现：
> 
> 由于连打 6 次关卡需要 126 理智，当前理智不足，为了最大化利用连战次数，将会吃下 120 理智药，导致当前理智变为 220，产生溢出

不过一般来说，这种情况至多仅会浪费 1 ~ 2 点理智，影响不大。

由于这个改动影响较大，可能产生 bug，欢迎你在遇到问题时使用【设置】的【问题反馈】功能生成日志压缩包，并将其反馈给我们~

----

#### One Run, Sixfold Reward

**!!! CN ONLY !!!**

This version introduces an adaptive adjustment feature for *Continuous Combat*, allowing MAA to fully utilize the 6th Anniversary improvements to Auto Deploy Command.

In simple terms, when you set the *Sanity Farming* feature’s *Continuous Combat Count* to "AUTO", MAA will automatically adjust the in-game repeat count based on how much Sanity you have after consuming *Sanity Potions* or *Originium*.

> For example, if your current Sanity is 14 and you use a Sanity Potion worth 80, it increases to 94. When running a stage that costs 21 Sanity, MAA will automatically adjust the repeat count to 4.

This can save a great deal of time — for instance, when farming 1-7 or trying to quickly use up Sanity Potions before they expire within 48 hours.

**However, there is a known issue: when you already have a high amount of Sanity and have set high usage for *Sanity Potions* or *Originium*, running high-cost stages can sometimes cause Sanity overflow — even with Dr. Grandet Mode enabled.**

> For example, if your current Sanity is 100, you’ve enabled potion usage, and you only have 120 Sanity worth of potions left, then:
> 
> Running a 21-cost stage 6 times would require 126 Sanity. Since you don’t have enough, MAA will use the full 120 from potions, bringing your Sanity to 220 and causing overflow.

In most cases, this will waste only 1–2 Sanity points, so the impact is minimal.

Because this is a significant feature change, it may introduce bugs. If you encounter any issues, please use the *Settings* → *Feedback* feature to generate a log archive and report any issues to us for troubleshooting.

----

以下是详细内容：
Changelog below:

### 新增 | New

* 新增 AUTO 选项 @ABA2396
* 实现自适应调整连战次数 (#12555) @Yoak3n @ABA2396

### 改进 | Improved

* 优化干员识别 (#12562) @ABA2396

### 修复 | Fix

* 无法进入中枢 @ABA2396
* 基建选人前收起职业栏(工作中干员被拉入宿舍) (#12556) @ABA2396
* correct variable name typo in cur_score (#12552) @ninekirin

### 其他 | Other

* 修改错误提示 @ABA2396
* english clarification for operbox @Constrat
* add templates for #12556 and i18n for minigame @Constrat
* YostarKR Infrast and texts (#12561) @HX3N
* YostarJP infrast and texts (#12559) @Manicsteiner
* update ignore templates @Constrat
