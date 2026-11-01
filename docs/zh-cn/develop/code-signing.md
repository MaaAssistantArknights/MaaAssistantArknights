---
icon: mdi:certificate-outline
order: 9
---

# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## 签名范围与发布流程

Windows 正式版和公测版（Beta）的签名由 GitHub Actions 提交至 SignPath；正式证书未就绪、审批未通过或签名校验失败时，发布流程停止。历史版本可能未签名，以文件的数字签名属性为准。

签名仅覆盖从本项目源代码及固定版本的 MaaUtils 子模块构建的以下文件：

- `MAA.exe`
- `MAA.dll`
- `MAA.Updater.exe`
- `MaaCore.dll`
- `MaaUtils.dll`

已有的有效上游签名会保留。部分依赖（包括 NetBeauty 提供的修改版 `hostfxr.dll`）可能未签名，不属于当前签名白名单。CI 会上传 `MAA-signpath-audit-<arch>` 产物，列出所有 EXE 和 DLL 的签名状态；依赖未签名不代表可以使用本项目的证书重签。

签名使用相同的产品版本，Windows 的 PR 构建与签名构建都会校验文件名、产品名及产品版本。私钥保留在 SignPath 的 HSM 中，Windows 显示的签名发布者为 SignPath Foundation。

正式版和公测版（Beta）使用 `.github/workflows/ci.yml`，在 GitHub 托管运行器构建，并在打包前完成签名校验。签名后的原生运行时文件也供下游包使用。每次发布签名需要批准人手动审批。内测版（Nightly）使用 `.github/workflows/release-nightly-ota.yml`，暂不进行 SignPath 签名，按原流程自动构建和发布，无需签名审批。

首次签名发布前，需要将仓库 Actions 变量 `SIGNPATH_RELEASE_CERTIFICATE_SHA256` 设为已签发正式证书的 SHA-256 指纹，格式为 64 位十六进制，不含空格或冒号。更换证书时同步更新此变量。CI 除了验证证书信任、发布者和时间戳，还会匹配该指纹。

`test-signing` 使用不受用户系统信任的测试证书，仅用于手动触发且不发布的 CI 验证，不能替代发布证书。

## 团队职责

- 仓库权限组：[Cpp Dev](https://github.com/orgs/MaaAssistantArknights/teams/cpp-dev) / [Cs Dev](https://github.com/orgs/MaaAssistantArknights/teams/cs-dev) / [Ci Dev](https://github.com/orgs/MaaAssistantArknights/teams/ci-dev) / [Collaborators](https://github.com/orgs/MaaAssistantArknights/teams/collaborators) / [i18n Dev](https://github.com/orgs/MaaAssistantArknights/teams/i18n-dev) / [Website Dev](https://github.com/orgs/MaaAssistantArknights/teams/website-dev)。
- 提交者与审阅者：具有本仓库写入权限的 MAA 维护者；其他贡献者的改动由维护者审阅后合并。
- 签名批准人：[uye / ABA2396](https://github.com/ABA2396)。
- 签名请求由专用的 `CI builds` 用户提交。

维护者必须为 SignPath 与 GitHub 账号启用多因素认证。

## 隐私

详见[用户协议的隐私原则与数据上传说明](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/terms-of-service.md#42-隐私原则)。MAA 默认启用向企鹅物流数据统计和明日方舟一图流提交匿名数据的功能，用户可以关闭上报；填写企鹅物流汇报 ID 后，上报包含该标识符。
