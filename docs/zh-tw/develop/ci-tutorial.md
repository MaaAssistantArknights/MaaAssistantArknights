---
order: 7
icon: devicon-plain:githubactions
---

# CI 系統解析

MAA 借助 Github Action 完成了大量的自動化工作，包括網站的構建，自動更新資源，最終文件的構建與發版等等過程。隨著時間的推移，這些 CI 逐漸開始嵌套，部分甚至引向了其他的存儲庫。本文檔旨在為想要對 MAA 的 CI 系統做出改進的各位做一個簡要的介紹。

閱讀本文之前，最好對 MAA 的項目結構以及組成有一個基本的概念。

::: tip
你可以通過在本頁面內搜尋 CI 文件名來快速導航到想看的部分
:::

工作流的文件均存放在 `.github/workflows` 下，各個文件可以按功能分為以下幾部分：

- [代碼測試](#代碼測試)
- [代碼構建](#代碼構建)
- [代碼安全檢查](#代碼安全檢查)
- [版本發布](#版本發布)
- [資源更新](#資源更新)
- [網站構建](#網站構建)
- [Issues 管理](#issues-管理)
- [Pull Requests 管理](#pull-requests-管理)
- [MirrorChyan 相關](#mirrorchyan-相關)
- [其他](#其他)

此外，我們還通過 [pre-commit.ci](https://pre-commit.ci/) 實現了代碼的自動格式化和圖片資源的自動優化，它在發起 PR 後會自動執行，一般無需特別在意。

## Github Action 部分

### 代碼測試

`smoke-testing.yml`

本工作流主要負責對 MaaCore 做出基本的檢測，包括資源文件加載，部分簡單 task 運行測試等等。

由於測試用例已經較久沒有更新，該工作流現在基本是為了保證資源文件不會出現錯誤，以及 MaaCore 的代碼沒有出現影響到構建的致命性錯誤。

### 代碼構建

`ci.yml`

本工作流負責對代碼進行全量構建工作，包含 MAA 的所有組件，構建成品即為可運行的 MAA。

除了必要的 MaaCore 外，Windows 構建產物會包含 MaaWpfGui，MacOS 構建產物會包含 MaaMacGui，Linux 構建產物會包含 MaaCLI。

該工作流在出現任何新 Commit 以及 PR 時都會自動運行，且當該工作流由發版 PR 觸發時，本次的構建產物將會直接用於發版，並且會創建一個 Release。

### 代碼安全檢查

代碼安全檢查通過 CodeQL 對代碼和工作流進行安全分析，具體工作流如下：

`codeql-core.yml`

本工作流負責對 MaaCore 和 MaaWpfGui 的 C++ 和 C# 代碼進行安全分析，檢測潛在的安全漏洞。

該工作流在修改相關源代碼的 PR 時自動運行，同時每天 UTC 時間 11:45 自動執行定期檢查。

`codeql-wf.yml`

本工作流負責對 GitHub Actions 工作流文件本身進行安全分析，確保 CI/CD 流程的安全性。

該工作流在修改工作流文件的 PR 時自動運行，同時每天 UTC 時間 12:00 自動執行定期檢查。

### 版本發布

版本發布，簡稱發版，是向用戶發布更新的必要操作，由以下工作流組成：

- `release-nightly-ota.yml` 發布內測版
- `release-ota.yml` 發布正式版/公測版
  - `release-preparation.yml` 為正式版/公測版生成 changelog 和準備發布
  - `pr-auto-tag.yml` 對正式版/公測版生成 tag

::: tip
上述文件名內的 ota 意為 Over-the-Air，也就是我們常說的「增量更新包」，因此 MAA 的發版過程實際上包含了對過往版本構建 OTA 包的步驟
:::

#### 內測版

`release-nightly-ota.yml`

本工作流會在每天 UTC 時間 22 點自動運行，以保證內測版的發版頻率。當然，你也可以在做出更改需要驗證時手動發版。

需要注意的是，內測版的發布僅針對 Windows 用戶，MacOS 與 Linux 用戶並不能接收到內測更新。

#### 正式版/公測版

這兩個通道的發版流程相對複雜一點，我們通過模擬一次發版步驟來解釋各工作流的作用：

1. 建立由 `dev` 到 `master` 分支的 pr，且該 pr 的名字需要為 `Release v******`
2. `release-preparation.yml` 會生成最近的正式版/公測版到當前版本的 changelog（以一個新 pr 的形式）
3. 對 changelog 進行手動調整，並且添加簡要描述
4. 合併 pr，觸發 `pr-auto-tag.yml`，創建 tag 並且同步分支
5. Release 事件觸發 `release-ota.yml`，對 master 打完 tag 後進行 ota 包的構建以及附件上傳

### 資源更新

這部分工作流主要負責 MAA 的資源更新以及優化，具體工作流如下：

- `res-update-game.yml` 定期執行，從指定的倉庫拉取遊戲資源
- `sync-resource.yml` 將資源同步到 MaaResource 倉庫，用於資源更新
- `optimize-templates.yml` 優化包括模板圖在內的圖片大小

### 網站構建

`website-workflow.yml`

本工作流主要負責 MAA 文檔站的構建與發布。

請注意，網站的發布與發版強綁定，平常修改網頁組件的時候只會進行構建以保證不會有錯誤，在發版時才會正式部署到 GitHub Pages 上。

### Issues 管理

`issue-checker.yml`

通過正則匹配給各個 Issue 打 Tag，以此來分類標記 Issue 內容，方便查看與管理。

`issue-checkbox-checker.yml`

通過正則匹配自動關閉勾選「我未仔細閱讀」的 Issue；若「我未仔細閱讀」未被勾選，則將所有勾選框折疊。

`stale.yml`

檢查超過 90 天沒有活動的 Bug Issue，將其標記並發起通知，7 天後若還沒有活動則關閉。

### Pull Requests 管理

`pr-checker.yml`

該工作流用於檢查 PR 中的 Commit Message 是否符合 [約定式提交](https://www.conventionalcommits.org/zh-hans/v1.0.0/)，以及是否包含 Merge Commit，若上述條件符合則會作出提示。

### MirrorChyan 相關

MirrorChyan 是有償的更新鏡像服務，與其相關的工作流如下：

- `mirrorchyan.yml` 同步更新包到 MirrorChyan
- `mirrorchyan_release_note.yml` 生成 MirrorChyan 的 Release Note

### 其他

`markdown-checker.yml`

負責檢查倉庫中的所有 Markdown 文件中是否包含無效鏈接。

`blame-ignore.yml`

自動忽略 Commit Message 包含 `blame ignore` 的提交，保證倉庫歷史的乾淨。

`cache-delete.yml`

在 PR 合併後清理相關的緩存，以此來節省緩存用量。

`update-submodules.yml`

定期更新 MaaMacGui 和 maa-cli 等子模組到最新版本。該工作流每天 UTC 時間 21:50 自動執行（在每日內測版發布之前），確保子模組保持最新狀態。
