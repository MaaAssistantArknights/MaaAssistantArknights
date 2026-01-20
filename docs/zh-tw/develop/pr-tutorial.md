---
order: 3
icon: mingcute:git-pull-request-fill
---

# 純網頁端 PR 教學

牛牛也能看懂的 GitHub Pull Request 使用指南 (\*´▽ ｀)ノノ

::: warning
本教學中對許多概念進行了簡化，為了讓更多朋友能實際上手，甚至有一些不那麼「優雅」~~但簡單~~的操作，以及一些不完全精確的解釋，還請各位大神手下留情。  
若您具備一定的 Git 使用經驗及程式設計基礎，~~那你還看這個 🔨~~，可以參考進階版的教學：[GitHub Pull Request 流程簡述](./development.md)。
:::

## 基本概念及名詞解釋

這一章節內容略顯枯燥，如果不感興趣可以直接跳到下面的實作部分，遇到不理解的地方再回來查看。

### Repository（倉庫）

簡稱 Repo 或倉庫，是存放程式碼及其他資源檔案的地方。

👇 可以簡單理解為目前這個網頁及其所有內容，就是 MAA 的倉庫（我們一般稱之為 MAA 的主倉庫）。

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/repository-light.png',
    dark: 'images/zh-cn/pr-tutorial/repository-dark.png'
  }
]" />

### Fork（複製）

複製，字面意思就是將 MAA 的程式碼完整複製一份到自己的帳號下，以便進行後續修改，避免不小心弄壞原本的內容。  
在中文語境中，「複製」一詞首先想到的可能是 copy，而 fork 也沒有其他明確的中文翻譯，所以我們通常直接說英文，例如：「把程式碼 Fork 一份走」。

為了區分，我們通常將原本的 MAA 倉庫稱為 **「主倉庫」** 或 **「Upstream（上游倉庫）」**。  
而從主倉庫 Fork 出來的則稱為 **「個人倉庫」** 或 **「Origin（遠端倉庫）」**。

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/fork-light.png',
    dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
  }
]" />

### Pull Request（拉取請求、合併請求）

簡稱 PR。因為「拉取請求」聽起來很生硬且字數較多，~~而且字太多了打起来太累~~，大家通常直接說：「發個 PR」。  
延續前文，當您在 Fork 出來的個人倉庫修改完成後，該如何將內容提供給主倉庫呢？這時就可以發起一個 PR，申請將自己修改的內容合併到主倉庫中。

當然，既然是「請求」，就代表需要經過審核。MAA Team 的成員會針對您的修改提供建議，大家可以理性討論，共同完善內容。

👇 以下是目前大佬發起的 PR，正等待審核中。

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/pull-request-light.png',
    dark: 'images/zh-cn/pr-tutorial/pull-request-dark.png'
  }
]" />

### Conflict（衝突）

假設主倉庫中有個 A 檔案，內容是 `111`。  
您 Fork 了一份並改為 `222`；但在您提交 PR 之前，另一位開發者張三也提交了 PR 並將 A 檔案改成了 `333`。  
這時系統會發現你們兩人都修改了同一個地方，且內容不同，程式不知道該聽誰的，這就是 Conflict（衝突）。  
衝突處理起來較為複雜，此處僅闡述概念，讓您在遇到時能理解發生了什麼事。

## 純網頁端 PR 操作全流程

1. 首先進入 MAA 主倉庫，點擊右上角的 Fork 按鈕。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/fork-light.png',
       dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
     }
   ]" />

2. 接著直接點擊 Create Fork。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-2-dark.png'
     }
   ]" />

3. 現在來到了您的個人倉庫，標題會顯示為 `您的帳號名稱/MaaAssistantArknights`，下方會有一行小字標註 `forked from MaaAssistantArknights/MaaAssistantArknights`（複製自 MAA 主倉庫）。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-3-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-3-dark.png'
     }
   ]" />

4. 找到要修改的檔案。可以點擊 Go to file 進行全域搜尋，如果您知道文件在哪裡的話，也可以直接在下方的資料夾翻找。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-4-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-4-dark.png'
     }
   ]" />

5. 開啟檔案後，點擊右上角的 ✏️ 按鈕進行編輯。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-5-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-5-dark.png'
     }
   ]" />

6. 開始修改！（如果是資源檔案，建議先在電腦上的 MAA 資料夾測試，確認沒問題後再貼到網頁上，以免改錯）。

7. 修改完成後，點擊右上角的 👇 這個按鈕開啟提交頁面，並寫下您的修改說明。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-1-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-1-dark.png'
     }
   ]" />

   我們有一套簡單的提交標題[命名格式](https://www.conventionalcommits.org/zh-hans/v1.0.0/)，建議盡量遵守；若暫時無法理解，也可以先簡單描述。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-2-dark.png'
     }
   ]" />

8. 還有其他檔案要改？或是發現改錯了想再修補？沒關係！重複步驟 4-7 即可。

9. 全部改好後，準備發起 PR！點擊 Code 回到**個人倉庫**首頁。  
   如果有出現 Compare & Pull Request 按鈕，請直接點擊它；若沒看到，點擊下方的 Contribute（貢獻）按鈕，再點 Open Pull Request 也是一樣的。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-9-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-9-dark.png'
     }
   ]" />

10. 此時會進入主倉庫的 PR 頁面，請核對提交內容是否正確。  
    如下圖所示，箭頭方向是將右側「您的帳號 / MAA」的 `dev` 分支合併到「主倉庫 / MAA」的 `dev` 分支。頁面下方會顯示兩者間的差異。

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-10-1-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-10-1-dark.png'
      }
    ]" />

    接著填寫標題與具體修改內容，最後點擊確認。  
    PR 標題同樣建議遵循[命名格式](https://www.conventionalcommits.org/zh-hans/v1.0.0/)。

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-10-2-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-10-2-dark.png'
      }
    ]" />

11. 接下來請耐心等待 MAA Team 的審核！當然他們可能也會提出自己的意見。
    👇 比如（純屬娛樂，請勿當真）

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-11-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-11-dark.png'
      }
    ]" />

12. 若大佬要求針對細節進行修正，請回到**您的個人倉庫**，重複步驟 4-7 即可。  
    請注意，不需要重新 Fork（步驟 1-2）或重新發起 PR（步驟 9-10）。您目前的 PR 仍處於開啟狀態，後續的修改會自動更新到同一個 PR 中。
    👇 可以看到最下面多了一條「重新修改演示」的內容

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-12-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-12-dark.png'
      }
    ]" />

13. 當大佬們審核通過並被合併（Merged）後，就大功告成了！您的修改已正式進入 MAA 主倉庫。

14. 下次若想再發起新的 PR，請先回到個人倉庫首頁，點擊 Sync fork 讓您的倉庫與主倉庫同步。  
    注意：若出現紅色的 Discard 1 commit，請優先點擊它；若無，則點擊綠色的 Update branch。接著即可重複步驟 4-10 進行新的修改。

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

在**版本發布後**，您的 GitHub 頭像將會自動出現在貢獻者名單中。非常感謝您的無私奉獻！
~~怎麼全是二次元啊，哦我也是，那沒事了~~
::: tip 貢獻/參與者
感謝所有參與開發與測試的朋友，是大家的幫助讓 MAA 變得更好！ (\*´▽ ｀)ノノ

[![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=105&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
:::