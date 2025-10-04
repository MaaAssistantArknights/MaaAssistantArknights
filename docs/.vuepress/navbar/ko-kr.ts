import { defineNavbarConfig } from 'vuepress-theme-plume'

export const kokrNavbar = defineNavbarConfig([
  {
    text: '사용자 설명서',
    icon: 'mdi:user',
    link: '/ko-kr/manual/newbie.html',
  },
  {
    text: '개발 문서',
    icon: 'ph:code-bold',
    link: '/ko-kr/develop/development.html',
  },
  {
    text: '프로토콜 문서',
    icon: 'basil:document-solid',
    link: '/ko-kr/protocol/integration.html',
  },
])
