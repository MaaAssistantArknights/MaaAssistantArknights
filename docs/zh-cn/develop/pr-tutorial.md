---
order: 3
icon: mingcute:git-pull-request-fill
---

# 纯网页端 PR 教程

牛牛也能看懂的 GitHub Pull Request 使用指南 (\*´▽ ｀)ノノ

::: warning
本教程中对很多概念进行了简化，为了让更多朋友能实际用起来，甚至有一些很不优雅~~但是简单~~的操作，还有一些不那么正确的解释，还请大佬们轻喷。  
若您有一定的 git 使用经验及编程基础，~~那你还看个 🔨~~，可以看稍微进阶一点的教程 [GitHub Pull Request 流程简述](./development.md)
:::

## 基本概念及名词解释

这一章节内容略微枯燥，不感兴趣可以直接跳过到下面实操部分，有不理解的再回来看

### Repository（仓库）

简称 repo，存放我们代码及其他资源文件的地方

👇 可以简单理解为当前这个网页及里面所有的内容，就是 MAA 的仓库（我们一般称之为 MAA 的主仓库）

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/repository-light.png',
    dark: 'images/zh-cn/pr-tutorial/repository-dark.png'
  }
]" />

### Fork（复制）

复制，字面意思，将 MAA 的代码复制一份，然后可以进行后续修改等等的操作，避免把原来的弄坏了  
但一般说中文“复制”我们可能首先想到的是 copy 的意思，fork 也没有其他明确的翻译，所以我们一般习惯直接说英文，比如“把代码 fork 一份走”

既然是复制后的，那就是 `MAA (1)`（bushi）  
为了和原本的仓库区分开，所以我们一般将原本的 MAA 仓库称为 “主仓库”、“upstream（上游仓库）”  
因为每个人都可以自己复制一份走，所以复制后的称之为“个人仓库”，“origin（原仓库）”

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/fork-light.png',
    dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
  }
]" />

### Pull Request（拉取请求、合并请求）

简称 PR，“拉取请求”这个太直译了，听起来很奇怪，~~而且字太多了打起来太累~~，所以也是一样的大家一般就直接说：“来个 PR”  
书接上文，你 fork（复制）的个人仓库，修改完了，怎么把内容提供给主仓库呢？这时候我们就可以开一个 PR，申请将自己修改的内容加入到主仓库中。

当然啦，既然是“请求”，那自然是需要审批的，MAA Team 的各位可能会针对你的修改提一些意见等，当然我们的意见也不一定完全正确，大家合理讨论~

👇 下面的是目前大佬们提的 PR，正在等待审批的

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/pull-request-light.png',
    dark: 'images/zh-cn/pr-tutorial/pull-request-dark.png'
  }
]" />

### Conflict（冲突）

假设一下，主仓库中有个 A 文件，它的内容是 111  
你 fork 了一份，将其内容改成了 222，但是你刚准备提交 PR，这时候张三也 fork 了一份并提交了 PR，并将 A 文件改成了 333  
这时候我们就会看到，你俩都修改了 A 文件，并且修改的不同，那听谁的好呢？这就是 Conflict（冲突）  
冲突解决起来比较麻烦，这里仅阐述概念，方便实际遇到时能理解发生了什么，暂不阐述解决方案

## 纯网页端 PR 操作全流程

1. 首先进入 MAA 主仓库，点右上角这个按钮 Fork 一份代码

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/fork-light.png',
       dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
     }
   ]" />

2. 然后直接点击 Create Fork

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-2-dark.png'
     }
   ]" />

3. 接下来来到了你的个人仓库，可以看到标题是 "你的名字/MaaAssistantArknights"，下面一行小字 forked from MaaAssistantArknights/MaaAssistantArknights （复制自 MAA 主仓库）

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-3-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-3-dark.png'
     }
   ]" />

4. 找到你要改的文件，可以点 "Go to file" 进行全局搜索，也可以直接在下面的文件夹里翻（如果你知道文件在哪的话）

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-4-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-4-dark.png'
     }
   ]" />

5. 打开文件后，直接点击文件右上角的 ✏️ 进行编辑

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-5-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-5-dark.png'
     }
   ]" />

6. 开改！（如果是资源文件这种，我们建议先在你电脑上的 MAA 文件夹里测试修改，确认没问题了再粘贴到网页上，避免改错了）
7. 改完了，点击右上角的 👇 这个按钮，打开提交页面，写一下你改了啥

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-1-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-1-dark.png'
     }
   ]" />

   我们有一个简单的提交标题[命名格式](https://www.conventionalcommits.org/zh-hans/v1.0.0/)，最好可以遵守一下，当然如果实在看不懂也可以先随便写

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-2-dark.png'
     }
   ]" />

8. 还有第二个文件要改的？改完了发现弄错了想再改改？都没关系！重复步骤 4-7 即可！
9. 全改好了进行 PR ！直接点 Code 回到**个人仓库**的主页  
   如果有 Compare & Pull Request 按钮，那最好，直接点他！  
   如果没有也不用着急，点下面的 Contribute（贡献）按钮，再点 Open Pull Request 也是一样的

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-9-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-9-dark.png'
     }
   ]" />

10. 这时候来到了主仓库的 PR 页面，请核对一下你要 PR 的是不是你想提交的  
    如图中，中间有个向左的箭头，是将右边的的 个人姓名/MAA 的 dev 分支，申请合并到 主仓库/MAA 的 dev 分支  
    而往下翻则是这两个分支之间的差异，即你都改了什么

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-10-1-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-10-1-dark.png'
      }
    ]" />

    然后写一下标题，你要修改了的具体内容等等的，然后点确认
    PR 的标题也需要遵守一下[命名格式](https://www.conventionalcommits.org/zh-hans/v1.0.0/)，当然如果还是看不懂也可以先随便写

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-10-2-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-10-2-dark.png'
      }
    ]" />

11. 等待 MAA Team 的大佬们审核吧！当然他们也可能会提意见  
    👇 比如（纯属娱乐切勿当真）

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-11-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-11-dark.png'
      }
    ]" />

12. 如果大佬们说要再修改一些小问题的话，回到 **你的个人仓库**，重复 步骤 4-7 即可！  
    注意不需要操作步骤 1-2（重新 fork）和步骤 9-10（重新 Pull Request），你当前的 Pull Request 仍处于待审核状态，后续的修改会直接进入到这个 Pull Request 中  
    👇 比如可以看到最下面多了一条再次修改的内容

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-12-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-12-dark.png'
      }
    ]" />

13. 等大佬们审批通过，就全部完成了，你修改的内容已经进入 MAA 主仓库啦！

14. 下次如果还想提别的 PR，请先回到你的个人仓库的主页，点击 Sync fork，让你的仓库和主仓库同步。
    这里注意啦，如果有一个红色的 Discard 1 commit，那就点红色的这个；如果没有，再点绿色的 Update branch。
    接下来就可以重复 4-10 的步骤再次修改啦

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-14-1-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-14-1-dark.png'
      },
      {
        light: 'images/zh-cn/pr-tutorial/pr-14-2-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-14-2-dark.png'
      }
    ]" />

在**版本发布后**，你的 GitHub 头像将会自动进入到贡献者列表名单中，非常感谢各位的无私奉献！  
~~怎么全是二次元啊，哦我也是啊，那没事了~~
::: tip 贡献/参与者
感谢所有参与到开发/测试中的朋友们，是大家的帮助让 MAA 越来越好！ (\*´▽ ｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=105&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
:::
