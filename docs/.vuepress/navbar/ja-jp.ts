import { navbar } from "vuepress-theme-hope";

export const jajpNavbar = navbar([
  {
    text: "主页",
    icon: "ic:round-home",
    link: "/ja-jp/",
  },
  {
    text: "使用説明",
    icon: "mdi:user",
    link: "/ja-jp/1.1-詳細説明",
  },
  {
    text: "開発関連",
    icon: "ph:code-bold",
    link: "/ja-jp/2.1-Linuxチュートリアル",
  },
  {
    text: "プロトコルドキュメント",
    icon: "basil:document-solid",
    link: "/ja-jp/3.1-統合ドキュメント",
  },
  {
    text: "MAA 公式サイト",
    icon: "mdi:cow",
    link: "https://maa.plus",
  },
]);
