import { sidebar } from "vuepress-theme-hope";

export const kokrSidebar = sidebar({
  "/ko-kr/": [
    {
      text: "MAA",
      icon: "ic:round-home",
      link: "/ko-kr/",
    },
    {
      text: "사용자 설명서",
      icon: "mdi:user",
      prefix: "manual/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "개발 관련",
      icon: "ph:code-bold",
      prefix: "develop/",
      collapsible: true,
      children: "structure",
    },
    {
      text: "프로토콜 문서",
      icon: "basil:document-solid",
      prefix: "protocol/",
      collapsible: true,
      children: "structure",
    },
  ],
});
