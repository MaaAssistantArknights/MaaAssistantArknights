## v5.16.2

### 比详细内容还要长的 | Highlight

#### 一份时间，六份收益

在这个版本，我们将会带来自适应调整连战次数功能，这个功能可以让牛牛充分利用六周年版本的代理指挥功能更新。

简单来说，就是当`刷理智`功能的`连战次数`设置为「AUTO」时，牛牛会尽可能保持6连战，仅在最后一次理智不足时（包括达到吃药设定上限无法继续补充时）动态调整连战次数。

> 典型流程示例：  
> 当前0理智，关卡消耗21理智（6连战需126理智）
> 
> 情况1：有2瓶80理智药，自动吃掉2瓶（+160）不触发调整 → 保持6连战  
> 情况2：有2瓶80理智药，设置只吃1瓶 吃1瓶（+80）达到吃药上限，关闭窗口 触发调整 → 自动调整为3连战  
> 情况3：有1瓶80理智药，自动吃掉1瓶（+80）无药可吃 触发调整 → 自动调整为3连战

**不过，目前有个已知问题，当已有较多理智的同时设置了较大的【吃理智药】【吃源石】次数，在打消耗理智较多的关卡时，容易产生理智溢出现象，即使开启了博朗台模式。**

> 例如，当前理智为 100，设置了吃理智药，且仅剩 120 理智药，在打消耗 21 理智的关卡时，容易出现：  
> 由于连打 6 次关卡需要 126 理智，当前理智不足，为了最大化利用连战次数，将会吃下 120 理智药，导致当前理智变为 220，产生溢出

不过一般来说，这种情况至多仅会浪费 1 ~ 2 点理智，影响不大。

如果您遇到任何意外行为，请通过【设置】→【问题反馈】生成日志并提交给我们~

----

#### One Run, Sixfold Rewards

**!!! CN ONLY !!!**

This update introduces an adaptive adjustment feature for `Combat`, enabling MAA to fully take advantage of the 6th Anniversary upgrade to the Auto Deploy system.

In simple terms, when the `Series` setting for `Sanity Farming` is set to 「AUTO」, MAA will:

1. Always attempt to maintain 6-repeat combat by default  
2. Only adjust the repeat count when:  
   - Sanity is insufficient for the full run  
   - Potion or Originium usage has reached your configured limit  

> Typical scenario examples:  
> Current Sanity: 0, Stage cost: 21 (6 repeats require 126 Sanity)  
>
> Case 1: Two 80-Sanity potions available, auto-consumes both (+160) → Maintains 6 repeats  
> Case 2: Two 80-Sanity potions available, but limit set to 1 → Consumes 1 (+80), hits cap → Auto-adjusts to 3 repeats  
> Case 3: One 80-Sanity potion available → Auto-consumes it (+80), no more available → Auto-adjusts to 3 repeats

**Note:** There is a known issue where, if you already have a high amount of Sanity and have allowed high potion/originium usage, running high-cost stages may cause Sanity overflow — even if *Dr. Grandet Mode* is enabled.

> Example: Current Sanity is 100, potion usage is enabled, and only 120 Sanity worth of potions remain.  
> To run a 21-cost stage 6 times (requires 126 Sanity), MAA will use all 120, raising your Sanity to 220 and causing overflow.

In most cases, this may result in wasting just 1–2 Sanity points, which has minimal impact.

If you experience any unexpected behavior, please use *Settings* → *Feedback* to generate a log archive and send it to us for troubleshooting.

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
