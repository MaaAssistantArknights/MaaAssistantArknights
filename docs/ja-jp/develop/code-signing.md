---
icon: mdi:certificate-outline
order: 9
---

# Code signing policy

Free code signing provided by [SignPath.io](https://signpath.io/), certificate by [SignPath Foundation](https://signpath.org/).

## 署名対象とリリース手順

Windows の安定版とベータ版は GitHub Actions から SignPath に署名を申請します。正式証明書が利用できない場合、承認されない場合、または署名の検証に失敗した場合、公開処理を停止します。過去のリリースには署名がない場合があるため、各ファイルのデジタル署名を確認してください。

署名対象は、このリポジトリと固定されたバージョンの MaaUtils ソースサブモジュールからビルドした次のファイルのみです。

- `MAA.exe`
- `MAA.dll`
- `MAA.Updater.exe`
- `MaaCore.dll`
- `MaaUtils.dll`

既存の有効な上流の署名は保持します。一部の依存ファイル（NetBeauty が提供する変更版 `hostfxr.dll` など）は署名されていない場合があり、現在の署名許可リストには含まれません。CI はすべての EXE と DLL の署名状態を記録した `MAA-signpath-audit-<arch>` 成果物をアップロードします。未署名であることだけを理由に、本プロジェクトの証明書で依存ファイルを再署名することはありません。

署名対象の製品バージョンを統一し、Windows の PR ビルドと署名ビルドでファイル名、製品名、製品バージョンを検証します。秘密鍵は SignPath の HSM 内に保管され、Windows には署名の発行元として SignPath Foundation が表示されます。

安定版とベータ版は `.github/workflows/ci.yml` を使用し、GitHub ホスト型ランナーでビルドして、パッケージ化する前に署名を検証します。署名済みのネイティブランタイムファイルは下流のパッケージにも使用します。すべてのリリース署名には手動承認が必要です。内部テスト版（Nightly）は `.github/workflows/release-nightly-ota.yml` を使用し、現在は SignPath による署名を行わず、署名承認なしで従来どおり自動ビルドと公開を継続します。

初回の署名付きリリース前に、リポジトリの Actions 変数 `SIGNPATH_RELEASE_CERTIFICATE_SHA256` に発行済み正式証明書の SHA-256 フィンガープリントを設定してください。空白やコロンを含まない 64 桁の十六進数を使用し、証明書を交換するときも更新します。CI は証明書の信頼、発行元、タイムスタンプに加え、このフィンガープリントを検証します。

`test-signing` は信頼されないテスト証明書を使用し、手動実行し、リリースしない CI 検証専用です。リリース証明書の代わりには使用しません。

## チームの役割

- リポジトリの権限グループ：[Cpp Dev](https://github.com/orgs/MaaAssistantArknights/teams/cpp-dev) / [Cs Dev](https://github.com/orgs/MaaAssistantArknights/teams/cs-dev) / [Ci Dev](https://github.com/orgs/MaaAssistantArknights/teams/ci-dev) / [Collaborators](https://github.com/orgs/MaaAssistantArknights/teams/collaborators) / [i18n Dev](https://github.com/orgs/MaaAssistantArknights/teams/i18n-dev) / [Website Dev](https://github.com/orgs/MaaAssistantArknights/teams/website-dev)。
- コミッターとレビュアー：このリポジトリへの書き込み権限を持つ MAA メンテナー。その他の貢献者による変更は、メンテナーがレビューしてからマージします。
- 署名承認者：[uye / ABA2396](https://github.com/ABA2396)。
- 専用の `CI builds` ユーザーが署名を申請します。

メンテナーは SignPath と GitHub の両方で多要素認証を使用する必要があります。

## プライバシー

[利用規約のプライバシーとデータ送信に関する説明](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/dev-v2/terms-of-service.md#42-隐私原则)をご確認ください。Penguin Statistics と Arknights Yituliu への匿名データ送信は初期状態で有効ですが、無効にできます。Penguin Statistics の報告 ID を入力した場合は、その ID も送信されます。
