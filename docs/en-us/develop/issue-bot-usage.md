---
order: 4
icon: 'bxs:bot'
---
# Issue Bot Usage Guide

The action used by Issue Bot is [issue-checker](https://github.com/zzyyyl/issue-checker), and the configuration file is [issue-checker.yml](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml).

::: warning
Pull requests are labeled with `ambiguous` because they were not submitted according to the commitizen specification.
:::

## Features

### Automatic Triggering

- Add labels to Issues and Pull Requests, including the `module` series, `Client` series, `ambiguous`, `translation required`, etc.  
  Issue Bot will automatically add classification labels based on keywords.  
  For specific keywords, refer to the [configuration file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml).
- Add the `MAA Team` label to Issues and Pull Requests authored by MAA members whose visibility is set to public.

#### Issues and Their Comments

- Add the `incomplete` label and comment on issues where file uploads failed, notifying the user of the upload failure.

#### Pull Requests

Issue Bot performs a simple review of the format of pull request titles. It will add the `ambiguous` label unless the pull request title starts with any of the following words:

- `build` `chore` `ci` `doc` `docs` `feat` `fix` `perf` `refactor` `rfc` `style` `test`
- `Merge` `merge` `Revert` `revert` `Reapply` `reapply`

### Manual Triggering

Use detailed descriptions with keywords to automatically trigger classification labels more often, and use the following commands less.  
**However, exceptions apply when you know your actions will cause Issue Bot to misinterpret.**

#### Issues and Pull Requests

- `Remove {LABEL_NAME}` can remove a label.
- `Remove labels` can remove all labels.
- `Skip {LABEL_NAME}` can skip a label.
- `Skip labels` can skip all labels.

#### Issue Comments and Pull Request Comments

- `Skip {LABEL_NAME}` ensures that the label will not be added.
- `Skip labels` ensures that no labels will be added.
- The following methods can add the `fixed` label to an issue:<sup>1</sup>
  - `https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH} fixed`
  - `fixed by https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH}`
  - `{VERSION} fixed`
  - `fixed by {VERSION}`
- `Duplicate of #{ISSUE_NUMBER}` can add the `duplicate` label to the current issue.
- **When you know your issue comment will cause Issue Bot to misinterpret, try to add some skip operations.**

::: info Note
<sup>1</sup> The COMMIT_HASH here requires the full 40 characters.
:::

#### Push

For any commit in a push:

- Including any of the following in the commit message can add the `fixed` label to the corresponding issue:
  - `fix #{ISSUE_NUMBER}`
  - `close #{ISSUE_NUMBER}`
  - `fix https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
  - `close https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
