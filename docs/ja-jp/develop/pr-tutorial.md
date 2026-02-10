---
order: 3
icon: mingcute:git-pull-request-fill
---

# 純WebサイトのPRチュートリアル

「パラスちゃん」も理解できるGitHubのPull Requestの使用ガイド (\*´▽｀)ノノ

この文書は機械翻訳です。もし可能であれば、中国語の文書を読んでください。もし誤りや修正の提案があれば、大変ありがたく思います。

::: warning
このガイドでは、多くの概念を簡略化し、より多くの人々が実際に使用できるように、不格好であるが簡単な操作が含まれています。また、正しくない説明もありますので、大変恐れ入りますが、専門家の方々にはお許しください。
Gitの使用経験とプログラミングの基礎がある場合、[GitHub Pull Request プロセス概要](./development.md#github-pull-request-プロセス概要)など、より上級のチュートリアルを見ることができます。
:::

## 基本概念および用語の説明

このセクションは少し退屈かもしれませんが、興味がない場合は直接実践部分に進むことができます。わからないことがあれば戻って読んでください。

### Repository（リポ）

repo、コードやその他のリソースファイルを保存する場所です。

👇 これはMAAのリポジトリです（一般的にMAAのメインリポジトリと呼ばれます）。

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/repository-light.png',
    dark: 'images/zh-cn/pr-tutorial/repository-dark.png'
  }
]" />

### Fork（複製）

フォーク、つまり、MAAのコードをコピーし、その後、変更などを行うことができるようにします。元のコードを壊さないようにするためです。  
しかし、通常、「複製」と言うと、最初にコピーすることを考えることが多いですが、フォークには明確な翻訳がないため、通常、英語を直接使用する傾向があります。

コピー後のリポジトリは MAA (1) と呼ばれます（おおよその意味は伝わるかと思います）。  
区別するために、元のMAAリポジトリは「主リポジトリ」「upstream（上流リポジトリ）」「origin（元のリポジトリ）」と呼ばれます。  
それぞれの人がコピーできるので、コピーしたものを「個人リポジトリ」と呼びます。

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/fork-light.png',
    dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
  }
]" />

### Pull Request（プルリクエスト、マージリクエスト）

略称PR，「プルリクエスト(Pull Request)」とも呼ばれます。ただ、「プルリクエスト」と直訳すると奇妙に聞こえるため、一般的には「PR」と省略され、「PRを送ってくれ」というように言われます。

先程の話に戻りますと、自分でフォーク（複製）した個人のリポジトリを変更した場合、その変更内容をメインのリポジトリに反映させるにはどうすればいいでしょうか？ この場合、「PR」を立てて、自分の変更内容をメインのリポジトリに追加するための申請を行います。

もちろん、これは「リクエスト」であるため、承認が必要です。MAA Teamのメンバーは、あなたの変更に対していくつかの提案を行うかもしれません。もちろん、私たちの提案が完全に正しいわけではないので、合理的に議論しましょう。

👇 以下は、現在承認待ちの提出されたPRの例です。

<ImageGrid :imageList="[
  {
    light: 'images/zh-cn/pr-tutorial/pull-request-light.png',
    dark: 'images/zh-cn/pr-tutorial/pull-request-dark.png'
  }
]" />

### Conflict（コンフリクト）

仮に、メインリポジトリにAというファイルがあり、その中身は「111」とします。  
あなたはフォークして、中身を「222」に変更しました。しかし、ちょうどあなたがPRを提出しようとしたとき、別の人がフォークしてAファイルを「333」に変更してPRを提出しました。  
これにより、Aファイルが2人によって異なる方法で変更され、どちらを受け入れるかが問題になります。これがConflict（コンフリクト）です。  
Conflictを解決するのは少し面倒ですが、ここでは概念について説明し、実際に遭遇した場合に何が起こっているかを理解するために説明します。解決策については詳しく説明しません。

## ウェブページでの PR 操作の全プロセス

1. まず、MAAのメインリポジトリにアクセスして、コードをforkします。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/fork-light.png',
       dark: 'images/zh-cn/pr-tutorial/fork-dark.png'
     }
   ]" />

2. 「Copy the master branch only」オプションを外して、Create Forkをクリックします。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-2-dark.png'
     }
   ]" />

3. 次に、あなたの個人リポジトリに移動し、「あなたの名前/ MaaAssistantArknights」というタイトルが表示され、下に「MAAメインリポジトリから複製されたMaaAssistantArknights/MaaAssistantArknights」という文言が表示されます。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-3-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-3-dark.png'
     }
   ]" />

4. 変更するファイルを探します。 "Go to file" をクリックしてグローバル検索を行うか、下のフォルダーから直接検索することもできます（ファイルの場所を知っている場合）。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-4-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-4-dark.png'
     }
   ]" />

