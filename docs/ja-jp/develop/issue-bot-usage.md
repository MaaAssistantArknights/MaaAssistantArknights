---
order: 4
icon: bxs:bot
---

# Issue Botの使う方

Issue Botのアクションは[issue-checker](https://github.com/zzyyyl/issue-checker)になっており、設定ファイルである[issue-checker.yml](.github/issue-checker.yml)を利用します。

::: warning
Commitizenの仕様でコミットしていない場合、マージリクエストは`ambiguous`とマークされることに注意してください。
:::

## 特徴

### 自動通知

- `module`, `Client`, `ambiguous`, `translation required`などのラベルを Issue および Pull Request に追加します。  
  Issue Botは、キーワードを元にカテゴリーを追加します。  
  キーワードは[設定ファイル](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml)を確認して下さい。
- 作成者が可視性がパブリックに設定されている MAA メンバーであるの Issue と Pull Request の場合、`MAA Team`ラベルを追加しました。

#### Issuesとコメント

- 必要なファイルが正常にアップロードされなかった Issue に `incomplete` タグを追加し、その旨を通知するコメントを追加します。

#### Pull Request

Issue Botは Pull Request のタイトルをチェックします。タイトルが以下のキーワードで始まっていない限り、 Pull Request は `ambiguous` としてマークされます。

- `build` `chore` `ci` `doc` `docs` `feat` `fix` `perf` `refactor` `rfc` `style` `test`
- `Merge` `merge` `Revert` `revert`

### 手動でトリガーを発動させる

キーワードを使用して問題を詳細に記述し、分類タグを自動的にトリガーし、次のコマンドの使用を減らします。  
ただし、**自分の行動が Issue Bot を混乱させる可能性がありそうな内容の場合**は除きます。

#### Issues と Pull Request

- `Remove {LABEL_NAME}` でラベルを削除します。
- `Remove labels` で全てのラベルを削除します。
- `Skip {LABEL_NAME}` でラベルをスキップします。
- `Skip labels` で全てのラベルをスキップします。

#### Issueコメントと Pull Requestコメント

- `Skip {LABEL_NAME}` は、指定されたタイプのラベルが追加されないようにします。
- `Skip labels` は、ラベルが追加されないようにします。
- `fixed` ラベルを追加する場合、以下のいずれかのコマンドを使用します:<sup>1</sup>
  - `https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH} fixed`
  - `fixed by https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH}`
  - `{VERSION} fixed`
  - `fixed by {VERSION}`
- `Duplicate of #{ISSUE_NUMBER}` と入力するとissueに `duplicate` ラベルを追加します。
- **あなたのコメントが Issue Bot を混乱させる可能性がある場合、下記の `skip` コマンドを追加できます。**

::: info 注意
ここの `COMMIT_HASH` は 40 文字の完全なハッシュです。
:::

#### Push

Push 中でも Commit 可能です:

- `fixed` ラベルは、コミットメッセージに以下のいずれかが含まれている場合に追加されます:
  - `fix #{ISSUE_NUMBER}`
  - `close #{ISSUE_NUMBER}`
  - `fix https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
  - `close https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
