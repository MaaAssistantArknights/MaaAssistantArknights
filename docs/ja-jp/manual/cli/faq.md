---
order: 2
icon: ph:question-fill
---

# よくある質問

この文書は機械翻訳です。もし可能であれば、中国語の文書を読んでください。もし誤りや修正の提案があれば、大変ありがたく思います。

## 1. macOS で `$HOME/.config/maa` を設定ディレクトリとして使用するには？

Rust ライブラリ [Directories](https://github.com/dirs-dev/directories-rs/) が macOS ではデフォルトで Apple 風のディレクトリを使用するため、maa-cli もデフォルトでは Apple 風の設定ディレクトリを使用します。しかし、コマンドラインプログラムには XDG 風のディレクトリの方が適しています。XDG 風のディレクトリを使用したい場合は、`export XDG_CONFIG_HOME="$HOME/.config"` のように環境変数 `XDG_CONFIG_HOME` を設定すれば、maa-cli は XDG 風の設定ディレクトリを使用します。環境変数を設定せずに XDG 風の設定ディレクトリを使用したい場合は、以下のコマンドでシンボリックリンクを作成できます：

```bash
mkdir -p "$HOME/.config/maa"
ln -s "$HOME/.config/maa" "$(maa dir config)"
```
