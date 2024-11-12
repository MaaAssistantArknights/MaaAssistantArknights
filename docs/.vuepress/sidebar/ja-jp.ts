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
      children: "structure",
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
