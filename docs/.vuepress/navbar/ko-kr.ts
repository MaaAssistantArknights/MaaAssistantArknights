import { navbar } from "vuepress-theme-hope";

export const kokrNavbar = navbar([
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
  {
    text: "MAA 공식 홈페이지",
    icon: "mdi:cow",
    link: "https://maa.plus",
  },
]);
