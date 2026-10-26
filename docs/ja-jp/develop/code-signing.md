---
icon: mdi:certificate-outline
order: 8
---

# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## 署名対象とリリース手順

Windows の正式版、ベータ版、ナイトリー版は GitHub Actions から SignPath に署名を申請します。正式証明書が利用できない場合、承認されない場合、または署名の検証に失敗した場合、公開処理を停止します。過去のリリースには署名がない場合があるため、各ファイルのデジタル署名を確認してください。

署名対象は、このリポジトリと固定されたバージョンの MaaUtils ソースサブモジュールからビルドした次のファイルのみです。

- `MAA.exe`
- `MAA.dll`
- `MAA.Updater.exe`
- `MaaCore.dll`
- `MaaUtils.dll`

サードパーティーの依存ファイルと署名は、上流から提供された状態を保持します。署名対象の製品バージョンを統一し、ファイル名と製品名も検証します。秘密鍵は SignPath の HSM 内に保管され、Windows には署名の発行元として SignPath Foundation が表示されます。

正式版とベータ版は `.github/workflows/ci.yml`、ナイトリー版は `.github/workflows/release-nightly-ota.yml` を使用します。いずれも GitHub ホスト型ランナーでビルドし、パッケージ化する前に署名を検証します。ナイトリー版を含め、すべてのリリース署名には手動承認が必要です。

`test-signing` は信頼されないテスト証明書を使用し、手動実行し、リリースしない CI 検証専用です。リリース証明書の代わりには使用しません。

## チームの役割

- リポジトリの権限グループ：[Cpp Dev](https://github.com/orgs/MaaAssistantArknights/teams/cpp-dev) / [Cs Dev](https://github.com/orgs/MaaAssistantArknights/teams/cs-dev) / [Ci Dev](https://github.com/orgs/MaaAssistantArknights/teams/ci-dev) / [Collaborators](https://github.com/orgs/MaaAssistantArknights/teams/collaborators) / [i18n Dev](https://github.com/orgs/MaaAssistantArknights/teams/i18n-dev) / [Website Dev](https://github.com/orgs/MaaAssistantArknights/teams/website-dev)。
- コミッターとレビュアー：このリポジトリへの書き込み権限を持つ MAA メンテナー。その他の貢献者による変更は、メンテナーがレビューしてからマージします。
- 署名承認者：[uye / ABA2396](https://github.com/ABA2396)。
- 専用の `CI builds` ユーザーが署名を申請します。

メンテナーは SignPath と GitHub の両方で多要素認証を使用する必要があります。

## プライバシー

[利用規約のプライバシーとデータ送信に関する説明](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/terms-of-service.md#42-隐私原则)をご確認ください。Penguin Statistics と Arknights Yituliu への匿名データ送信は初期状態で有効ですが、無効にできます。Penguin Statistics の報告 ID を入力した場合は、その ID も送信されます。
