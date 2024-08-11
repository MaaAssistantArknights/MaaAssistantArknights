---
order: 3
icon: mingcute:git-pull-request-fill
---

# GitHub Pull Request User Guide

This document is machine-translated. If you have the ability, please refer to the Chinese version. We would greatly appreciate any errors or suggestions for improvement.

::: warning
Many concepts in this tutorial have been simplified in order to make it more accessible to a wider audience, even at the expense of some elegance and accuracy.
For those who have experience using Git and programming knowledge, you may refer to the more advanced tutorial in [Development](./development.md)
:::

## Basic concepts and terminology

This chapter contains slightly dry content. If you're not interested, you can skip directly to the practical section below. If there is anything you don't understand, you can come back to read this chapter later.

### Repository

"Repo" is short for "repository", which is the place where we store our code and other resource files.

👇 You can think of the current web page and all of its contents as the repository for MAA (we usually refer to it as MAA's main repository).

```component Image1
{
  "imageList": [
    {
      "light": "image/zh-cn/pr-tutorial/repository-light.png",
      "dark": "image/zh-cn/pr-tutorial/repository-dark.png"
    }
  ]
}
```

### Fork

To fork,  literally meaning to make a copy of MAA's code, and then perform subsequent modifications and operations to avoid damaging the original repository.

Since it's a copy, let's assume it's called `MAA (1)`.  
To distinguish it from the original repository, we usually refer to the original MAA repository as "main repository" or "upstream".  
Since everyone can make a copy of their own, the copied repository is called a "personal repository" or "origin".

```component Image1
{
  "imageList": [
    {
      "light": "image/zh-cn/pr-tutorial/fork-light.png",
      "dark": "image/zh-cn/pr-tutorial/fork-dark.png"
    }
  ]
}
```

### Pull Request

Continuing from the previous section, after you have made changes to your forked personal repository, how do you provide the changes to the main repository? This is when you can open a pull request (PR) to request that your modified content be added to the main repository.

Of course, since it's a "request", it naturally requires approval. MAA Team may provide feedback on your modifications, but our feedback may not necessarily be entirely correct. Let's have a reasonable discussion together~

👇 The following are the PRs currently submitted by contributors and awaiting approval from MAA Team.

```component Image1
{
  "imageList": [
    {
      "light": "image/zh-cn/pr-tutorial/pull-request-light.png",
      "dark": "image/zh-cn/pr-tutorial/pull-request-dark.png"
    }
  ]
}
```

### Conflict

Assuming that there is a file called A in the main repository with the content 111. You forked a copy and changed its content to 222. However, just as you were about to submit the PR, another person named Zhangsan forked a copy and submitted a PR with changes to the A file, changing it to 333.
Now we have a conflict, as both of you have modified the A file but in different ways. This conflict can be difficult to resolve, and here we are only explaining the concept to help you understand what has happened when you encounter it, without going into the details of the solution.

## Full Process of Pure Web-based PR Operation

1. First, go to the MAA main repository and click the button in the top right corner to fork a copy of the code.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/fork-light.png",
          "dark": "image/zh-cn/pr-tutorial/fork-dark.png"
        }
      ]
    }
    ```

2. Then, click on 'Create Fork' directly.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-2-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-2-dark.png"
        }
      ]
    }
    ```

3. You will be taken to your personal repository, which will have a title of "your_name/MaaAssistantArknights", with a small note below stating "forked from MaaAssistantArknights/MaaAssistantArknights" (copied from the MAA main repository).

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-3-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-3-dark.png"
        }
      ]
    }
    ```

4. Find the file you want to edit. You can click 'Go to file' to search globally, or you can browse through the folders below if you know where the file is located.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-4-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-4-dark.png"
        }
      ]
    }
    ```

5. After opening the file, click on the ✏️ icon on the top right corner of the file to start editing.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-5-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-5-dark.png"
        }
      ]
    }
    ```

6. Make your changes! (If it's a resource file, we recommend testing the changes in your local MAA folder before pasting them in the web page to avoid errors).
7. Once you've finished editing, click the button 👇 in the top right corner to open the commit page and write a brief description of what you changed.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-7-1-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-7-1-dark.png"
        }
      ]
    }
    ```

    We have a simple commit title [naming format](https://www.conventionalcommits.org/zh-hans/v1.0.0/), which you should try to follow. However, if you find it difficult to understand, you can just write something for now.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-7-2-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-7-2-dark.png"
        }
      ]
    }
    ```

8. Need to edit another file? Made a mistake and want to change it again? No problem! Simply repeat steps 4-7 for the other file.
9. When all changes are made, create a Pull Request by going to the Pull Request tab in your personal repository.
    If there is a "Compare & Pull Request" button, that's great, just click it!
    If not, don't worry, clicking "Open Pull Request" below is also the same.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-9-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-9-dark.png"
        }
      ]
    }
    ```

10. Now you are in the main repository, please double-check the changes you want to PR.
    As shown in the figure, there is a leftward arrow, which means you want to merge the dev branch from your forked repository "your_name/MAA" to the dev branch of the main repository "MAA".
    Scrolling down shows the differences between these two branches, indicating what changes you have made.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-10-1-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-10-1-dark.png"
        }
      ]
    }
    ```

    Then, write a title and description of your changes, and click "Create Pull Request".
    The PR title should also follow the [naming format](https://www.conventionalcommits.org/zh-hans/v1.0.0/). However, if you still don't understand it, you can also just write something for now.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-10-2-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-10-2-dark.png"
        }
      ]
    }
    ```

11. Just wait for the MAA Team members to review it! Of course, they may also provide some comments,
    👇 for example (for entertainment purposes only, do not take it seriously):

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-11-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-11-dark.png"
        }
      ]
    }
    ```

12. If they request further changes, go back to **your personal repository** and repeat steps 4-7!
    Note that you do not need to redo step 1-2 (re-fork) or step 9-10 (re-Pull Request), your current Pull Request is still pending review, and subsequent changes will be added directly to this Pull Request.
    👇 For example, you can see that "revised demo" has been added at the bottom.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-12-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-12-dark.png"
        }
      ]
    }
    ```

13. Once the experts approve the PR, you're all set! Your changes have been merged into the main MAA repository!

14. If you want to submit another PR next time, please first go back to the homepage of your personal repository and click 'Sync fork' to synchronize your repository with the main repository.
    Note here: If there is a red 'Discard 1 commit', click the red one. If there isn't, then click the green 'Update branch'.
    Next, you can repeat steps 4-10 to make further modifications.

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-14-1-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-14-1-dark.png"
        },
        {
          "light": "image/zh-cn/pr-tutorial/pr-14-2-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-14-2-dark.png"
        }
      ]
    }
    ```

**After the version is released**, your GitHub avatar will automatically be added to the contributors list. Thank you very much for your selfless dedication!
::: tip Contributors/Participators
Thank you to everyone involved in development and testing. Your help is going to ~~Make MAA Great Again~~! (\*´▽ ｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=114514&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
:::
