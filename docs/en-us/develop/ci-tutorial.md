---
order: 7
icon: 'devicon-plain:githubactions'
---
# CI System Overview

MAA utilizes GitHub Actions to automate a significant amount of work, including website builds, automatic resource updates, final artifact builds and releases, and more. Over time, these CI workflows have become increasingly nested, with some even referencing other repositories. This document aims to provide a brief introduction for those looking to improve MAA's CI system.

Before reading this document, it's best to have a basic understanding of MAA's project structure and components.

::: tip
You can quickly navigate to the desired section by searching for the CI filename within this page.
:::

All workflow files are located in `.github/workflows`. These files can be categorized by functionality as follows:

- [Code Testing](#code-testing)
- [Code Building](#code-building)
- [Code Security Checks](#code-security-checks)
- [Version Releases](#version-releases)
- [Resource Updates](#resource-updates)
- [Website Building](#website-building)
- [Issues Management](#issues-management)
- [Pull Requests Management](#pull-requests-management)
- [MirrorChyan Related](#mirrorchyan-related)
- [Other](#other)

Additionally, we use [pre-commit.ci](https://pre-commit.ci/) to automatically format code and optimize image resources. It runs automatically after creating a PR and generally requires no special attention.

## GitHub Actions Section

### Code Testing

`smoke-testing.yml`

This workflow is primarily responsible for basic testing of MaaCore, including resource file loading and running tests for some simple tasks.

Since the test cases haven't been updated for a while, this workflow now essentially ensures that resource files don't contain errors and that MaaCore's code doesn't have fatal errors affecting the build.

### Code Building

`ci.yml`

This workflow is responsible for fully building the code, including all MAA components. The build artifacts are the runnable MAA.

In addition to the essential MaaCore, the Windows build artifact includes MaaWpfGui, the MacOS build artifact includes MaaMacGui, and the Linux build artifact includes MaaCLI.

This workflow runs automatically on any new commit or PR. When triggered by a release PR, the build artifacts from this run are directly used for the release, and a Release is created.

### Code Security Checks

Code security checks analyze code and workflows for security vulnerabilities using CodeQL. The specific workflows are:

`codeql-core.yml`

This workflow is responsible for security analysis of the C++ and C# code in MaaCore and MaaWpfGui, detecting potential security vulnerabilities.

This workflow runs automatically on PRs that modify the relevant source code and also performs a scheduled check daily at 11:45 UTC.

`codeql-wf.yml`

This workflow is responsible for security analysis of the GitHub Actions workflow files themselves, ensuring the security of the CI/CD process.

This workflow runs automatically on PRs that modify workflow files and also performs a scheduled check daily at 12:00 UTC.

### Version Releases

Version releases, referred to simply as releases, are necessary operations to publish updates to users. They consist of the following workflows:

- `release-nightly-ota.yml` Releases the beta version
- `release-ota.yml` Releases the stable/public beta version
  - `release-preparation.yml` Generates the changelog and prepares the release for stable/public beta versions
  - `pr-auto-tag.yml` Creates tags for stable/public beta versions

::: tip
The "ota" in the filenames above stands for Over-the-Air, which is what we commonly call "incremental update packages." Therefore, MAA's release process actually includes steps to build OTA packages for past versions.
:::

#### Beta Version

`release-nightly-ota.yml`

This workflow runs automatically daily at 22:00 UTC to maintain the release frequency of the beta version. Of course, you can also manually trigger a release when changes need verification.

Note that beta releases are only for Windows users; MacOS and Linux users do not receive beta updates.

#### Stable/Public Beta Version

The release process for these two channels is a bit more complex. We'll explain the role of each workflow by simulating a release step-by-step:

1. Create a PR from the `dev-v2` branch to the `master` branch, and the PR title must be `Release v******`
2. `release-preparation.yml` generates a changelog from the recent stable/public beta version to the current version (in the form of a new PR)
3. Manually adjust the changelog and add a brief description
4. Merge the PR, triggering `pr-auto-tag.yml`, which creates a tag and synchronizes the branches
5. The Release event triggers `release-ota.yml`, which builds OTA packages and uploads attachments after tagging the master branch

### Resource Updates

These workflows are primarily responsible for updating and optimizing MAA's resources. The specific workflows are:

- `res-update-game.yml` Runs periodically, pulling game resources from a specified repository
- `sync-resource.yml` Synchronizes resources to the MaaResource repository for resource updates
- `optimize-templates.yml` Optimizes image sizes, including template images

### Website Building

`website-workflow.yml`

This workflow is primarily responsible for building and publishing the MAA documentation site.

Please note that website deployment is strongly tied to releases. When modifying web components, the site is only built to ensure no errors occur. It is officially deployed to GitHub Pages only during a release.

### Issues Management

`issue-checker.yml`

Uses regular expression matching to tag Issues, categorizing and marking Issue content for easier viewing and management.

`issue-checkbox-checker.yml`

Uses regular expression matching to automatically close Issues where "I have not read carefully" is checked. If "I have not read carefully" is not checked, it collapses all checkboxes.

`stale.yml`

Checks for Bug Issues that have been inactive for over 90 days, marks them, and sends a notification. If there is still no activity after 7 days, the issue is closed.

### Pull Requests Management

`pr-checker.yml`

This workflow is used to check if the Commit Messages in a PR comply with [Conventional Commits](https://www.conventionalcommits.org/zh-hans/v1.0.0/) and if they contain Merge Commits. If these conditions are met, a prompt will be issued.

### MirrorChyan Related

MirrorChyan is a paid update mirroring service. Workflows related to it are:

- `release-package-distribution.yml` Synchronizes update packages to MirrorChyan
- `mirrorchyan_release_note.yml` Generates Release Notes for MirrorChyan

### Other

`markdown-checker.yml`

Responsible for checking all Markdown files in the repository for invalid links.

`blame-ignore.yml`

Automatically ignores commits whose messages contain `blame ignore`, ensuring a clean repository history.

`cache-delete.yml`

Cleans up related caches after a PR is merged to save cache usage.

`update-submodules.yml`

Periodically updates submodules like MaaMacGui and maa-cli to their latest versions. This workflow runs automatically daily at 21:50 UTC (before the daily beta release) to ensure submodules remain up-to-date.
