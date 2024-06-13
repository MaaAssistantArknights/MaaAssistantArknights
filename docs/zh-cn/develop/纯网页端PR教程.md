---
order: 3
icon: mingcute:git-pull-request-fill
---

# 纯网页端 PR 教程

牛牛也能看懂的 GitHub Pull Request 使用指南 (\*´▽ ｀)ノノ

::: warning
本教程中对很多概念进行了简化，为了让更多朋友能实际用起来，甚至有一些很不优雅~~但是简单~~的操作，还有一些不那么正确的解释，还请大佬们轻喷。  
若您有一定的 git 使用经验及编程基础，~~那你还看个~~🔨，可以看稍微进阶一点的教程 [Github Pull Request 流程简述](./开始开发.md#github-pull-request-流程简述)
:::

## 基本概念及名词解释

这一章节内容略微枯燥，不感兴趣可以直接跳过到下面实操部分，有不理解的再回来看

### Repository（仓库）

简称 repo，存放我们代码及其他资源文件的地方

👇 可以简单理解为当前这个网页及里面所有的内容，就是 MAA 的仓库（我们一般称之为 MAA 的主仓库）

![image](https://user-images.githubusercontent.com/18511905/193747349-5964bd12-de3c-4ce7-b444-29b0bd104acc.png)

### Fork（复制）

复制，字面意思，将 MAA 的代码复制一份，然后可以进行后续修改等等的操作，避免把原来的弄坏了  
但一般说中文“复制”我们可能首先想到的是 copy 的意思，fork 也没有其他明确的翻译，所以我们一般习惯直接说英文，比如“把代码 fork 一份走”

既然是复制后的，那就是 `MAA (1)`（bushi）  
为了和原本的仓库区分开，所以我们一般将原本的 MAA 仓库称为 “主仓库”、“upstream（上游仓库）”、“origin（原仓库）”；  
因为每个人都可以自己复制一份走，所以复制后的称之为“个人仓库”

![image](https://user-images.githubusercontent.com/18511905/193750507-b8167df5-7a70-48d4-ba69-5dda8327e8ec.png)

### Pull Request（拉取请求、合并请求）

简称 PR，“拉取请求”这个太直译了，听起来很奇怪，~~而且字太多了打起来太累~~，所以也是一样的大家一般就直接说：“来个 PR”  
书接上文，你 fork（复制）的个人仓库，修改完了，怎么把内容提供给主仓库呢？这时候我们就可以开一个 PR，申请将自己修改的内容加入到主仓库中。

当然啦，既然是“请求”，那自然是需要审批的，MAA Team 的各位可能会针对你的修改提一些意见等，当然我们的意见也不一定完全正确，大家合理讨论~

👇 下面的是目前大佬们提的 PR，正在等待审批的

![image](https://user-images.githubusercontent.com/18511905/193750539-9106d425-2087-4116-a599-61904690718b.png)

### Conflict（冲突）

假设一下，主仓库中有个 A 文件，它的内容是 111  
你 fork 了一份，将其内容改成了 222，但是你刚准备提交 PR，这时候张三也 fork 了一份并提交了 PR，并将 A 文件改成了 333  
这时候我们就会看到，你俩都修改了 A 文件，并且修改的不同，那听谁的好呢？这就是 Conflict（冲突）  
冲突解决起来比较麻烦，这里仅阐述概念，方便实际遇到时能理解发生了什么，暂不阐述解决方案

## 纯网页端 PR 操作全流程

1. 首先进入 MAA 主仓库，fork 一份代码

   ![~ NKHB1CIE8`G(UK@ %3`H](https://user-images.githubusercontent.com/18511905/193751017-c052c3d4-fe77-433c-af21-eb8138f4b32e.png)

2. 把 “仅 master 分支” 这个选项去掉，然后点击 Create Fork

   ![20221004144039](https://user-images.githubusercontent.com/18511905/193751300-ba9890fd-0916-4c85-8a46-756e686608b1.png)

3. 接下来来到了你的个人仓库，可以看到标题是 “你的名字/MaaAssistantArknights”，下面一行小字 forked from MaaAssistantArknights/MaaAssistantArknights （复制自 MAA 主仓库）

   ![image](https://user-images.githubusercontent.com/18511905/193751864-0d2d0caf-b5ef-4c91-9331-d9827f23f36b.png)

4. 切换到 dev 分支（分支这个概念和本文关系不大，~~其实是我懒得写~~，有兴趣可以搜索了解下，这里只需要这样操作即可，暂时不用关心原理）

   ![image](https://user-images.githubusercontent.com/18511905/193752379-90d5b317-b1aa-4563-b8b0-583c78373f9b.png)

5. 找到你要改的文件，可以点 “Go to file” 进行全局搜索，也可以直接在下面的文件夹里翻（如果你知道文件在哪的话）

   ![image](https://user-images.githubusercontent.com/18511905/193752691-7102a405-dc08-4dce-9617-7f862b0b32b9.png)

6. 打开文件后，直接点击文件右上角的 ✏️ 进行编辑

   ![image](https://user-images.githubusercontent.com/18511905/193752862-a9cf6019-b363-4c22-b7c7-35f4aca7377f.png)

7. 开改！（当然如果是资源文件这种，我们建议先在你电脑上的 MAA 文件夹里测试修改，确认没问题了再粘贴到网页上，避免改错了）
8. 改完了，翻到最下面，写一下你改了啥

   ![image](https://user-images.githubusercontent.com/18511905/193754154-b21f4176-1418-49c8-87a3-dab088868fdc.png)

9. 还有第二个文件要改的？改完了发现弄错了想再改改？都没关系！重复 5-8 即可！
10. 全改好了进行 PR ！直接点 **个人仓库** 里的 Pull Request 标签页  
    如果有 Compare & Pull Request 按钮，那最好，直接点他！如果没有也不用着急，点下面的 New Pull Request 也是一样的（请看步骤 11）

    ![image](https://user-images.githubusercontent.com/18511905/193755450-59137215-4e0b-4eca-9ec9-8b35b52cd5ff.png)

11. 这时候来到了主仓库，请核对一下你要 PR 的是否确认。  
    如图中，中间有个向左的箭头，是将右边的的 个人姓名/MAA 的 dev 分支，申请合并到 主仓库/MAA 的 dev 分支。  
    然后写一下标题，你要修改了的具体内容等等的，然后点确认

    ![20221004151004](https://user-images.githubusercontent.com/18511905/193756875-556df699-96b3-411f-815e-47050e283f4d.png)

12. 等待 MAA Team 的大佬们审核吧！当然他们也可能会提意见  
    👇 比如（纯属娱乐切勿当真）
    ![image](https://user-images.githubusercontent.com/18511905/193757006-75170e78-4c8d-4cd2-b8eb-ca590ea7aa50.png)

13. 如果大佬们说要再修改一些小问题的话，回到 **你的个人仓库**，切换到先前的 dev 分支，重复 步骤 3-9 即可！  
    注意不需要操作步骤 2（重新 fork），也不需要操作步骤 10（重新 Pull Request），你当前的 Pull Request 仍处于待审核状态，后续的修改会直接进入到这个 Pull Request 中  
    👇 比如可以看到最下面多了一条“重新修改演示”的内容
    ![image](https://user-images.githubusercontent.com/18511905/193757668-4064273c-576d-4259-bbaa-e9f65ae486c1.png)

14. 等大佬们审批通过，就全部完成啦！**版本发布后**，你的 GitHub 头像将会自动进入到贡献者列表名单中，非常感谢各位的无私奉献！  
    ~~怎么全是二次元啊，哦我也是啊，那没事了~~
    ::: tip 贡献/参与者
    感谢所有参与到开发/测试中的朋友们，是大家的帮助让 MAA 越来越好！ (\*´▽ ｀)ノノ

    [![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=114514&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
    :::

15. 下次如果还想提别的 PR，请先切换到 dev 分支，然后直接如下图操作  
    ::: warning
    这个操作会强制将你的个人仓库同步到和主仓库一模一样的状态，这是最简单粗暴但行之有效的解决冲突的方法。但如果你的个人仓库已经有额外的编辑了，会被直接删掉！
    :::
    如果确定不会造成冲突，请使用右侧绿色的 `Update Branch` 按钮  

    如果你不清楚/不 care 我上面说的这一大堆，也请点击左侧的按钮

    ![image](https://user-images.githubusercontent.com/18511905/194709964-3ea0d5b0-1bfe-4d0e-a1dc-bf4f735af655.png)

    接着重复步骤 3-14，修改、提 PR 即可~
