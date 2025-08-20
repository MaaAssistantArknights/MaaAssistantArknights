import { defineNavbarConfig } from "vuepress-theme-plume";

export const jajpNavbar = defineNavbarConfig([
  {
    text: "ホームページ",
    icon: "ic:round-home",
    link: "/ja-jp/",
  },
  {
    text: "使用説明",
    icon: "mdi:user",
    link: "/ja-jp/manual/",
  },
  {
    text: "開発関連",
    icon: "ph:code-bold",
    link: "/ja-jp/develop/",
  },
  {
    text: "プロトコルドキュメント",
    icon: "basil:document-solid",
    link: "/ja-jp/protocol/",
  },
]);