5. ファイルを開いたら、ファイルの右上隅にある✏️をクリックして編集を開始します。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-5-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-5-dark.png'
     }
   ]" />

6. 変更を加えます！（もちろん、リソースファイルなどの場合は、まずMAAフォルダー内で変更をテストし、問題がないことを確認してから、ウェブページに貼り付けて変更を行ってください）
7. 変更が完了したら、一番下までスクロールして、変更内容を記述します。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-1-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-1-dark.png'
     }
   ]" />

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-7-2-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-7-2-dark.png'
     }
   ]" />

8. 変更するもう1つのファイルがある場合は、5-8を繰り返してください。
9. 変更が完了したら、PRを行います！個人リポジトリのPull Requestタブをクリックします。  
   「Compare & Pull Request」ボタンがある場合は、それをクリックしてください。ない場合は、「New Pull Request」をクリックしてください。

   <ImageGrid :imageList="[
     {
       light: 'images/zh-cn/pr-tutorial/pr-9-light.png',
       dark: 'images/zh-cn/pr-tutorial/pr-9-dark.png'
     }
   ]" />

10. これでメインリポジトリに移動します。PRする内容を確認してください。  
    スクリーンショットのように、真ん中に左向きの矢印があり、右側の「個人名/MAA」のdevブランチを「メインリポジトリ/MAA」のdevブランチにマージすることを申請しています。

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-10-1-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-10-1-dark.png'
      }
    ]" />

    次に、タイトルや変更内容などを記述して、承認をリクエストします。

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-10-2-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-10-2-dark.png'
      }
    ]" />

11. MAAチームの大佬たちに承認していただきましょう！もちろん、意見を提供してくださることもあるかもしれません  
    👇例えば（純粋に娯楽のために、本気にしないでください）

    <ImageGrid :imageList="[
        {
          light: 'images/zh-cn/pr-tutorial/pr-11-light.png',
          dark: 'images/zh-cn/pr-tutorial/pr-11-dark.png'
        }
      ]" />

12. 大佬たちがさらに小さな問題を修正するように言った場合、あなたの個人のリポジトリに戻って、以前のdevブランチに切り替え、手順3-9を繰り返すだけで大丈夫です！  
    手順2（再度フォークする）を実行する必要はなく、手順10（再度プルリクエストする）を実行する必要もありません。現在のプルリクエストはまだ承認待ちの状態にあり、後続の変更はこのプルリクエストに直接反映されます。  
    👇 以下は例です。最下部に「再度変更デモを変更」というメッセージが追加されたことがわかります。

    <ImageGrid :imageList="[
        {
          light: 'images/zh-cn/pr-tutorial/pr-12-light.png',
          dark: 'images/zh-cn/pr-tutorial/pr-12-dark.png'
        }
      ]" />

13. 大佬たちの承認を得たら、すべて完了です！ バージョンがリリースされた後、あなたのGitHubプロフィールアイコンは自動的に貢献者リストに追加されます。皆様のご奉仕に深く感謝申し上げます！  
    ~~なんで全部二次元キャラなんだろう、あ、私も二次元好きだったわ。ま、いいか~~
    ::: tip 貢献/参加者
    開発/テストに参加してくれたすべての友人たちに感謝します。皆さんの助けでMAAはますます良くなります！ (\*´▽ ｀)ノノ

    [![Contributors](https://contributors-img.web.app/image?repo=MaaAssistantArknights/MaaAssistantArknights&max=105&columns=15)](https://github.com/MaaAssistantArknights/MaaAssistantArknights/graphs/contributors)
    :::

14. 次回別のPRを提出する場合は、まずdevブランチに切り替えて、以下の図のように直接操作してください。

    ::: warning
    この操作は、あなたの個人リポジトリをメインリポジトリとまったく同じ状態に強制的に同期するものであり、最も簡単で効果的な競合解消方法です。ただし、あなたの個人リポジトリにすでに追加されているものは削除されます！  
    :::
    競合が生じないことを確認できる場合は、右側の緑色の「Update Branch」ボタンを使用してください。

    私が言ったことが何を意味するのか分からない/関係ない場合は、左側のボタンをクリックしてください。

    <ImageGrid :imageList="[
      {
        light: 'images/zh-cn/pr-tutorial/pr-14-1-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-14-1-dark.png'
      },
      {
        light: 'images/zh-cn/pr-tutorial/pr-14-2-light.png',
        dark: 'images/zh-cn/pr-tutorial/pr-14-2-dark.png'
      }
    ]" />

    その後、ステップ3-14を繰り返し、変更を加え、PRを提出することができます。
