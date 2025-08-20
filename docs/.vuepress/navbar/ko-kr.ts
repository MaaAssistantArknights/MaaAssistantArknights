import { defineNavbarConfig } from "vuepress-theme-plume";

export const kokrNavbar = defineNavbarConfig([
  {
    text: "홈페이지",
    icon: "ic:round-home",
    link: "/ko-kr/",
  },
  {
    text: "사용자 설명서",
    icon: "mdi:user",
    link: "/ko-kr/manual/",
  },
  {
    text: "개발 문서",
    icon: "ph:code-bold",
    link: "/ko-kr/develop/",
  },
  {
    text: "프로토콜 문서",
    icon: "basil:document-solid",
    link: "/ko-kr/protocol/",
  },
]);
