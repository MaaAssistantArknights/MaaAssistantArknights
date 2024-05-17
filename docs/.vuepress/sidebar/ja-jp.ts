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
      prefix: "manual/",
      collapsible: true,
      children: [
        {
          text: "新手上路(translation required)",
          icon: "ri:guide-fill",
          link: "newbie",
        },
        {
          text: "詳細説明",
          icon: "mdi:information-outline",
          link: "introduction",
        },
        {
          text: "连接设置(translation required)",
          icon: "mdi:plug",
          link: "connection",
        },
        {
          text: "よくある質問",
          icon: "ph:question-fill",
          link: "faq",
        },
        {
          text: "エミュレータのサポート(translation required)",
          icon: "mingcute:laptop-fill",
          prefix: "devices/",
          collapsible: true,
          children: "structure",
        },
        {
          text: "CLIユーザーガイド",
          icon: "material-symbols:terminal",
          prefix: "cli/",
          collapsible: true,
          children: "structure",
        },
      ],
    },
    {
      text: "開発関連",
      icon: "ph:code-bold",
      prefix: "develop/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "プロトコルドキュメント",
      icon: "basil:document-solid",
      prefix: "protocol/",
      collapsible: true,
      children: "structure",
    },
  ],
});
