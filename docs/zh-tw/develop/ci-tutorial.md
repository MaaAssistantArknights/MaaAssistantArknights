---
order: 7
icon: devicon-plain:githubactions
---

# CI 系統解析

MAA 藉由 GitHub Actions 完成了大量的自動化工作，包括網站建置、資源自動更新、最終檔案的編譯與發布等過程。隨著時間推移，這些 CI 逐漸開始嵌套，部分甚至連結到了其他的倉庫。本文檔旨在為想要改進 MAA CI 系統的開發者提供一份簡要介紹。

閱讀本文之前，建議先對 MAA 的專案結構與組成有基本的概念。

::: tip
您可以透過在本頁面內搜尋 CI 檔名來快速導航到想看的內容。
:::

工作流（Workflow）檔案均存放於 `.github/workflows` 目錄下，各檔案可依功能分為以下幾部分：

- [程式碼測試](#程式碼測試)
- [程式碼建置](#程式碼建置)
- [程式碼安全檢查](#程式碼安全檢查)
- [版本發布](#版本發布)
- [資源更新](#資源更新)
- [網站建置](#網站建置)
- [Issues 管理](#issues-管理)
- [Pull Requests 管理](#pull-requests-管理)
- [MirrorChyan 相關](#mirrorchyan-相關)
- [其他](#其他)

此外，我們還透過 [pre-commit.ci](https://pre-commit.ci/) 實現了程式碼自動格式化與圖片資源自動優化，它在發起 PR 後會自動執行，一般無需特別在意。

## GitHub Actions 部分

### 程式碼測試

`smoke-testing.yml`

本工作流主要負責對 MaaCore 進行基礎檢測，包括資源檔案載入、部分簡單任務（Task）執行測試等。

由於測試案例（Test Cases）已較久未更新，該工作流目前的初衷是確保資源檔案不會出錯，以及 MaaCore 的程式碼沒有出現影響建置的致命性錯誤。

### 程式碼建置

`ci.yml`

本工作流負責對程式碼進行全量建置工作，包含 MAA 的所有組件，建置產物即為可執行的 MAA。

除了必要的 MaaCore 外，Windows 建置產物會包含 MaaWpfGui，macOS 建置產物會包含 MaaMacGui，Linux 建置產物則包含 MaaCLI。

該工作流在有任何新提交（Commit）或 PR 時都會自動執行；當該工作流由發版 PR 觸發時，本次的建置產物將直接用於發布，並自動建立一個 Release。

### 程式碼安全檢查

程式碼安全檢查透過 CodeQL 對程式碼與工作流進行安全分析，具體工作流如下：

`codeql-core.yml`

負責對 MaaCore 與 MaaWpfGui 的 C++ 和 C# 程式碼進行安全分析，偵測潛在的安全漏洞。

該工作流在修改相關原始碼的 PR 時會自動執行，同時於每天 UTC 時間 11:45 進行定期檢查。

`codeql-wf.yml`

負責對 GitHub Actions 工作流檔案本身進行安全分析，確保 CI/CD 流程的安全性。

該工作流在修改工作流檔案的 PR 時會自動執行，同時於每天 UTC 時間 12:00 進行定期檢查。

### 版本發布

版本發布（簡稱發版）是向用戶發布更新的必要操作，由以下工作流組成：

- `release-nightly-ota.yml` 發布內測版（Nightly）
- `release-ota.yml` 發布正式版 / 公測版（Beta）
  - `release-preparation.yml` 為正式版 / 公測版產生 Changelog 並準備發布
  - `pr-auto-tag.yml` 對正式版 / 公測版產生 Tag

::: tip
上述檔名內的 ota 意為 Over-the-Air（雲端更新），也就是我們常說的「增量更新包」，因此 MAA 的發版過程實際上包含了對過往版本建置 OTA 包的步驟。
:::

#### 內測版

`release-nightly-ota.yml`

本工作流會在每天 UTC 時間 22 點自動執行，以保證內測版的更新頻率。當然，您也可以在做出更改需要驗證時手動觸發發布。

需要注意的是，內測版發布僅針對 Windows 用戶，macOS 與 Linux 用戶目前無法接收到內測更新。

#### 正式版 / 公測版

這兩個通道的發布流程相對複雜，我們透過模擬一次發布步驟來解釋各工作流的作用：

1. 建立從 `dev` 到 `master` 分支的 PR，且該 PR 的標題需為 `Release v******`。
2. `release-preparation.yml` 會產生從最近版本到當前版本的 Changelog（以一個新 PR 的形式呈現）。
3. 對 Changelog 進行手動調整，並加入簡要描述。
4. 合併 PR，觸發 `pr-auto-tag.yml` 建立 Tag 並同步分支。
5. Release 事件觸發 `release-ota.yml`，對 master 標上 Tag 後進行 OTA 包建置及附件上傳。

### 資源更新

這部分工作流主要負責 MAA 的資源更新與優化，具體如下：

- `res-update-game.yml` 定期執行，從指定倉庫拉取遊戲資源。
- `sync-resource.yml` 將資源同步到 MaaResource 倉庫，用於資源更新。
- `optimize-templates.yml` 優化包括模板圖在內的圖片檔案大小。

### 網站建置

`website-workflow.yml`

本工作流主要負責 MAA 文件站的建置與發布。

請注意，網站的發布與版本更新強烈綁定。平時修改網頁組件時僅會進行建置以確保無誤，只有在正式發布版本時才會部署到 GitHub Pages。

### Issues 管理

`issue-checker.yml`

透過正規表達式（Regex）比對為各個 Issue 標記 Tag，以此分類 Issue 內容，方便查看與管理。

`issue-checkbox-checker.yml`

透過正規表達式自動關閉勾選「我未仔細閱讀」的 Issue；若該項未被勾選，則將所有勾選框（Checkbox）折疊。

`stale.yml`

檢查超過 90 天沒有活動的 Bug Issue 並標記通知，若 7 天後仍無活動則自動關閉。

### Pull Requests 管理

`pr-checker.yml`

檢查 PR 中的 Commit Message 是否符合 [約定式提交](https://www.conventionalcommits.org/zh-hans/v1.0.0/)，以及是否包含 Merge Commit，若不符合則會發出提示。

### MirrorChyan 相關

MirrorChyan 是有償的更新鏡像服務，相關工作流如下：

- `release-package-distribution.yml` 同步更新包至 MirrorChyan。
- `mirrorchyan_release_note.yml` 產生 MirrorChyan 的 Release Note。

### 其他

`markdown-checker.yml`

檢查倉庫中所有 Markdown 檔案是否包含無效連結（死連結）。

`blame-ignore.yml`

自動忽略 Commit Message 包含 `blame ignore` 的提交，以保持 Git Blame 歷史紀錄的簡潔。

`cache-delete.yml`

在 PR 合併後清理相關快取（Cache），以節省 GitHub 儲存空間。

`update-submodules.yml`

定期將 MaaMacGui 與 maa-cli 等子模組更新至最新版本。該工作流於每天 UTC 時間 21:50 自動執行（在每日內測版發布之前），確保子模組保持在最新狀態。