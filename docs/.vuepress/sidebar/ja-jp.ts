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
      collapsible: true,
      children: [
        {
          text: "詳細説明",
          link: "/ja-jp/1.1-詳細説明",
        },
        {
          text: "よくある質問",
          link: "/ja-jp/1.2-よくある質問",
        },
        {
          text: "エミュレータのサポート",
          icon: "laptop-code",
          link: "/ja-jp/1.3-エミュレータのサポート",
        },
      ],
    },
    {
      text: "開発関連",
      icon: "ph:code-bold",
      collapsible: true,
      children: [
        {
          text: "Linuxコンパイルチュートリアル",
          link: "/ja-jp/2.1-Linuxチュートリアル",
        },
        {
          text: "開発関連",
          link: "/ja-jp/2.2-プルリクエスト",
        },
        {
          text: "IssueBotの使用方法",
          link: "/ja-jp/2.3-IssueBotの使用方法",
        },
        {
          text: "純WebサイトのPRチュートリアル",
          link: "/ja-jp/2.4-純WebサイトのPRチュートリアル",
        },
        {
          text: "グローバル版を含む海外クライアントの対応について",
          link: "/ja-jp/2.5-OVERSEAS_CLIENTS_ADAPTATION",
        },
      ],
    },
    {
      text: "プロトコルドキュメント",
      icon: "basil:document-solid",
      collapsible: true,
      children: [
        {
          text: "インテグレーション",
          link: "/ja-jp/3.1-統合ドキュメント",
        },
        {
          text: "コールバック図解",
          link: "/ja-jp/3.2-コールバックAPI",
        },
        {
          text: "自動作戦図解",
          link: "/ja-jp/3.3-自動作戦API",
        },
        {
          text: "タスク図解",
          link: "/ja-jp/3.4-タスクAPI",
        },
        {
          text: "統合戦略図解",
          link: "/ja-jp/3.5-統合戦略API",
        },
        {
          text: "インフラスケジュール図解",
          link: "/ja-jp/3.6-インフラスケジュール設定",
        },
        {
          text: "セキュリティプレゼンス図解",
          link: "/ja-jp/3.7-SECURITY_PRESENCE_SCHEMA",
        },
      ],
    },
  ],
});
