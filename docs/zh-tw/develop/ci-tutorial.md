---
order: 7
icon: devicon-plain:githubactions
---

# CI 系統解析

MAA 利用 GitHub Actions 完成了大量的自動化工作，包括網站的構建，自動更新資源，最終檔案的構建與發版等等過程。隨著時間的推移，這些 CI 逐漸開始嵌套，部分甚至引向了其他的存儲庫。本文檔旨在為想要對 MAA 的 CI 系統做出改進的各位做一個簡要的介紹。

閱讀本文當前，最好對 MAA 的項目結構以及組成有一個基本的概念。

::: tip
你可以通過在本頁面內搜索 CI 檔案名來快速導航到想看的部分
:::

工作流程的檔案均存放在 `.github/workflows` 下，各個檔案可以按功能分為以下幾部分：

+ [程式碼測試](#程式碼測試)
+ [程式碼構建](#程式碼構建)
+ [版本發布](#版本發布)
+ [資源更新](#資源更新)
+ [網站構建](#網站構建)
+ [Issues 管理](#issues-管理)
+ [Pull Requests 管理](#pull-requests-管理)
+ [MirrorChyan 相關](#mirrorchyan-相關)
+ [其他](#其他)

此外，我們還通過 [pre-commit.ci](https://pre-commit.ci/) 實現了程式碼的自動格式化和圖片資源的自動優化，它在發起 PR 後會自動執行，一般無需特別在意。

## GitHub Action 部分

### 程式碼測試

`smoke-testing.yml`

本工作流程主要負責對 MaaCore 做出基本的檢測，包括資源檔案加載，部分簡單 task 運行測試等等。

由於測試用例已經較久沒有更新，該工作流程現在基本是為了保證資源檔案不會出現錯誤，以及 MaaCore 的程式碼沒有出現致命性錯誤（影響到構建的那種）。

### 程式碼構建

`ci.yml`

本工作流程負責對程式碼進行全量構建工作，包含 MAA 的所有組件，構建成品即為可運行的 MAA。

除了必要的 MaaCore 外，Windows 構建產物會包含 MaaWpfGui，macOS 構建產物會包含 MaaMacGui，Linux 構建產物會包含 MaaCLI。

該工作流程在出現任何新 Commit 以及 PR 時都會自動運行，且當該工作流程由發版 PR 觸發時，本次的構建產物將會直接用於發版，並且會創建一個 Release。

### 版本發布

版本發布，簡稱發版，是向用戶發布更新的必要操作，由以下工作流程組成：

+ `release-nightly-ota.yml` 發布內測版
+ `release-ota.yml` 發布正式版/公測版
  + `gen-changelog.yml` 為正式版/公測版生成 changelog
  + `pr-auto-tag.yml` 對正式版/公測版生成 tag

::: tip
上述檔案名內的 ota 意為 Over-the-Air，也就是我們常說的"增量更新包"，因此 MAA 的發版過程實際上包含了對過往版本構建 OTA 包的步驟
:::

#### 內測版

`release-nightly-ota.yml`

本工作流程會在每天 UTC 時間 22 點自動運行，以保證內測版的發版頻率。當然，你也可以在做出更改需要驗證時手動發版。

需要注意的是，內測版的發布僅針對 Windows 用戶，macOS 與 Linux 用戶並不能接收到內測更新。

#### 正式版/公測版

這兩個通道的發版過程稍微複雜一些，我們通過模擬一次發版過程來說明各個工作流程的作用：

1. 從 `dev` 分支向 `master` 分支發起一個 PR，PR 的名稱遵循 `Release v******` 的格式
2. `gen-changelog.yml` 會從近期的正式版/公測版至當前版本生成一份 changelog（以一個新 PR 的形式）
3. 手動調整 changelog 並添加一個簡短的描述
4. 合併 PR，會觸發 `pr-auto-tag.yml`，它會創建一個 tag，同時同步分支
5. Release 事件會在對 master 分支打 tag 後觸發 `release-ota.yml`，它會構建 OTA 包並上傳附件

### 資源更新

本部分工作流程主要負責 MAA 的資源更新與優化，具體為：

+ `res-update-game.yml` 定期運行，從指定倉庫拉取遊戲資源
+ `sync-resource.yml` 同步資源至 MaaResource 倉庫，用於資源更新
+ `optimize-templates.yml` 優化模板圖片的大小

### 網站構建

`website-workflow.yml`

本工作流程主要負責構建並發布 MAA 的官方網站，包括主頁與文檔部分。

需要注意的是，網站的發布與版本發布強綁定，平時對網站組件的修改只會觸發構建以確保沒有錯誤，而正式部署到 Azure 僅會在發版時進行。

### Issues 管理

`issue-checker.yml`

通過正則匹配為 Issues 打上標籤，對 Issue 內容進行分類與標記，方便查看與管理。

`stale.yml`

檢查超過 90 天沒有活動的 Bug Issues，標記並發送通知。如果在 7 天後依然沒有活動，則關閉該 Issue。

### Pull Requests 管理

`pr-checker.yml`

本工作流程會檢查 PR 的 Commit Message 是否符合 [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) 以及是否含有 Merge Commit，如果存在這些情況，則會給出提示。

### MirrorChyan 相關

MirrorChyan 是一個付費的更新鏡像服務，相關的工作流程包括：

+ `mirrorchyan.yml` 同步更新包至 MirrorChyan
+ `mirrorchyan_release_note.yml` 生成 MirrorChyan 的 Release Note

### 其他

`markdown-checker.yml`

檢查倉庫內所有 Markdown 文件的無效鏈接

`blame-ignore.yml`

自動忽略 Commit Message 中含有 blame ignore 的提交，以保持倉庫歷史的乾淨

`cache-delete.yml`

在 PR 合併後清除相關緩存，以節省緩存用量
