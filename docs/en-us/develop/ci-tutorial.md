---
order: 7
icon: devicon-plain:githubactions
---

# CI System Analysis

MAA utilizes GitHub Actions to accomplish a wide range of automation tasks, including website building, automatic resource updates, final file building, and version releases. Over time, these CI workflows have become increasingly nested, with some even linking to other repositories. This document aims to provide a brief introduction for those who want to improve MAA's CI system.

Before reading this document, it's best to have a basic understanding of MAA's project structure and components.

::: tip
You can quickly navigate to the section you want to see by searching for the CI filename on this page
:::

The workflow files are stored in `.github/workflows`, and can be categorized by function as follows:

+ [Code Testing](#code-testing)
+ [Code Building](#code-building)
+ [Version Release](#version-release)
+ [Resource Updates](#resource-updates)
+ [Website Building](#website-building)
+ [Issues Management](#issues-management)
+ [Pull Requests Management](#pull-requests-management)
+ [MirrorChyan Related](#mirrorchyan-related)
+ [Other](#other)

Additionally, we use [pre-commit.ci](https://pre-commit.ci/) to implement automatic code formatting and image resource optimization. It runs automatically after a PR is submitted and generally doesn't require special attention.

## GitHub Action Section

### Code Testing

`smoke-testing.yml`

This workflow is mainly responsible for basic testing of MaaCore, including resource file loading, simple task execution tests, etc.

Since the test cases have not been updated for a long time, this workflow now primarily ensures that resource files don't contain errors and that MaaCore code doesn't have fatal errors (the kind that affects building).

### Code Building

`ci.yml`

This workflow is responsible for full-scale code building, including all MAA components. The build output is the runnable MAA.

In addition to the essential MaaCore, Windows build artifacts include MaaWpfGui, macOS build artifacts include MaaMacGui, and Linux build artifacts include MaaCLI.

This workflow runs automatically whenever there are new commits and PRs. When triggered by a release PR, the build artifacts will be used directly for the release, and a Release will be created.

### Version Release

Version release is a necessary operation to publish updates to users, consisting of the following workflows:

+ `release-nightly-ota.yml` for releasing nightly builds
+ `release-ota.yml` for releasing stable/beta versions
  + `gen-changelog.yml` for generating changelogs for stable/beta versions
  + `pr-auto-tag.yml` for generating tags for stable/beta versions

::: tip
The "ota" in the filenames above stands for "Over-the-Air," referring to what we commonly call "incremental update packages." Therefore, MAA's release process includes steps to build OTA packages for previous versions.
:::

#### Nightly Build

`release-nightly-ota.yml`

This workflow runs automatically at 22:00 UTC daily to ensure regular nightly releases. Of course, you can also manually trigger a release when you need to verify changes.

Note that nightly releases are only available to Windows users; macOS and Linux users cannot receive nightly updates.

#### Stable/Beta Version

The release process for these two channels is a bit more complex. We'll explain the role of each workflow by simulating a release process:

1. Create a PR from the `dev` branch to the `master` branch, with the PR name following the format `Release v******`
2. `gen-changelog.yml` will generate a changelog from the recent stable/beta version to the current version (in the form of a new PR)
3. Manually adjust the changelog and add a brief description
4. Merge the PR, triggering `pr-auto-tag.yml`, which creates a tag and synchronizes branches
5. The Release event triggers `release-ota.yml`, which builds OTA packages and uploads attachments after tagging the master branch

### Resource Updates

This section of workflows is mainly responsible for MAA's resource updates and optimization, specifically:

+ `res-update-game.yml` runs periodically to pull game resources from specified repositories
+ `sync-resource.yml` synchronizes resources to the MaaResource repository for resource updates
+ `optimize-templates.yml` optimizes template image sizes

### Website Building

`website-workflow.yml`

This workflow is mainly responsible for building and publishing the MAA official website, including both the main page and documentation components.

Note that website publication is strongly tied to version releases. Regular modifications to website components only trigger building to ensure there are no errors, while formal deployment to Azure only happens during version releases.

### Issues Management

`issue-checker.yml`

Uses regex matching to tag Issues, categorizing and marking Issue content for easier viewing and management.

`stale.yml`

Checks Bug Issues with no activity for over 90 days, marks them and sends notifications. If there's still no activity after 7 days, the issue is closed.

### Pull Requests Management

`pr-checker.yml`

This workflow checks whether PR Commit Messages comply with [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) and whether they contain Merge Commits. If these conditions are met, it will provide a notification.

### MirrorChyan Related

MirrorChyan is a paid update mirror service. Related workflows include:

+ `mirrorchyan.yml` synchronizes update packages to MirrorChyan
+ `mirrorchyan_release_note.yml` generates Release Notes for MirrorChyan

### Other

`markdown-checker.yml`

Checks all Markdown files in the repository for invalid links

`blame-ignore.yml`

Automatically ignores commits with "blame ignore" in their Commit Message to keep the repository history clean

`cache-delete.yml`

Clears related caches after PRs are merged to save cache usage
