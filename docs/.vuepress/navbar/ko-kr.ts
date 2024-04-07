import { navbar } from "vuepress-theme-hope";

export const kokrNavbar = navbar([
  {
    text: "主页",
    icon: "ic:round-home",
    link: "/ko-kr/",
  },
  {
    text: "사용자 설명서",
    icon: "mdi:user",
    link: "/ko-kr/사용자설명서/사용자설명서",
  },
  {
    text: "개발 관련",
    icon: "ph:code-bold",
    link: "/ko-kr/개발문서/1.개발시작",
  },
  {
    text: "프로토콜 문서",
    icon: "basil:document-solid",
    link: "/ko-kr/스키마/1.연동",
  },
  {
    text: "MAA 공식 홈페이지",
    icon: "mdi:cow",
    link: "https://maa.plus",
  },
]);
