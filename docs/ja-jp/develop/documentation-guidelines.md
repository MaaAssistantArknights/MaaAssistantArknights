---
order: 6
icon: jam:write-f
---

# ドキュメントのガイドライン

::: tip
このドキュメントの目的は、ドキュメント作成者がテーマが提供する機能をより良く活用し、読みやすさを向上させることを指導することです。
:::

私たちのドキュメントは [vuepress](https://github.com/vuejs/vuepress) に基づいて構築されており、[vuepress-theme-hope](https://github.com/vuepress-theme-hope/vuepress-theme-hope) テーマが使用されています。詳細な説明については[公式ドキュメント](https://theme-hope.vuejs.press/)をご覧いただけます。ここでは一部の一般的な機能について紹介します。

## コンテナ

~~Dockerのコンテナではありません~~

このテーマでは、ヒント、ノート、情報、注意、警告、詳細などのカスタムコンテナをサポートしており、これらの機能を利用して特定のコンテンツを強調することができます。

コンテナの使用方法：

```markdown
::: [コンテナの種類] [コンテナのタイトル（オプション）]
書きたい内容
:::
```

受け入れられるコンテナの内容とデフォルトのタイトルは次のとおりです：

- `tip` ヒント
- `note` ノート
- `info` 情報
- `warning` 注意
- `danger` 警告
- `details` 詳細

### コンテナの例

::: tip
これはヒントのコンテナです
:::

::: note
これはノートのコンテナです
:::

::: info
これは情報のコンテナです
:::

::: warning
これは注意のコンテナです
:::

::: danger
これは警告のコンテナです
:::

::: details
これは詳細のコンテナです
:::

## アイコン

このテーマではアイコンがサポートされており、次の場所でアイコンを使用できます:

- ドキュメントのタイトル：frontmatter でドキュメントのタイトルの隣にアイコンを設定する

- ナビゲーションバー/サイドバー：ナビゲーションバーとサイドバーに表示されるアイコンを設定する

- ドキュメントのコンテンツ：ドキュメント内でアイコンを使用する

### ドキュメントのアイコンを設定する

ドキュメントの [frontmatter](#frontmatter) で、icon を使用してドキュメントのアイコンを設定できます。

このアイコンはドキュメントのタイトルの隣に表示されます。

::: details このドキュメントの frontmatter 設定

```markdown
---
icon: jam:write-f
---
```

:::

### ドキュメント内でアイコンを使用する

Markdown 内で `<HopeIcon />` コンポーネントを使用してアイコンを追加できます。このコンポーネントには以下の属性があります：

- `icon` アイコンのキーワードや URL を受け入れます。例: `jam:write-f`、`ic:round-home` など
- `color` CSSスタイルのカラー値を受け入れます。例: `#fff`、`red` など（このオプションはSVGアイコンにのみ有効です）
- `size` CSSスタイルのサイズを受け入れます。例: `1rem`、`2em`、`100px` など

::: details 例
<HopeIcon icon="ic:round-home" color="#1f1e33"/>

```markdown
<HopeIcon icon="ic:round-home" color="#1f1e33"/>
```

<HopeIcon icon="/images/maa-logo_512x512.png" size="4rem" />
```markdown
<HopeIcon icon="/images/maa-logo_512x512.png" size="4rem" />
```
:::

### アイコンキーワードの取得

このドキュメントで使用されているアイコンは [iconify](https://iconify.design/) から取得されており、お好みのアイコンを検索するには、提供されている [アイコン検索画面](https://icon-sets.iconify.design/) で検索し、そのキーワードをコピーしてください。

## Frontmatter

Frontmatter は Markdown ドキュメントの先頭に `---` で囲まれたセクションであり、内部では YAML 構文が使用されます。Frontmatter を使用することで、文書の編集日時、使用されるアイコン、カテゴリ、タグなどを識別することができます。

::: details サンプル

```markdown
---
date: 1919-08-10
icon: jam:write-f
order: 1
---

# ドキュメントのタイトル

...
```

:::

各フィールドの意味は次のとおりです：

- `date` ドキュメントの編集日時
- `icon` ドキュメントのタイトルの隣のアイコン
- `order` サイドバー内でのドキュメントの並び順
