---
order: 3
icon: 'mingcute:git-pull-request-fill'
---
# Web-based PR Tutorial

A GitHub Pull Request usage guide even cows can understand (&#42;&acute;&#9661; &#65344;)&#12494;&#12494;

::: warning
This tutorial simplifies many concepts to make it more accessible for friends to actually use it. It includes some inelegant ~~but simple~~ operations and some less-than-correct explanations. Please go easy on us, experts.  
If you have some git experience and programming basics, ~~then why are you reading this 🔨~~, you can check out the slightly more advanced tutorial [GitHub Pull Request Process Overview](./development.md).
:::

## Basic Concepts and Terminology

This chapter is a bit dry. You can skip directly to the practical part below if not interested and come back if something is unclear. We also welcome all ~~cows~~ newcomers to learn a little bit of Git knowledge to contribute more to MAA 💪

### Repository (Repo)

Short for repo, it's where our code and other resource files are stored.

👇 You can simply understand that this webpage and all its content is the MAA repository (we generally call it MAA's **main repository**).

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/repository-light.png',
    dark: 'images/zh-cn/pr-tutorial/repository-dark.png'
  }
]" />

### Fork (Copy/Branch)

Copy, or branch, literally meaning to copy MAA's code, then you can make subsequent modifications and other operations, avoiding breaking the original.  
But when we say "copy" in Chinese, we might first think of the meaning of copy. Fork doesn't have another clear translation, so we generally prefer to use the English term directly, like "fork the code away."

Since it's copied, it's `MAA (1)` (bushi)  
To distinguish it from the original repository, we generally call the original MAA repository the "main repository," "upstream repository."  
Because everyone can copy one themselves, the copied one is called a "personal repository," "origin (original repository)."

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/fork-light.png',
    dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
  }
]" />

### Pull Request (PR)

Abbreviated as **PR**. "Pull Request" is too literal a translation, sounds weird, ~~and has too many characters to type~~, so similarly, people generally just say: "Submit a PR."  
Continuing from above, after you fork (copy) the personal repository and finish modifying, how do you provide the content to the main repository? At this point, we can open a PR, requesting to add your modified content to the main repository.

Of course, since it's a "request," it naturally needs approval. Members of MAA Team may have some feedback on your modifications, etc. Of course, our opinions may not always be completely correct; let's discuss reasonably~

👇 Below are the PRs currently submitted by experts, waiting for approval.

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/pull-request-light.png',
    dark: 'images/zh-cn/pr-tutorial/pull-request-dark.png'
  }
]" />

### Conflict

Imagine, there is a file A in the main repository, its content is 111  
You fork a copy, change its content to 222, but just as you're about to submit a PR, Zhang San also forks a copy and submits a PR, changing file A to 333  
At this point, we see that both of you modified file A, and the modifications are different. Whose should we accept? This is a Conflict.  
Resolving conflicts is relatively troublesome. Here, we only explain the concept to help understand what happened when actually encountered, without elaborating on solutions for now. If interested, you can search for **Git usage tutorials**.

## Complete Web-based PR Operation Process

1. First, go to the MAA main repository, click the button in the upper right corner to Fork a copy of the code.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/fork-light.png',
       dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
     }
   ]" />

2. Then directly click Create Fork.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-2-dark.png'
     }
   ]" />

3. Next, you arrive at your personal repository. You can see the title is "YourName/MaaAssistantArknights," with a line below saying forked from <u>MaaAssistantArknights/MaaAssistantArknights</u> (forked from MAA main repository).

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-3-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-3-dark.png'
     }
   ]" />

4. Find the file you want to modify. You can click "Go to file" for a global search, or directly browse the folders below (if you know where the file is).

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-4-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-4-dark.png'
     }
   ]" />

5. After opening the file, directly click the ✏️ in the upper right corner of the file to edit.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-5-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-5-dark.png'
     }
   ]" />

6. Start modifying! (If it's a resource file, we suggest first testing the modification in your local MAA folder, confirm it works, then paste it on the webpage to avoid mistakes.)
7. After finishing modifications, click the 👇 button in the upper right corner to open the commit page, and write what you changed.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-1-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-1-dark.png'
     }
   ]" />

   We have a simple [naming format](https://www.conventionalcommits.org/zh-hans/v1.0.0/) for commit titles. It's best to follow it, but if you really can't understand, you can write whatever for now.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-2-dark.png'
     }
   ]" />

8. Have a second file to modify? Finished modifying and found a mistake and want to change it again? No problem! Repeat steps 4-7!
9. After all modifications are done, proceed to PR! Directly click Code to return to the **personal repository** homepage.  
   If there is a Compare & Pull Request button, great, click it directly!  
   If not, don't worry, click the Contribute button below, then click Open Pull Request; it's the same.

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-9-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-9-dark.png'
     }
   ]" />

10. Now you've arrived at the PR page of the main repository. Please verify if what you're PR-ing is what you want to submit.  
    As shown in the image, there's a left arrow in the middle, indicating requesting to merge the right branch (personal name/MAA dev branch) into the main repository/MAA dev branch.  
    Scrolling down shows the differences between these two branches, i.e., what you changed.

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-10-1-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-10-1-dark.png'
      }
    ]" />

    Then write a title, the specific content you modified, etc., and click confirm.  
    The PR title also needs to follow the [naming format](https://www.conventionalcommits.org/zh-hans/v1.0.0/), but if you still don't understand, you can write whatever for now.

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-10-2-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-10-2-dark.png'
      }
    ]" />

11. Wait for the experts from MAA Team to review! Of course, they might also provide feedback.  
    👇 For example (purely for entertainment, don't take it seriously)

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-11-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-11-dark.png'
      }
    ]" />

12. If the experts say some minor issues need to be fixed, return to **your personal repository** and repeat steps 4-7!  
    Note: you don't need to do steps 1-2 (re-fork) and steps 9-10 (re-Pull Request). Your current Pull Request is still pending review, and subsequent modifications will directly enter this Pull Request.  
    👇 For example, you can see an additional modification entry at the bottom.

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-12-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-12-dark.png'
      }
    ]" />

13. Wait for the experts to approve, then everything is complete. Your modified content is now in the MAA main repository!

14. Next time if you want to submit another PR, please first return to your personal repository homepage, click Sync fork to synchronize your repository with the main repository.  
    Note here: if there is a red Discard 1 commit, click the red one; if not, then click the green Update branch.  
    Then you can repeat steps 4-10 to make modifications again.

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

After **version release**, your GitHub avatar will automatically be added to the contributor list. Thank you all for your selfless contributions!  
~~Why are they all anime characters? Oh, I am too, never mind then.~~
::: tip Contributors
Thanks to all friends who participated in development/testing. Your help makes MAA better and better!
Make MAA Great Again!(&#42;&acute;&#9661; &#65344;)&#12494;&#12494;

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=105&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
:::
