---
icon: mdi:certificate-outline
order: 8
---

# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## Scope and release process

GitHub Actions submits Windows stable and beta builds to SignPath. Publication stops if the release certificate is unavailable, approval is missing, or signature verification fails. Older releases may be unsigned; check each file's digital signature properties.

We sign only the following binaries built from this repository and its pinned MaaUtils source submodule:

- `MAA.exe`
- `MAA.dll`
- `MAA.Updater.exe`
- `MaaCore.dll`
- `MaaUtils.dll`

Third-party dependencies retain the files and signatures supplied by upstream. All signed files share a product version, with file names and product names checked before signing. Private keys remain in SignPath's HSM. Windows identifies the signing publisher as SignPath Foundation.

Stable and beta releases use `.github/workflows/ci.yml`, build on GitHub-hosted runners, and verify signatures before packaging. Every release signing request requires manual approval. Nightly builds use `.github/workflows/release-nightly-ota.yml` and currently skip SignPath signing, so they continue to build and publish automatically without signing approval.

The `test-signing` policy uses an untrusted test certificate only for manually triggered CI checks that are never published. It cannot replace the release certificate.

## Team roles

- Repository permission groups: [Cpp Dev](https://github.com/orgs/MaaAssistantArknights/teams/cpp-dev) / [Cs Dev](https://github.com/orgs/MaaAssistantArknights/teams/cs-dev) / [Ci Dev](https://github.com/orgs/MaaAssistantArknights/teams/ci-dev) / [Collaborators](https://github.com/orgs/MaaAssistantArknights/teams/collaborators) / [i18n Dev](https://github.com/orgs/MaaAssistantArknights/teams/i18n-dev) / [Website Dev](https://github.com/orgs/MaaAssistantArknights/teams/website-dev).
- Committers and reviewers: MAA maintainers with write access to this repository. Changes by other contributors are reviewed by maintainers before merging.
- Signing approver: [uye / ABA2396](https://github.com/ABA2396).
- The dedicated `CI builds` user submits signing requests.

Maintainers must use multi-factor authentication for both SignPath and GitHub.

## Privacy

See the [privacy and data upload sections of our user agreement](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/terms-of-service.md#42-隐私原则). Anonymous reporting to Penguin Statistics and Arknights Yituliu is enabled by default and can be disabled. Reports include a Penguin Statistics reporting ID when the user supplies one.
