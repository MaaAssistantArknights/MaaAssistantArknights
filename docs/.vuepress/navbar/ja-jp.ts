import { defineNavbarConfig } from 'vuepress-theme-plume';

export const jajpNavbar = defineNavbarConfig([
  {
    text: '使用説明',
    icon: 'mdi:user',
    link: '/ja-jp/manual/newbie.html',
  },
  {
    text: '開発関連',
    icon: 'ph:code-bold',
    link: '/ja-jp/develop/development.html',
  },
  {
    text: 'プロトコルドキュメント',
    icon: 'basil:document-solid',
    link: '/ja-jp/protocol/integration.html',
  },
]);
