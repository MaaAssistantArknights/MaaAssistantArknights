## v5.16.0

### 基建又改了…… | Highlight

本次版本更新是六周年版本更新第一弹，紧急修复了一些由于新基建等导致的问题。

#### 新的基建

六周年版本对基建的改动主要体现在：一是增加了一列活动室，二是将左上角的“进驻总览”和“建造模式”的背景改为半透明。

我们在这个版本作了适配，并且开始了对基建队列与干员休整功能的初步适配。

如果你在使用基建功能的过程中遇到问题，请及时反馈~

#### 新的活动

这个版本我们适配了六周年活动推荐月卡、幸运墙等活动内容，适配了SideStory「众生行记」导航，应该没有遗漏了的吧（x

#### 新的技能识别

我们更新了牛牛的技能识别模型，应该能更好的处理一些特殊情况，例如血条挡住技能图标之类的。

如果你在使用自动战斗功能的过程中遇到问题，请及时反馈~

----

以下是详细内容：

### 新增 | New

* 特殊月卡扩大识别范围 @SherkeyXD
* 幸运墙适配六周年 @SherkeyXD
* SideStory「众生行记」导航 @SherkeyXD
* 更新技能识别模型，采用更全面的数据集训练 (#12490) @Plumess
* 适配基建队列轮换与干员休整 (#11252) @Lemon-miaow @ABA2396
* 优化技能识别模型 (#11984) @Plumess @ABA2396

### 改进 | Improved

* 优化训练室和加工站进站逻辑 @ABA2396
* 修改小游戏描述 @Daydreamer114

### 修复 | Fix

* 适配新基建 (#12500) @ABA2396
* 修正送抽任务 @SherkeyXD
* CheckLevelMax too fast without delay (#12498) @BxFS
* 构建日期显示错误 @ABA2396
* 下载源选择 MirrorChyan 时如果关闭自动下载，手动点击软件更新无提示 @ABA2396
* RecruitSupportOperator 拼写错误 @ABA2396
* vc++ 检测移动至文件检测后 @ABA2396
* EN RoutingRefreshNodeConfirm roi too small (#12491) @BxFS
* EN RoutingRefreshNode dimension too large (#12484) @BxFS
* KR RoutingRefreshNode dimension too large (#12487) @HX3N

### 文档 | Docs

* 移除过期描述 @ABA2396

### 其他 | Other

* manually update version.json @Constrat
* Manual update resources @Constrat
* 不启用外部通知时隐藏可选项 @ABA2396
* ignore templates for global @Constrat
