---
order: 4
icon: bxs:bot
---

# Issue Bot 使用方法

Issue Bot 使用的 Action 為 [issue-checker](https://github.com/zzyyyl/issue-checker)，配置檔案為 [issue-checker.yml](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml)。

::: warning
拉取請求（PR）被加上 `ambiguous` 標籤是因為沒有按照 Commitizen 規範提交。
:::

## 功能

### 自動觸發

- 針對議題 (Issue) 與拉取請求 (Pull Request)，Issue Bot 會根據關鍵字自動進行分類，並加上 `module` 系列、`Client` 系列、`ambiguous`、`translation required` 等標籤。    
  具體關鍵字可以參閱 [設定檔案](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml)。
- 針對身分設為公開 (Public) 的 MAA 成員，為其發起的議題與拉取請求加上 `MAA Team` 標籤。

#### 議題 (Issue) 及其評論

- 為上傳檔案失敗的議題增加 `incomplete` 標籤並留言評論，提醒用戶檔案上傳失敗。

#### 拉取請求 (Pull Request)

Issue Bot 會對拉取請求標題的格式進行簡單審查。除非標題以下列任一單字開頭，否則它會增加 `ambiguous` 標籤：

- `build`, `chore`, `ci`, `doc`, `docs`, `feat`, `fix`, `perf`, `refactor`, `rfc`, `style`, `test`
- `Merge`, `merge`, `Revert`, `revert`, `Reapply`, `reapply`

### 手動觸發

請多利用關鍵字詳細描述問題以自動觸發分類標籤，盡量少使用下列指令。  
但**當您知道自己的行為會導致 Issue Bot 誤解時除外**。

#### 議題 (Issue) 及拉取請求 (Pull Request)

- `Remove {LABEL_NAME}`：可以刪除一個標籤。
- `Remove labels`：可以刪除所有標籤。
- `Skip {LABEL_NAME}`：可以跳過一個標籤。
- `Skip labels`：可以跳過所有標籤。

#### 議題評論 (Issue Comments) 及拉取請求評論 (Pull Request Comments)

- `Skip {LABEL_NAME}`：可以保證不增加特定標籤。
- `Skip labels`：可以保證不增加任何標籤。
- 以下幾種方法可以為議題增加 `fixed` 標籤：<sup>1</sup>
  - `https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH} fixed`
  - `fixed by https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH}`
  - `{VERSION} fixed`
  - `fixed by {VERSION}`
- `Duplicate of #{ISSUE_NUMBER}`：可以為目前議題增加 `duplicate` 標籤。
- **當您知道自己的議題評論會導致 Issue Bot 誤解時，請盡量添加一些 skip 操作。**

::: info 注意
<sup>1</sup> 這裡的 COMMIT_HASH 需要完整的 40 位元字元。
:::

#### 推送 (Push)

對於一次推送中的任意提交（Commit）：

- 在 Commit Message 中包含以下任一內容，可以為對應議題加上 `fixed` 標籤：
  - `fix #{ISSUE_NUMBER}`
  - `close #{ISSUE_NUMBER}`
  - `fix https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
  - `close https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`