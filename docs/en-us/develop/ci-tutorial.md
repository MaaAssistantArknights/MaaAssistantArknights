---
order: 7
icon: devicon-plain:githubactions
---

# CI System Analysis

MAA leverages GitHub Actions to complete a large amount of automation work, including website building, automatic resource updates, final file building and release processes, etc. Over time, these CI workflows have gradually become nested, with some even pointing to other repositories. This document aims to provide a brief introduction for those who want to improve MAA's CI system.

Before reading this document, it's best to have a basic understanding of MAA's project structure and components.

::: tip
You can quickly navigate to the section you want to see by searching for CI file names within this page
:::

Workflow files are all stored under `.github/workflows`, and each file can be categorized into the following functional parts:

+ [Code Testing](#code-testing)
+ [Code Building](#code-building)
+ [Version Release](#version-release)
+ [Resource Updates](#resource-updates)
+ [Website Building](#website-building)
+ [Issues Management](#issues-management)
+ [Pull Requests Management](#pull-requests-management)
+ [MirrorChyan Related](#mirrorchyan-related)
+ [Others](#others)

Additionally, we use [pre-commit.ci](https://pre-commit.ci/) to implement automatic code formatting and image resource optimization, which runs automatically after creating PRs and generally requires no special attention.

## GitHub Actions Section

### Code Testing

`smoke-testing.yml`

This workflow is mainly responsible for basic testing of MaaCore, including resource file loading, simple task execution tests, etc.

Since test cases haven't been updated for a long time, this workflow is now basically used to ensure resource files don't contain errors and that MaaCore code doesn't have fatal errors (the kind that affects building).

### Code Building

`ci.yml`

This workflow is responsible for full code building work, including all MAA components. The build artifacts are runnable MAA instances.

In addition to the necessary MaaCore, Windows build artifacts include MaaWpfGui, macOS build artifacts include MaaMacGui, and Linux build artifacts include MaaCLI.

This workflow runs automatically on any new commit and PR. When triggered by a release PR, the build artifacts from this run will be used directly for release and will create a Release.

### Version Release

Version release is the necessary operation to publish updates to users, consisting of the following workflows:

+ `release-nightly-ota.yml` Release nightly builds
+ `release-ota.yml` Release stable/beta versions
  + `gen-changelog.yml` Generate changelog for stable/beta versions
  + `pr-auto-tag.yml` Generate tags for stable/beta versions

::: tip
The "ota" in the above file names stands for Over-the-Air, which is what we commonly call "incremental update packages". Therefore, MAA's release process actually includes building OTA packages for past versions.
:::

#### Nightly Builds

`release-nightly-ota.yml`

This workflow runs automatically at 22:00 UTC daily to ensure nightly build release frequency. Of course, you can also manually release when making changes that need verification.

Note that nightly builds are only released for Windows users; macOS and Linux users cannot receive nightly updates.

#### Stable/Beta Versions

The release process for these two channels is relatively more complex. We'll explain the role of each workflow by simulating a release process:

1. Create a PR from `dev` to `master` branch, and the PR name must be `Release v******`
2. `gen-changelog.yml` generates a changelog from the most recent stable/beta version to the current version (as a new PR)
3. Manually adjust the changelog and add brief descriptions
4. Merge the PR, triggering `pr-auto-tag.yml` to create tags and sync branches
5. The Release event triggers `release-ota.yml`, which builds OTA packages and uploads attachments after tagging master

### Resource Updates

This section of workflows is mainly responsible for MAA's resource updates and optimization, with the following specific workflows:

+ `res-update-game.yml` Executes periodically to pull game resources from specified repositories
+ `sync-resource.yml` Syncs resources to the MaaResource repository for resource updates
+ `optimize-templates.yml` Optimizes template image sizes

### Website Building

`website-workflow.yml`

This workflow is mainly responsible for building and publishing MAA's official website, including both the main page and documentation components.

Please note that website publishing is tightly bound to releases. Regular modifications to web components only trigger builds to ensure no errors occur, and actual deployment to Azure only happens during releases.

### Issues Management

`issue-checker.yml`

Uses regex matching to tag various Issues, categorizing and marking Issue content for easier viewing and management.

`issue-checkbox-checker.yml`

Uses regex matching to automatically close Issues that check "I have not read carefully".
If "I have not read carefully" is not checked, all checkboxes are collapsed.

`stale.yml`

Checks Bug Issues that have had no activity for more than 90 days, marks them and sends notifications, then closes them after 7 more days if still inactive.

### Pull Requests Management

`pr-checker.yml`

This workflow checks whether commit messages in PRs conform to [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) and whether they contain merge commits, providing reminders if these conditions are met.

### MirrorChyan Related

MirrorChyan is a paid update mirror service, with the following related workflows:

+ `mirrorchyan.yml` Sync update packages to MirrorChyan
+ `mirrorchyan_release_note.yml` Generate MirrorChyan Release Notes

### Others

`markdown-checker.yml`

Responsible for checking all Markdown files in the repository for invalid links.

`blame-ignore.yml`

Automatically ignores commits with commit messages containing `blame ignore` to keep repository history clean.

`cache-delete.yml`

Cleans up related caches after PR merges to save cache usage.
