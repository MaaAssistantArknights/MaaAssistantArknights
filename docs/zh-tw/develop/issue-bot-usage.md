---
order: 4
icon: bxs:bot
---
# Issue Bot 使用方法

Issue Bot 使用的 action 為 [issue-checker](https://github.com/zzyyyl/issue-checker) ，配置文件為 [issue-checker.yml](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml) 。

::: warning
Pull Request 被增加 `ambiguous` 標籤是因為沒有按照 commitizen 規則提交
:::

## 功能

### 自動觸發

- 給 issue 和 Pull Request 增加標籤，包括 `module` 系列、`Client` 系列、`ambiguous`、`translation required` 等。  
  Issue Bot 會根據關鍵字自動增加分類標籤。  
  具體關鍵字可以參考 [配置檔案](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml)。
- 給作者是可見性設定為 public 的 MAA 成員的 issue 和 Pull Request 增加 `MAA Team` 標籤。

#### Issue 及其評論

- 給文件上傳失敗的 issue 增加 `incomplete` 標籤並評論，告訴用戶文件上傳失敗。

#### Pull Request

Issue Bot 會對 Pull Request 標題的格式進行簡單審查。它會增加 `ambiguous` 標籤，除非 Pull Request 標題以下列任一單詞開頭：

- `build` `chore` `ci` `doc` `docs` `feat` `fix` `perf` `refactor` `rfc` `style` `test`
- `Merge` `merge` `Revert` `revert`

### 手動觸發

多使用關鍵字詳細描述問題來自動觸發分類標籤，少使用下列指令。  
但 **當你知道自己的行為會導致 Issue Bot 誤解時除外**。

#### Issue 及 Pull Request

- `Remove {LABEL_NAME}` 可以刪除一個標籤。
- `Remove labels` 可以刪除所有標籤。
- `Skip {LABEL_NAME}` 可以跳過一個標籤。
- `Skip labels` 可以跳過所有標籤。

#### Issue Comments 及 Pull Request Comments

- `Skip {LABEL_NAME}` 可以保證不增加標籤。
- `Skip labels` 可以保證不增加任何標籤。
- 以下幾種方法可以為 issue 增加 `fixed` 標籤：<sup>1</sup>  
  - `https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH} fixed`
  - `fixed by https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH}`
  - `{VERSION} fixed`
  - `fixed by {VERSION}`
- `Duplicate of #{ISSUE_NUMBER}` 可以在目前 issue 增加 `duplicate` 標籤。
- **當你知道自己的 issue 評論會導致 Issue Bot 誤解時，盡量添加一些 skip 操作。**

::: info 注意
<sup>1</sup> 這裡的 COMMIT_HASH 需要完整的 40 位
:::

#### 推送（Push）

對於一個推送中的任意提交：

- 在 commit message 中包含以下幾種任意一個，可以在對應 issue 加上 `fixed` 標籤：
  - `fix #{ISSUE_NUMBER}`
  - `close #{ISSUE_NUMBER}`
  - `fix https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
  - `close https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
