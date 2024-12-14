---
order: 4
icon: bxs:bot
---

# How to use Issue Bot

The action of Issue Bot is [issue-checker](https://github.com/zzyyyl/issue-checker), with configuration file [issue-checker.yml](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml).

**Note that your merge request will be marked as `ambiguous` when you are not committing with Commitizen specification.**

## Features

### Auto Notification

- Adds labels to issues and pull requests, e.g., `module`, `Client`, `ambiguous`, `translation required`, etc.
    Issue Bot will add categories based on keywords.
    Please refer to the [configuration file](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/master/.github/issue-checker.yml) for the keywords.
- Adds the `MAA Team` label to issues and pull requests for MAA public team members.

#### Issues and Their Comments

- Adds label `incomplete` and comments to notify users that the files of their issues are not uploaded successfully.

#### Pull Request

Issue Bot will check the title of your pull request. Unless the title starts with one of the following keywords, the pull request will be marked as `ambiguous`.

- `build` `chore` `ci` `doc` `docs` `feat` `fix` `perf` `refactor` `rfc` `style` `test`
- `Merge` `merge` `Revert` `revert`

### Triggering Manually

It is better to trigger Issue Bot with keywords instead of commands listed below, unless that **you are aware that your action may confuse Issue Bot**.

#### Issues and Pull Requests

- `Remove {LABEL_NAME}` will delete a label.
- `Remove labels` will delete all labels.
- `Skip {LABEL_NAME}` will skip a label.
- `Skip labels` will skip all labels.

#### Issue Comments and Pull Request Comments

- `Skip {LABEL_NAME}` will make sure no labels of the specified type are added.
- `Skip labels` will make sure no labels are added.
- To add `fixed` label, you can use any of the commands below:<sup>1</sup>
  - `https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH} fixed`
  - `fixed by https://github.com/MaaAssistantArknights/MaaAssistantArknights/commit/{COMMIT_HASH}`
  - `{VERSION} fixed`
  - `fixed by {VERSION}`
- `Duplicate of #{ISSUE_NUMBER}` will add `duplicate` label to the issue.
- **You can add some `skip` operations if your comment may confuse Issue Bot.**

_Note<sup>1</sup>: The `COMMIT_HASH` here is the full 40-character hash._

#### Push

For any commit in a push:

- `fixed` label will be added if the commit message contains one of the following:
  - `fix #{ISSUE_NUMBER}`
  - `close #{ISSUE_NUMBER}`
  - `fix https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
  - `close https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/{ISSUE_NUMBER}`
