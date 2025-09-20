import { defineNavbarConfig } from 'vuepress-theme-plume';

export const zhcnNavbar = defineNavbarConfig([
  {
    text: '用户手册',
    icon: 'mdi:user',
    link: '/zh-cn/manual/',
  },
  {
    text: '开发文档',
    icon: 'ph:code-bold',
    link: '/zh-cn/develop/',
  },
  {
    text: '协议文档',
    icon: 'basil:document-solid',
    link: '/zh-cn/protocol/',
  },
]);
