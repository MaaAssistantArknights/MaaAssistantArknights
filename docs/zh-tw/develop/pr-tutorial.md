---
order: 3
icon: mingcute:git-pull-request-fill
---
# 純網頁端 PR 教學

牛牛也能看懂的 GitHub Pull Request 使用指南 (\*´▽｀)ノノ

::: warning
本教學中對很多概念進行了簡化，為了讓更多朋友能實際用起來，甚至有一些很不優雅~~但是簡單~~的操作，還有一些不那麽正確的解釋，還請大佬們輕噴。  
若您有一定的 git 使用經驗及程式基礎，~~那你還看個~~🔨，可以看稍微進階一點的教學 [Github Pull Request 流程簡述](development.md)
:::

## 基本概念及名詞解釋

這一章節內容略微枯燥，不感興趣可以直接跳過到下面實作部分，有不理解的再回來看

### Repository（倉庫）

簡稱 repo，存放我們代碼及其他資源檔案的地方

👇 可以簡單理解為目前這個網頁及裡面所有的內容，就是 MAA 的倉庫（我們一般稱之為 MAA 的主倉庫）

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

### Fork（複製）

複製，字面意思，將 MAA 的代碼複製一份，然後可以進行後續修改等等的操作，避免把原來的弄壞了  
但一般說中文 “複製” 我們可能首先想到的是 copy 的意思，fork 也沒有其他明確的翻譯，所以我們一般習慣直接說英文，比如 “把代碼 fork 一份走”

既然是複製後的，那就是 `MAA (1)`（bushi）  
為了和原本的倉庫區分開，所以我們一般將原本的 MAA 倉庫稱為 “主倉庫”、“upstream （上遊倉庫）”、“origin （原倉庫）”；  
因為每個人都可以自己複製一份走，所以複製後的稱之為 “個人倉庫”

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

### Pull Request（拉取請求、合併請求）

簡稱 PR，“拉取請求” 這個太直譯了，聽起來很奇怪，~~而且字太多了打起來太累~~，所以也是一樣的大家一般就直接說： “來個 PR”  
書接上文，你 fork（複製）的個人倉庫，修改完了，怎麽把內容提供給主倉庫呢？這時候我們就可以開一個 PR，申請將自己修改的內容加入到主倉庫中。

當然啦，既然是 “請求”，那自然是需要審批的，MAA Team 的各位可能會針對你的修改提一些意見等，當然我們的意見也不一定完全正確，大家合理討論 ~

👇 下面的是目前大佬們提的 PR，正在等待審批的

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

### Conflict（衝突）

假設一下，主倉庫中有個 A 檔案，它的內容是 111  
你 fork 了一份，將其內容改成了 222，但是你剛準備提交 PR，這時候張三也 fork 了一份並提交了 PR，並將 A 檔案改成了 333  
這時候我們就會看到，你倆都修改了 A 檔案，並且修改的不同，那聽誰的好呢？這就是 Conflict（衝突）  
衝突解決起來比較麻煩，這裡僅闡述概念，方便實際遇到時能理解發生了什麽，暫不闡述解決方案

## 純網頁端 PR 操作全流程

1. 首先進入 MAA 主倉庫，fork 一份代碼

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

2. 把 “僅 master 分支” 這個選項去掉，然後點擊 Create Fork

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

3. 接下來來到了你的個人倉庫，可以看到標題是 “你的名字/MaaAssistantArknights”，下面一行小字 forked from MaaAssistantArknights/MaaAssistantArknights （複製自 MAA 主倉庫）

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

4. 找到你要改的檔案，可以點 “Go to file” 進行全局搜尋，也可以直接在下面的資料夾裡翻（如果你知道檔案在哪的話）

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

5. 打開檔案後，直接點擊檔案右上角的 ✏️ 進行編輯

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

6. 開改！（當然如果是資源檔案這種，我們建議先在你電腦上的 MAA 資料夾裡測試修改，確認沒問題了再貼上到網頁上，避免改錯了）
7. 改完了，翻到最下面，寫一下你改了啥

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

8. 還有第二個檔案要改的？改完了發現弄錯了想再改改？都沒關係！重複 5-8 即可！
9. 全改好了進行 PR ！直接點 **個人倉庫** 裡的 Pull Request 標籤頁  
    如果有 Compare & Pull Request 按鈕，那最好，直接點他！如果沒有也不用著急，點下面的 New Pull Request 也是一樣的（請看步驟 11）

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

10. 這時候來到了主倉庫，請核對一下你要 PR 的是否確認。  
    如圖中，中間有個向左的箭頭，是將右邊的 個人姓名/MAA 的 dev 分支，申請合併到 主倉庫/MAA 的 dev 分支。  

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

    然後寫一下標題，你修改的具體內容等等的，然後點確認

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

11. 等待 MAA Team 的大佬們審核吧！當然他們也可能會提意見  
  👇 比如（純屬娛樂切勿當真）

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

12. 如果大佬們說要再修改一些小問題的話，回到 **你的個人倉庫**，切換到先前的 dev 分支，重複 步驟 3-9 即可！  
  注意不需要操作步驟 2（重新 fork），也不需要操作步驟 10（重新 Pull Request），你目前的 Pull Request 仍處於待審核狀態，後續的修改會直接進入到這個 Pull Request 中  
  👇 比如可以看到最下面多了一條 “重新修改演示” 的內容

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

13. 等大佬們審批通過，就全部完成啦！**版本發布後**，你的 GitHub 頭像將會自動進入到貢獻者列表名單中，非常感謝各位的無私奉獻！  
    ~~怎麽全是二次元啊，哦我也是啊，那沒事了~~
    ::: tip 貢獻 / 參與者
    感謝所有參與到開發 / 測試中的朋友們，是大家的幫助讓 MAA 越來越好！ (*´▽｀)ノノ

    [![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=114514&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
    :::

14. 下次如果還想提別的 PR，請先切換到 dev 分支，然後直接如下圖操作  
    ::: warning
    這個操作會強制將你的個人倉庫同步到和主倉庫一模一樣的狀態，這是最簡單粗暴但行之有效的解決衝突的方法。但如果你的個人倉庫已經有額外的編輯了，會被直接刪掉！
    :::
    如果確定不會造成衝突，請使用右側綠色的 `Update Branch` 按鈕  

    如果你不清楚 / 不 care 我上面說的這一大堆，也請點擊左側的按鈕

    ```component Image1
    {
      "imageList": [
        {
          "light": "image/zh-cn/pr-tutorial/pr-14-light.png",
          "dark": "image/zh-cn/pr-tutorial/pr-14-dark.png"
        }
      ]
    }
    ```

    接著重複步驟 3-14，修改、提 PR 即可 ~
