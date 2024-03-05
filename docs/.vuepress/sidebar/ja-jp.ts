import { sidebar } from "vuepress-theme-hope";

export const jajpSidebar = sidebar({
  "/ja-jp/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/ja-jp/",
    },
    {
      text: "使用説明",
      icon: "mdi:user",
      prefix: "使用説明/",
      collapsible: true,
      children: [
        {
          text: "詳細説明",
          icon: "mdi:information-outline",
          link: "詳細説明",
        },
        {
          text: "よくある質問",
          icon: "ph:question-fill",
          link: "よくある質問",
        },
        {
          text: "エミュレータのサポート",
          icon: "mingcute:laptop-fill",
          prefix: "エミュレータのサポート/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "CLIユーザーガイド",
          icon: "material-symbols:terminal",
          link: "CLIユーザーガイド",
        },
      ],
    },
    {
      text: "開発関連",
      icon: "ph:code-bold",
      prefix: "開発関連/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "プロトコルドキュメント",
      icon: "basil:document-solid",
      prefix: "プロトコルドキュメント/",
      collapsible: true,
      children: "structure",
    },
  ],
});
