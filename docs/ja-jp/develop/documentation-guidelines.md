---
order: 6
icon: jam:write-f
---

# ドキュメントのガイドライン

::: tip
このドキュメントの目的は、ドキュメント作成者がテーマが提供する機能をより良く活用し、読みやすさを向上させることを指導することです。
:::

私たちのドキュメントは [vuepress](https://github.com/vuejs/vuepress) に基づいて構築されており、[vuepress-theme-plume](https://github.com/pengzhanbo/vuepress-theme-plume) テーマが使用されています。詳細な説明については[公式ドキュメント](https://theme-plume.vuejs.press/)をご覧いただけます。ここでは一部の一般的な機能について紹介します。

## ローカルデプロイ

1. [pnpm](https://pnpm.io/installation) をインストールし、[Pull Request ワークフローの概要](./development.md#github-pull-request-プロセス概要)を参照してリポジトリをローカルにクローンします。
2. `docs` ディレクトリでターミナルを開き、`pnpm i` を実行して依存関係をインストールします。
3. `pnpm run dev` を実行してデプロイします。

## コンテナ

~~Dockerのコンテナではありません~~

このテーマでは、ヒント、ノート、情報、注意、警告、詳細などのカスタムコンテナをサポートしており、これらの機能を利用して特定のコンテンツを強調することができます。

コンテナ内にコンテナをネストする場合、親コンテナは子コンテナよりもコロン `:` を1つ多く記述する必要があります。

コンテナの使用方法：

```markdown
::: [コンテナの種類] [コンテナのタイトル（オプション）]
書きたい内容
:::
```

または GitHub スタイルの構文を使用する方法：

```markdown
> [!コンテナの種類]
> 書きたい内容
```

受け入れられるコンテナの内容とデフォルトのタイトルは次のとおりです：

- `tip` ヒント
- `note` ノート
- `info` 情報
- `warning` 注意
- `danger` 警告
- `details` 詳細
- `window` ==特殊なコンテナ==

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

::: window
これは特殊なコンテナです
:::

## マーカー

マーカー構文を使用して、強調したい内容にマークを付けることができます。

使用方法：`==マーカーの内容=={マーカーの色（オプション）}` の構文でマークを付けます。両端にスペースが必要な点に注意してください。

**入力：**

```markdown
MaaAssistantArknights は ==たくさんの豚== によって開発されました
```

**出力：**

MaaAssistantArknights は ==たくさんの豚== によって開発されました

テーマには以下のカラースキームが組み込まれています：

- **default**: `==Default==` - ==Default==
- **info**: `==Info=={.info}` - ==Info=={.info}
- **note**: `==Note=={.note}` - ==Note=={.note}
- **tip**: `==Tip=={.tip}` - ==Tip=={.tip}
- **warning**: `==Warning=={.warning}` - ==Warning=={.warning}
- **danger**: `==Danger=={.danger}` - ==Danger=={.danger}
- **caution**: `==Caution=={.caution}` - ==Caution=={.caution}
- **important**: `==Important=={.important}` - ==Important=={.important}

## 隠しテキスト

何らかの理由でドキュメントの一部を一時的に隠す必要がある場合、隠しテキスト機能を使用できます。

`!!隠したい内容!!{設定（オプション）}` の構文で使用でき、デフォルトの効果は以下の通りです：

!!なんかニコニコ大百科（仮）を読んでる気がする（取り消し線!!

以下の設定が使用できます：

::: window
入力：

```markdown
+ マスクエフェクト + マウスホバー：!!マウスホバーで見えます!!{.mask .hover}
+ マスクエフェクト + クリック：!!クリックで見えます!!{.mask .click}
+ テキストぼかしエフェクト + マウスホバー：!!マウスホバーで見えます!!{.blur .hover}
+ テキストぼかしエフェクト + クリック：!!クリックで見えます!!{.blur .click}
```

出力：

- マスクエフェクト + マウスホバー：!!マウスホバーで見えます!!{.mask .hover}
- マスクエフェクト + クリック：!!クリックで見えます!!{.mask .click}
- テキストぼかしエフェクト + マウスホバー：!!マウスホバーで見えます!!{.blur .hover}
- テキストぼかしエフェクト + クリック：!!クリックで見えます!!{.blur .click}

:::

## ステップ

ステップバイステップのチュートリアルを書くとき、番号付きリストはネストによって階層感を失うことがあります。そのような場合、`steps` コンテナが最善の選択です。

入力：

````markdown
:::: steps
1. ステップ 1

   ```ts
   console.log('Hello World!')
   ```

2. ステップ 2

   ステップ 2 の関連内容はこちら

3. ステップ 3

   ::: tip
   ヒントコンテナ
   :::

4. 終わり
::::
````

出力：

:::: steps

1. ステップ 1

   ```ts
   console.log('Hello World!')
   ```

2. ステップ 2

   ステップ 2 の関連内容はこちら

3. ステップ 3

   ::: tip
   ヒントコンテナ
   :::

4. 終わり

::::

## スマート画像コンテナ

テーマが提供する機能をベースに画像コンテナをラッパーしました。このコンテナはライト/ダークテーマに応じて対応する画像を自動的に表示し、自動レイアウトをサポートします。

Markdown 本文で `<ImageGrid>` コンポーネントを使用してこのメソッドを呼び出すことができます。具体的な構文と効果は以下の通りです：

::: window

構文：

```markdown
<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  },
  {
    light: 'images/zh-cn/readme/3-light.png',
    dark: 'images/zh-cn/readme/3-dark.png'
  },
  {
    light: 'images/zh-cn/readme/4-light.png',
    dark: 'images/zh-cn/readme/4-dark.png'
  }
]" />
```

レンダリング結果：

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/readme/1-light.png',
    dark: 'images/zh-cn/readme/1-dark.png'
  },
  {
    light: 'images/zh-cn/readme/2-light.png',
    dark: 'images/zh-cn/readme/2-dark.png'
  }
]" />

:::

## フィールドコンテナ

この構文はやや複雑なため、[公式ドキュメント](https://theme-plume.vuejs.press/guide/markdown/field/)を参照して使用してください。

効果は以下の通りです：

:::: field-group
::: field name="theme" type="ThemeConfig" required default="{ base: '/' }"
テーマ設定
:::

::: field name="enabled" type="boolean" optional default="true"
有効かどうか
:::

::: field name="callback" type="(...args: any[]) => void" optional default="() => {}"
<Badge type="tip" text="v1.0.0 追加"  />
コールバック関数
:::

::: field name="other" type="string" deprecated
<Badge type="danger" text="v0.9.0 廃止"  />
廃止されたプロパティ
:::
::::

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
