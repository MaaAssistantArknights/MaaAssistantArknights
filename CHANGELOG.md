## v5.16.2

### Highlight

#### 一份时间，六份收益

在这个版本，我们带来了智能连战次数调整功能。当`刷理智`功能的`连战次数`设置为「AUTO」时，牛牛会尽可能保持6连战，仅在最后一次理智不足时（包括达到吃药设定上限无法继续补充时）动态调整连战次数。

> 典型流程示例：  
> 当前0理智，关卡消耗21理智（6连战需126理智）
> 
> 情况1：有2瓶80理智药
> - 自动吃掉2瓶（+160）
> - 不触发调整 → 保持6连战
> 
> 情况2：有2瓶80理智药，设置只吃1瓶
> - 第一次吃1瓶（+80）
> - 第二次达到吃药上限，关闭窗口
> - 触发调整 → 自动调整为3连战
> 
> 情况3：有1瓶80理智药
> - 自动吃掉1瓶（+80）
> - 无药可吃时
> - 触发调整 → 自动调整为3连战

**不过，目前有个已知问题，当已有较多理智的同时设置了较大的【吃理智药】【吃源石】次数，在打消耗理智较多的关卡时，容易产生理智溢出现象，即使开启了博朗台模式。**

> 例如，当前理智为 100，设置了吃理智药，且仅剩 120 理智药，在打消耗 21 理智的关卡时，容易出现：  
> 由于连打 6 次关卡需要 126 理智，当前理智不足，为了最大化利用连战次数，将会吃下 120 理智药，导致当前理智变为 220，产生溢出

不过一般来说，这种情况至多仅会浪费 1 ~ 2 点理智，影响不大。

如果您遇到任何意外行为，请通过【设置】→【问题反馈】生成日志并提交给我们~

----

#### One Run, Sixfold Reward

**!!! CN ONLY !!!**

This version introduces an intelligent repeat count adjustment feature. When you set the `Series` to 「AUTO」, MAA will:

1. Always attempt to maintain 6-repeat combat by default
2. Only adjust the count when:
   - Insufficient Sanity for the next run
   - Reached the potion/originium usage limit

> Typical workflow examples:  
> Current Sanity: 0, Stage cost: 21 (6 repeats require 126 Sanity)
>
> Case 1: Two 80-Sanity potions available
> - Auto-consume both (+160)
> - No adjustment needed → Maintains 6 repeats
>
> Case 2: Two 80-Sanity potions available, but usage limited to 1
> - First consume 1 potion (+80)
> - Reaches usage limit, closes potion window
> - Triggers adjustment → Auto-adjusts to 3 repeats
>
> Case 3: One 80-Sanity potion available
> - Auto-consumes the potion (+80)
> - No more potions available
> - Triggers adjustment → Auto-adjusts to 3 repeats

**However, there is a known issue: when you already have a high amount of Sanity and have set high usage for *Sanity Potions* or *Originium*, running high-cost stages can sometimes cause Sanity overflow — even with Dr. Grandet Mode enabled.**

> For example, if your current Sanity is 100, you’ve enabled potion usage, and you only have 120 Sanity worth of potions left, then:  
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
