# Issue bot 使用方法

Issue bot 使用的 action 为 [issue-checker](https://github.com/zzyyyl/issue-checker)，配置文件为 [issue-checker.yml](.github/issue-checker.yml)。

## 功能

### 自动触发

- 给上传文件失败的议题评论，告诉用户文件上传失败。
- 给 议题 和 拉取请求 增加标签，包括 `module` 系列，`Client` 系列，`ambiguous`，`translation required` 等。  
  Issue bot 会根据关键词自动增加分类标签，也可以通过手动添加的方式增加或删除标签。  
  具体关键词可以参考[配置文件](.github/issue-checker.yml)。
- 给可见性设置为 public 的 MAA 成员增加 `MAA Team` 标签。

### 手动触发

尽量不要手动增加或删除分类标签，因为手动增加或删除的分类标签可能在修改后会被恢复原样。

#### 议题（Issue）及其评论

在 **修改/新增** 一个议题时：

- `Add {LABEL_NAME}` 可以增加一个标签。
- `Remove {LABEL_NAME}` 可以删除一个标签。
- `Remove labels` 可以删除所有标签。

在 **修改/新增** 一个议题评论时：

- `Add {LABEL_NAME}` 可以增加一个标签。<sup>1</sup>
- `Remove {LABEL_NAME}` 可以删除一个标签。<sup>2</sup>
- `Remove labels` 可以删除所有标签。<sup>2</sup>
- 以下几种方法可以为议题增加 `fixed` 标签：<sup>3</sup>  
  - `https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH} fixed`
  - `fixed by https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH}`
  - `{VERSION} fixed`
  - `fixed by {VERSION}`
- `Duplicate of #{ISSUE_NUMBER}` 可以为当前议题增加 `duplicate` 标签。
- **当你知道自己的议题评论会导致 Issue Bot 误解时，尽量添加一些 Remove 操作。**

_Note<sup>1</sup>: `Add ambiguous` 不会增加 `ambiguous` 标签，但可以保证不删除 `ambiguous` 标签。_

_Note<sup>2</sup>: `Remove {LABEL_NAME}` 和 `Remove labels` 不会删除 `ambiguous` 外的任何标签，但可以保证不增加标签。_

_Note<sup>3</sup>: 这里的 COMMIT_HASH 需要完整的 40 位。_

#### 拉取请求（Pull Request）及其评论

与[议题（Issue）及其评论](#议题issue及其评论)基本相同。另外：

- `Release {VERSION}` 会增加 `release` 标签。
- **不会**自动添加 `fixed`, `duplicate` 标签。

#### 推送（Push）

- 在 commit message 中包含以下几种任意一个，可以为对应议题加上 `fixed` 标签：
  - `fix #{ISSUE_NUMBER}`
  - `close #{ISSUE_NUMBER}`
  - `fix https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
  - `close https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
